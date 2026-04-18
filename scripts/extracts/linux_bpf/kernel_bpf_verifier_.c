// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 67/132



static int check_reference_leak(struct bpf_verifier_env *env, bool exception_exit)
{
	struct bpf_verifier_state *state = env->cur_state;
	enum bpf_prog_type type = resolve_prog_type(env->prog);
	struct bpf_reg_state *reg = reg_state(env, BPF_REG_0);
	bool refs_lingering = false;
	int i;

	if (!exception_exit && cur_func(env)->frameno)
		return 0;

	for (i = 0; i < state->acquired_refs; i++) {
		if (state->refs[i].type != REF_TYPE_PTR)
			continue;
		/* Allow struct_ops programs to return a referenced kptr back to
		 * kernel. Type checks are performed later in check_return_code.
		 */
		if (type == BPF_PROG_TYPE_STRUCT_OPS && !exception_exit &&
		    reg->ref_obj_id == state->refs[i].id)
			continue;
		verbose(env, "Unreleased reference id=%d alloc_insn=%d\n",
			state->refs[i].id, state->refs[i].insn_idx);
		refs_lingering = true;
	}
	return refs_lingering ? -EINVAL : 0;
}

static int check_resource_leak(struct bpf_verifier_env *env, bool exception_exit, bool check_lock, const char *prefix)
{
	int err;

	if (check_lock && env->cur_state->active_locks) {
		verbose(env, "%s cannot be used inside bpf_spin_lock-ed region\n", prefix);
		return -EINVAL;
	}

	err = check_reference_leak(env, exception_exit);
	if (err) {
		verbose(env, "%s would lead to reference leak\n", prefix);
		return err;
	}

	if (check_lock && env->cur_state->active_irq_id) {
		verbose(env, "%s cannot be used inside bpf_local_irq_save-ed region\n", prefix);
		return -EINVAL;
	}

	if (check_lock && env->cur_state->active_rcu_locks) {
		verbose(env, "%s cannot be used inside bpf_rcu_read_lock-ed region\n", prefix);
		return -EINVAL;
	}

	if (check_lock && env->cur_state->active_preempt_locks) {
		verbose(env, "%s cannot be used inside bpf_preempt_disable-ed region\n", prefix);
		return -EINVAL;
	}

	return 0;
}

static int check_bpf_snprintf_call(struct bpf_verifier_env *env,
				   struct bpf_reg_state *regs)
{
	struct bpf_reg_state *fmt_reg = &regs[BPF_REG_3];
	struct bpf_reg_state *data_len_reg = &regs[BPF_REG_5];
	struct bpf_map *fmt_map = fmt_reg->map_ptr;
	struct bpf_bprintf_data data = {};
	int err, fmt_map_off, num_args;
	u64 fmt_addr;
	char *fmt;

	/* data must be an array of u64 */
	if (data_len_reg->var_off.value % 8)
		return -EINVAL;
	num_args = data_len_reg->var_off.value / 8;

	/* fmt being ARG_PTR_TO_CONST_STR guarantees that var_off is const
	 * and map_direct_value_addr is set.
	 */
	fmt_map_off = fmt_reg->var_off.value;
	err = fmt_map->ops->map_direct_value_addr(fmt_map, &fmt_addr,
						  fmt_map_off);
	if (err) {
		verbose(env, "failed to retrieve map value address\n");
		return -EFAULT;
	}
	fmt = (char *)(long)fmt_addr + fmt_map_off;

	/* We are also guaranteed that fmt+fmt_map_off is NULL terminated, we
	 * can focus on validating the format specifiers.
	 */
	err = bpf_bprintf_prepare(fmt, UINT_MAX, NULL, num_args, &data);
	if (err < 0)
		verbose(env, "Invalid format string\n");

	return err;
}

static int check_get_func_ip(struct bpf_verifier_env *env)
{
	enum bpf_prog_type type = resolve_prog_type(env->prog);
	int func_id = BPF_FUNC_get_func_ip;

	if (type == BPF_PROG_TYPE_TRACING) {
		if (!bpf_prog_has_trampoline(env->prog)) {
			verbose(env, "func %s#%d supported only for fentry/fexit/fsession/fmod_ret programs\n",
				func_id_name(func_id), func_id);
			return -ENOTSUPP;
		}
		return 0;
	} else if (type == BPF_PROG_TYPE_KPROBE) {
		return 0;
	}

	verbose(env, "func %s#%d not supported for program type %d\n",
		func_id_name(func_id), func_id, type);
	return -ENOTSUPP;
}

static struct bpf_insn_aux_data *cur_aux(const struct bpf_verifier_env *env)
{
	return &env->insn_aux_data[env->insn_idx];
}

static bool loop_flag_is_zero(struct bpf_verifier_env *env)
{
	struct bpf_reg_state *reg = reg_state(env, BPF_REG_4);
	bool reg_is_null = bpf_register_is_null(reg);

	if (reg_is_null)
		mark_chain_precision(env, BPF_REG_4);

	return reg_is_null;
}

static void update_loop_inline_state(struct bpf_verifier_env *env, u32 subprogno)
{
	struct bpf_loop_inline_state *state = &cur_aux(env)->loop_inline_state;

	if (!state->initialized) {
		state->initialized = 1;
		state->fit_for_inline = loop_flag_is_zero(env);
		state->callback_subprogno = subprogno;
		return;
	}

	if (!state->fit_for_inline)
		return;

	state->fit_for_inline = (loop_flag_is_zero(env) &&
				 state->callback_subprogno == subprogno);
}

/* Returns whether or not the given map type can potentially elide
 * lookup return value nullness check. This is possible if the key
 * is statically known.
 */
static bool can_elide_value_nullness(enum bpf_map_type type)
{
	switch (type) {
	case BPF_MAP_TYPE_ARRAY:
	case BPF_MAP_TYPE_PERCPU_ARRAY:
		return true;
	default:
		return false;
	}
}

int bpf_get_helper_proto(struct bpf_verifier_env *env, int func_id,
			 const struct bpf_func_proto **ptr)
{
	if (func_id < 0 || func_id >= __BPF_FUNC_MAX_ID)
		return -ERANGE;

	if (!env->ops->get_func_proto)
		return -EINVAL;