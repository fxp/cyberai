// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 112/132



	switch (prog_type) {
	case BPF_PROG_TYPE_LSM:
		/* See return_retval_range, for BPF_LSM_CGROUP can be 0 or 0-1 depending on hook. */
		if (prog->expected_attach_type != BPF_LSM_CGROUP &&
		    !prog->aux->attach_func_proto->type)
			return true;
		break;
	case BPF_PROG_TYPE_STRUCT_OPS:
		if (!prog->aux->attach_func_proto->type)
			return true;
		break;
	case BPF_PROG_TYPE_EXT:
		/*
		 * If the actual program is an extension, let it
		 * return void - attaching will succeed only if the
		 * program being replaced also returns void, and since
		 * it has passed verification its actual type doesn't matter.
		 */
		if (subprog_returns_void(env, 0))
			return true;
		break;
	default:
		break;
	}
	return false;
}

static int check_return_code(struct bpf_verifier_env *env, int regno, const char *reg_name)
{
	const char *exit_ctx = "At program exit";
	struct tnum enforce_attach_type_range = tnum_unknown;
	const struct bpf_prog *prog = env->prog;
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct bpf_retval_range range = retval_range(0, 1);
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);
	struct bpf_func_state *frame = env->cur_state->frame[0];
	const struct btf_type *reg_type, *ret_type = NULL;
	int err;

	/* LSM and struct_ops func-ptr's return type could be "void" */
	if (!frame->in_async_callback_fn && program_returns_void(env))
		return 0;

	if (prog_type == BPF_PROG_TYPE_STRUCT_OPS) {
		/* Allow a struct_ops program to return a referenced kptr if it
		 * matches the operator's return type and is in its unmodified
		 * form. A scalar zero (i.e., a null pointer) is also allowed.
		 */
		reg_type = reg->btf ? btf_type_by_id(reg->btf, reg->btf_id) : NULL;
		ret_type = btf_type_resolve_ptr(prog->aux->attach_btf,
						prog->aux->attach_func_proto->type,
						NULL);
		if (ret_type && ret_type == reg_type && reg->ref_obj_id)
			return __check_ptr_off_reg(env, reg, regno, false);
	}

	/* eBPF calling convention is such that R0 is used
	 * to return the value from eBPF program.
	 * Make sure that it's readable at this time
	 * of bpf_exit, which means that program wrote
	 * something into it earlier
	 */
	err = check_reg_arg(env, regno, SRC_OP);
	if (err)
		return err;

	if (is_pointer_value(env, regno)) {
		verbose(env, "R%d leaks addr as return value\n", regno);
		return -EACCES;
	}

	if (frame->in_async_callback_fn) {
		exit_ctx = "At async callback return";
		range = frame->callback_ret_range;
		goto enforce_retval;
	}

	if (prog_type == BPF_PROG_TYPE_STRUCT_OPS && !ret_type)
		return 0;

	if (prog_type == BPF_PROG_TYPE_CGROUP_SKB && (env->prog->expected_attach_type == BPF_CGROUP_INET_EGRESS))
		enforce_attach_type_range = tnum_range(2, 3);

	if (!return_retval_range(env, &range))
		return 0;

enforce_retval:
	if (reg->type != SCALAR_VALUE) {
		verbose(env, "%s the register R%d is not a known value (%s)\n",
			exit_ctx, regno, reg_type_str(env, reg->type));
		return -EINVAL;
	}

	err = mark_chain_precision(env, regno);
	if (err)
		return err;

	if (!retval_range_within(range, reg)) {
		verbose_invalid_scalar(env, reg, range, exit_ctx, reg_name);
		if (prog->expected_attach_type == BPF_LSM_CGROUP &&
		    prog_type == BPF_PROG_TYPE_LSM &&
		    !prog->aux->attach_func_proto->type)
			verbose(env, "Note, BPF_LSM_CGROUP that attach to void LSM hooks can't modify return value!\n");
		return -EINVAL;
	}

	if (!tnum_is_unknown(enforce_attach_type_range) &&
	    tnum_in(enforce_attach_type_range, reg->var_off))
		env->prog->enforce_expected_attach_type = 1;
	return 0;
}

static int check_global_subprog_return_code(struct bpf_verifier_env *env)
{
	struct bpf_reg_state *reg = reg_state(env, BPF_REG_0);
	struct bpf_func_state *cur_frame = cur_func(env);
	int err;

	if (subprog_returns_void(env, cur_frame->subprogno))
		return 0;

	err = check_reg_arg(env, BPF_REG_0, SRC_OP);
	if (err)
		return err;

	if (is_pointer_value(env, BPF_REG_0)) {
		verbose(env, "R%d leaks addr as return value\n", BPF_REG_0);
		return -EACCES;
	}

	if (reg->type != SCALAR_VALUE) {
		verbose(env, "At subprogram exit the register R0 is not a scalar value (%s)\n",
			reg_type_str(env, reg->type));
		return -EINVAL;
	}

	return 0;
}

/* Bitmask with 1s for all caller saved registers */
#define ALL_CALLER_SAVED_REGS ((1u << CALLER_SAVED_REGS) - 1)

/* True if do_misc_fixups() replaces calls to helper number 'imm',
 * replacement patch is presumed to follow bpf_fastcall contract
 * (see mark_fastcall_pattern_for_call() below).
 */
bool bpf_verifier_inlines_helper_call(struct bpf_verifier_env *env, s32 imm)
{
	switch (imm) {
#ifdef CONFIG_X86_64
	case BPF_FUNC_get_smp_processor_id:
#ifdef CONFIG_SMP
	case BPF_FUNC_get_current_task_btf:
	case BPF_FUNC_get_current_task:
#endif
		return env->prog->jit_requested && bpf_jit_supports_percpu_insn();
#endif
	default:
		return false;
	}
}