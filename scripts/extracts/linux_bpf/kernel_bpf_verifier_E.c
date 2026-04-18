// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 37/132



	if (subprog[idx].priv_stack_mode == PRIV_STACK_ADAPTIVE) {
		if (subprog_depth > MAX_BPF_STACK) {
			verbose(env, "stack size of subprog %d is %d. Too large\n",
				idx, subprog_depth);
			return -EACCES;
		}
	} else {
		depth += subprog_depth;
		if (depth > MAX_BPF_STACK) {
			total = 0;
			for (tmp = idx; tmp >= 0; tmp = dinfo[tmp].caller)
				total++;

			verbose(env, "combined stack size of %d calls is %d. Too large\n",
				total, depth);
			return -EACCES;
		}
	}
continue_func:
	subprog_end = subprog[idx + 1].start;
	for (; i < subprog_end; i++) {
		int next_insn, sidx;

		if (bpf_pseudo_kfunc_call(insn + i) && !insn[i].off) {
			bool err = false;

			if (!is_bpf_throw_kfunc(insn + i))
				continue;
			for (tmp = idx; tmp >= 0 && !err; tmp = dinfo[tmp].caller) {
				if (subprog[tmp].is_cb) {
					err = true;
					break;
				}
			}
			if (!err)
				continue;
			verbose(env,
				"bpf_throw kfunc (insn %d) cannot be called from callback subprog %d\n",
				i, idx);
			return -EINVAL;
		}

		if (!bpf_pseudo_call(insn + i) && !bpf_pseudo_func(insn + i))
			continue;
		/* remember insn and function to return to */

		/* find the callee */
		next_insn = i + insn[i].imm + 1;
		sidx = bpf_find_subprog(env, next_insn);
		if (verifier_bug_if(sidx < 0, env, "callee not found at insn %d", next_insn))
			return -EFAULT;
		if (subprog[sidx].is_async_cb) {
			if (subprog[sidx].has_tail_call) {
				verifier_bug(env, "subprog has tail_call and async cb");
				return -EFAULT;
			}
			/* async callbacks don't increase bpf prog stack size unless called directly */
			if (!bpf_pseudo_call(insn + i))
				continue;
			if (subprog[sidx].is_exception_cb) {
				verbose(env, "insn %d cannot call exception cb directly", i);
				return -EINVAL;
			}
		}

		/* store caller info for after we return from callee */
		dinfo[idx].frame = frame;
		dinfo[idx].ret_insn = i + 1;

		/* push caller idx into callee's dinfo */
		dinfo[sidx].caller = idx;

		i = next_insn;

		idx = sidx;
		if (!priv_stack_supported)
			subprog[idx].priv_stack_mode = NO_PRIV_STACK;

		if (subprog[idx].has_tail_call)
			tail_call_reachable = true;

		frame = bpf_subprog_is_global(env, idx) ? 0 : frame + 1;
		if (frame >= MAX_CALL_FRAMES) {
			verbose(env, "the call stack of %d frames is too deep !\n",
				frame);
			return -E2BIG;
		}
		goto process_func;
	}
	/* if tail call got detected across bpf2bpf calls then mark each of the
	 * currently present subprog frames as tail call reachable subprogs;
	 * this info will be utilized by JIT so that we will be preserving the
	 * tail call counter throughout bpf2bpf calls combined with tailcalls
	 */
	if (tail_call_reachable)
		for (tmp = idx; tmp >= 0; tmp = dinfo[tmp].caller) {
			if (subprog[tmp].is_exception_cb) {
				verbose(env, "cannot tail call within exception cb\n");
				return -EINVAL;
			}
			subprog[tmp].tail_call_reachable = true;
		}
	if (subprog[0].tail_call_reachable)
		env->prog->aux->tail_call_reachable = true;

	/* end of for() loop means the last insn of the 'subprog'
	 * was reached. Doesn't matter whether it was JA or EXIT
	 */
	if (frame == 0 && dinfo[idx].caller < 0)
		return 0;
	if (subprog[idx].priv_stack_mode != PRIV_STACK_ADAPTIVE)
		depth -= round_up_stack_depth(env, subprog[idx].stack_depth);

	/* pop caller idx from callee */
	idx = dinfo[idx].caller;

	/* retrieve caller state from its frame */
	frame = dinfo[idx].frame;
	i = dinfo[idx].ret_insn;

	goto continue_func;
}

static int check_max_stack_depth(struct bpf_verifier_env *env)
{
	enum priv_stack_mode priv_stack_mode = PRIV_STACK_UNKNOWN;
	struct bpf_subprog_call_depth_info *dinfo;
	struct bpf_subprog_info *si = env->subprog_info;
	bool priv_stack_supported;
	int ret;

	dinfo = kvcalloc(env->subprog_cnt, sizeof(*dinfo), GFP_KERNEL_ACCOUNT);
	if (!dinfo)
		return -ENOMEM;

	for (int i = 0; i < env->subprog_cnt; i++) {
		if (si[i].has_tail_call) {
			priv_stack_mode = NO_PRIV_STACK;
			break;
		}
	}

	if (priv_stack_mode == PRIV_STACK_UNKNOWN)
		priv_stack_mode = bpf_enable_priv_stack(env->prog);

	/* All async_cb subprogs use normal kernel stack. If a particular
	 * subprog appears in both main prog and async_cb subtree, that
	 * subprog will use normal kernel stack to avoid potential nesting.
	 * The reverse subprog traversal ensures when main prog subtree is
	 * checked, the subprogs appearing in async_cb subtrees are already
	 * marked as using normal kernel stack, so stack size checking can
	 * be done properly.
	 */
	for (int i = env->subprog_cnt - 1; i >= 0; i--) {
		if (!i || si[i].is_async_cb) {
			priv_stack_supported = !i && priv_stack_mode == PRIV_STACK_ADAPTIVE;
			ret = check_max_stack_depth_subprog(env, i, dinfo,
					priv_stack_supported);
			if (ret < 0) {
				kvfree(dinfo);
				return ret;
			}
		}
	}

	for (int i = 0; i < env->subprog_cnt; i++) {
		if (si[i].priv_stack_mode == PRIV_STACK_ADAPTIVE) {
			env->prog->aux->jits_use_priv_stack = true;
			break;
		}
	}

	kvfree(dinfo);

	return 0;
}