// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 44/132



		err = check_flow_keys_access(env, off, size);
		if (!err && t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else if (type_is_sk_pointer(reg->type)) {
		if (t == BPF_WRITE) {
			verbose(env, "R%d cannot write into %s\n",
				regno, reg_type_str(env, reg->type));
			return -EACCES;
		}
		err = check_sock_access(env, insn_idx, regno, off, size, t);
		if (!err && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else if (reg->type == PTR_TO_TP_BUFFER) {
		err = check_tp_buffer_access(env, reg, regno, off, size);
		if (!err && t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else if (base_type(reg->type) == PTR_TO_BTF_ID &&
		   !type_may_be_null(reg->type)) {
		err = check_ptr_to_btf_access(env, regs, regno, off, size, t,
					      value_regno);
	} else if (reg->type == CONST_PTR_TO_MAP) {
		err = check_ptr_to_map_access(env, regs, regno, off, size, t,
					      value_regno);
	} else if (base_type(reg->type) == PTR_TO_BUF &&
		   !type_may_be_null(reg->type)) {
		bool rdonly_mem = type_is_rdonly_mem(reg->type);
		u32 *max_access;

		if (rdonly_mem) {
			if (t == BPF_WRITE) {
				verbose(env, "R%d cannot write into %s\n",
					regno, reg_type_str(env, reg->type));
				return -EACCES;
			}
			max_access = &env->prog->aux->max_rdonly_access;
		} else {
			max_access = &env->prog->aux->max_rdwr_access;
		}

		err = check_buffer_access(env, reg, regno, off, size, false,
					  max_access);

		if (!err && value_regno >= 0 && (rdonly_mem || t == BPF_READ))
			mark_reg_unknown(env, regs, value_regno);
	} else if (reg->type == PTR_TO_ARENA) {
		if (t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else {
		verbose(env, "R%d invalid mem access '%s'\n", regno,
			reg_type_str(env, reg->type));
		return -EACCES;
	}

	if (!err && size < BPF_REG_SIZE && value_regno >= 0 && t == BPF_READ &&
	    regs[value_regno].type == SCALAR_VALUE) {
		if (!is_ldsx)
			/* b/h/w load zero-extends, mark upper bits as known 0 */
			coerce_reg_to_size(&regs[value_regno], size);
		else
			coerce_reg_to_size_sx(&regs[value_regno], size);
	}
	return err;
}

static int save_aux_ptr_type(struct bpf_verifier_env *env, enum bpf_reg_type type,
			     bool allow_trust_mismatch);

static int check_load_mem(struct bpf_verifier_env *env, struct bpf_insn *insn,
			  bool strict_alignment_once, bool is_ldsx,
			  bool allow_trust_mismatch, const char *ctx)
{
	struct bpf_reg_state *regs = cur_regs(env);
	enum bpf_reg_type src_reg_type;
	int err;

	/* check src operand */
	err = check_reg_arg(env, insn->src_reg, SRC_OP);
	if (err)
		return err;

	/* check dst operand */
	err = check_reg_arg(env, insn->dst_reg, DST_OP_NO_MARK);
	if (err)
		return err;

	src_reg_type = regs[insn->src_reg].type;

	/* Check if (src_reg + off) is readable. The state of dst_reg will be
	 * updated by this call.
	 */
	err = check_mem_access(env, env->insn_idx, insn->src_reg, insn->off,
			       BPF_SIZE(insn->code), BPF_READ, insn->dst_reg,
			       strict_alignment_once, is_ldsx);
	err = err ?: save_aux_ptr_type(env, src_reg_type,
				       allow_trust_mismatch);
	err = err ?: reg_bounds_sanity_check(env, &regs[insn->dst_reg], ctx);

	return err;
}

static int check_store_reg(struct bpf_verifier_env *env, struct bpf_insn *insn,
			   bool strict_alignment_once)
{
	struct bpf_reg_state *regs = cur_regs(env);
	enum bpf_reg_type dst_reg_type;
	int err;

	/* check src1 operand */
	err = check_reg_arg(env, insn->src_reg, SRC_OP);
	if (err)
		return err;

	/* check src2 operand */
	err = check_reg_arg(env, insn->dst_reg, SRC_OP);
	if (err)
		return err;

	dst_reg_type = regs[insn->dst_reg].type;

	/* Check if (dst_reg + off) is writeable. */
	err = check_mem_access(env, env->insn_idx, insn->dst_reg, insn->off,
			       BPF_SIZE(insn->code), BPF_WRITE, insn->src_reg,
			       strict_alignment_once, false);
	err = err ?: save_aux_ptr_type(env, dst_reg_type, false);

	return err;
}

static int check_atomic_rmw(struct bpf_verifier_env *env,
			    struct bpf_insn *insn)
{
	int load_reg;
	int err;

	if (BPF_SIZE(insn->code) != BPF_W && BPF_SIZE(insn->code) != BPF_DW) {
		verbose(env, "invalid atomic operand size\n");
		return -EINVAL;
	}

	/* check src1 operand */
	err = check_reg_arg(env, insn->src_reg, SRC_OP);
	if (err)
		return err;

	/* check src2 operand */
	err = check_reg_arg(env, insn->dst_reg, SRC_OP);
	if (err)
		return err;

	if (insn->imm == BPF_CMPXCHG) {
		/* Check comparison of R0 with memory location */
		const u32 aux_reg = BPF_REG_0;

		err = check_reg_arg(env, aux_reg, SRC_OP);
		if (err)
			return err;

		if (is_pointer_value(env, aux_reg)) {
			verbose(env, "R%d leaks addr into mem\n", aux_reg);
			return -EACCES;
		}
	}

	if (is_pointer_value(env, insn->src_reg)) {
		verbose(env, "R%d leaks addr into mem\n", insn->src_reg);
		return -EACCES;
	}