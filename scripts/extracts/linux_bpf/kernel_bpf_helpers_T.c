// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 20/30



__bpf_kfunc int bpf_wq_start(struct bpf_wq *wq, unsigned int flags)
{
	struct bpf_async_kern *async = (struct bpf_async_kern *)wq;
	struct bpf_work *w;

	if (flags)
		return -EINVAL;

	w = READ_ONCE(async->work);
	if (!w || !READ_ONCE(w->cb.prog))
		return -EINVAL;

	if (!refcount_inc_not_zero(&w->cb.refcnt))
		return -ENOENT;

	if (!defer_timer_wq_op()) {
		schedule_work(&w->work);
		bpf_async_refcount_put(&w->cb);
		return 0;
	} else {
		return bpf_async_schedule_op(&w->cb, BPF_ASYNC_START, 0, 0);
	}
}

__bpf_kfunc int bpf_wq_set_callback(struct bpf_wq *wq,
				    int (callback_fn)(void *map, int *key, void *value),
				    unsigned int flags,
				    struct bpf_prog_aux *aux)
{
	struct bpf_async_kern *async = (struct bpf_async_kern *)wq;

	if (flags)
		return -EINVAL;

	return __bpf_async_set_callback(async, callback_fn, aux->prog);
}

__bpf_kfunc void bpf_preempt_disable(void)
{
	preempt_disable();
}

__bpf_kfunc void bpf_preempt_enable(void)
{
	preempt_enable();
}

struct bpf_iter_bits {
	__u64 __opaque[2];
} __aligned(8);

#define BITS_ITER_NR_WORDS_MAX 511

struct bpf_iter_bits_kern {
	union {
		__u64 *bits;
		__u64 bits_copy;
	};
	int nr_bits;
	int bit;
} __aligned(8);

/* On 64-bit hosts, unsigned long and u64 have the same size, so passing
 * a u64 pointer and an unsigned long pointer to find_next_bit() will
 * return the same result, as both point to the same 8-byte area.
 *
 * For 32-bit little-endian hosts, using a u64 pointer or unsigned long
 * pointer also makes no difference. This is because the first iterated
 * unsigned long is composed of bits 0-31 of the u64 and the second unsigned
 * long is composed of bits 32-63 of the u64.
 *
 * However, for 32-bit big-endian hosts, this is not the case. The first
 * iterated unsigned long will be bits 32-63 of the u64, so swap these two
 * ulong values within the u64.
 */
static void swap_ulong_in_u64(u64 *bits, unsigned int nr)
{
#if (BITS_PER_LONG == 32) && defined(__BIG_ENDIAN)
	unsigned int i;

	for (i = 0; i < nr; i++)
		bits[i] = (bits[i] >> 32) | ((u64)(u32)bits[i] << 32);
#endif
}

/**
 * bpf_iter_bits_new() - Initialize a new bits iterator for a given memory area
 * @it: The new bpf_iter_bits to be created
 * @unsafe_ptr__ign: A pointer pointing to a memory area to be iterated over
 * @nr_words: The size of the specified memory area, measured in 8-byte units.
 * The maximum value of @nr_words is @BITS_ITER_NR_WORDS_MAX. This limit may be
 * further reduced by the BPF memory allocator implementation.
 *
 * This function initializes a new bpf_iter_bits structure for iterating over
 * a memory area which is specified by the @unsafe_ptr__ign and @nr_words. It
 * copies the data of the memory area to the newly created bpf_iter_bits @it for
 * subsequent iteration operations.
 *
 * On success, 0 is returned. On failure, ERR is returned.
 */
__bpf_kfunc int
bpf_iter_bits_new(struct bpf_iter_bits *it, const u64 *unsafe_ptr__ign, u32 nr_words)
{
	struct bpf_iter_bits_kern *kit = (void *)it;
	u32 nr_bytes = nr_words * sizeof(u64);
	u32 nr_bits = BYTES_TO_BITS(nr_bytes);
	int err;

	BUILD_BUG_ON(sizeof(struct bpf_iter_bits_kern) != sizeof(struct bpf_iter_bits));
	BUILD_BUG_ON(__alignof__(struct bpf_iter_bits_kern) !=
		     __alignof__(struct bpf_iter_bits));

	kit->nr_bits = 0;
	kit->bits_copy = 0;
	kit->bit = -1;

	if (!unsafe_ptr__ign || !nr_words)
		return -EINVAL;
	if (nr_words > BITS_ITER_NR_WORDS_MAX)
		return -E2BIG;

	/* Optimization for u64 mask */
	if (nr_bits == 64) {
		err = bpf_probe_read_kernel_common(&kit->bits_copy, nr_bytes, unsafe_ptr__ign);
		if (err)
			return -EFAULT;

		swap_ulong_in_u64(&kit->bits_copy, nr_words);

		kit->nr_bits = nr_bits;
		return 0;
	}

	if (bpf_mem_alloc_check_size(false, nr_bytes))
		return -E2BIG;

	/* Fallback to memalloc */
	kit->bits = bpf_mem_alloc(&bpf_global_ma, nr_bytes);
	if (!kit->bits)
		return -ENOMEM;

	err = bpf_probe_read_kernel_common(kit->bits, nr_bytes, unsafe_ptr__ign);
	if (err) {
		bpf_mem_free(&bpf_global_ma, kit->bits);
		return err;
	}

	swap_ulong_in_u64(kit->bits, nr_words);

	kit->nr_bits = nr_bits;
	return 0;
}

/**
 * bpf_iter_bits_next() - Get the next bit in a bpf_iter_bits
 * @it: The bpf_iter_bits to be checked
 *
 * This function returns a pointer to a number representing the value of the
 * next bit in the bits.
 *
 * If there are no further bits available, it returns NULL.
 */
__bpf_kfunc int *bpf_iter_bits_next(struct bpf_iter_bits *it)
{
	struct bpf_iter_bits_kern *kit = (void *)it;
	int bit = kit->bit, nr_bits = kit->nr_bits;
	const void *bits;

	if (!nr_bits || bit >= nr_bits)
		return NULL;

	bits = nr_bits == 64 ? &kit->bits_copy : kit->bits;
	bit = find_next_bit(bits, nr_bits, bit + 1);
	if (bit >= nr_bits) {
		kit->bit = bit;
		return NULL;
	}

	kit->bit = bit;
	return &kit->bit;
}