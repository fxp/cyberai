// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 124/132



/* Lazily verify all global functions based on their BTF, if they are called
 * from main BPF program or any of subprograms transitively.
 * BPF global subprogs called from dead code are not validated.
 * All callable global functions must pass verification.
 * Otherwise the whole program is rejected.
 * Consider:
 * int bar(int);
 * int foo(int f)
 * {
 *    return bar(f);
 * }
 * int bar(int b)
 * {
 *    ...
 * }
 * foo() will be verified first for R1=any_scalar_value. During verification it
 * will be assumed that bar() already verified successfully and call to bar()
 * from foo() will be checked for type match only. Later bar() will be verified
 * independently to check that it's safe for R1=any_scalar_value.
 */
static int do_check_subprogs(struct bpf_verifier_env *env)
{
	struct bpf_prog_aux *aux = env->prog->aux;
	struct bpf_func_info_aux *sub_aux;
	int i, ret, new_cnt;

	if (!aux->func_info)
		return 0;

	/* exception callback is presumed to be always called */
	if (env->exception_callback_subprog)
		subprog_aux(env, env->exception_callback_subprog)->called = true;

again:
	new_cnt = 0;
	for (i = 1; i < env->subprog_cnt; i++) {
		if (!bpf_subprog_is_global(env, i))
			continue;

		sub_aux = subprog_aux(env, i);
		if (!sub_aux->called || sub_aux->verified)
			continue;

		env->insn_idx = env->subprog_info[i].start;
		WARN_ON_ONCE(env->insn_idx == 0);
		ret = do_check_common(env, i);
		if (ret) {
			return ret;
		} else if (env->log.level & BPF_LOG_LEVEL) {
			verbose(env, "Func#%d ('%s') is safe for any args that match its prototype\n",
				i, subprog_name(env, i));
		}

		/* We verified new global subprog, it might have called some
		 * more global subprogs that we haven't verified yet, so we
		 * need to do another pass over subprogs to verify those.
		 */
		sub_aux->verified = true;
		new_cnt++;
	}

	/* We can't loop forever as we verify at least one global subprog on
	 * each pass.
	 */
	if (new_cnt)
		goto again;

	return 0;
}

static int do_check_main(struct bpf_verifier_env *env)
{
	int ret;

	env->insn_idx = 0;
	ret = do_check_common(env, 0);
	if (!ret)
		env->prog->aux->stack_depth = env->subprog_info[0].stack_depth;
	return ret;
}


static void print_verification_stats(struct bpf_verifier_env *env)
{
	int i;

	if (env->log.level & BPF_LOG_STATS) {
		verbose(env, "verification time %lld usec\n",
			div_u64(env->verification_time, 1000));
		verbose(env, "stack depth ");
		for (i = 0; i < env->subprog_cnt; i++) {
			u32 depth = env->subprog_info[i].stack_depth;

			verbose(env, "%d", depth);
			if (i + 1 < env->subprog_cnt)
				verbose(env, "+");
		}
		verbose(env, "\n");
	}
	verbose(env, "processed %d insns (limit %d) max_states_per_insn %d "
		"total_states %d peak_states %d mark_read %d\n",
		env->insn_processed, BPF_COMPLEXITY_LIMIT_INSNS,
		env->max_states_per_insn, env->total_states,
		env->peak_states, env->longest_mark_read_walk);
}

int bpf_prog_ctx_arg_info_init(struct bpf_prog *prog,
			       const struct bpf_ctx_arg_aux *info, u32 cnt)
{
	prog->aux->ctx_arg_info = kmemdup_array(info, cnt, sizeof(*info), GFP_KERNEL_ACCOUNT);
	prog->aux->ctx_arg_info_size = cnt;

	return prog->aux->ctx_arg_info ? 0 : -ENOMEM;
}

static int check_struct_ops_btf_id(struct bpf_verifier_env *env)
{
	const struct btf_type *t, *func_proto;
	const struct bpf_struct_ops_desc *st_ops_desc;
	const struct bpf_struct_ops *st_ops;
	const struct btf_member *member;
	struct bpf_prog *prog = env->prog;
	bool has_refcounted_arg = false;
	u32 btf_id, member_idx, member_off;
	struct btf *btf;
	const char *mname;
	int i, err;

	if (!prog->gpl_compatible) {
		verbose(env, "struct ops programs must have a GPL compatible license\n");
		return -EINVAL;
	}

	if (!prog->aux->attach_btf_id)
		return -ENOTSUPP;

	btf = prog->aux->attach_btf;
	if (btf_is_module(btf)) {
		/* Make sure st_ops is valid through the lifetime of env */
		env->attach_btf_mod = btf_try_get_module(btf);
		if (!env->attach_btf_mod) {
			verbose(env, "struct_ops module %s is not found\n",
				btf_get_name(btf));
			return -ENOTSUPP;
		}
	}

	btf_id = prog->aux->attach_btf_id;
	st_ops_desc = bpf_struct_ops_find(btf, btf_id);
	if (!st_ops_desc) {
		verbose(env, "attach_btf_id %u is not a supported struct\n",
			btf_id);
		return -ENOTSUPP;
	}
	st_ops = st_ops_desc->st_ops;

	t = st_ops_desc->type;
	member_idx = prog->expected_attach_type;
	if (member_idx >= btf_type_vlen(t)) {
		verbose(env, "attach to invalid member idx %u of struct %s\n",
			member_idx, st_ops->name);
		return -EINVAL;
	}

	member = &btf_type_member(t)[member_idx];
	mname = btf_name_by_offset(btf, member->name_off);
	func_proto = btf_type_resolve_func_ptr(btf, member->type,
					       NULL);
	if (!func_proto) {
		verbose(env, "attach to invalid member %s(@idx %u) of struct %s\n",
			mname, member_idx, st_ops->name);
		return -EINVAL;
	}