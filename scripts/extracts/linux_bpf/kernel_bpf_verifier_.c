// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 74/132



BTF_ID_LIST(special_kfunc_list)
BTF_ID(func, bpf_obj_new_impl)
BTF_ID(func, bpf_obj_new)
BTF_ID(func, bpf_obj_drop_impl)
BTF_ID(func, bpf_obj_drop)
BTF_ID(func, bpf_refcount_acquire_impl)
BTF_ID(func, bpf_refcount_acquire)
BTF_ID(func, bpf_list_push_front_impl)
BTF_ID(func, bpf_list_push_front)
BTF_ID(func, bpf_list_push_back_impl)
BTF_ID(func, bpf_list_push_back)
BTF_ID(func, bpf_list_pop_front)
BTF_ID(func, bpf_list_pop_back)
BTF_ID(func, bpf_list_front)
BTF_ID(func, bpf_list_back)
BTF_ID(func, bpf_cast_to_kern_ctx)
BTF_ID(func, bpf_rdonly_cast)
BTF_ID(func, bpf_rcu_read_lock)
BTF_ID(func, bpf_rcu_read_unlock)
BTF_ID(func, bpf_rbtree_remove)
BTF_ID(func, bpf_rbtree_add_impl)
BTF_ID(func, bpf_rbtree_add)
BTF_ID(func, bpf_rbtree_first)
BTF_ID(func, bpf_rbtree_root)
BTF_ID(func, bpf_rbtree_left)
BTF_ID(func, bpf_rbtree_right)
#ifdef CONFIG_NET
BTF_ID(func, bpf_dynptr_from_skb)
BTF_ID(func, bpf_dynptr_from_xdp)
BTF_ID(func, bpf_dynptr_from_skb_meta)
BTF_ID(func, bpf_xdp_pull_data)
#else
BTF_ID_UNUSED
BTF_ID_UNUSED
BTF_ID_UNUSED
BTF_ID_UNUSED
#endif
BTF_ID(func, bpf_dynptr_slice)
BTF_ID(func, bpf_dynptr_slice_rdwr)
BTF_ID(func, bpf_dynptr_clone)
BTF_ID(func, bpf_percpu_obj_new_impl)
BTF_ID(func, bpf_percpu_obj_new)
BTF_ID(func, bpf_percpu_obj_drop_impl)
BTF_ID(func, bpf_percpu_obj_drop)
BTF_ID(func, bpf_throw)
BTF_ID(func, bpf_wq_set_callback)
BTF_ID(func, bpf_preempt_disable)
BTF_ID(func, bpf_preempt_enable)
#ifdef CONFIG_CGROUPS
BTF_ID(func, bpf_iter_css_task_new)
#else
BTF_ID_UNUSED
#endif
#ifdef CONFIG_BPF_EVENTS
BTF_ID(func, bpf_session_cookie)
#else
BTF_ID_UNUSED
#endif
BTF_ID(func, bpf_get_kmem_cache)
BTF_ID(func, bpf_local_irq_save)
BTF_ID(func, bpf_local_irq_restore)
BTF_ID(func, bpf_iter_num_new)
BTF_ID(func, bpf_iter_num_next)
BTF_ID(func, bpf_iter_num_destroy)
#ifdef CONFIG_BPF_LSM
BTF_ID(func, bpf_set_dentry_xattr)
BTF_ID(func, bpf_remove_dentry_xattr)
#else
BTF_ID_UNUSED
BTF_ID_UNUSED
#endif
BTF_ID(func, bpf_res_spin_lock)
BTF_ID(func, bpf_res_spin_unlock)
BTF_ID(func, bpf_res_spin_lock_irqsave)
BTF_ID(func, bpf_res_spin_unlock_irqrestore)
BTF_ID(func, bpf_dynptr_from_file)
BTF_ID(func, bpf_dynptr_file_discard)
BTF_ID(func, __bpf_trap)
BTF_ID(func, bpf_task_work_schedule_signal)
BTF_ID(func, bpf_task_work_schedule_resume)
BTF_ID(func, bpf_arena_alloc_pages)
BTF_ID(func, bpf_arena_free_pages)
BTF_ID(func, bpf_arena_reserve_pages)
BTF_ID(func, bpf_session_is_return)
BTF_ID(func, bpf_stream_vprintk)
BTF_ID(func, bpf_stream_print_stack)

static bool is_bpf_obj_new_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_obj_new] ||
	       func_id == special_kfunc_list[KF_bpf_obj_new_impl];
}

static bool is_bpf_percpu_obj_new_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_percpu_obj_new] ||
	       func_id == special_kfunc_list[KF_bpf_percpu_obj_new_impl];
}

static bool is_bpf_obj_drop_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_obj_drop] ||
	       func_id == special_kfunc_list[KF_bpf_obj_drop_impl];
}

static bool is_bpf_percpu_obj_drop_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_percpu_obj_drop] ||
	       func_id == special_kfunc_list[KF_bpf_percpu_obj_drop_impl];
}

static bool is_bpf_refcount_acquire_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_refcount_acquire] ||
	       func_id == special_kfunc_list[KF_bpf_refcount_acquire_impl];
}

static bool is_bpf_list_push_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_list_push_front] ||
	       func_id == special_kfunc_list[KF_bpf_list_push_front_impl] ||
	       func_id == special_kfunc_list[KF_bpf_list_push_back] ||
	       func_id == special_kfunc_list[KF_bpf_list_push_back_impl];
}

static bool is_bpf_rbtree_add_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_rbtree_add] ||
	       func_id == special_kfunc_list[KF_bpf_rbtree_add_impl];
}

static bool is_task_work_add_kfunc(u32 func_id)
{
	return func_id == special_kfunc_list[KF_bpf_task_work_schedule_signal] ||
	       func_id == special_kfunc_list[KF_bpf_task_work_schedule_resume];
}

static bool is_kfunc_ret_null(struct bpf_kfunc_call_arg_meta *meta)
{
	if (is_bpf_refcount_acquire_kfunc(meta->func_id) && meta->arg_owning_ref)
		return false;

	return meta->kfunc_flags & KF_RET_NULL;
}

static bool is_kfunc_bpf_rcu_read_lock(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->func_id == special_kfunc_list[KF_bpf_rcu_read_lock];
}

static bool is_kfunc_bpf_rcu_read_unlock(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->func_id == special_kfunc_list[KF_bpf_rcu_read_unlock];
}

static bool is_kfunc_bpf_preempt_disable(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->func_id == special_kfunc_list[KF_bpf_preempt_disable];
}

static bool is_kfunc_bpf_preempt_enable(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->func_id == special_kfunc_list[KF_bpf_preempt_enable];
}