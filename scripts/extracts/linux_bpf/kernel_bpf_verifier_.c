// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 71/132



	if ((func_id == BPF_FUNC_get_stack ||
	     func_id == BPF_FUNC_get_task_stack) &&
	    !env->prog->has_callchain_buf) {
		const char *err_str;

#ifdef CONFIG_PERF_EVENTS
		err = get_callchain_buffers(sysctl_perf_event_max_stack);
		err_str = "cannot get callchain buffer for func %s#%d\n";
#else
		err = -ENOTSUPP;
		err_str = "func %s#%d not supported without CONFIG_PERF_EVENTS\n";
#endif
		if (err) {
			verbose(env, err_str, func_id_name(func_id), func_id);
			return err;
		}

		env->prog->has_callchain_buf = true;
	}

	if (func_id == BPF_FUNC_get_stackid || func_id == BPF_FUNC_get_stack)
		env->prog->call_get_stack = true;

	if (func_id == BPF_FUNC_get_func_ip) {
		if (check_get_func_ip(env))
			return -ENOTSUPP;
		env->prog->call_get_func_ip = true;
	}

	if (func_id == BPF_FUNC_tail_call) {
		if (env->cur_state->curframe) {
			struct bpf_verifier_state *branch;

			mark_reg_scratched(env, BPF_REG_0);
			branch = push_stack(env, env->insn_idx + 1, env->insn_idx, false);
			if (IS_ERR(branch))
				return PTR_ERR(branch);
			clear_all_pkt_pointers(env);
			mark_reg_unknown(env, regs, BPF_REG_0);
			err = prepare_func_exit(env, &env->insn_idx);
			if (err)
				return err;
			env->insn_idx--;
		} else {
			changes_data = false;
		}
	}

	if (changes_data)
		clear_all_pkt_pointers(env);
	return 0;
}

/* mark_btf_func_reg_size() is used when the reg size is determined by
 * the BTF func_proto's return value size and argument.
 */
static void __mark_btf_func_reg_size(struct bpf_verifier_env *env, struct bpf_reg_state *regs,
				     u32 regno, size_t reg_size)
{
	struct bpf_reg_state *reg = &regs[regno];

	if (regno == BPF_REG_0) {
		/* Function return value */
		reg->subreg_def = reg_size == sizeof(u64) ?
			DEF_NOT_SUBREG : env->insn_idx + 1;
	} else if (reg_size == sizeof(u64)) {
		/* Function argument */
		mark_insn_zext(env, reg);
	}
}

static void mark_btf_func_reg_size(struct bpf_verifier_env *env, u32 regno,
				   size_t reg_size)
{
	return __mark_btf_func_reg_size(env, cur_regs(env), regno, reg_size);
}

static bool is_kfunc_acquire(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_ACQUIRE;
}

static bool is_kfunc_release(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_RELEASE;
}


static bool is_kfunc_destructive(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_DESTRUCTIVE;
}

static bool is_kfunc_rcu(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_RCU;
}

static bool is_kfunc_rcu_protected(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_RCU_PROTECTED;
}

static bool is_kfunc_arg_mem_size(const struct btf *btf,
				  const struct btf_param *arg,
				  const struct bpf_reg_state *reg)
{
	const struct btf_type *t;

	t = btf_type_skip_modifiers(btf, arg->type, NULL);
	if (!btf_type_is_scalar(t) || reg->type != SCALAR_VALUE)
		return false;

	return btf_param_match_suffix(btf, arg, "__sz");
}

static bool is_kfunc_arg_const_mem_size(const struct btf *btf,
					const struct btf_param *arg,
					const struct bpf_reg_state *reg)
{
	const struct btf_type *t;

	t = btf_type_skip_modifiers(btf, arg->type, NULL);
	if (!btf_type_is_scalar(t) || reg->type != SCALAR_VALUE)
		return false;

	return btf_param_match_suffix(btf, arg, "__szk");
}

static bool is_kfunc_arg_constant(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__k");
}

static bool is_kfunc_arg_ignore(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__ign");
}

static bool is_kfunc_arg_map(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__map");
}

static bool is_kfunc_arg_alloc_obj(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__alloc");
}

static bool is_kfunc_arg_uninit(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__uninit");
}

static bool is_kfunc_arg_refcounted_kptr(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__refcounted_kptr");
}

static bool is_kfunc_arg_nullable(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__nullable");
}

static bool is_kfunc_arg_const_str(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__str");
}

static bool is_kfunc_arg_irq_flag(const struct btf *btf, const struct btf_param *arg)
{
	return btf_param_match_suffix(btf, arg, "__irq_flag");
}

static bool is_kfunc_arg_scalar_with_name(const struct btf *btf,
					  const struct btf_param *arg,
					  const char *name)
{
	int len, target_len = strlen(name);
	const char *param_name;