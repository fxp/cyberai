// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 78/132



static bool is_sync_callback_calling_kfunc(u32 btf_id)
{
	return is_bpf_rbtree_add_kfunc(btf_id);
}

static bool is_async_callback_calling_kfunc(u32 btf_id)
{
	return is_bpf_wq_set_callback_kfunc(btf_id) ||
	       is_task_work_add_kfunc(btf_id);
}

static bool is_bpf_throw_kfunc(struct bpf_insn *insn)
{
	return bpf_pseudo_kfunc_call(insn) && insn->off == 0 &&
	       insn->imm == special_kfunc_list[KF_bpf_throw];
}

static bool is_bpf_wq_set_callback_kfunc(u32 btf_id)
{
	return btf_id == special_kfunc_list[KF_bpf_wq_set_callback];
}

static bool is_callback_calling_kfunc(u32 btf_id)
{
	return is_sync_callback_calling_kfunc(btf_id) ||
	       is_async_callback_calling_kfunc(btf_id);
}

static bool is_rbtree_lock_required_kfunc(u32 btf_id)
{
	return is_bpf_rbtree_api_kfunc(btf_id);
}

static bool check_kfunc_is_graph_root_api(struct bpf_verifier_env *env,
					  enum btf_field_type head_field_type,
					  u32 kfunc_btf_id)
{
	bool ret;

	switch (head_field_type) {
	case BPF_LIST_HEAD:
		ret = is_bpf_list_api_kfunc(kfunc_btf_id);
		break;
	case BPF_RB_ROOT:
		ret = is_bpf_rbtree_api_kfunc(kfunc_btf_id);
		break;
	default:
		verbose(env, "verifier internal error: unexpected graph root argument type %s\n",
			btf_field_type_name(head_field_type));
		return false;
	}

	if (!ret)
		verbose(env, "verifier internal error: %s head arg for unknown kfunc\n",
			btf_field_type_name(head_field_type));
	return ret;
}

static bool check_kfunc_is_graph_node_api(struct bpf_verifier_env *env,
					  enum btf_field_type node_field_type,
					  u32 kfunc_btf_id)
{
	bool ret;

	switch (node_field_type) {
	case BPF_LIST_NODE:
		ret = is_bpf_list_push_kfunc(kfunc_btf_id);
		break;
	case BPF_RB_NODE:
		ret = (is_bpf_rbtree_add_kfunc(kfunc_btf_id) ||
		       kfunc_btf_id == special_kfunc_list[KF_bpf_rbtree_remove] ||
		       kfunc_btf_id == special_kfunc_list[KF_bpf_rbtree_left] ||
		       kfunc_btf_id == special_kfunc_list[KF_bpf_rbtree_right]);
		break;
	default:
		verbose(env, "verifier internal error: unexpected graph node argument type %s\n",
			btf_field_type_name(node_field_type));
		return false;
	}

	if (!ret)
		verbose(env, "verifier internal error: %s node arg for unknown kfunc\n",
			btf_field_type_name(node_field_type));
	return ret;
}

static int
__process_kf_arg_ptr_to_graph_root(struct bpf_verifier_env *env,
				   struct bpf_reg_state *reg, u32 regno,
				   struct bpf_kfunc_call_arg_meta *meta,
				   enum btf_field_type head_field_type,
				   struct btf_field **head_field)
{
	const char *head_type_name;
	struct btf_field *field;
	struct btf_record *rec;
	u32 head_off;

	if (meta->btf != btf_vmlinux) {
		verifier_bug(env, "unexpected btf mismatch in kfunc call");
		return -EFAULT;
	}

	if (!check_kfunc_is_graph_root_api(env, head_field_type, meta->func_id))
		return -EFAULT;

	head_type_name = btf_field_type_name(head_field_type);
	if (!tnum_is_const(reg->var_off)) {
		verbose(env,
			"R%d doesn't have constant offset. %s has to be at the constant offset\n",
			regno, head_type_name);
		return -EINVAL;
	}

	rec = reg_btf_record(reg);
	head_off = reg->var_off.value;
	field = btf_record_find(rec, head_off, head_field_type);
	if (!field) {
		verbose(env, "%s not found at offset=%u\n", head_type_name, head_off);
		return -EINVAL;
	}

	/* All functions require bpf_list_head to be protected using a bpf_spin_lock */
	if (check_reg_allocation_locked(env, reg)) {
		verbose(env, "bpf_spin_lock at off=%d must be held for %s\n",
			rec->spin_lock_off, head_type_name);
		return -EINVAL;
	}

	if (*head_field) {
		verifier_bug(env, "repeating %s arg", head_type_name);
		return -EFAULT;
	}
	*head_field = field;
	return 0;
}

static int process_kf_arg_ptr_to_list_head(struct bpf_verifier_env *env,
					   struct bpf_reg_state *reg, u32 regno,
					   struct bpf_kfunc_call_arg_meta *meta)
{
	return __process_kf_arg_ptr_to_graph_root(env, reg, regno, meta, BPF_LIST_HEAD,
							  &meta->arg_list_head.field);
}

static int process_kf_arg_ptr_to_rbtree_root(struct bpf_verifier_env *env,
					     struct bpf_reg_state *reg, u32 regno,
					     struct bpf_kfunc_call_arg_meta *meta)
{
	return __process_kf_arg_ptr_to_graph_root(env, reg, regno, meta, BPF_RB_ROOT,
							  &meta->arg_rbtree_root.field);
}

static int
__process_kf_arg_ptr_to_graph_node(struct bpf_verifier_env *env,
				   struct bpf_reg_state *reg, u32 regno,
				   struct bpf_kfunc_call_arg_meta *meta,
				   enum btf_field_type head_field_type,
				   enum btf_field_type node_field_type,
				   struct btf_field **node_field)
{
	const char *node_type_name;
	const struct btf_type *et, *t;
	struct btf_field *field;
	u32 node_off;

	if (meta->btf != btf_vmlinux) {
		verifier_bug(env, "unexpected btf mismatch in kfunc call");
		return -EFAULT;
	}

	if (!check_kfunc_is_graph_node_api(env, node_field_type, meta->func_id))
		return -EFAULT;