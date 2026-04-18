// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 11/30



	type = bpf_dynptr_get_type(dst);

	switch (type) {
	case BPF_DYNPTR_TYPE_LOCAL:
	case BPF_DYNPTR_TYPE_RINGBUF:
		if (flags)
			return -EINVAL;
		/* Source and destination may possibly overlap, hence use memmove to
		 * copy the data. E.g. bpf_dynptr_from_mem may create two dynptr
		 * pointing to overlapping PTR_TO_MAP_VALUE regions.
		 */
		memmove(dst->data + dst->offset + offset, src, len);
		return 0;
	case BPF_DYNPTR_TYPE_SKB:
		return __bpf_skb_store_bytes(dst->data, dst->offset + offset, src, len,
					     flags);
	case BPF_DYNPTR_TYPE_XDP:
		if (flags)
			return -EINVAL;
		return __bpf_xdp_store_bytes(dst->data, dst->offset + offset, src, len);
	case BPF_DYNPTR_TYPE_SKB_META:
		return __bpf_skb_meta_store_bytes(dst->data, dst->offset + offset, src,
						  len, flags);
	default:
		WARN_ONCE(true, "bpf_dynptr_write: unknown dynptr type %d\n", type);
		return -EFAULT;
	}
}

BPF_CALL_5(bpf_dynptr_write, const struct bpf_dynptr_kern *, dst, u64, offset, void *, src,
	   u64, len, u64, flags)
{
	return __bpf_dynptr_write(dst, offset, src, len, flags);
}

static const struct bpf_func_proto bpf_dynptr_write_proto = {
	.func		= bpf_dynptr_write,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_DYNPTR | MEM_RDONLY,
	.arg2_type	= ARG_ANYTHING,
	.arg3_type	= ARG_PTR_TO_MEM | MEM_RDONLY,
	.arg4_type	= ARG_CONST_SIZE_OR_ZERO,
	.arg5_type	= ARG_ANYTHING,
};

BPF_CALL_3(bpf_dynptr_data, const struct bpf_dynptr_kern *, ptr, u64, offset, u64, len)
{
	enum bpf_dynptr_type type;
	int err;

	if (!ptr->data)
		return 0;

	err = bpf_dynptr_check_off_len(ptr, offset, len);
	if (err)
		return 0;

	if (__bpf_dynptr_is_rdonly(ptr))
		return 0;

	type = bpf_dynptr_get_type(ptr);

	switch (type) {
	case BPF_DYNPTR_TYPE_LOCAL:
	case BPF_DYNPTR_TYPE_RINGBUF:
		return (unsigned long)(ptr->data + ptr->offset + offset);
	case BPF_DYNPTR_TYPE_SKB:
	case BPF_DYNPTR_TYPE_XDP:
	case BPF_DYNPTR_TYPE_SKB_META:
		/* skb and xdp dynptrs should use bpf_dynptr_slice / bpf_dynptr_slice_rdwr */
		return 0;
	default:
		WARN_ONCE(true, "bpf_dynptr_data: unknown dynptr type %d\n", type);
		return 0;
	}
}

static const struct bpf_func_proto bpf_dynptr_data_proto = {
	.func		= bpf_dynptr_data,
	.gpl_only	= false,
	.ret_type	= RET_PTR_TO_DYNPTR_MEM_OR_NULL,
	.arg1_type	= ARG_PTR_TO_DYNPTR | MEM_RDONLY,
	.arg2_type	= ARG_ANYTHING,
	.arg3_type	= ARG_CONST_ALLOC_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_get_current_task_proto __weak;
const struct bpf_func_proto bpf_get_current_task_btf_proto __weak;
const struct bpf_func_proto bpf_probe_read_user_proto __weak;
const struct bpf_func_proto bpf_probe_read_user_str_proto __weak;
const struct bpf_func_proto bpf_probe_read_kernel_proto __weak;
const struct bpf_func_proto bpf_probe_read_kernel_str_proto __weak;
const struct bpf_func_proto bpf_task_pt_regs_proto __weak;
const struct bpf_func_proto bpf_perf_event_read_proto __weak;
const struct bpf_func_proto bpf_send_signal_proto __weak;
const struct bpf_func_proto bpf_send_signal_thread_proto __weak;
const struct bpf_func_proto bpf_get_task_stack_sleepable_proto __weak;
const struct bpf_func_proto bpf_get_task_stack_proto __weak;
const struct bpf_func_proto bpf_get_branch_snapshot_proto __weak;