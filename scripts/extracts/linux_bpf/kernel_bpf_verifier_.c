// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 63/132



	caller = state->frame[state->curframe];
	err = btf_check_subprog_call(env, subprog, caller->regs);
	if (err == -EFAULT)
		return err;

	/* set_callee_state is used for direct subprog calls, but we are
	 * interested in validating only BPF helpers that can call subprogs as
	 * callbacks
	 */
	env->subprog_info[subprog].is_cb = true;
	if (bpf_pseudo_kfunc_call(insn) &&
	    !is_callback_calling_kfunc(insn->imm)) {
		verifier_bug(env, "kfunc %s#%d not marked as callback-calling",
			     func_id_name(insn->imm), insn->imm);
		return -EFAULT;
	} else if (!bpf_pseudo_kfunc_call(insn) &&
		   !is_callback_calling_function(insn->imm)) { /* helper */
		verifier_bug(env, "helper %s#%d not marked as callback-calling",
			     func_id_name(insn->imm), insn->imm);
		return -EFAULT;
	}

	if (bpf_is_async_callback_calling_insn(insn)) {
		struct bpf_verifier_state *async_cb;

		/* there is no real recursion here. timer and workqueue callbacks are async */
		env->subprog_info[subprog].is_async_cb = true;
		async_cb = push_async_cb(env, env->subprog_info[subprog].start,
					 insn_idx, subprog,
					 is_async_cb_sleepable(env, insn));
		if (IS_ERR(async_cb))
			return PTR_ERR(async_cb);
		callee = async_cb->frame[0];
		callee->async_entry_cnt = caller->async_entry_cnt + 1;

		/* Convert bpf_timer_set_callback() args into timer callback args */
		err = set_callee_state_cb(env, caller, callee, insn_idx);
		if (err)
			return err;

		return 0;
	}

	/* for callback functions enqueue entry to callback and
	 * proceed with next instruction within current frame.
	 */
	callback_state = push_stack(env, env->subprog_info[subprog].start, insn_idx, false);
	if (IS_ERR(callback_state))
		return PTR_ERR(callback_state);

	err = setup_func_entry(env, subprog, insn_idx, set_callee_state_cb,
			       callback_state);
	if (err)
		return err;

	callback_state->callback_unroll_depth++;
	callback_state->frame[callback_state->curframe - 1]->callback_depth++;
	caller->callback_depth = 0;
	return 0;
}

static int check_func_call(struct bpf_verifier_env *env, struct bpf_insn *insn,
			   int *insn_idx)
{
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_func_state *caller;
	int err, subprog, target_insn;

	target_insn = *insn_idx + insn->imm + 1;
	subprog = bpf_find_subprog(env, target_insn);
	if (verifier_bug_if(subprog < 0, env, "target of func call at insn %d is not a program",
			    target_insn))
		return -EFAULT;

	caller = state->frame[state->curframe];
	err = btf_check_subprog_call(env, subprog, caller->regs);
	if (err == -EFAULT)
		return err;
	if (bpf_subprog_is_global(env, subprog)) {
		const char *sub_name = subprog_name(env, subprog);

		if (env->cur_state->active_locks) {
			verbose(env, "global function calls are not allowed while holding a lock,\n"
				     "use static function instead\n");
			return -EINVAL;
		}

		if (env->subprog_info[subprog].might_sleep && !in_sleepable_context(env)) {
			verbose(env, "sleepable global function %s() called in %s\n",
				sub_name, non_sleepable_context_description(env));
			return -EINVAL;
		}

		if (err) {
			verbose(env, "Caller passes invalid args into func#%d ('%s')\n",
				subprog, sub_name);
			return err;
		}

		if (env->log.level & BPF_LOG_LEVEL)
			verbose(env, "Func#%d ('%s') is global and assumed valid.\n",
				subprog, sub_name);
		if (env->subprog_info[subprog].changes_pkt_data)
			clear_all_pkt_pointers(env);
		/* mark global subprog for verifying after main prog */
		subprog_aux(env, subprog)->called = true;
		clear_caller_saved_regs(env, caller->regs);

		/* All non-void global functions return a 64-bit SCALAR_VALUE. */
		if (!subprog_returns_void(env, subprog)) {
			mark_reg_unknown(env, caller->regs, BPF_REG_0);
			caller->regs[BPF_REG_0].subreg_def = DEF_NOT_SUBREG;
		}

		/* continue with next insn after call */
		return 0;
	}

	/* for regular function entry setup new frame and continue
	 * from that frame.
	 */
	err = setup_func_entry(env, subprog, *insn_idx, set_callee_state, state);
	if (err)
		return err;

	clear_caller_saved_regs(env, caller->regs);

	/* and go analyze first insn of the callee */
	*insn_idx = env->subprog_info[subprog].start - 1;

	if (env->log.level & BPF_LOG_LEVEL) {
		verbose(env, "caller:\n");
		print_verifier_state(env, state, caller->frameno, true);
		verbose(env, "callee:\n");
		print_verifier_state(env, state, state->curframe, true);
	}

	return 0;
}

int map_set_for_each_callback_args(struct bpf_verifier_env *env,
				   struct bpf_func_state *caller,
				   struct bpf_func_state *callee)
{
	/* bpf_for_each_map_elem(struct bpf_map *map, void *callback_fn,
	 *      void *callback_ctx, u64 flags);
	 * callback_fn(struct bpf_map *map, void *key, void *value,
	 *      void *callback_ctx);
	 */
	callee->regs[BPF_REG_1] = caller->regs[BPF_REG_1];

	callee->regs[BPF_REG_2].type = PTR_TO_MAP_KEY;
	__mark_reg_known_zero(&callee->regs[BPF_REG_2]);
	callee->regs[BPF_REG_2].map_ptr = caller->regs[BPF_REG_1].map_ptr;