// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 105/132



			t = tnum_intersect(tnum_subreg(reg1->var_off), tnum_subreg(reg2->var_off));
			reg1->var_off = tnum_with_subreg(reg1->var_off, t);
			reg2->var_off = tnum_with_subreg(reg2->var_off, t);
		} else {
			reg1->umin_value = max(reg1->umin_value, reg2->umin_value);
			reg1->umax_value = min(reg1->umax_value, reg2->umax_value);
			reg1->smin_value = max(reg1->smin_value, reg2->smin_value);
			reg1->smax_value = min(reg1->smax_value, reg2->smax_value);
			reg2->umin_value = reg1->umin_value;
			reg2->umax_value = reg1->umax_value;
			reg2->smin_value = reg1->smin_value;
			reg2->smax_value = reg1->smax_value;

			reg1->var_off = tnum_intersect(reg1->var_off, reg2->var_off);
			reg2->var_off = reg1->var_off;
		}
		break;
	case BPF_JNE:
		if (!is_reg_const(reg2, is_jmp32))
			swap(reg1, reg2);
		if (!is_reg_const(reg2, is_jmp32))
			break;

		/* try to recompute the bound of reg1 if reg2 is a const and
		 * is exactly the edge of reg1.
		 */
		val = reg_const_value(reg2, is_jmp32);
		if (is_jmp32) {
			/* u32_min_value is not equal to 0xffffffff at this point,
			 * because otherwise u32_max_value is 0xffffffff as well,
			 * in such a case both reg1 and reg2 would be constants,
			 * jump would be predicted and regs_refine_cond_op()
			 * wouldn't be called.
			 *
			 * Same reasoning works for all {u,s}{min,max}{32,64} cases
			 * below.
			 */
			if (reg1->u32_min_value == (u32)val)
				reg1->u32_min_value++;
			if (reg1->u32_max_value == (u32)val)
				reg1->u32_max_value--;
			if (reg1->s32_min_value == (s32)val)
				reg1->s32_min_value++;
			if (reg1->s32_max_value == (s32)val)
				reg1->s32_max_value--;
		} else {
			if (reg1->umin_value == (u64)val)
				reg1->umin_value++;
			if (reg1->umax_value == (u64)val)
				reg1->umax_value--;
			if (reg1->smin_value == (s64)val)
				reg1->smin_value++;
			if (reg1->smax_value == (s64)val)
				reg1->smax_value--;
		}
		break;
	case BPF_JSET:
		if (!is_reg_const(reg2, is_jmp32))
			swap(reg1, reg2);
		if (!is_reg_const(reg2, is_jmp32))
			break;
		val = reg_const_value(reg2, is_jmp32);
		/* BPF_JSET (i.e., TRUE branch, *not* BPF_JSET | BPF_X)
		 * requires single bit to learn something useful. E.g., if we
		 * know that `r1 & 0x3` is true, then which bits (0, 1, or both)
		 * are actually set? We can learn something definite only if
		 * it's a single-bit value to begin with.
		 *
		 * BPF_JSET | BPF_X (i.e., negation of BPF_JSET) doesn't have
		 * this restriction. I.e., !(r1 & 0x3) means neither bit 0 nor
		 * bit 1 is set, which we can readily use in adjustments.
		 */
		if (!is_power_of_2(val))
			break;
		if (is_jmp32) {
			t = tnum_or(tnum_subreg(reg1->var_off), tnum_const(val));
			reg1->var_off = tnum_with_subreg(reg1->var_off, t);
		} else {
			reg1->var_off = tnum_or(reg1->var_off, tnum_const(val));
		}
		break;
	case BPF_JSET | BPF_X: /* reverse of BPF_JSET, see rev_opcode() */
		if (!is_reg_const(reg2, is_jmp32))
			swap(reg1, reg2);
		if (!is_reg_const(reg2, is_jmp32))
			break;
		val = reg_const_value(reg2, is_jmp32);
		/* Forget the ranges before narrowing tnums, to avoid invariant
		 * violations if we're on a dead branch.
		 */
		__mark_reg_unbounded(reg1);
		if (is_jmp32) {
			t = tnum_and(tnum_subreg(reg1->var_off), tnum_const(~val));
			reg1->var_off = tnum_with_subreg(reg1->var_off, t);
		} else {
			reg1->var_off = tnum_and(reg1->var_off, tnum_const(~val));
		}
		break;
	case BPF_JLE:
		if (is_jmp32) {
			reg1->u32_max_value = min(reg1->u32_max_value, reg2->u32_max_value);
			reg2->u32_min_value = max(reg1->u32_min_value, reg2->u32_min_value);
		} else {
			reg1->umax_value = min(reg1->umax_value, reg2->umax_value);
			reg2->umin_value = max(reg1->umin_value, reg2->umin_value);
		}
		break;
	case BPF_JLT:
		if (is_jmp32) {
			reg1->u32_max_value = min(reg1->u32_max_value, reg2->u32_max_value - 1);
			reg2->u32_min_value = max(reg1->u32_min_value + 1, reg2->u32_min_value);
		} else {
			reg1->umax_value = min(reg1->umax_value, reg2->umax_value - 1);
			reg2->umin_value = max(reg1->umin_value + 1, reg2->umin_value);
		}
		break;
	case BPF_JSLE:
		if (is_jmp32) {
			reg1->s32_max_value = min(reg1->s32_max_value, reg2->s32_max_value);
			reg2->s32_min_value = max(reg1->s32_min_value, reg2->s32_min_value);
		} else {
			reg1->smax_value = min(reg1->smax_value, reg2->smax_value);
			reg2->smin_value = max(reg1->smin_value, reg2->smin_value);
		}
		break;
	case BPF_JSLT:
		if (is_jmp32) {
			reg1->s32_max_value = min(reg1->s32_max_value, reg2->s32_max_value - 1);
			reg2->s32_min_value = max(reg1->s32_min_value + 1, reg2->s32_min_value);
		} else {
			reg1->smax_value = min(reg1->smax_value, reg2->smax_value - 1);
			reg2->smin_value = max(reg1->smin_value + 1, reg2->smin_value);
		}
		break;
	default:
		return;
	}
}

/* Check for invariant violations on the registers for both branches of a condition */
static int regs_bounds_sanity_check_branches(struct bpf_verifier_env *env)
{
	int err;