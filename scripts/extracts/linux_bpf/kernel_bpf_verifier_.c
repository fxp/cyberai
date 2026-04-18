// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 95/132



	*dst_umin = 0;
	*dst_umax = min(*dst_umax, res_max);

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->smin_value = S64_MIN;
	dst_reg->smax_value = S64_MAX;
	reset_reg32_and_tnum(dst_reg);
}

static void scalar32_min_max_smod(struct bpf_reg_state *dst_reg,
				  struct bpf_reg_state *src_reg)
{
	s32 *dst_smin = &dst_reg->s32_min_value;
	s32 *dst_smax = &dst_reg->s32_max_value;
	s32 src_val = src_reg->s32_min_value; /* non-zero, const divisor */

	/*
	 * Safe absolute value calculation:
	 * If src_val == S32_MIN (-2147483648), src_abs becomes 2147483648.
	 * Here use unsigned integer to avoid overflow.
	 */
	u32 src_abs = (src_val > 0) ? (u32)src_val : -(u32)src_val;

	/*
	 * Calculate the maximum possible absolute value of the result.
	 * Even if src_abs is 2147483648 (S32_MIN), subtracting 1 gives
	 * 2147483647 (S32_MAX), which fits perfectly in s32.
	 */
	s32 res_max_abs = src_abs - 1;

	/*
	 * If the dividend is already within the result range,
	 * the result remains unchanged. e.g., [-2, 5] % 10 = [-2, 5].
	 */
	if (*dst_smin >= -res_max_abs && *dst_smax <= res_max_abs)
		return;

	/* General case: result has the same sign as the dividend. */
	if (*dst_smin >= 0) {
		*dst_smin = 0;
		*dst_smax = min(*dst_smax, res_max_abs);
	} else if (*dst_smax <= 0) {
		*dst_smax = 0;
		*dst_smin = max(*dst_smin, -res_max_abs);
	} else {
		*dst_smin = -res_max_abs;
		*dst_smax = res_max_abs;
	}

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->u32_min_value = 0;
	dst_reg->u32_max_value = U32_MAX;
	reset_reg64_and_tnum(dst_reg);
}

static void scalar_min_max_smod(struct bpf_reg_state *dst_reg,
				struct bpf_reg_state *src_reg)
{
	s64 *dst_smin = &dst_reg->smin_value;
	s64 *dst_smax = &dst_reg->smax_value;
	s64 src_val = src_reg->smin_value; /* non-zero, const divisor */

	/*
	 * Safe absolute value calculation:
	 * If src_val == S64_MIN (-2^63), src_abs becomes 2^63.
	 * Here use unsigned integer to avoid overflow.
	 */
	u64 src_abs = (src_val > 0) ? (u64)src_val : -(u64)src_val;

	/*
	 * Calculate the maximum possible absolute value of the result.
	 * Even if src_abs is 2^63 (S64_MIN), subtracting 1 gives
	 * 2^63 - 1 (S64_MAX), which fits perfectly in s64.
	 */
	s64 res_max_abs = src_abs - 1;

	/*
	 * If the dividend is already within the result range,
	 * the result remains unchanged. e.g., [-2, 5] % 10 = [-2, 5].
	 */
	if (*dst_smin >= -res_max_abs && *dst_smax <= res_max_abs)
		return;

	/* General case: result has the same sign as the dividend. */
	if (*dst_smin >= 0) {
		*dst_smin = 0;
		*dst_smax = min(*dst_smax, res_max_abs);
	} else if (*dst_smax <= 0) {
		*dst_smax = 0;
		*dst_smin = max(*dst_smin, -res_max_abs);
	} else {
		*dst_smin = -res_max_abs;
		*dst_smax = res_max_abs;
	}

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->umin_value = 0;
	dst_reg->umax_value = U64_MAX;
	reset_reg32_and_tnum(dst_reg);
}

static void scalar32_min_max_and(struct bpf_reg_state *dst_reg,
				 struct bpf_reg_state *src_reg)
{
	bool src_known = tnum_subreg_is_const(src_reg->var_off);
	bool dst_known = tnum_subreg_is_const(dst_reg->var_off);
	struct tnum var32_off = tnum_subreg(dst_reg->var_off);
	u32 umax_val = src_reg->u32_max_value;

	if (src_known && dst_known) {
		__mark_reg32_known(dst_reg, var32_off.value);
		return;
	}

	/* We get our minimum from the var_off, since that's inherently
	 * bitwise.  Our maximum is the minimum of the operands' maxima.
	 */
	dst_reg->u32_min_value = var32_off.value;
	dst_reg->u32_max_value = min(dst_reg->u32_max_value, umax_val);

	/* Safe to set s32 bounds by casting u32 result into s32 when u32
	 * doesn't cross sign boundary. Otherwise set s32 bounds to unbounded.
	 */
	if ((s32)dst_reg->u32_min_value <= (s32)dst_reg->u32_max_value) {
		dst_reg->s32_min_value = dst_reg->u32_min_value;
		dst_reg->s32_max_value = dst_reg->u32_max_value;
	} else {
		dst_reg->s32_min_value = S32_MIN;
		dst_reg->s32_max_value = S32_MAX;
	}
}

static void scalar_min_max_and(struct bpf_reg_state *dst_reg,
			       struct bpf_reg_state *src_reg)
{
	bool src_known = tnum_is_const(src_reg->var_off);
	bool dst_known = tnum_is_const(dst_reg->var_off);
	u64 umax_val = src_reg->umax_value;

	if (src_known && dst_known) {
		__mark_reg_known(dst_reg, dst_reg->var_off.value);
		return;
	}

	/* We get our minimum from the var_off, since that's inherently
	 * bitwise.  Our maximum is the minimum of the operands' maxima.
	 */
	dst_reg->umin_value = dst_reg->var_off.value;
	dst_reg->umax_value = min(dst_reg->umax_value, umax_val);