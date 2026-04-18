// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 47/132



			max_access = &env->prog->aux->max_rdonly_access;
		} else {
			max_access = &env->prog->aux->max_rdwr_access;
		}
		return check_buffer_access(env, reg, regno, 0,
					   access_size, zero_size_allowed,
					   max_access);
	case PTR_TO_STACK:
		return check_stack_range_initialized(
				env,
				regno, 0, access_size,
				zero_size_allowed, access_type, meta);
	case PTR_TO_BTF_ID:
		return check_ptr_to_btf_access(env, regs, regno, 0,
					       access_size, BPF_READ, -1);
	case PTR_TO_CTX:
		/* Only permit reading or writing syscall context using helper calls. */
		if (is_var_ctx_off_allowed(env->prog)) {
			int err = check_mem_region_access(env, regno, 0, access_size, U16_MAX,
							  zero_size_allowed);
			if (err)
				return err;
			if (env->prog->aux->max_ctx_offset < reg->umax_value + access_size)
				env->prog->aux->max_ctx_offset = reg->umax_value + access_size;
			return 0;
		}
		fallthrough;
	default: /* scalar_value or invalid ptr */
		/* Allow zero-byte read from NULL, regardless of pointer type */
		if (zero_size_allowed && access_size == 0 &&
		    bpf_register_is_null(reg))
			return 0;

		verbose(env, "R%d type=%s ", regno,
			reg_type_str(env, reg->type));
		verbose(env, "expected=%s\n", reg_type_str(env, PTR_TO_STACK));
		return -EACCES;
	}
}

/* verify arguments to helpers or kfuncs consisting of a pointer and an access
 * size.
 *
 * @regno is the register containing the access size. regno-1 is the register
 * containing the pointer.
 */
static int check_mem_size_reg(struct bpf_verifier_env *env,
			      struct bpf_reg_state *reg, u32 regno,
			      enum bpf_access_type access_type,
			      bool zero_size_allowed,
			      struct bpf_call_arg_meta *meta)
{
	int err;

	/* This is used to refine r0 return value bounds for helpers
	 * that enforce this value as an upper bound on return values.
	 * See do_refine_retval_range() for helpers that can refine
	 * the return value. C type of helper is u32 so we pull register
	 * bound from umax_value however, if negative verifier errors
	 * out. Only upper bounds can be learned because retval is an
	 * int type and negative retvals are allowed.
	 */
	meta->msize_max_value = reg->umax_value;

	/* The register is SCALAR_VALUE; the access check happens using
	 * its boundaries. For unprivileged variable accesses, disable
	 * raw mode so that the program is required to initialize all
	 * the memory that the helper could just partially fill up.
	 */
	if (!tnum_is_const(reg->var_off))
		meta = NULL;

	if (reg->smin_value < 0) {
		verbose(env, "R%d min value is negative, either use unsigned or 'var &= const'\n",
			regno);
		return -EACCES;
	}

	if (reg->umin_value == 0 && !zero_size_allowed) {
		verbose(env, "R%d invalid zero-sized read: u64=[%lld,%lld]\n",
			regno, reg->umin_value, reg->umax_value);
		return -EACCES;
	}

	if (reg->umax_value >= BPF_MAX_VAR_SIZ) {
		verbose(env, "R%d unbounded memory access, use 'var &= const' or 'if (var < const)'\n",
			regno);
		return -EACCES;
	}
	err = check_helper_mem_access(env, regno - 1, reg->umax_value,
				      access_type, zero_size_allowed, meta);
	if (!err)
		err = mark_chain_precision(env, regno);
	return err;
}

static int check_mem_reg(struct bpf_verifier_env *env, struct bpf_reg_state *reg,
			 u32 regno, u32 mem_size)
{
	bool may_be_null = type_may_be_null(reg->type);
	struct bpf_reg_state saved_reg;
	int err;

	if (bpf_register_is_null(reg))
		return 0;

	/* Assuming that the register contains a value check if the memory
	 * access is safe. Temporarily save and restore the register's state as
	 * the conversion shouldn't be visible to a caller.
	 */
	if (may_be_null) {
		saved_reg = *reg;
		mark_ptr_not_null_reg(reg);
	}

	int size = base_type(reg->type) == PTR_TO_STACK ? -(int)mem_size : mem_size;

	err = check_helper_mem_access(env, regno, size, BPF_READ, true, NULL);
	err = err ?: check_helper_mem_access(env, regno, size, BPF_WRITE, true, NULL);

	if (may_be_null)
		*reg = saved_reg;

	return err;
}

static int check_kfunc_mem_size_reg(struct bpf_verifier_env *env, struct bpf_reg_state *reg,
				    u32 regno)
{
	struct bpf_reg_state *mem_reg = &cur_regs(env)[regno - 1];
	bool may_be_null = type_may_be_null(mem_reg->type);
	struct bpf_reg_state saved_reg;
	struct bpf_call_arg_meta meta;
	int err;

	WARN_ON_ONCE(regno < BPF_REG_2 || regno > BPF_REG_5);

	memset(&meta, 0, sizeof(meta));

	if (may_be_null) {
		saved_reg = *mem_reg;
		mark_ptr_not_null_reg(mem_reg);
	}

	err = check_mem_size_reg(env, reg, regno, BPF_READ, true, &meta);
	err = err ?: check_mem_size_reg(env, reg, regno, BPF_WRITE, true, &meta);

	if (may_be_null)
		*mem_reg = saved_reg;

	return err;
}

enum {
	PROCESS_SPIN_LOCK = (1 << 0),
	PROCESS_RES_LOCK  = (1 << 1),
	PROCESS_LOCK_IRQ  = (1 << 2),
};