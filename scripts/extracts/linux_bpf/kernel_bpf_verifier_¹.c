// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 121/132



	switch (opcode) {
	case BPF_CALL:
		if (BPF_SRC(insn->code) != BPF_K ||
		    (insn->src_reg != BPF_PSEUDO_KFUNC_CALL && insn->off != 0) ||
		    (insn->src_reg != BPF_REG_0 && insn->src_reg != BPF_PSEUDO_CALL &&
		     insn->src_reg != BPF_PSEUDO_KFUNC_CALL) ||
		    insn->dst_reg != BPF_REG_0 || class == BPF_JMP32) {
			verbose(env, "BPF_CALL uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_JA:
		if (BPF_SRC(insn->code) == BPF_X) {
			if (insn->src_reg != BPF_REG_0 || insn->imm != 0 || insn->off != 0) {
				verbose(env, "BPF_JA|BPF_X uses reserved fields\n");
				return -EINVAL;
			}
		} else if (insn->src_reg != BPF_REG_0 || insn->dst_reg != BPF_REG_0 ||
			   (class == BPF_JMP && insn->imm != 0) ||
			   (class == BPF_JMP32 && insn->off != 0)) {
			verbose(env, "BPF_JA uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_EXIT:
		if (BPF_SRC(insn->code) != BPF_K || insn->imm != 0 ||
		    insn->src_reg != BPF_REG_0 || insn->dst_reg != BPF_REG_0 ||
		    class == BPF_JMP32) {
			verbose(env, "BPF_EXIT uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_JCOND:
		if (insn->code != (BPF_JMP | BPF_JCOND) || insn->src_reg != BPF_MAY_GOTO ||
		    insn->dst_reg || insn->imm) {
			verbose(env, "invalid may_goto imm %d\n", insn->imm);
			return -EINVAL;
		}
		return 0;
	default:
		if (BPF_SRC(insn->code) == BPF_X) {
			if (insn->imm != 0) {
				verbose(env, "BPF_JMP/JMP32 uses reserved fields\n");
				return -EINVAL;
			}
		} else if (insn->src_reg != BPF_REG_0) {
			verbose(env, "BPF_JMP/JMP32 uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	}
}

static int check_insn_fields(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	switch (BPF_CLASS(insn->code)) {
	case BPF_ALU:
	case BPF_ALU64:
		return check_alu_fields(env, insn);
	case BPF_LDX:
		if ((BPF_MODE(insn->code) != BPF_MEM && BPF_MODE(insn->code) != BPF_MEMSX) ||
		    insn->imm != 0) {
			verbose(env, "BPF_LDX uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_STX:
		if (BPF_MODE(insn->code) == BPF_ATOMIC)
			return 0;
		if (BPF_MODE(insn->code) != BPF_MEM || insn->imm != 0) {
			verbose(env, "BPF_STX uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_ST:
		if (BPF_MODE(insn->code) != BPF_MEM || insn->src_reg != BPF_REG_0) {
			verbose(env, "BPF_ST uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_JMP:
	case BPF_JMP32:
		return check_jmp_fields(env, insn);
	case BPF_LD: {
		u8 mode = BPF_MODE(insn->code);

		if (mode == BPF_ABS || mode == BPF_IND) {
			if (insn->dst_reg != BPF_REG_0 || insn->off != 0 ||
			    BPF_SIZE(insn->code) == BPF_DW ||
			    (mode == BPF_ABS && insn->src_reg != BPF_REG_0)) {
				verbose(env, "BPF_LD_[ABS|IND] uses reserved fields\n");
				return -EINVAL;
			}
		} else if (mode != BPF_IMM) {
			verbose(env, "invalid BPF_LD mode\n");
			return -EINVAL;
		}
		return 0;
	}
	default:
		verbose(env, "unknown insn class %d\n", BPF_CLASS(insn->code));
		return -EINVAL;
	}
}

/*
 * Check that insns are sane and rewrite pseudo imm in ld_imm64 instructions:
 *
 * 1. if it accesses map FD, replace it with actual map pointer.
 * 2. if it accesses btf_id of a VAR, replace it with pointer to the var.
 *
 * NOTE: btf_vmlinux is required for converting pseudo btf_id.
 */
static int check_and_resolve_insns(struct bpf_verifier_env *env)
{
	struct bpf_insn *insn = env->prog->insnsi;
	int insn_cnt = env->prog->len;
	int i, err;

	err = bpf_prog_calc_tag(env->prog);
	if (err)
		return err;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->dst_reg >= MAX_BPF_REG) {
			verbose(env, "R%d is invalid\n", insn->dst_reg);
			return -EINVAL;
		}
		if (insn->src_reg >= MAX_BPF_REG) {
			verbose(env, "R%d is invalid\n", insn->src_reg);
			return -EINVAL;
		}
		if (insn[0].code == (BPF_LD | BPF_IMM | BPF_DW)) {
			struct bpf_insn_aux_data *aux;
			struct bpf_map *map;
			int map_idx;
			u64 addr;
			u32 fd;

			if (i == insn_cnt - 1 || insn[1].code != 0 ||
			    insn[1].dst_reg != 0 || insn[1].src_reg != 0 ||
			    insn[1].off != 0) {
				verbose(env, "invalid bpf_ld_imm64 insn\n");
				return -EINVAL;
			}

			if (insn[0].off != 0) {
				verbose(env, "BPF_LD_IMM64 uses reserved fields\n");
				return -EINVAL;
			}

			if (insn[0].src_reg == 0)
				/* valid generic load 64-bit imm */
				goto next_insn;

			if (insn[0].src_reg == BPF_PSEUDO_BTF_ID) {
				aux = &env->insn_aux_data[i];
				err = check_pseudo_btf_id(env, insn, aux);
				if (err)
					return err;
				goto next_insn;
			}

			if (insn[0].src_reg == BPF_PSEUDO_FUNC) {
				aux = &env->insn_aux_data[i];
				aux->ptr_type = PTR_TO_FUNC;
				goto next_insn;
			}