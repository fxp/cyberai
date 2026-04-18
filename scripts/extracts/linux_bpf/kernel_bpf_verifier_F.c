// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 38/132



static int __check_buffer_access(struct bpf_verifier_env *env,
				 const char *buf_info,
				 const struct bpf_reg_state *reg,
				 int regno, int off, int size)
{
	if (off < 0) {
		verbose(env,
			"R%d invalid %s buffer access: off=%d, size=%d\n",
			regno, buf_info, off, size);
		return -EACCES;
	}
	if (!tnum_is_const(reg->var_off)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
		verbose(env,
			"R%d invalid variable buffer offset: off=%d, var_off=%s\n",
			regno, off, tn_buf);
		return -EACCES;
	}

	return 0;
}

static int check_tp_buffer_access(struct bpf_verifier_env *env,
				  const struct bpf_reg_state *reg,
				  int regno, int off, int size)
{
	int err;

	err = __check_buffer_access(env, "tracepoint", reg, regno, off, size);
	if (err)
		return err;

	env->prog->aux->max_tp_access = max(reg->var_off.value + off + size,
					    env->prog->aux->max_tp_access);

	return 0;
}

static int check_buffer_access(struct bpf_verifier_env *env,
			       const struct bpf_reg_state *reg,
			       int regno, int off, int size,
			       bool zero_size_allowed,
			       u32 *max_access)
{
	const char *buf_info = type_is_rdonly_mem(reg->type) ? "rdonly" : "rdwr";
	int err;

	err = __check_buffer_access(env, buf_info, reg, regno, off, size);
	if (err)
		return err;

	*max_access = max(reg->var_off.value + off + size, *max_access);

	return 0;
}

/* BPF architecture zero extends alu32 ops into 64-bit registesr */
static void zext_32_to_64(struct bpf_reg_state *reg)
{
	reg->var_off = tnum_subreg(reg->var_off);
	__reg_assign_32_into_64(reg);
}

/* truncate register to smaller size (in bytes)
 * must be called with size < BPF_REG_SIZE
 */
static void coerce_reg_to_size(struct bpf_reg_state *reg, int size)
{
	u64 mask;

	/* clear high bits in bit representation */
	reg->var_off = tnum_cast(reg->var_off, size);

	/* fix arithmetic bounds */
	mask = ((u64)1 << (size * 8)) - 1;
	if ((reg->umin_value & ~mask) == (reg->umax_value & ~mask)) {
		reg->umin_value &= mask;
		reg->umax_value &= mask;
	} else {
		reg->umin_value = 0;
		reg->umax_value = mask;
	}
	reg->smin_value = reg->umin_value;
	reg->smax_value = reg->umax_value;

	/* If size is smaller than 32bit register the 32bit register
	 * values are also truncated so we push 64-bit bounds into
	 * 32-bit bounds. Above were truncated < 32-bits already.
	 */
	if (size < 4)
		__mark_reg32_unbounded(reg);

	reg_bounds_sync(reg);
}

static void set_sext64_default_val(struct bpf_reg_state *reg, int size)
{
	if (size == 1) {
		reg->smin_value = reg->s32_min_value = S8_MIN;
		reg->smax_value = reg->s32_max_value = S8_MAX;
	} else if (size == 2) {
		reg->smin_value = reg->s32_min_value = S16_MIN;
		reg->smax_value = reg->s32_max_value = S16_MAX;
	} else {
		/* size == 4 */
		reg->smin_value = reg->s32_min_value = S32_MIN;
		reg->smax_value = reg->s32_max_value = S32_MAX;
	}
	reg->umin_value = reg->u32_min_value = 0;
	reg->umax_value = U64_MAX;
	reg->u32_max_value = U32_MAX;
	reg->var_off = tnum_unknown;
}

static void coerce_reg_to_size_sx(struct bpf_reg_state *reg, int size)
{
	s64 init_s64_max, init_s64_min, s64_max, s64_min, u64_cval;
	u64 top_smax_value, top_smin_value;
	u64 num_bits = size * 8;

	if (tnum_is_const(reg->var_off)) {
		u64_cval = reg->var_off.value;
		if (size == 1)
			reg->var_off = tnum_const((s8)u64_cval);
		else if (size == 2)
			reg->var_off = tnum_const((s16)u64_cval);
		else
			/* size == 4 */
			reg->var_off = tnum_const((s32)u64_cval);

		u64_cval = reg->var_off.value;
		reg->smax_value = reg->smin_value = u64_cval;
		reg->umax_value = reg->umin_value = u64_cval;
		reg->s32_max_value = reg->s32_min_value = u64_cval;
		reg->u32_max_value = reg->u32_min_value = u64_cval;
		return;
	}

	top_smax_value = ((u64)reg->smax_value >> num_bits) << num_bits;
	top_smin_value = ((u64)reg->smin_value >> num_bits) << num_bits;

	if (top_smax_value != top_smin_value)
		goto out;

	/* find the s64_min and s64_min after sign extension */
	if (size == 1) {
		init_s64_max = (s8)reg->smax_value;
		init_s64_min = (s8)reg->smin_value;
	} else if (size == 2) {
		init_s64_max = (s16)reg->smax_value;
		init_s64_min = (s16)reg->smin_value;
	} else {
		init_s64_max = (s32)reg->smax_value;
		init_s64_min = (s32)reg->smin_value;
	}

	s64_max = max(init_s64_max, init_s64_min);
	s64_min = min(init_s64_max, init_s64_min);

	/* both of s64_max/s64_min positive or negative */
	if ((s64_max >= 0) == (s64_min >= 0)) {
		reg->s32_min_value = reg->smin_value = s64_min;
		reg->s32_max_value = reg->smax_value = s64_max;
		reg->u32_min_value = reg->umin_value = s64_min;
		reg->u32_max_value = reg->umax_value = s64_max;
		reg->var_off = tnum_range(s64_min, s64_max);
		return;
	}

out:
	set_sext64_default_val(reg, size);
}