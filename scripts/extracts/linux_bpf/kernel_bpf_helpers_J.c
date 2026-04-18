// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 10/30



/* Since the upper 8 bits of dynptr->size is reserved, the
 * maximum supported size is 2^24 - 1.
 */
#define DYNPTR_MAX_SIZE	((1UL << 24) - 1)
#define DYNPTR_TYPE_SHIFT	28
#define DYNPTR_SIZE_MASK	0xFFFFFF
#define DYNPTR_RDONLY_BIT	BIT(31)

bool __bpf_dynptr_is_rdonly(const struct bpf_dynptr_kern *ptr)
{
	return ptr->size & DYNPTR_RDONLY_BIT;
}

void bpf_dynptr_set_rdonly(struct bpf_dynptr_kern *ptr)
{
	ptr->size |= DYNPTR_RDONLY_BIT;
}

static void bpf_dynptr_set_type(struct bpf_dynptr_kern *ptr, enum bpf_dynptr_type type)
{
	ptr->size |= type << DYNPTR_TYPE_SHIFT;
}

static enum bpf_dynptr_type bpf_dynptr_get_type(const struct bpf_dynptr_kern *ptr)
{
	return (ptr->size & ~(DYNPTR_RDONLY_BIT)) >> DYNPTR_TYPE_SHIFT;
}

u64 __bpf_dynptr_size(const struct bpf_dynptr_kern *ptr)
{
	if (bpf_dynptr_get_type(ptr) == BPF_DYNPTR_TYPE_FILE) {
		struct bpf_dynptr_file_impl *df = ptr->data;

		return df->size;
	}

	return ptr->size & DYNPTR_SIZE_MASK;
}

static void bpf_dynptr_advance_offset(struct bpf_dynptr_kern *ptr, u64 off)
{
	if (bpf_dynptr_get_type(ptr) == BPF_DYNPTR_TYPE_FILE) {
		struct bpf_dynptr_file_impl *df = ptr->data;

		df->offset += off;
		return;
	}
	ptr->offset += off;
}

static void bpf_dynptr_set_size(struct bpf_dynptr_kern *ptr, u64 new_size)
{
	u32 metadata = ptr->size & ~DYNPTR_SIZE_MASK;

	if (bpf_dynptr_get_type(ptr) == BPF_DYNPTR_TYPE_FILE) {
		struct bpf_dynptr_file_impl *df = ptr->data;

		df->size = new_size;
		return;
	}
	ptr->size = (u32)new_size | metadata;
}

int bpf_dynptr_check_size(u64 size)
{
	return size > DYNPTR_MAX_SIZE ? -E2BIG : 0;
}

static int bpf_file_fetch_bytes(struct bpf_dynptr_file_impl *df, u64 offset, void *buf, u64 len)
{
	const void *ptr;

	if (!buf)
		return -EINVAL;

	df->freader.buf = buf;
	df->freader.buf_sz = len;
	ptr = freader_fetch(&df->freader, offset + df->offset, len);
	if (!ptr)
		return df->freader.err;

	if (ptr != buf) /* Force copying into the buffer */
		memcpy(buf, ptr, len);

	return 0;
}

void bpf_dynptr_init(struct bpf_dynptr_kern *ptr, void *data,
		     enum bpf_dynptr_type type, u32 offset, u32 size)
{
	ptr->data = data;
	ptr->offset = offset;
	ptr->size = size;
	bpf_dynptr_set_type(ptr, type);
}

void bpf_dynptr_set_null(struct bpf_dynptr_kern *ptr)
{
	memset(ptr, 0, sizeof(*ptr));
}

BPF_CALL_4(bpf_dynptr_from_mem, void *, data, u64, size, u64, flags, struct bpf_dynptr_kern *, ptr)
{
	int err;

	BTF_TYPE_EMIT(struct bpf_dynptr);

	err = bpf_dynptr_check_size(size);
	if (err)
		goto error;

	/* flags is currently unsupported */
	if (flags) {
		err = -EINVAL;
		goto error;
	}

	bpf_dynptr_init(ptr, data, BPF_DYNPTR_TYPE_LOCAL, 0, size);

	return 0;

error:
	bpf_dynptr_set_null(ptr);
	return err;
}

static const struct bpf_func_proto bpf_dynptr_from_mem_proto = {
	.func		= bpf_dynptr_from_mem,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_UNINIT_MEM,
	.arg2_type	= ARG_CONST_SIZE_OR_ZERO,
	.arg3_type	= ARG_ANYTHING,
	.arg4_type	= ARG_PTR_TO_DYNPTR | DYNPTR_TYPE_LOCAL | MEM_UNINIT | MEM_WRITE,
};

static int __bpf_dynptr_read(void *dst, u64 len, const struct bpf_dynptr_kern *src,
			     u64 offset, u64 flags)
{
	enum bpf_dynptr_type type;
	int err;

	if (!src->data || flags)
		return -EINVAL;

	err = bpf_dynptr_check_off_len(src, offset, len);
	if (err)
		return err;

	type = bpf_dynptr_get_type(src);

	switch (type) {
	case BPF_DYNPTR_TYPE_LOCAL:
	case BPF_DYNPTR_TYPE_RINGBUF:
		/* Source and destination may possibly overlap, hence use memmove to
		 * copy the data. E.g. bpf_dynptr_from_mem may create two dynptr
		 * pointing to overlapping PTR_TO_MAP_VALUE regions.
		 */
		memmove(dst, src->data + src->offset + offset, len);
		return 0;
	case BPF_DYNPTR_TYPE_SKB:
		return __bpf_skb_load_bytes(src->data, src->offset + offset, dst, len);
	case BPF_DYNPTR_TYPE_XDP:
		return __bpf_xdp_load_bytes(src->data, src->offset + offset, dst, len);
	case BPF_DYNPTR_TYPE_SKB_META:
		memmove(dst, bpf_skb_meta_pointer(src->data, src->offset + offset), len);
		return 0;
	case BPF_DYNPTR_TYPE_FILE:
		return bpf_file_fetch_bytes(src->data, offset, dst, len);
	default:
		WARN_ONCE(true, "bpf_dynptr_read: unknown dynptr type %d\n", type);
		return -EFAULT;
	}
}

BPF_CALL_5(bpf_dynptr_read, void *, dst, u64, len, const struct bpf_dynptr_kern *, src,
	   u64, offset, u64, flags)
{
	return __bpf_dynptr_read(dst, len, src, offset, flags);
}

static const struct bpf_func_proto bpf_dynptr_read_proto = {
	.func		= bpf_dynptr_read,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_UNINIT_MEM,
	.arg2_type	= ARG_CONST_SIZE_OR_ZERO,
	.arg3_type	= ARG_PTR_TO_DYNPTR | MEM_RDONLY,
	.arg4_type	= ARG_ANYTHING,
	.arg5_type	= ARG_ANYTHING,
};

int __bpf_dynptr_write(const struct bpf_dynptr_kern *dst, u64 offset, void *src,
		       u64 len, u64 flags)
{
	enum bpf_dynptr_type type;
	int err;

	if (!dst->data || __bpf_dynptr_is_rdonly(dst))
		return -EINVAL;

	err = bpf_dynptr_check_off_len(dst, offset, len);
	if (err)
		return err;