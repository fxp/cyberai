// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 123/132



		list_for_each_safe(pos, tmp, head) {
			sl = container_of(pos, struct bpf_verifier_state_list, node);
			bpf_free_verifier_state(&sl->state, false);
			kfree(sl);
		}
		INIT_LIST_HEAD(&env->explored_states[i]);
	}
}

static int do_check_common(struct bpf_verifier_env *env, int subprog)
{
	bool pop_log = !(env->log.level & BPF_LOG_LEVEL2);
	struct bpf_subprog_info *sub = subprog_info(env, subprog);
	struct bpf_prog_aux *aux = env->prog->aux;
	struct bpf_verifier_state *state;
	struct bpf_reg_state *regs;
	int ret, i;

	env->prev_linfo = NULL;
	env->pass_cnt++;

	state = kzalloc_obj(struct bpf_verifier_state, GFP_KERNEL_ACCOUNT);
	if (!state)
		return -ENOMEM;
	state->curframe = 0;
	state->speculative = false;
	state->branches = 1;
	state->in_sleepable = env->prog->sleepable;
	state->frame[0] = kzalloc_obj(struct bpf_func_state, GFP_KERNEL_ACCOUNT);
	if (!state->frame[0]) {
		kfree(state);
		return -ENOMEM;
	}
	env->cur_state = state;
	init_func_state(env, state->frame[0],
			BPF_MAIN_FUNC /* callsite */,
			0 /* frameno */,
			subprog);
	state->first_insn_idx = env->subprog_info[subprog].start;
	state->last_insn_idx = -1;

	regs = state->frame[state->curframe]->regs;
	if (subprog || env->prog->type == BPF_PROG_TYPE_EXT) {
		const char *sub_name = subprog_name(env, subprog);
		struct bpf_subprog_arg_info *arg;
		struct bpf_reg_state *reg;

		if (env->log.level & BPF_LOG_LEVEL)
			verbose(env, "Validating %s() func#%d...\n", sub_name, subprog);
		ret = btf_prepare_func_args(env, subprog);
		if (ret)
			goto out;

		if (subprog_is_exc_cb(env, subprog)) {
			state->frame[0]->in_exception_callback_fn = true;

			/*
			 * Global functions are scalar or void, make sure
			 * we return a scalar.
			 */
			if (subprog_returns_void(env, subprog)) {
				verbose(env, "exception cb cannot return void\n");
				ret = -EINVAL;
				goto out;
			}

			/* Also ensure the callback only has a single scalar argument. */
			if (sub->arg_cnt != 1 || sub->args[0].arg_type != ARG_ANYTHING) {
				verbose(env, "exception cb only supports single integer argument\n");
				ret = -EINVAL;
				goto out;
			}
		}
		for (i = BPF_REG_1; i <= sub->arg_cnt; i++) {
			arg = &sub->args[i - BPF_REG_1];
			reg = &regs[i];

			if (arg->arg_type == ARG_PTR_TO_CTX) {
				reg->type = PTR_TO_CTX;
				mark_reg_known_zero(env, regs, i);
			} else if (arg->arg_type == ARG_ANYTHING) {
				reg->type = SCALAR_VALUE;
				mark_reg_unknown(env, regs, i);
			} else if (arg->arg_type == (ARG_PTR_TO_DYNPTR | MEM_RDONLY)) {
				/* assume unspecial LOCAL dynptr type */
				__mark_dynptr_reg(reg, BPF_DYNPTR_TYPE_LOCAL, true, ++env->id_gen);
			} else if (base_type(arg->arg_type) == ARG_PTR_TO_MEM) {
				reg->type = PTR_TO_MEM;
				reg->type |= arg->arg_type &
					     (PTR_MAYBE_NULL | PTR_UNTRUSTED | MEM_RDONLY);
				mark_reg_known_zero(env, regs, i);
				reg->mem_size = arg->mem_size;
				if (arg->arg_type & PTR_MAYBE_NULL)
					reg->id = ++env->id_gen;
			} else if (base_type(arg->arg_type) == ARG_PTR_TO_BTF_ID) {
				reg->type = PTR_TO_BTF_ID;
				if (arg->arg_type & PTR_MAYBE_NULL)
					reg->type |= PTR_MAYBE_NULL;
				if (arg->arg_type & PTR_UNTRUSTED)
					reg->type |= PTR_UNTRUSTED;
				if (arg->arg_type & PTR_TRUSTED)
					reg->type |= PTR_TRUSTED;
				mark_reg_known_zero(env, regs, i);
				reg->btf = bpf_get_btf_vmlinux(); /* can't fail at this point */
				reg->btf_id = arg->btf_id;
				reg->id = ++env->id_gen;
			} else if (base_type(arg->arg_type) == ARG_PTR_TO_ARENA) {
				/* caller can pass either PTR_TO_ARENA or SCALAR */
				mark_reg_unknown(env, regs, i);
			} else {
				verifier_bug(env, "unhandled arg#%d type %d",
					     i - BPF_REG_1, arg->arg_type);
				ret = -EFAULT;
				goto out;
			}
		}
	} else {
		/* if main BPF program has associated BTF info, validate that
		 * it's matching expected signature, and otherwise mark BTF
		 * info for main program as unreliable
		 */
		if (env->prog->aux->func_info_aux) {
			ret = btf_prepare_func_args(env, 0);
			if (ret || sub->arg_cnt != 1 || sub->args[0].arg_type != ARG_PTR_TO_CTX)
				env->prog->aux->func_info_aux[0].unreliable = true;
		}

		/* 1st arg to a function */
		regs[BPF_REG_1].type = PTR_TO_CTX;
		mark_reg_known_zero(env, regs, BPF_REG_1);
	}

	/* Acquire references for struct_ops program arguments tagged with "__ref" */
	if (!subprog && env->prog->type == BPF_PROG_TYPE_STRUCT_OPS) {
		for (i = 0; i < aux->ctx_arg_info_size; i++)
			aux->ctx_arg_info[i].ref_obj_id = aux->ctx_arg_info[i].refcounted ?
							  acquire_reference(env, 0) : 0;
	}

	ret = do_check(env);
out:
	if (!ret && pop_log)
		bpf_vlog_reset(&env->log, 0);
	free_states(env);
	return ret;
}