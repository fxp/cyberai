// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 103/132



	/* Jump (TRUE) branch */
	regs_refine_cond_op(&env->true_reg1, &env->true_reg2, opcode, is_jmp32);
	reg_bounds_sync(&env->true_reg1);
	reg_bounds_sync(&env->true_reg2);
	/*
	 * If there is a range bounds violation in *any* of the abstract values in either
	 * reg_states in the TRUE branch (i.e. true_reg1, true_reg2), the TRUE branch must be dead.
	 * Only FALSE branch will be taken.
	 */
	if (range_bounds_violation(&env->true_reg1) || range_bounds_violation(&env->true_reg2))
		return 0;

	/* Both branches are possible, we can't determine which one will be taken. */
	return -1;
}

/*
 * <reg1> <op> <reg2>, currently assuming reg2 is a constant
 */
static int is_scalar_branch_taken(struct bpf_verifier_env *env, struct bpf_reg_state *reg1,
				  struct bpf_reg_state *reg2, u8 opcode, bool is_jmp32)
{
	struct tnum t1 = is_jmp32 ? tnum_subreg(reg1->var_off) : reg1->var_off;
	struct tnum t2 = is_jmp32 ? tnum_subreg(reg2->var_off) : reg2->var_off;
	u64 umin1 = is_jmp32 ? (u64)reg1->u32_min_value : reg1->umin_value;
	u64 umax1 = is_jmp32 ? (u64)reg1->u32_max_value : reg1->umax_value;
	s64 smin1 = is_jmp32 ? (s64)reg1->s32_min_value : reg1->smin_value;
	s64 smax1 = is_jmp32 ? (s64)reg1->s32_max_value : reg1->smax_value;
	u64 umin2 = is_jmp32 ? (u64)reg2->u32_min_value : reg2->umin_value;
	u64 umax2 = is_jmp32 ? (u64)reg2->u32_max_value : reg2->umax_value;
	s64 smin2 = is_jmp32 ? (s64)reg2->s32_min_value : reg2->smin_value;
	s64 smax2 = is_jmp32 ? (s64)reg2->s32_max_value : reg2->smax_value;

	if (reg1 == reg2) {
		switch (opcode) {
		case BPF_JGE:
		case BPF_JLE:
		case BPF_JSGE:
		case BPF_JSLE:
		case BPF_JEQ:
			return 1;
		case BPF_JGT:
		case BPF_JLT:
		case BPF_JSGT:
		case BPF_JSLT:
		case BPF_JNE:
			return 0;
		case BPF_JSET:
			if (tnum_is_const(t1))
				return t1.value != 0;
			else
				return (smin1 <= 0 && smax1 >= 0) ? -1 : 1;
		default:
			return -1;
		}
	}

	switch (opcode) {
	case BPF_JEQ:
		/* constants, umin/umax and smin/smax checks would be
		 * redundant in this case because they all should match
		 */
		if (tnum_is_const(t1) && tnum_is_const(t2))
			return t1.value == t2.value;
		if (!tnum_overlap(t1, t2))
			return 0;
		/* non-overlapping ranges */
		if (umin1 > umax2 || umax1 < umin2)
			return 0;
		if (smin1 > smax2 || smax1 < smin2)
			return 0;
		if (!is_jmp32) {
			/* if 64-bit ranges are inconclusive, see if we can
			 * utilize 32-bit subrange knowledge to eliminate
			 * branches that can't be taken a priori
			 */
			if (reg1->u32_min_value > reg2->u32_max_value ||
			    reg1->u32_max_value < reg2->u32_min_value)
				return 0;
			if (reg1->s32_min_value > reg2->s32_max_value ||
			    reg1->s32_max_value < reg2->s32_min_value)
				return 0;
		}
		break;
	case BPF_JNE:
		/* constants, umin/umax and smin/smax checks would be
		 * redundant in this case because they all should match
		 */
		if (tnum_is_const(t1) && tnum_is_const(t2))
			return t1.value != t2.value;
		if (!tnum_overlap(t1, t2))
			return 1;
		/* non-overlapping ranges */
		if (umin1 > umax2 || umax1 < umin2)
			return 1;
		if (smin1 > smax2 || smax1 < smin2)
			return 1;
		if (!is_jmp32) {
			/* if 64-bit ranges are inconclusive, see if we can
			 * utilize 32-bit subrange knowledge to eliminate
			 * branches that can't be taken a priori
			 */
			if (reg1->u32_min_value > reg2->u32_max_value ||
			    reg1->u32_max_value < reg2->u32_min_value)
				return 1;
			if (reg1->s32_min_value > reg2->s32_max_value ||
			    reg1->s32_max_value < reg2->s32_min_value)
				return 1;
		}
		break;
	case BPF_JSET:
		if (!is_reg_const(reg2, is_jmp32)) {
			swap(reg1, reg2);
			swap(t1, t2);
		}
		if (!is_reg_const(reg2, is_jmp32))
			return -1;
		if ((~t1.mask & t1.value) & t2.value)
			return 1;
		if (!((t1.mask | t1.value) & t2.value))
			return 0;
		break;
	case BPF_JGT:
		if (umin1 > umax2)
			return 1;
		else if (umax1 <= umin2)
			return 0;
		break;
	case BPF_JSGT:
		if (smin1 > smax2)
			return 1;
		else if (smax1 <= smin2)
			return 0;
		break;
	case BPF_JLT:
		if (umax1 < umin2)
			return 1;
		else if (umin1 >= umax2)
			return 0;
		break;
	case BPF_JSLT:
		if (smax1 < smin2)
			return 1;
		else if (smin1 >= smax2)
			return 0;
		break;
	case BPF_JGE:
		if (umin1 >= umax2)
			return 1;
		else if (umax1 < umin2)
			return 0;
		break;
	case BPF_JSGE:
		if (smin1 >= smax2)
			return 1;
		else if (smax1 < smin2)
			return 0;
		break;
	case BPF_JLE:
		if (umax1 <= umin2)
			return 1;
		else if (umin1 > umax2)
			return 0;
		break;
	case BPF_JSLE:
		if (smax1 <= smin2)
			return 1;
		else if (smin1 > smax2)
			return 0;
		break;
	}

	return simulate_both_branches_taken(env, opcode, is_jmp32);
}