// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 62/132



	/* callee cannot access r0, r6 - r9 for reading and has to write
	 * into its own stack before reading from it.
	 * callee can read/write into caller's stack
	 */
	init_func_state(env, callee,
			/* remember the callsite, it will be used by bpf_exit */
			callsite,
			state->curframe + 1 /* frameno within this callchain */,
			subprog /* subprog number within this prog */);
	err = set_callee_state_cb(env, caller, callee, callsite);
	if (err)
		goto err_out;

	/* only increment it after check_reg_arg() finished */
	state->curframe++;

	return 0;

err_out:
	free_func_state(callee);
	state->frame[state->curframe + 1] = NULL;
	return err;
}

static int btf_check_func_arg_match(struct bpf_verifier_env *env, int subprog,
				    const struct btf *btf,
				    struct bpf_reg_state *regs)
{
	struct bpf_subprog_info *sub = subprog_info(env, subprog);
	struct bpf_verifier_log *log = &env->log;
	u32 i;
	int ret;

	ret = btf_prepare_func_args(env, subprog);
	if (ret)
		return ret;

	/* check that BTF function arguments match actual types that the
	 * verifier sees.
	 */
	for (i = 0; i < sub->arg_cnt; i++) {
		u32 regno = i + 1;
		struct bpf_reg_state *reg = &regs[regno];
		struct bpf_subprog_arg_info *arg = &sub->args[i];

		if (arg->arg_type == ARG_ANYTHING) {
			if (reg->type != SCALAR_VALUE) {
				bpf_log(log, "R%d is not a scalar\n", regno);
				return -EINVAL;
			}
		} else if (arg->arg_type & PTR_UNTRUSTED) {
			/*
			 * Anything is allowed for untrusted arguments, as these are
			 * read-only and probe read instructions would protect against
			 * invalid memory access.
			 */
		} else if (arg->arg_type == ARG_PTR_TO_CTX) {
			ret = check_func_arg_reg_off(env, reg, regno, ARG_PTR_TO_CTX);
			if (ret < 0)
				return ret;
			/* If function expects ctx type in BTF check that caller
			 * is passing PTR_TO_CTX.
			 */
			if (reg->type != PTR_TO_CTX) {
				bpf_log(log, "arg#%d expects pointer to ctx\n", i);
				return -EINVAL;
			}
		} else if (base_type(arg->arg_type) == ARG_PTR_TO_MEM) {
			ret = check_func_arg_reg_off(env, reg, regno, ARG_DONTCARE);
			if (ret < 0)
				return ret;
			if (check_mem_reg(env, reg, regno, arg->mem_size))
				return -EINVAL;
			if (!(arg->arg_type & PTR_MAYBE_NULL) && (reg->type & PTR_MAYBE_NULL)) {
				bpf_log(log, "arg#%d is expected to be non-NULL\n", i);
				return -EINVAL;
			}
		} else if (base_type(arg->arg_type) == ARG_PTR_TO_ARENA) {
			/*
			 * Can pass any value and the kernel won't crash, but
			 * only PTR_TO_ARENA or SCALAR make sense. Everything
			 * else is a bug in the bpf program. Point it out to
			 * the user at the verification time instead of
			 * run-time debug nightmare.
			 */
			if (reg->type != PTR_TO_ARENA && reg->type != SCALAR_VALUE) {
				bpf_log(log, "R%d is not a pointer to arena or scalar.\n", regno);
				return -EINVAL;
			}
		} else if (arg->arg_type == (ARG_PTR_TO_DYNPTR | MEM_RDONLY)) {
			ret = check_func_arg_reg_off(env, reg, regno, ARG_PTR_TO_DYNPTR);
			if (ret)
				return ret;

			ret = process_dynptr_func(env, regno, -1, arg->arg_type, 0);
			if (ret)
				return ret;
		} else if (base_type(arg->arg_type) == ARG_PTR_TO_BTF_ID) {
			struct bpf_call_arg_meta meta;
			int err;

			if (bpf_register_is_null(reg) && type_may_be_null(arg->arg_type))
				continue;

			memset(&meta, 0, sizeof(meta)); /* leave func_id as zero */
			err = check_reg_type(env, regno, arg->arg_type, &arg->btf_id, &meta);
			err = err ?: check_func_arg_reg_off(env, reg, regno, arg->arg_type);
			if (err)
				return err;
		} else {
			verifier_bug(env, "unrecognized arg#%d type %d", i, arg->arg_type);
			return -EFAULT;
		}
	}

	return 0;
}

/* Compare BTF of a function call with given bpf_reg_state.
 * Returns:
 * EFAULT - there is a verifier bug. Abort verification.
 * EINVAL - there is a type mismatch or BTF is not available.
 * 0 - BTF matches with what bpf_reg_state expects.
 * Only PTR_TO_CTX and SCALAR_VALUE states are recognized.
 */
static int btf_check_subprog_call(struct bpf_verifier_env *env, int subprog,
				  struct bpf_reg_state *regs)
{
	struct bpf_prog *prog = env->prog;
	struct btf *btf = prog->aux->btf;
	u32 btf_id;
	int err;

	if (!prog->aux->func_info)
		return -EINVAL;

	btf_id = prog->aux->func_info[subprog].type_id;
	if (!btf_id)
		return -EFAULT;

	if (prog->aux->func_info_aux[subprog].unreliable)
		return -EINVAL;

	err = btf_check_func_arg_match(env, subprog, btf, regs);
	/* Compiler optimizations can remove arguments from static functions
	 * or mismatched type can be passed into a global function.
	 * In such cases mark the function as unreliable from BTF point of view.
	 */
	if (err)
		prog->aux->func_info_aux[subprog].unreliable = true;
	return err;
}

static int push_callback_call(struct bpf_verifier_env *env, struct bpf_insn *insn,
			      int insn_idx, int subprog,
			      set_callee_state_fn set_callee_state_cb)
{
	struct bpf_verifier_state *state = env->cur_state, *callback_state;
	struct bpf_func_state *caller, *callee;
	int err;