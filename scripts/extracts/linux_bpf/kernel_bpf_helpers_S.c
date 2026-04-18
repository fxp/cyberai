// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 19/30



/**
 * bpf_dynptr_copy() - Copy data from one dynptr to another.
 * @dst_ptr: Destination dynptr - where data should be copied to
 * @dst_off: Offset into the destination dynptr
 * @src_ptr: Source dynptr - where data should be copied from
 * @src_off: Offset into the source dynptr
 * @size: Length of the data to copy from source to destination
 *
 * Copies data from source dynptr to destination dynptr.
 * Returns 0 on success; negative error, otherwise.
 */
__bpf_kfunc int bpf_dynptr_copy(struct bpf_dynptr *dst_ptr, u64 dst_off,
				struct bpf_dynptr *src_ptr, u64 src_off, u64 size)
{
	struct bpf_dynptr_kern *dst = (struct bpf_dynptr_kern *)dst_ptr;
	struct bpf_dynptr_kern *src = (struct bpf_dynptr_kern *)src_ptr;
	void *src_slice, *dst_slice;
	char buf[256];
	u64 off;

	src_slice = bpf_dynptr_slice(src_ptr, src_off, NULL, size);
	dst_slice = bpf_dynptr_slice_rdwr(dst_ptr, dst_off, NULL, size);

	if (src_slice && dst_slice) {
		memmove(dst_slice, src_slice, size);
		return 0;
	}

	if (src_slice)
		return __bpf_dynptr_write(dst, dst_off, src_slice, size, 0);

	if (dst_slice)
		return __bpf_dynptr_read(dst_slice, size, src, src_off, 0);

	if (bpf_dynptr_check_off_len(dst, dst_off, size) ||
	    bpf_dynptr_check_off_len(src, src_off, size))
		return -E2BIG;

	off = 0;
	while (off < size) {
		u64 chunk_sz = min_t(u64, sizeof(buf), size - off);
		int err;

		err = __bpf_dynptr_read(buf, chunk_sz, src, src_off + off, 0);
		if (err)
			return err;
		err = __bpf_dynptr_write(dst, dst_off + off, buf, chunk_sz, 0);
		if (err)
			return err;

		off += chunk_sz;
	}
	return 0;
}

/**
 * bpf_dynptr_memset() - Fill dynptr memory with a constant byte.
 * @p: Destination dynptr - where data will be filled
 * @offset: Offset into the dynptr to start filling from
 * @size: Number of bytes to fill
 * @val: Constant byte to fill the memory with
 *
 * Fills the @size bytes of the memory area pointed to by @p
 * at @offset with the constant byte @val.
 * Returns 0 on success; negative error, otherwise.
 */
__bpf_kfunc int bpf_dynptr_memset(struct bpf_dynptr *p, u64 offset, u64 size, u8 val)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)p;
	u64 chunk_sz, write_off;
	char buf[256];
	void* slice;
	int err;

	slice = bpf_dynptr_slice_rdwr(p, offset, NULL, size);
	if (likely(slice)) {
		memset(slice, val, size);
		return 0;
	}

	if (__bpf_dynptr_is_rdonly(ptr))
		return -EINVAL;

	err = bpf_dynptr_check_off_len(ptr, offset, size);
	if (err)
		return err;

	/* Non-linear data under the dynptr, write from a local buffer */
	chunk_sz = min_t(u64, sizeof(buf), size);
	memset(buf, val, chunk_sz);

	for (write_off = 0; write_off < size; write_off += chunk_sz) {
		chunk_sz = min_t(u64, sizeof(buf), size - write_off);
		err = __bpf_dynptr_write(ptr, offset + write_off, buf, chunk_sz, 0);
		if (err)
			return err;
	}

	return 0;
}

__bpf_kfunc void *bpf_cast_to_kern_ctx(void *obj)
{
	return obj;
}

__bpf_kfunc void *bpf_rdonly_cast(const void *obj__ign, u32 btf_id__k)
{
	return (void *)obj__ign;
}

__bpf_kfunc void bpf_rcu_read_lock(void)
{
	rcu_read_lock();
}

__bpf_kfunc void bpf_rcu_read_unlock(void)
{
	rcu_read_unlock();
}

struct bpf_throw_ctx {
	struct bpf_prog_aux *aux;
	u64 sp;
	u64 bp;
	int cnt;
};

static bool bpf_stack_walker(void *cookie, u64 ip, u64 sp, u64 bp)
{
	struct bpf_throw_ctx *ctx = cookie;
	struct bpf_prog *prog;

	/*
	 * The RCU read lock is held to safely traverse the latch tree, but we
	 * don't need its protection when accessing the prog, since it has an
	 * active stack frame on the current stack trace, and won't disappear.
	 */
	rcu_read_lock();
	prog = bpf_prog_ksym_find(ip);
	rcu_read_unlock();
	if (!prog)
		return !ctx->cnt;
	ctx->cnt++;
	if (bpf_is_subprog(prog))
		return true;
	ctx->aux = prog->aux;
	ctx->sp = sp;
	ctx->bp = bp;
	return false;
}

__bpf_kfunc void bpf_throw(u64 cookie)
{
	struct bpf_throw_ctx ctx = {};

	arch_bpf_stack_walk(bpf_stack_walker, &ctx);
	WARN_ON_ONCE(!ctx.aux);
	if (ctx.aux)
		WARN_ON_ONCE(!ctx.aux->exception_boundary);
	WARN_ON_ONCE(!ctx.bp);
	WARN_ON_ONCE(!ctx.cnt);
	/* Prevent KASAN false positives for CONFIG_KASAN_STACK by unpoisoning
	 * deeper stack depths than ctx.sp as we do not return from bpf_throw,
	 * which skips compiler generated instrumentation to do the same.
	 */
	kasan_unpoison_task_stack_below((void *)(long)ctx.sp);
	ctx.aux->bpf_exception_cb(cookie, ctx.sp, ctx.bp, 0, 0);
	WARN(1, "A call to BPF exception callback should never return\n");
}

__bpf_kfunc int bpf_wq_init(struct bpf_wq *wq, void *p__map, unsigned int flags)
{
	struct bpf_async_kern *async = (struct bpf_async_kern *)wq;
	struct bpf_map *map = p__map;

	BUILD_BUG_ON(sizeof(struct bpf_async_kern) > sizeof(struct bpf_wq));
	BUILD_BUG_ON(__alignof__(struct bpf_async_kern) != __alignof__(struct bpf_wq));

	if (flags)
		return -EINVAL;

	return __bpf_async_init(async, map, flags, BPF_ASYNC_TYPE_WQ);
}