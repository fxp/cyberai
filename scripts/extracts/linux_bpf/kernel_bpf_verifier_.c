// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 94/132



	if (check_mul_overflow(*dst_umax, src_reg->umax_value, dst_umax) ||
	    check_mul_overflow(*dst_umin, src_reg->umin_value, dst_umin)) {
		/* Overflow possible, we know nothing */
		*dst_umin = 0;
		*dst_umax = U64_MAX;
	}
	if (check_mul_overflow(*dst_smin, src_reg->smin_value, &tmp_prod[0]) ||
	    check_mul_overflow(*dst_smin, src_reg->smax_value, &tmp_prod[1]) ||
	    check_mul_overflow(*dst_smax, src_reg->smin_value, &tmp_prod[2]) ||
	    check_mul_overflow(*dst_smax, src_reg->smax_value, &tmp_prod[3])) {
		/* Overflow possible, we know nothing */
		*dst_smin = S64_MIN;
		*dst_smax = S64_MAX;
	} else {
		*dst_smin = min_array(tmp_prod, 4);
		*dst_smax = max_array(tmp_prod, 4);
	}
}

static void scalar32_min_max_udiv(struct bpf_reg_state *dst_reg,
				  struct bpf_reg_state *src_reg)
{
	u32 *dst_umin = &dst_reg->u32_min_value;
	u32 *dst_umax = &dst_reg->u32_max_value;
	u32 src_val = src_reg->u32_min_value; /* non-zero, const divisor */

	*dst_umin = *dst_umin / src_val;
	*dst_umax = *dst_umax / src_val;

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->s32_min_value = S32_MIN;
	dst_reg->s32_max_value = S32_MAX;
	reset_reg64_and_tnum(dst_reg);
}

static void scalar_min_max_udiv(struct bpf_reg_state *dst_reg,
				struct bpf_reg_state *src_reg)
{
	u64 *dst_umin = &dst_reg->umin_value;
	u64 *dst_umax = &dst_reg->umax_value;
	u64 src_val = src_reg->umin_value; /* non-zero, const divisor */

	*dst_umin = div64_u64(*dst_umin, src_val);
	*dst_umax = div64_u64(*dst_umax, src_val);

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->smin_value = S64_MIN;
	dst_reg->smax_value = S64_MAX;
	reset_reg32_and_tnum(dst_reg);
}

static void scalar32_min_max_sdiv(struct bpf_reg_state *dst_reg,
				  struct bpf_reg_state *src_reg)
{
	s32 *dst_smin = &dst_reg->s32_min_value;
	s32 *dst_smax = &dst_reg->s32_max_value;
	s32 src_val = src_reg->s32_min_value; /* non-zero, const divisor */
	s32 res1, res2;

	/* BPF div specification: S32_MIN / -1 = S32_MIN */
	if (*dst_smin == S32_MIN && src_val == -1) {
		/*
		 * If the dividend range contains more than just S32_MIN,
		 * we cannot precisely track the result, so it becomes unbounded.
		 * e.g., [S32_MIN, S32_MIN+10]/(-1),
		 *     = {S32_MIN} U [-(S32_MIN+10), -(S32_MIN+1)]
		 *     = {S32_MIN} U [S32_MAX-9, S32_MAX] = [S32_MIN, S32_MAX]
		 * Otherwise (if dividend is exactly S32_MIN), result remains S32_MIN.
		 */
		if (*dst_smax != S32_MIN) {
			*dst_smin = S32_MIN;
			*dst_smax = S32_MAX;
		}
		goto reset;
	}

	res1 = *dst_smin / src_val;
	res2 = *dst_smax / src_val;
	*dst_smin = min(res1, res2);
	*dst_smax = max(res1, res2);

reset:
	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->u32_min_value = 0;
	dst_reg->u32_max_value = U32_MAX;
	reset_reg64_and_tnum(dst_reg);
}

static void scalar_min_max_sdiv(struct bpf_reg_state *dst_reg,
				struct bpf_reg_state *src_reg)
{
	s64 *dst_smin = &dst_reg->smin_value;
	s64 *dst_smax = &dst_reg->smax_value;
	s64 src_val = src_reg->smin_value; /* non-zero, const divisor */
	s64 res1, res2;

	/* BPF div specification: S64_MIN / -1 = S64_MIN */
	if (*dst_smin == S64_MIN && src_val == -1) {
		/*
		 * If the dividend range contains more than just S64_MIN,
		 * we cannot precisely track the result, so it becomes unbounded.
		 * e.g., [S64_MIN, S64_MIN+10]/(-1),
		 *     = {S64_MIN} U [-(S64_MIN+10), -(S64_MIN+1)]
		 *     = {S64_MIN} U [S64_MAX-9, S64_MAX] = [S64_MIN, S64_MAX]
		 * Otherwise (if dividend is exactly S64_MIN), result remains S64_MIN.
		 */
		if (*dst_smax != S64_MIN) {
			*dst_smin = S64_MIN;
			*dst_smax = S64_MAX;
		}
		goto reset;
	}

	res1 = div64_s64(*dst_smin, src_val);
	res2 = div64_s64(*dst_smax, src_val);
	*dst_smin = min(res1, res2);
	*dst_smax = max(res1, res2);

reset:
	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->umin_value = 0;
	dst_reg->umax_value = U64_MAX;
	reset_reg32_and_tnum(dst_reg);
}

static void scalar32_min_max_umod(struct bpf_reg_state *dst_reg,
				  struct bpf_reg_state *src_reg)
{
	u32 *dst_umin = &dst_reg->u32_min_value;
	u32 *dst_umax = &dst_reg->u32_max_value;
	u32 src_val = src_reg->u32_min_value; /* non-zero, const divisor */
	u32 res_max = src_val - 1;

	/*
	 * If dst_umax <= res_max, the result remains unchanged.
	 * e.g., [2, 5] % 10 = [2, 5].
	 */
	if (*dst_umax <= res_max)
		return;

	*dst_umin = 0;
	*dst_umax = min(*dst_umax, res_max);

	/* Reset other ranges/tnum to unbounded/unknown. */
	dst_reg->s32_min_value = S32_MIN;
	dst_reg->s32_max_value = S32_MAX;
	reset_reg64_and_tnum(dst_reg);
}

static void scalar_min_max_umod(struct bpf_reg_state *dst_reg,
				struct bpf_reg_state *src_reg)
{
	u64 *dst_umin = &dst_reg->umin_value;
	u64 *dst_umax = &dst_reg->umax_value;
	u64 src_val = src_reg->umin_value; /* non-zero, const divisor */
	u64 res_max = src_val - 1;

	/*
	 * If dst_umax <= res_max, the result remains unchanged.
	 * e.g., [2, 5] % 10 = [2, 5].
	 */
	if (*dst_umax <= res_max)
		return;