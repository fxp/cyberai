// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 73/132



		member_type = btf_type_skip_modifiers(btf, member->type, NULL);
		if (btf_type_is_struct(member_type)) {
			if (rec >= 3) {
				verbose(env, "max struct nesting depth exceeded\n");
				return false;
			}
			if (!__btf_type_is_scalar_struct(env, btf, member_type, rec + 1))
				return false;
			continue;
		}
		if (btf_type_is_array(member_type)) {
			array = btf_array(member_type);
			if (!array->nelems)
				return false;
			member_type = btf_type_skip_modifiers(btf, array->type, NULL);
			if (!btf_type_is_scalar(member_type))
				return false;
			continue;
		}
		if (!btf_type_is_scalar(member_type))
			return false;
	}
	return true;
}

enum kfunc_ptr_arg_type {
	KF_ARG_PTR_TO_CTX,
	KF_ARG_PTR_TO_ALLOC_BTF_ID,    /* Allocated object */
	KF_ARG_PTR_TO_REFCOUNTED_KPTR, /* Refcounted local kptr */
	KF_ARG_PTR_TO_DYNPTR,
	KF_ARG_PTR_TO_ITER,
	KF_ARG_PTR_TO_LIST_HEAD,
	KF_ARG_PTR_TO_LIST_NODE,
	KF_ARG_PTR_TO_BTF_ID,	       /* Also covers reg2btf_ids conversions */
	KF_ARG_PTR_TO_MEM,
	KF_ARG_PTR_TO_MEM_SIZE,	       /* Size derived from next argument, skip it */
	KF_ARG_PTR_TO_CALLBACK,
	KF_ARG_PTR_TO_RB_ROOT,
	KF_ARG_PTR_TO_RB_NODE,
	KF_ARG_PTR_TO_NULL,
	KF_ARG_PTR_TO_CONST_STR,
	KF_ARG_PTR_TO_MAP,
	KF_ARG_PTR_TO_TIMER,
	KF_ARG_PTR_TO_WORKQUEUE,
	KF_ARG_PTR_TO_IRQ_FLAG,
	KF_ARG_PTR_TO_RES_SPIN_LOCK,
	KF_ARG_PTR_TO_TASK_WORK,
};

enum special_kfunc_type {
	KF_bpf_obj_new_impl,
	KF_bpf_obj_new,
	KF_bpf_obj_drop_impl,
	KF_bpf_obj_drop,
	KF_bpf_refcount_acquire_impl,
	KF_bpf_refcount_acquire,
	KF_bpf_list_push_front_impl,
	KF_bpf_list_push_front,
	KF_bpf_list_push_back_impl,
	KF_bpf_list_push_back,
	KF_bpf_list_pop_front,
	KF_bpf_list_pop_back,
	KF_bpf_list_front,
	KF_bpf_list_back,
	KF_bpf_cast_to_kern_ctx,
	KF_bpf_rdonly_cast,
	KF_bpf_rcu_read_lock,
	KF_bpf_rcu_read_unlock,
	KF_bpf_rbtree_remove,
	KF_bpf_rbtree_add_impl,
	KF_bpf_rbtree_add,
	KF_bpf_rbtree_first,
	KF_bpf_rbtree_root,
	KF_bpf_rbtree_left,
	KF_bpf_rbtree_right,
	KF_bpf_dynptr_from_skb,
	KF_bpf_dynptr_from_xdp,
	KF_bpf_dynptr_from_skb_meta,
	KF_bpf_xdp_pull_data,
	KF_bpf_dynptr_slice,
	KF_bpf_dynptr_slice_rdwr,
	KF_bpf_dynptr_clone,
	KF_bpf_percpu_obj_new_impl,
	KF_bpf_percpu_obj_new,
	KF_bpf_percpu_obj_drop_impl,
	KF_bpf_percpu_obj_drop,
	KF_bpf_throw,
	KF_bpf_wq_set_callback,
	KF_bpf_preempt_disable,
	KF_bpf_preempt_enable,
	KF_bpf_iter_css_task_new,
	KF_bpf_session_cookie,
	KF_bpf_get_kmem_cache,
	KF_bpf_local_irq_save,
	KF_bpf_local_irq_restore,
	KF_bpf_iter_num_new,
	KF_bpf_iter_num_next,
	KF_bpf_iter_num_destroy,
	KF_bpf_set_dentry_xattr,
	KF_bpf_remove_dentry_xattr,
	KF_bpf_res_spin_lock,
	KF_bpf_res_spin_unlock,
	KF_bpf_res_spin_lock_irqsave,
	KF_bpf_res_spin_unlock_irqrestore,
	KF_bpf_dynptr_from_file,
	KF_bpf_dynptr_file_discard,
	KF___bpf_trap,
	KF_bpf_task_work_schedule_signal,
	KF_bpf_task_work_schedule_resume,
	KF_bpf_arena_alloc_pages,
	KF_bpf_arena_free_pages,
	KF_bpf_arena_reserve_pages,
	KF_bpf_session_is_return,
	KF_bpf_stream_vprintk,
	KF_bpf_stream_print_stack,
};