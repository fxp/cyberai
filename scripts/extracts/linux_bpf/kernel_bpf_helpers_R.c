// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 18/30



/**
 * bpf_dynptr_slice_rdwr() - Obtain a writable pointer to the dynptr data.
 * @p: The dynptr whose data slice to retrieve
 * @offset: Offset into the dynptr
 * @buffer__nullable: User-provided buffer to copy contents into. May be NULL
 * @buffer__szk: Size (in bytes) of the buffer if present. This is the
 *               length of the requested slice. This must be a constant.
 *
 * For non-skb and non-xdp type dynptrs, there is no difference between
 * bpf_dynptr_slice and bpf_dynptr_data.
 *
 * If buffer__nullable is NULL, the call will fail if buffer_opt was needed.
 *
 * The returned pointer is writable and may point to either directly the dynptr
 * data at the requested offset or to the buffer if unable to obtain a direct
 * data pointer to (example: the requested slice is to the paged area of an skb
 * packet). In the case where the returned pointer is to the buffer, the user
 * is responsible for persisting writes through calling bpf_dynptr_write(). This
 * usually looks something like this pattern:
 *
 * struct eth_hdr *eth = bpf_dynptr_slice_rdwr(&dynptr, 0, buffer, sizeof(buffer));
 * if (!eth)
 *	return TC_ACT_SHOT;
 *
 * // mutate eth header //
 *
 * if (eth == buffer)
 *	bpf_dynptr_write(&ptr, 0, buffer, sizeof(buffer), 0);
 *
 * Please note that, as in the example above, the user must check that the
 * returned pointer is not null before using it.
 *
 * Please also note that in the case of skb and xdp dynptrs, bpf_dynptr_slice_rdwr
 * does not change the underlying packet data pointers, so a call to
 * bpf_dynptr_slice_rdwr will not invalidate any ctx->data/data_end pointers in
 * the bpf program.
 *
 * Return: NULL if the call failed (eg invalid dynptr), pointer to a
 * data slice (can be either direct pointer to the data or a pointer to the user
 * provided buffer, with its contents containing the data, if unable to obtain
 * direct pointer)
 */
__bpf_kfunc void *bpf_dynptr_slice_rdwr(const struct bpf_dynptr *p, u64 offset,
					void *buffer__nullable, u64 buffer__szk)
{
	const struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;

	if (!ptr->data || __bpf_dynptr_is_rdonly(ptr))
		return NULL;

	/* bpf_dynptr_slice_rdwr is the same logic as bpf_dynptr_slice.
	 *
	 * For skb-type dynptrs, it is safe to write into the returned pointer
	 * if the bpf program allows skb data writes. There are two possibilities
	 * that may occur when calling bpf_dynptr_slice_rdwr:
	 *
	 * 1) The requested slice is in the head of the skb. In this case, the
	 * returned pointer is directly to skb data, and if the skb is cloned, the
	 * verifier will have uncloned it (see bpf_unclone_prologue()) already.
	 * The pointer can be directly written into.
	 *
	 * 2) Some portion of the requested slice is in the paged buffer area.
	 * In this case, the requested data will be copied out into the buffer
	 * and the returned pointer will be a pointer to the buffer. The skb
	 * will not be pulled. To persist the write, the user will need to call
	 * bpf_dynptr_write(), which will pull the skb and commit the write.
	 *
	 * Similarly for xdp programs, if the requested slice is not across xdp
	 * fragments, then a direct pointer will be returned, otherwise the data
	 * will be copied out into the buffer and the user will need to call
	 * bpf_dynptr_write() to commit changes.
	 */
	return bpf_dynptr_slice(p, offset, buffer__nullable, buffer__szk);
}

__bpf_kfunc int bpf_dynptr_adjust(const struct bpf_dynptr *p, u64 start, u64 end)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;
	u64 size;

	if (!ptr->data || start > end)
		return -EINVAL;

	size = __bpf_dynptr_size(ptr);

	if (start > size || end > size)
		return -ERANGE;

	bpf_dynptr_advance_offset(ptr, start);
	bpf_dynptr_set_size(ptr, end - start);

	return 0;
}

__bpf_kfunc bool bpf_dynptr_is_null(const struct bpf_dynptr *p)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;

	return !ptr->data;
}

__bpf_kfunc bool bpf_dynptr_is_rdonly(const struct bpf_dynptr *p)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;

	if (!ptr->data)
		return false;

	return __bpf_dynptr_is_rdonly(ptr);
}

__bpf_kfunc u64 bpf_dynptr_size(const struct bpf_dynptr *p)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;

	if (!ptr->data)
		return -EINVAL;

	return __bpf_dynptr_size(ptr);
}

__bpf_kfunc int bpf_dynptr_clone(const struct bpf_dynptr *p,
				 struct bpf_dynptr *clone__uninit)
{
	struct bpf_dynptr_kern *clone = (struct bpf_dynptr_kern *)clone__uninit;
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;

	if (!ptr->data) {
		bpf_dynptr_set_null(clone);
		return -EINVAL;
	}

	*clone = *ptr;

	return 0;
}