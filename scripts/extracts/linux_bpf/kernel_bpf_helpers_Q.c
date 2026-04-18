// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 17/30



const struct bpf_func_proto bpf_current_task_under_cgroup_proto = {
	.func           = bpf_current_task_under_cgroup,
	.gpl_only       = false,
	.ret_type       = RET_INTEGER,
	.arg1_type      = ARG_CONST_MAP_PTR,
	.arg2_type      = ARG_ANYTHING,
};

/**
 * bpf_task_get_cgroup1 - Acquires the associated cgroup of a task within a
 * specific cgroup1 hierarchy. The cgroup1 hierarchy is identified by its
 * hierarchy ID.
 * @task: The target task
 * @hierarchy_id: The ID of a cgroup1 hierarchy
 *
 * On success, the cgroup is returen. On failure, NULL is returned.
 */
__bpf_kfunc struct cgroup *
bpf_task_get_cgroup1(struct task_struct *task, int hierarchy_id)
{
	struct cgroup *cgrp = task_get_cgroup1(task, hierarchy_id);

	if (IS_ERR(cgrp))
		return NULL;
	return cgrp;
}
#endif /* CONFIG_CGROUPS */

/**
 * bpf_task_from_pid - Find a struct task_struct from its pid by looking it up
 * in the root pid namespace idr. If a task is returned, it must either be
 * stored in a map, or released with bpf_task_release().
 * @pid: The pid of the task being looked up.
 */
__bpf_kfunc struct task_struct *bpf_task_from_pid(s32 pid)
{
	struct task_struct *p;

	rcu_read_lock();
	p = find_task_by_pid_ns(pid, &init_pid_ns);
	if (p)
		p = bpf_task_acquire(p);
	rcu_read_unlock();

	return p;
}

/**
 * bpf_task_from_vpid - Find a struct task_struct from its vpid by looking it up
 * in the pid namespace of the current task. If a task is returned, it must
 * either be stored in a map, or released with bpf_task_release().
 * @vpid: The vpid of the task being looked up.
 */
__bpf_kfunc struct task_struct *bpf_task_from_vpid(s32 vpid)
{
	struct task_struct *p;

	rcu_read_lock();
	p = find_task_by_vpid(vpid);
	if (p)
		p = bpf_task_acquire(p);
	rcu_read_unlock();

	return p;
}

/**
 * bpf_dynptr_slice() - Obtain a read-only pointer to the dynptr data.
 * @p: The dynptr whose data slice to retrieve
 * @offset: Offset into the dynptr
 * @buffer__nullable: User-provided buffer to copy contents into.  May be NULL
 * @buffer__szk: Size (in bytes) of the buffer if present. This is the
 *               length of the requested slice. This must be a constant.
 *
 * For non-skb and non-xdp type dynptrs, there is no difference between
 * bpf_dynptr_slice and bpf_dynptr_data.
 *
 *  If buffer__nullable is NULL, the call will fail if buffer_opt was needed.
 *
 * If the intention is to write to the data slice, please use
 * bpf_dynptr_slice_rdwr.
 *
 * The user must check that the returned pointer is not null before using it.
 *
 * Please note that in the case of skb and xdp dynptrs, bpf_dynptr_slice
 * does not change the underlying packet data pointers, so a call to
 * bpf_dynptr_slice will not invalidate any ctx->data/data_end pointers in
 * the bpf program.
 *
 * Return: NULL if the call failed (eg invalid dynptr), pointer to a read-only
 * data slice (can be either direct pointer to the data or a pointer to the user
 * provided buffer, with its contents containing the data, if unable to obtain
 * direct pointer)
 */
__bpf_kfunc void *bpf_dynptr_slice(const struct bpf_dynptr *p, u64 offset,
				   void *buffer__nullable, u64 buffer__szk)
{
	const struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;
	enum bpf_dynptr_type type;
	u64 len = buffer__szk;
	int err;

	if (!ptr->data)
		return NULL;

	err = bpf_dynptr_check_off_len(ptr, offset, len);
	if (err)
		return NULL;

	type = bpf_dynptr_get_type(ptr);

	switch (type) {
	case BPF_DYNPTR_TYPE_LOCAL:
	case BPF_DYNPTR_TYPE_RINGBUF:
		return ptr->data + ptr->offset + offset;
	case BPF_DYNPTR_TYPE_SKB:
		if (buffer__nullable)
			return skb_header_pointer(ptr->data, ptr->offset + offset, len, buffer__nullable);
		else
			return skb_pointer_if_linear(ptr->data, ptr->offset + offset, len);
	case BPF_DYNPTR_TYPE_XDP:
	{
		void *xdp_ptr = bpf_xdp_pointer(ptr->data, ptr->offset + offset, len);
		if (!IS_ERR_OR_NULL(xdp_ptr))
			return xdp_ptr;

		if (!buffer__nullable)
			return NULL;
		bpf_xdp_copy_buf(ptr->data, ptr->offset + offset, buffer__nullable, len, false);
		return buffer__nullable;
	}
	case BPF_DYNPTR_TYPE_SKB_META:
		return bpf_skb_meta_pointer(ptr->data, ptr->offset + offset);
	case BPF_DYNPTR_TYPE_FILE:
		err = bpf_file_fetch_bytes(ptr->data, offset, buffer__nullable, buffer__szk);
		return err ? NULL : buffer__nullable;
	default:
		WARN_ONCE(true, "unknown dynptr type %d\n", type);
		return NULL;
	}
}