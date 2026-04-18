// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 92/132



	switch (opcode) {
	case BPF_ADD:
		/*
		 * dst_reg gets the pointer type and since some positive
		 * integer value was added to the pointer, give it a new 'id'
		 * if it's a PTR_TO_PACKET.
		 * this creates a new 'base' pointer, off_reg (variable) gets
		 * added into the variable offset, and we copy the fixed offset
		 * from ptr_reg.
		 */
		if (check_add_overflow(smin_ptr, smin_val, &dst_reg->smin_value) ||
		    check_add_overflow(smax_ptr, smax_val, &dst_reg->smax_value)) {
			dst_reg->smin_value = S64_MIN;
			dst_reg->smax_value = S64_MAX;
		}
		if (check_add_overflow(umin_ptr, umin_val, &dst_reg->umin_value) ||
		    check_add_overflow(umax_ptr, umax_val, &dst_reg->umax_value)) {
			dst_reg->umin_value = 0;
			dst_reg->umax_value = U64_MAX;
		}
		dst_reg->var_off = tnum_add(ptr_reg->var_off, off_reg->var_off);
		dst_reg->raw = ptr_reg->raw;
		if (reg_is_pkt_pointer(ptr_reg)) {
			if (!known)
				dst_reg->id = ++env->id_gen;
			/*
			 * Clear range for unknown addends since we can't know
			 * where the pkt pointer ended up. Also clear AT_PKT_END /
			 * BEYOND_PKT_END from prior comparison as any pointer
			 * arithmetic invalidates them.
			 */
			if (!known || dst_reg->range < 0)
				memset(&dst_reg->raw, 0, sizeof(dst_reg->raw));
		}
		break;
	case BPF_SUB:
		if (dst_reg == off_reg) {
			/* scalar -= pointer.  Creates an unknown scalar */
			verbose(env, "R%d tried to subtract pointer from scalar\n",
				dst);
			return -EACCES;
		}
		/* We don't allow subtraction from FP, because (according to
		 * test_verifier.c test "invalid fp arithmetic", JITs might not
		 * be able to deal with it.
		 */
		if (ptr_reg->type == PTR_TO_STACK) {
			verbose(env, "R%d subtraction from stack pointer prohibited\n",
				dst);
			return -EACCES;
		}
		/* A new variable offset is created.  If the subtrahend is known
		 * nonnegative, then any reg->range we had before is still good.
		 */
		if (check_sub_overflow(smin_ptr, smax_val, &dst_reg->smin_value) ||
		    check_sub_overflow(smax_ptr, smin_val, &dst_reg->smax_value)) {
			/* Overflow possible, we know nothing */
			dst_reg->smin_value = S64_MIN;
			dst_reg->smax_value = S64_MAX;
		}
		if (umin_ptr < umax_val) {
			/* Overflow possible, we know nothing */
			dst_reg->umin_value = 0;
			dst_reg->umax_value = U64_MAX;
		} else {
			/* Cannot overflow (as long as bounds are consistent) */
			dst_reg->umin_value = umin_ptr - umax_val;
			dst_reg->umax_value = umax_ptr - umin_val;
		}
		dst_reg->var_off = tnum_sub(ptr_reg->var_off, off_reg->var_off);
		dst_reg->raw = ptr_reg->raw;
		if (reg_is_pkt_pointer(ptr_reg)) {
			if (!known)
				dst_reg->id = ++env->id_gen;
			/*
			 * Clear range if the subtrahend may be negative since
			 * pkt pointer could move past its bounds. A positive
			 * subtrahend moves it backwards keeping positive range
			 * intact. Also clear AT_PKT_END / BEYOND_PKT_END from
			 * prior comparison as arithmetic invalidates them.
			 */
			if ((!known && smin_val < 0) || dst_reg->range < 0)
				memset(&dst_reg->raw, 0, sizeof(dst_reg->raw));
		}
		break;
	case BPF_AND:
	case BPF_OR:
	case BPF_XOR:
		/* bitwise ops on pointers are troublesome, prohibit. */
		verbose(env, "R%d bitwise operator %s on pointer prohibited\n",
			dst, bpf_alu_string[opcode >> 4]);
		return -EACCES;
	default:
		/* other operators (e.g. MUL,LSH) produce non-pointer results */
		verbose(env, "R%d pointer arithmetic with %s operator prohibited\n",
			dst, bpf_alu_string[opcode >> 4]);
		return -EACCES;
	}

	if (!check_reg_sane_offset_ptr(env, dst_reg, ptr_reg->type))
		return -EINVAL;
	reg_bounds_sync(dst_reg);
	bounds_ret = sanitize_check_bounds(env, insn, dst_reg);
	if (bounds_ret == -EACCES)
		return bounds_ret;
	if (sanitize_needed(opcode)) {
		ret = sanitize_ptr_alu(env, insn, dst_reg, off_reg, dst_reg,
				       &info, true);
		if (verifier_bug_if(!can_skip_alu_sanitation(env, insn)
				    && !env->cur_state->speculative
				    && bounds_ret
				    && !ret,
				    env, "Pointer type unsupported by sanitize_check_bounds() not rejected by retrieve_ptr_limit() as required")) {
			return -EFAULT;
		}
		if (ret < 0)
			return sanitize_err(env, insn, ret, off_reg, dst_reg);
	}

	return 0;
}

static void scalar32_min_max_add(struct bpf_reg_state *dst_reg,
				 struct bpf_reg_state *src_reg)
{
	s32 *dst_smin = &dst_reg->s32_min_value;
	s32 *dst_smax = &dst_reg->s32_max_value;
	u32 *dst_umin = &dst_reg->u32_min_value;
	u32 *dst_umax = &dst_reg->u32_max_value;
	u32 umin_val = src_reg->u32_min_value;
	u32 umax_val = src_reg->u32_max_value;
	bool min_overflow, max_overflow;

	if (check_add_overflow(*dst_smin, src_reg->s32_min_value, dst_smin) ||
	    check_add_overflow(*dst_smax, src_reg->s32_max_value, dst_smax)) {
		*dst_smin = S32_MIN;
		*dst_smax = S32_MAX;
	}