// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 72/132



	param_name = btf_name_by_offset(btf, arg->name_off);
	if (str_is_empty(param_name))
		return false;
	len = strlen(param_name);
	if (len != target_len)
		return false;
	if (strcmp(param_name, name))
		return false;

	return true;
}

enum {
	KF_ARG_DYNPTR_ID,
	KF_ARG_LIST_HEAD_ID,
	KF_ARG_LIST_NODE_ID,
	KF_ARG_RB_ROOT_ID,
	KF_ARG_RB_NODE_ID,
	KF_ARG_WORKQUEUE_ID,
	KF_ARG_RES_SPIN_LOCK_ID,
	KF_ARG_TASK_WORK_ID,
	KF_ARG_PROG_AUX_ID,
	KF_ARG_TIMER_ID
};

BTF_ID_LIST(kf_arg_btf_ids)
BTF_ID(struct, bpf_dynptr)
BTF_ID(struct, bpf_list_head)
BTF_ID(struct, bpf_list_node)
BTF_ID(struct, bpf_rb_root)
BTF_ID(struct, bpf_rb_node)
BTF_ID(struct, bpf_wq)
BTF_ID(struct, bpf_res_spin_lock)
BTF_ID(struct, bpf_task_work)
BTF_ID(struct, bpf_prog_aux)
BTF_ID(struct, bpf_timer)

static bool __is_kfunc_ptr_arg_type(const struct btf *btf,
				    const struct btf_param *arg, int type)
{
	const struct btf_type *t;
	u32 res_id;

	t = btf_type_skip_modifiers(btf, arg->type, NULL);
	if (!t)
		return false;
	if (!btf_type_is_ptr(t))
		return false;
	t = btf_type_skip_modifiers(btf, t->type, &res_id);
	if (!t)
		return false;
	return btf_types_are_same(btf, res_id, btf_vmlinux, kf_arg_btf_ids[type]);
}

static bool is_kfunc_arg_dynptr(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_DYNPTR_ID);
}

static bool is_kfunc_arg_list_head(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_LIST_HEAD_ID);
}

static bool is_kfunc_arg_list_node(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_LIST_NODE_ID);
}

static bool is_kfunc_arg_rbtree_root(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_RB_ROOT_ID);
}

static bool is_kfunc_arg_rbtree_node(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_RB_NODE_ID);
}

static bool is_kfunc_arg_timer(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_TIMER_ID);
}

static bool is_kfunc_arg_wq(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_WORKQUEUE_ID);
}

static bool is_kfunc_arg_task_work(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_TASK_WORK_ID);
}

static bool is_kfunc_arg_res_spin_lock(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_RES_SPIN_LOCK_ID);
}

static bool is_rbtree_node_type(const struct btf_type *t)
{
	return t == btf_type_by_id(btf_vmlinux, kf_arg_btf_ids[KF_ARG_RB_NODE_ID]);
}

static bool is_list_node_type(const struct btf_type *t)
{
	return t == btf_type_by_id(btf_vmlinux, kf_arg_btf_ids[KF_ARG_LIST_NODE_ID]);
}

static bool is_kfunc_arg_callback(struct bpf_verifier_env *env, const struct btf *btf,
				  const struct btf_param *arg)
{
	const struct btf_type *t;

	t = btf_type_resolve_func_ptr(btf, arg->type, NULL);
	if (!t)
		return false;

	return true;
}

static bool is_kfunc_arg_prog_aux(const struct btf *btf, const struct btf_param *arg)
{
	return __is_kfunc_ptr_arg_type(btf, arg, KF_ARG_PROG_AUX_ID);
}

/*
 * A kfunc with KF_IMPLICIT_ARGS has two prototypes in BTF:
 *   - the _impl prototype with full arg list (meta->func_proto)
 *   - the BPF API prototype w/o implicit args (func->type in BTF)
 * To determine whether an argument is implicit, we compare its position
 * against the number of arguments in the prototype w/o implicit args.
 */
static bool is_kfunc_arg_implicit(const struct bpf_kfunc_call_arg_meta *meta, u32 arg_idx)
{
	const struct btf_type *func, *func_proto;
	u32 argn;

	if (!(meta->kfunc_flags & KF_IMPLICIT_ARGS))
		return false;

	func = btf_type_by_id(meta->btf, meta->func_id);
	func_proto = btf_type_by_id(meta->btf, func->type);
	argn = btf_type_vlen(func_proto);

	return argn <= arg_idx;
}

/* Returns true if struct is composed of scalars, 4 levels of nesting allowed */
static bool __btf_type_is_scalar_struct(struct bpf_verifier_env *env,
					const struct btf *btf,
					const struct btf_type *t, int rec)
{
	const struct btf_type *member_type;
	const struct btf_member *member;
	u32 i;

	if (!btf_type_is_struct(t))
		return false;

	for_each_member(i, t, member) {
		const struct btf_array *array;