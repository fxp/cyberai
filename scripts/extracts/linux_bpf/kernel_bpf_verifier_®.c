// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 110/132



	/* detect if R == 0 where R is returned from bpf_map_lookup_elem().
	 * Also does the same detection for a register whose the value is
	 * known to be 0.
	 * NOTE: these optimizations below are related with pointer comparison
	 *       which will never be JMP32.
	 */
	if (!is_jmp32 && (opcode == BPF_JEQ || opcode == BPF_JNE) &&
	    type_may_be_null(dst_reg->type) &&
	    ((BPF_SRC(insn->code) == BPF_K && insn->imm == 0) ||
	     (BPF_SRC(insn->code) == BPF_X && bpf_register_is_null(src_reg)))) {
		/* Mark all identical registers in each branch as either
		 * safe or unknown depending R == 0 or R != 0 conditional.
		 */
		mark_ptr_or_null_regs(this_branch, insn->dst_reg,
				      opcode == BPF_JNE);
		mark_ptr_or_null_regs(other_branch, insn->dst_reg,
				      opcode == BPF_JEQ);
	} else if (!try_match_pkt_pointers(insn, dst_reg, &regs[insn->src_reg],
					   this_branch, other_branch) &&
		   is_pointer_value(env, insn->dst_reg)) {
		verbose(env, "R%d pointer comparison prohibited\n",
			insn->dst_reg);
		return -EACCES;
	}
	if (env->log.level & BPF_LOG_LEVEL)
		print_insn_state(env, this_branch, this_branch->curframe);
	return 0;
}

/* verify BPF_LD_IMM64 instruction */
static int check_ld_imm(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	struct bpf_insn_aux_data *aux = cur_aux(env);
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *dst_reg;
	struct bpf_map *map;
	int err;

	if (BPF_SIZE(insn->code) != BPF_DW) {
		verbose(env, "invalid BPF_LD_IMM insn\n");
		return -EINVAL;
	}

	err = check_reg_arg(env, insn->dst_reg, DST_OP);
	if (err)
		return err;

	dst_reg = &regs[insn->dst_reg];
	if (insn->src_reg == 0) {
		u64 imm = ((u64)(insn + 1)->imm << 32) | (u32)insn->imm;

		dst_reg->type = SCALAR_VALUE;
		__mark_reg_known(&regs[insn->dst_reg], imm);
		return 0;
	}

	/* All special src_reg cases are listed below. From this point onwards
	 * we either succeed and assign a corresponding dst_reg->type after
	 * zeroing the offset, or fail and reject the program.
	 */
	mark_reg_known_zero(env, regs, insn->dst_reg);

	if (insn->src_reg == BPF_PSEUDO_BTF_ID) {
		dst_reg->type = aux->btf_var.reg_type;
		switch (base_type(dst_reg->type)) {
		case PTR_TO_MEM:
			dst_reg->mem_size = aux->btf_var.mem_size;
			break;
		case PTR_TO_BTF_ID:
			dst_reg->btf = aux->btf_var.btf;
			dst_reg->btf_id = aux->btf_var.btf_id;
			break;
		default:
			verifier_bug(env, "pseudo btf id: unexpected dst reg type");
			return -EFAULT;
		}
		return 0;
	}

	if (insn->src_reg == BPF_PSEUDO_FUNC) {
		struct bpf_prog_aux *aux = env->prog->aux;
		u32 subprogno = bpf_find_subprog(env,
						 env->insn_idx + insn->imm + 1);

		if (!aux->func_info) {
			verbose(env, "missing btf func_info\n");
			return -EINVAL;
		}
		if (aux->func_info_aux[subprogno].linkage != BTF_FUNC_STATIC) {
			verbose(env, "callback function not static\n");
			return -EINVAL;
		}

		dst_reg->type = PTR_TO_FUNC;
		dst_reg->subprogno = subprogno;
		return 0;
	}

	map = env->used_maps[aux->map_index];

	if (insn->src_reg == BPF_PSEUDO_MAP_VALUE ||
	    insn->src_reg == BPF_PSEUDO_MAP_IDX_VALUE) {
		if (map->map_type == BPF_MAP_TYPE_ARENA) {
			__mark_reg_unknown(env, dst_reg);
			dst_reg->map_ptr = map;
			return 0;
		}
		__mark_reg_known(dst_reg, aux->map_off);
		dst_reg->type = PTR_TO_MAP_VALUE;
		dst_reg->map_ptr = map;
		WARN_ON_ONCE(map->map_type != BPF_MAP_TYPE_INSN_ARRAY &&
			     map->max_entries != 1);
		/* We want reg->id to be same (0) as map_value is not distinct */
	} else if (insn->src_reg == BPF_PSEUDO_MAP_FD ||
		   insn->src_reg == BPF_PSEUDO_MAP_IDX) {
		dst_reg->type = CONST_PTR_TO_MAP;
		dst_reg->map_ptr = map;
	} else {
		verifier_bug(env, "unexpected src reg value for ldimm64");
		return -EFAULT;
	}

	return 0;
}

static bool may_access_skb(enum bpf_prog_type type)
{
	switch (type) {
	case BPF_PROG_TYPE_SOCKET_FILTER:
	case BPF_PROG_TYPE_SCHED_CLS:
	case BPF_PROG_TYPE_SCHED_ACT:
		return true;
	default:
		return false;
	}
}

/* verify safety of LD_ABS|LD_IND instructions:
 * - they can only appear in the programs where ctx == skb
 * - since they are wrappers of function calls, they scratch R1-R5 registers,
 *   preserve R6-R9, and store return value into R0
 *
 * Implicit input:
 *   ctx == skb == R6 == CTX
 *
 * Explicit input:
 *   SRC == any register
 *   IMM == 32-bit immediate
 *
 * Output:
 *   R0 - 8/16/32-bit skb data converted to cpu endianness
 */
static int check_ld_abs(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	struct bpf_reg_state *regs = cur_regs(env);
	static const int ctx_reg = BPF_REG_6;
	u8 mode = BPF_MODE(insn->code);
	int i, err;

	if (!may_access_skb(resolve_prog_type(env->prog))) {
		verbose(env, "BPF_LD_[ABS|IND] instructions not allowed for this program type\n");
		return -EINVAL;
	}

	if (!env->ops->gen_ld_abs) {
		verifier_bug(env, "gen_ld_abs is null");
		return -EFAULT;
	}