// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 31/132



	/* We may have adjusted the register pointing to memory region, so we
	 * need to try adding each of min_value and max_value to off
	 * to make sure our theoretical access will be safe.
	 *
	 * The minimum value is only important with signed
	 * comparisons where we can't assume the floor of a
	 * value is 0.  If we are using signed variables for our
	 * index'es we need to make sure that whatever we use
	 * will have a set floor within our range.
	 */
	if (reg->smin_value < 0 &&
	    (reg->smin_value == S64_MIN ||
	     (off + reg->smin_value != (s64)(s32)(off + reg->smin_value)) ||
	      reg->smin_value + off < 0)) {
		verbose(env, "R%d min value is negative, either use unsigned index or do a if (index >=0) check.\n",
			regno);
		return -EACCES;
	}
	err = __check_mem_access(env, regno, reg->smin_value + off, size,
				 mem_size, zero_size_allowed);
	if (err) {
		verbose(env, "R%d min value is outside of the allowed memory range\n",
			regno);
		return err;
	}

	/* If we haven't set a max value then we need to bail since we can't be
	 * sure we won't do bad things.
	 * If reg->umax_value + off could overflow, treat that as unbounded too.
	 */
	if (reg->umax_value >= BPF_MAX_VAR_OFF) {
		verbose(env, "R%d unbounded memory access, make sure to bounds check any such access\n",
			regno);
		return -EACCES;
	}
	err = __check_mem_access(env, regno, reg->umax_value + off, size,
				 mem_size, zero_size_allowed);
	if (err) {
		verbose(env, "R%d max value is outside of the allowed memory range\n",
			regno);
		return err;
	}

	return 0;
}

static int __check_ptr_off_reg(struct bpf_verifier_env *env,
			       const struct bpf_reg_state *reg, int regno,
			       bool fixed_off_ok)
{
	/* Access to this pointer-typed register or passing it to a helper
	 * is only allowed in its original, unmodified form.
	 */

	if (!tnum_is_const(reg->var_off)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
		verbose(env, "variable %s access var_off=%s disallowed\n",
			reg_type_str(env, reg->type), tn_buf);
		return -EACCES;
	}

	if (reg->smin_value < 0) {
		verbose(env, "negative offset %s ptr R%d off=%lld disallowed\n",
			reg_type_str(env, reg->type), regno, reg->var_off.value);
		return -EACCES;
	}

	if (!fixed_off_ok && reg->var_off.value != 0) {
		verbose(env, "dereference of modified %s ptr R%d off=%lld disallowed\n",
			reg_type_str(env, reg->type), regno, reg->var_off.value);
		return -EACCES;
	}

	return 0;
}

static int check_ptr_off_reg(struct bpf_verifier_env *env,
		             const struct bpf_reg_state *reg, int regno)
{
	return __check_ptr_off_reg(env, reg, regno, false);
}

static int map_kptr_match_type(struct bpf_verifier_env *env,
			       struct btf_field *kptr_field,
			       struct bpf_reg_state *reg, u32 regno)
{
	const char *targ_name = btf_type_name(kptr_field->kptr.btf, kptr_field->kptr.btf_id);
	int perm_flags;
	const char *reg_name = "";

	if (base_type(reg->type) != PTR_TO_BTF_ID)
		goto bad_type;

	if (btf_is_kernel(reg->btf)) {
		perm_flags = PTR_MAYBE_NULL | PTR_TRUSTED | MEM_RCU;

		/* Only unreferenced case accepts untrusted pointers */
		if (kptr_field->type == BPF_KPTR_UNREF)
			perm_flags |= PTR_UNTRUSTED;
	} else {
		perm_flags = PTR_MAYBE_NULL | MEM_ALLOC;
		if (kptr_field->type == BPF_KPTR_PERCPU)
			perm_flags |= MEM_PERCPU;
	}

	if (type_flag(reg->type) & ~perm_flags)
		goto bad_type;

	/* We need to verify reg->type and reg->btf, before accessing reg->btf */
	reg_name = btf_type_name(reg->btf, reg->btf_id);

	/* For ref_ptr case, release function check should ensure we get one
	 * referenced PTR_TO_BTF_ID, and that its fixed offset is 0. For the
	 * normal store of unreferenced kptr, we must ensure var_off is zero.
	 * Since ref_ptr cannot be accessed directly by BPF insns, check for
	 * reg->ref_obj_id is not needed here.
	 */
	if (__check_ptr_off_reg(env, reg, regno, true))
		return -EACCES;