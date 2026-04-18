// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 13/30



	switch (func_id) {
	case BPF_FUNC_trace_printk:
		return bpf_get_trace_printk_proto();
	case BPF_FUNC_get_current_task:
		return &bpf_get_current_task_proto;
	case BPF_FUNC_get_current_task_btf:
		return &bpf_get_current_task_btf_proto;
	case BPF_FUNC_get_current_comm:
		return &bpf_get_current_comm_proto;
	case BPF_FUNC_probe_read_user:
		return &bpf_probe_read_user_proto;
	case BPF_FUNC_probe_read_kernel:
		return security_locked_down(LOCKDOWN_BPF_READ_KERNEL) < 0 ?
		       NULL : &bpf_probe_read_kernel_proto;
	case BPF_FUNC_probe_read_user_str:
		return &bpf_probe_read_user_str_proto;
	case BPF_FUNC_probe_read_kernel_str:
		return security_locked_down(LOCKDOWN_BPF_READ_KERNEL) < 0 ?
		       NULL : &bpf_probe_read_kernel_str_proto;
	case BPF_FUNC_copy_from_user:
		return &bpf_copy_from_user_proto;
	case BPF_FUNC_copy_from_user_task:
		return &bpf_copy_from_user_task_proto;
	case BPF_FUNC_snprintf_btf:
		return &bpf_snprintf_btf_proto;
	case BPF_FUNC_snprintf:
		return &bpf_snprintf_proto;
	case BPF_FUNC_task_pt_regs:
		return &bpf_task_pt_regs_proto;
	case BPF_FUNC_trace_vprintk:
		return bpf_get_trace_vprintk_proto();
	case BPF_FUNC_perf_event_read_value:
		return bpf_get_perf_event_read_value_proto();
	case BPF_FUNC_perf_event_read:
		return &bpf_perf_event_read_proto;
	case BPF_FUNC_send_signal:
		return &bpf_send_signal_proto;
	case BPF_FUNC_send_signal_thread:
		return &bpf_send_signal_thread_proto;
	case BPF_FUNC_get_task_stack:
		return prog->sleepable ? &bpf_get_task_stack_sleepable_proto
				       : &bpf_get_task_stack_proto;
	case BPF_FUNC_get_branch_snapshot:
		return &bpf_get_branch_snapshot_proto;
	case BPF_FUNC_find_vma:
		return &bpf_find_vma_proto;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL_GPL(bpf_base_func_proto);

void bpf_list_head_free(const struct btf_field *field, void *list_head,
			struct bpf_spin_lock *spin_lock)
{
	struct list_head *head = list_head, *orig_head = list_head;

	BUILD_BUG_ON(sizeof(struct list_head) > sizeof(struct bpf_list_head));
	BUILD_BUG_ON(__alignof__(struct list_head) > __alignof__(struct bpf_list_head));

	/* Do the actual list draining outside the lock to not hold the lock for
	 * too long, and also prevent deadlocks if tracing programs end up
	 * executing on entry/exit of functions called inside the critical
	 * section, and end up doing map ops that call bpf_list_head_free for
	 * the same map value again.
	 */
	__bpf_spin_lock_irqsave(spin_lock);
	if (!head->next || list_empty(head))
		goto unlock;
	head = head->next;
unlock:
	INIT_LIST_HEAD(orig_head);
	__bpf_spin_unlock_irqrestore(spin_lock);

	while (head != orig_head) {
		void *obj = head;

		obj -= field->graph_root.node_offset;
		head = head->next;
		/* The contained type can also have resources, including a
		 * bpf_list_head which needs to be freed.
		 */
		__bpf_obj_drop_impl(obj, field->graph_root.value_rec, false);
	}
}

/* Like rbtree_postorder_for_each_entry_safe, but 'pos' and 'n' are
 * 'rb_node *', so field name of rb_node within containing struct is not
 * needed.
 *
 * Since bpf_rb_tree's node type has a corresponding struct btf_field with
 * graph_root.node_offset, it's not necessary to know field name
 * or type of node struct
 */
#define bpf_rbtree_postorder_for_each_entry_safe(pos, n, root) \
	for (pos = rb_first_postorder(root); \
	    pos && ({ n = rb_next_postorder(pos); 1; }); \
	    pos = n)

void bpf_rb_root_free(const struct btf_field *field, void *rb_root,
		      struct bpf_spin_lock *spin_lock)
{
	struct rb_root_cached orig_root, *root = rb_root;
	struct rb_node *pos, *n;
	void *obj;

	BUILD_BUG_ON(sizeof(struct rb_root_cached) > sizeof(struct bpf_rb_root));
	BUILD_BUG_ON(__alignof__(struct rb_root_cached) > __alignof__(struct bpf_rb_root));

	__bpf_spin_lock_irqsave(spin_lock);
	orig_root = *root;
	*root = RB_ROOT_CACHED;
	__bpf_spin_unlock_irqrestore(spin_lock);

	bpf_rbtree_postorder_for_each_entry_safe(pos, n, &orig_root.rb_root) {
		obj = pos;
		obj -= field->graph_root.node_offset;


		__bpf_obj_drop_impl(obj, field->graph_root.value_rec, false);
	}
}

__bpf_kfunc_start_defs();

/**
 * bpf_obj_new() - allocate an object described by program BTF
 * @local_type_id__k: type ID in program BTF
 * @meta: verifier-supplied struct metadata
 *
 * Allocate an object of the type identified by @local_type_id__k and
 * initialize its special fields. BPF programs can use
 * bpf_core_type_id_local() to provide @local_type_id__k. The verifier
 * rewrites @meta; BPF programs do not set it.
 *
 * Return: Pointer to the allocated object, or %NULL on failure.
 */
__bpf_kfunc void *bpf_obj_new(u64 local_type_id__k, struct btf_struct_meta *meta)
{
	u64 size = local_type_id__k;
	void *p;

	p = bpf_mem_alloc(&bpf_global_ma, size);
	if (!p)
		return NULL;
	if (meta)
		bpf_obj_init(meta->record, p);

	return p;
}

__bpf_kfunc void *bpf_obj_new_impl(u64 local_type_id__k, void *meta__ign)
{
	return bpf_obj_new(local_type_id__k, meta__ign);
}