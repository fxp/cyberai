// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 79/132



	node_type_name = btf_field_type_name(node_field_type);
	if (!tnum_is_const(reg->var_off)) {
		verbose(env,
			"R%d doesn't have constant offset. %s has to be at the constant offset\n",
			regno, node_type_name);
		return -EINVAL;
	}

	node_off = reg->var_off.value;
	field = reg_find_field_offset(reg, node_off, node_field_type);
	if (!field) {
		verbose(env, "%s not found at offset=%u\n", node_type_name, node_off);
		return -EINVAL;
	}

	field = *node_field;

	et = btf_type_by_id(field->graph_root.btf, field->graph_root.value_btf_id);
	t = btf_type_by_id(reg->btf, reg->btf_id);
	if (!btf_struct_ids_match(&env->log, reg->btf, reg->btf_id, 0, field->graph_root.btf,
				  field->graph_root.value_btf_id, true)) {
		verbose(env, "operation on %s expects arg#1 %s at offset=%d "
			"in struct %s, but arg is at offset=%d in struct %s\n",
			btf_field_type_name(head_field_type),
			btf_field_type_name(node_field_type),
			field->graph_root.node_offset,
			btf_name_by_offset(field->graph_root.btf, et->name_off),
			node_off, btf_name_by_offset(reg->btf, t->name_off));
		return -EINVAL;
	}
	meta->arg_btf = reg->btf;
	meta->arg_btf_id = reg->btf_id;

	if (node_off != field->graph_root.node_offset) {
		verbose(env, "arg#1 offset=%d, but expected %s at offset=%d in struct %s\n",
			node_off, btf_field_type_name(node_field_type),
			field->graph_root.node_offset,
			btf_name_by_offset(field->graph_root.btf, et->name_off));
		return -EINVAL;
	}

	return 0;
}

static int process_kf_arg_ptr_to_list_node(struct bpf_verifier_env *env,
					   struct bpf_reg_state *reg, u32 regno,
					   struct bpf_kfunc_call_arg_meta *meta)
{
	return __process_kf_arg_ptr_to_graph_node(env, reg, regno, meta,
						  BPF_LIST_HEAD, BPF_LIST_NODE,
						  &meta->arg_list_head.field);
}

static int process_kf_arg_ptr_to_rbtree_node(struct bpf_verifier_env *env,
					     struct bpf_reg_state *reg, u32 regno,
					     struct bpf_kfunc_call_arg_meta *meta)
{
	return __process_kf_arg_ptr_to_graph_node(env, reg, regno, meta,
						  BPF_RB_ROOT, BPF_RB_NODE,
						  &meta->arg_rbtree_root.field);
}

/*
 * css_task iter allowlist is needed to avoid dead locking on css_set_lock.
 * LSM hooks and iters (both sleepable and non-sleepable) are safe.
 * Any sleepable progs are also safe since bpf_check_attach_target() enforce
 * them can only be attached to some specific hook points.
 */
static bool check_css_task_iter_allowlist(struct bpf_verifier_env *env)
{
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);

	switch (prog_type) {
	case BPF_PROG_TYPE_LSM:
		return true;
	case BPF_PROG_TYPE_TRACING:
		if (env->prog->expected_attach_type == BPF_TRACE_ITER)
			return true;
		fallthrough;
	default:
		return in_sleepable(env);
	}
}

static int check_kfunc_args(struct bpf_verifier_env *env, struct bpf_kfunc_call_arg_meta *meta,
			    int insn_idx)
{
	const char *func_name = meta->func_name, *ref_tname;
	const struct btf *btf = meta->btf;
	const struct btf_param *args;
	struct btf_record *rec;
	u32 i, nargs;
	int ret;

	args = (const struct btf_param *)(meta->func_proto + 1);
	nargs = btf_type_vlen(meta->func_proto);
	if (nargs > MAX_BPF_FUNC_REG_ARGS) {
		verbose(env, "Function %s has %d > %d args\n", func_name, nargs,
			MAX_BPF_FUNC_REG_ARGS);
		return -EINVAL;
	}

	/* Check that BTF function arguments match actual types that the
	 * verifier sees.
	 */
	for (i = 0; i < nargs; i++) {
		struct bpf_reg_state *regs = cur_regs(env), *reg = &regs[i + 1];
		const struct btf_type *t, *ref_t, *resolve_ret;
		enum bpf_arg_type arg_type = ARG_DONTCARE;
		u32 regno = i + 1, ref_id, type_size;
		bool is_ret_buf_sz = false;
		int kf_arg_type;

		if (is_kfunc_arg_prog_aux(btf, &args[i])) {
			/* Reject repeated use bpf_prog_aux */
			if (meta->arg_prog) {
				verifier_bug(env, "Only 1 prog->aux argument supported per-kfunc");
				return -EFAULT;
			}
			meta->arg_prog = true;
			cur_aux(env)->arg_prog = regno;
			continue;
		}

		if (is_kfunc_arg_ignore(btf, &args[i]) || is_kfunc_arg_implicit(meta, i))
			continue;

		t = btf_type_skip_modifiers(btf, args[i].type, NULL);

		if (btf_type_is_scalar(t)) {
			if (reg->type != SCALAR_VALUE) {
				verbose(env, "R%d is not a scalar\n", regno);
				return -EINVAL;
			}

			if (is_kfunc_arg_constant(meta->btf, &args[i])) {
				if (meta->arg_constant.found) {
					verifier_bug(env, "only one constant argument permitted");
					return -EFAULT;
				}
				if (!tnum_is_const(reg->var_off)) {
					verbose(env, "R%d must be a known constant\n", regno);
					return -EINVAL;
				}
				ret = mark_chain_precision(env, regno);
				if (ret < 0)
					return ret;
				meta->arg_constant.found = true;
				meta->arg_constant.value = reg->var_off.value;
			} else if (is_kfunc_arg_scalar_with_name(btf, &args[i], "rdonly_buf_size")) {
				meta->r0_rdonly = true;
				is_ret_buf_sz = true;
			} else if (is_kfunc_arg_scalar_with_name(btf, &args[i], "rdwr_buf_size")) {
				is_ret_buf_sz = true;
			}