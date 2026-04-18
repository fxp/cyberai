// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 75/132



bool bpf_is_kfunc_pkt_changing(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->func_id == special_kfunc_list[KF_bpf_xdp_pull_data];
}

static enum kfunc_ptr_arg_type
get_kfunc_ptr_arg_type(struct bpf_verifier_env *env,
		       struct bpf_kfunc_call_arg_meta *meta,
		       const struct btf_type *t, const struct btf_type *ref_t,
		       const char *ref_tname, const struct btf_param *args,
		       int argno, int nargs)
{
	u32 regno = argno + 1;
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *reg = &regs[regno];
	bool arg_mem_size = false;

	if (meta->func_id == special_kfunc_list[KF_bpf_cast_to_kern_ctx] ||
	    meta->func_id == special_kfunc_list[KF_bpf_session_is_return] ||
	    meta->func_id == special_kfunc_list[KF_bpf_session_cookie])
		return KF_ARG_PTR_TO_CTX;

	if (argno + 1 < nargs &&
	    (is_kfunc_arg_mem_size(meta->btf, &args[argno + 1], &regs[regno + 1]) ||
	     is_kfunc_arg_const_mem_size(meta->btf, &args[argno + 1], &regs[regno + 1])))
		arg_mem_size = true;

	/* In this function, we verify the kfunc's BTF as per the argument type,
	 * leaving the rest of the verification with respect to the register
	 * type to our caller. When a set of conditions hold in the BTF type of
	 * arguments, we resolve it to a known kfunc_ptr_arg_type.
	 */
	if (btf_is_prog_ctx_type(&env->log, meta->btf, t, resolve_prog_type(env->prog), argno))
		return KF_ARG_PTR_TO_CTX;

	if (is_kfunc_arg_nullable(meta->btf, &args[argno]) && bpf_register_is_null(reg) &&
	    !arg_mem_size)
		return KF_ARG_PTR_TO_NULL;

	if (is_kfunc_arg_alloc_obj(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_ALLOC_BTF_ID;

	if (is_kfunc_arg_refcounted_kptr(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_REFCOUNTED_KPTR;

	if (is_kfunc_arg_dynptr(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_DYNPTR;

	if (is_kfunc_arg_iter(meta, argno, &args[argno]))
		return KF_ARG_PTR_TO_ITER;

	if (is_kfunc_arg_list_head(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_LIST_HEAD;

	if (is_kfunc_arg_list_node(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_LIST_NODE;

	if (is_kfunc_arg_rbtree_root(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_RB_ROOT;

	if (is_kfunc_arg_rbtree_node(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_RB_NODE;

	if (is_kfunc_arg_const_str(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_CONST_STR;

	if (is_kfunc_arg_map(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_MAP;

	if (is_kfunc_arg_wq(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_WORKQUEUE;

	if (is_kfunc_arg_timer(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_TIMER;

	if (is_kfunc_arg_task_work(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_TASK_WORK;

	if (is_kfunc_arg_irq_flag(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_IRQ_FLAG;

	if (is_kfunc_arg_res_spin_lock(meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_RES_SPIN_LOCK;

	if ((base_type(reg->type) == PTR_TO_BTF_ID || reg2btf_ids[base_type(reg->type)])) {
		if (!btf_type_is_struct(ref_t)) {
			verbose(env, "kernel function %s args#%d pointer type %s %s is not supported\n",
				meta->func_name, argno, btf_type_str(ref_t), ref_tname);
			return -EINVAL;
		}
		return KF_ARG_PTR_TO_BTF_ID;
	}

	if (is_kfunc_arg_callback(env, meta->btf, &args[argno]))
		return KF_ARG_PTR_TO_CALLBACK;

	/* This is the catch all argument type of register types supported by
	 * check_helper_mem_access. However, we only allow when argument type is
	 * pointer to scalar, or struct composed (recursively) of scalars. When
	 * arg_mem_size is true, the pointer can be void *.
	 */
	if (!btf_type_is_scalar(ref_t) && !__btf_type_is_scalar_struct(env, meta->btf, ref_t, 0) &&
	    (arg_mem_size ? !btf_type_is_void(ref_t) : 1)) {
		verbose(env, "arg#%d pointer type %s %s must point to %sscalar, or struct with scalar\n",
			argno, btf_type_str(ref_t), ref_tname, arg_mem_size ? "void, " : "");
		return -EINVAL;
	}
	return arg_mem_size ? KF_ARG_PTR_TO_MEM_SIZE : KF_ARG_PTR_TO_MEM;
}

static int process_kf_arg_ptr_to_btf_id(struct bpf_verifier_env *env,
					struct bpf_reg_state *reg,
					const struct btf_type *ref_t,
					const char *ref_tname, u32 ref_id,
					struct bpf_kfunc_call_arg_meta *meta,
					int argno)
{
	const struct btf_type *reg_ref_t;
	bool strict_type_match = false;
	const struct btf *reg_btf;
	const char *reg_ref_tname;
	bool taking_projection;
	bool struct_same;
	u32 reg_ref_id;

	if (base_type(reg->type) == PTR_TO_BTF_ID) {
		reg_btf = reg->btf;
		reg_ref_id = reg->btf_id;
	} else {
		reg_btf = btf_vmlinux;
		reg_ref_id = *reg2btf_ids[base_type(reg->type)];
	}