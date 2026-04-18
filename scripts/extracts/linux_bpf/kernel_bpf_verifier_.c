// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 93/132



	/* If either all additions overflow or no additions overflow, then
	 * it is okay to set: dst_umin = dst_umin + src_umin, dst_umax =
	 * dst_umax + src_umax. Otherwise (some additions overflow), set
	 * the output bounds to unbounded.
	 */
	min_overflow = check_add_overflow(*dst_umin, umin_val, dst_umin);
	max_overflow = check_add_overflow(*dst_umax, umax_val, dst_umax);

	if (!min_overflow && max_overflow) {
		*dst_umin = 0;
		*dst_umax = U32_MAX;
	}
}

static void scalar_min_max_add(struct bpf_reg_state *dst_reg,
			       struct bpf_reg_state *src_reg)
{
	s64 *dst_smin = &dst_reg->smin_value;
	s64 *dst_smax = &dst_reg->smax_value;
	u64 *dst_umin = &dst_reg->umin_value;
	u64 *dst_umax = &dst_reg->umax_value;
	u64 umin_val = src_reg->umin_value;
	u64 umax_val = src_reg->umax_value;
	bool min_overflow, max_overflow;

	if (check_add_overflow(*dst_smin, src_reg->smin_value, dst_smin) ||
	    check_add_overflow(*dst_smax, src_reg->smax_value, dst_smax)) {
		*dst_smin = S64_MIN;
		*dst_smax = S64_MAX;
	}

	/* If either all additions overflow or no additions overflow, then
	 * it is okay to set: dst_umin = dst_umin + src_umin, dst_umax =
	 * dst_umax + src_umax. Otherwise (some additions overflow), set
	 * the output bounds to unbounded.
	 */
	min_overflow = check_add_overflow(*dst_umin, umin_val, dst_umin);
	max_overflow = check_add_overflow(*dst_umax, umax_val, dst_umax);

	if (!min_overflow && max_overflow) {
		*dst_umin = 0;
		*dst_umax = U64_MAX;
	}
}

static void scalar32_min_max_sub(struct bpf_reg_state *dst_reg,
				 struct bpf_reg_state *src_reg)
{
	s32 *dst_smin = &dst_reg->s32_min_value;
	s32 *dst_smax = &dst_reg->s32_max_value;
	u32 *dst_umin = &dst_reg->u32_min_value;
	u32 *dst_umax = &dst_reg->u32_max_value;
	u32 umin_val = src_reg->u32_min_value;
	u32 umax_val = src_reg->u32_max_value;
	bool min_underflow, max_underflow;

	if (check_sub_overflow(*dst_smin, src_reg->s32_max_value, dst_smin) ||
	    check_sub_overflow(*dst_smax, src_reg->s32_min_value, dst_smax)) {
		/* Overflow possible, we know nothing */
		*dst_smin = S32_MIN;
		*dst_smax = S32_MAX;
	}

	/* If either all subtractions underflow or no subtractions
	 * underflow, it is okay to set: dst_umin = dst_umin - src_umax,
	 * dst_umax = dst_umax - src_umin. Otherwise (some subtractions
	 * underflow), set the output bounds to unbounded.
	 */
	min_underflow = check_sub_overflow(*dst_umin, umax_val, dst_umin);
	max_underflow = check_sub_overflow(*dst_umax, umin_val, dst_umax);

	if (min_underflow && !max_underflow) {
		*dst_umin = 0;
		*dst_umax = U32_MAX;
	}
}

static void scalar_min_max_sub(struct bpf_reg_state *dst_reg,
			       struct bpf_reg_state *src_reg)
{
	s64 *dst_smin = &dst_reg->smin_value;
	s64 *dst_smax = &dst_reg->smax_value;
	u64 *dst_umin = &dst_reg->umin_value;
	u64 *dst_umax = &dst_reg->umax_value;
	u64 umin_val = src_reg->umin_value;
	u64 umax_val = src_reg->umax_value;
	bool min_underflow, max_underflow;

	if (check_sub_overflow(*dst_smin, src_reg->smax_value, dst_smin) ||
	    check_sub_overflow(*dst_smax, src_reg->smin_value, dst_smax)) {
		/* Overflow possible, we know nothing */
		*dst_smin = S64_MIN;
		*dst_smax = S64_MAX;
	}

	/* If either all subtractions underflow or no subtractions
	 * underflow, it is okay to set: dst_umin = dst_umin - src_umax,
	 * dst_umax = dst_umax - src_umin. Otherwise (some subtractions
	 * underflow), set the output bounds to unbounded.
	 */
	min_underflow = check_sub_overflow(*dst_umin, umax_val, dst_umin);
	max_underflow = check_sub_overflow(*dst_umax, umin_val, dst_umax);

	if (min_underflow && !max_underflow) {
		*dst_umin = 0;
		*dst_umax = U64_MAX;
	}
}

static void scalar32_min_max_mul(struct bpf_reg_state *dst_reg,
				 struct bpf_reg_state *src_reg)
{
	s32 *dst_smin = &dst_reg->s32_min_value;
	s32 *dst_smax = &dst_reg->s32_max_value;
	u32 *dst_umin = &dst_reg->u32_min_value;
	u32 *dst_umax = &dst_reg->u32_max_value;
	s32 tmp_prod[4];

	if (check_mul_overflow(*dst_umax, src_reg->u32_max_value, dst_umax) ||
	    check_mul_overflow(*dst_umin, src_reg->u32_min_value, dst_umin)) {
		/* Overflow possible, we know nothing */
		*dst_umin = 0;
		*dst_umax = U32_MAX;
	}
	if (check_mul_overflow(*dst_smin, src_reg->s32_min_value, &tmp_prod[0]) ||
	    check_mul_overflow(*dst_smin, src_reg->s32_max_value, &tmp_prod[1]) ||
	    check_mul_overflow(*dst_smax, src_reg->s32_min_value, &tmp_prod[2]) ||
	    check_mul_overflow(*dst_smax, src_reg->s32_max_value, &tmp_prod[3])) {
		/* Overflow possible, we know nothing */
		*dst_smin = S32_MIN;
		*dst_smax = S32_MAX;
	} else {
		*dst_smin = min_array(tmp_prod, 4);
		*dst_smax = max_array(tmp_prod, 4);
	}
}

static void scalar_min_max_mul(struct bpf_reg_state *dst_reg,
			       struct bpf_reg_state *src_reg)
{
	s64 *dst_smin = &dst_reg->smin_value;
	s64 *dst_smax = &dst_reg->smax_value;
	u64 *dst_umin = &dst_reg->umin_value;
	u64 *dst_umax = &dst_reg->umax_value;
	s64 tmp_prod[4];