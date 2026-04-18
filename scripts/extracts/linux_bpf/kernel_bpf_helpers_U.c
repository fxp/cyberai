// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 21/30



/**
 * bpf_iter_bits_destroy() - Destroy a bpf_iter_bits
 * @it: The bpf_iter_bits to be destroyed
 *
 * Destroy the resource associated with the bpf_iter_bits.
 */
__bpf_kfunc void bpf_iter_bits_destroy(struct bpf_iter_bits *it)
{
	struct bpf_iter_bits_kern *kit = (void *)it;

	if (kit->nr_bits <= 64)
		return;
	bpf_mem_free(&bpf_global_ma, kit->bits);
}

/**
 * bpf_copy_from_user_str() - Copy a string from an unsafe user address
 * @dst:             Destination address, in kernel space.  This buffer must be
 *                   at least @dst__sz bytes long.
 * @dst__sz:         Maximum number of bytes to copy, includes the trailing NUL.
 * @unsafe_ptr__ign: Source address, in user space.
 * @flags:           The only supported flag is BPF_F_PAD_ZEROS
 *
 * Copies a NUL-terminated string from userspace to BPF space. If user string is
 * too long this will still ensure zero termination in the dst buffer unless
 * buffer size is 0.
 *
 * If BPF_F_PAD_ZEROS flag is set, memset the tail of @dst to 0 on success and
 * memset all of @dst on failure.
 */
__bpf_kfunc int bpf_copy_from_user_str(void *dst, u32 dst__sz, const void __user *unsafe_ptr__ign, u64 flags)
{
	int ret;

	if (unlikely(flags & ~BPF_F_PAD_ZEROS))
		return -EINVAL;

	if (unlikely(!dst__sz))
		return 0;

	ret = strncpy_from_user(dst, unsafe_ptr__ign, dst__sz - 1);
	if (ret < 0) {
		if (flags & BPF_F_PAD_ZEROS)
			memset((char *)dst, 0, dst__sz);

		return ret;
	}

	if (flags & BPF_F_PAD_ZEROS)
		memset((char *)dst + ret, 0, dst__sz - ret);
	else
		((char *)dst)[ret] = '\0';

	return ret + 1;
}

/**
 * bpf_copy_from_user_task_str() - Copy a string from an task's address space
 * @dst:             Destination address, in kernel space.  This buffer must be
 *                   at least @dst__sz bytes long.
 * @dst__sz:         Maximum number of bytes to copy, includes the trailing NUL.
 * @unsafe_ptr__ign: Source address in the task's address space.
 * @tsk:             The task whose address space will be used
 * @flags:           The only supported flag is BPF_F_PAD_ZEROS
 *
 * Copies a NUL terminated string from a task's address space to @dst__sz
 * buffer. If user string is too long this will still ensure zero termination
 * in the @dst__sz buffer unless buffer size is 0.
 *
 * If BPF_F_PAD_ZEROS flag is set, memset the tail of @dst__sz to 0 on success
 * and memset all of @dst__sz on failure.
 *
 * Return: The number of copied bytes on success including the NUL terminator.
 * A negative error code on failure.
 */
__bpf_kfunc int bpf_copy_from_user_task_str(void *dst, u32 dst__sz,
					    const void __user *unsafe_ptr__ign,
					    struct task_struct *tsk, u64 flags)
{
	int ret;

	if (unlikely(flags & ~BPF_F_PAD_ZEROS))
		return -EINVAL;

	if (unlikely(dst__sz == 0))
		return 0;

	ret = copy_remote_vm_str(tsk, (unsigned long)unsafe_ptr__ign, dst, dst__sz, 0);
	if (ret < 0) {
		if (flags & BPF_F_PAD_ZEROS)
			memset(dst, 0, dst__sz);
		return ret;
	}

	if (flags & BPF_F_PAD_ZEROS)
		memset(dst + ret, 0, dst__sz - ret);

	return ret + 1;
}

/* Keep unsinged long in prototype so that kfunc is usable when emitted to
 * vmlinux.h in BPF programs directly, but note that while in BPF prog, the
 * unsigned long always points to 8-byte region on stack, the kernel may only
 * read and write the 4-bytes on 32-bit.
 */
__bpf_kfunc void bpf_local_irq_save(unsigned long *flags__irq_flag)
{
	local_irq_save(*flags__irq_flag);
}

__bpf_kfunc void bpf_local_irq_restore(unsigned long *flags__irq_flag)
{
	local_irq_restore(*flags__irq_flag);
}

__bpf_kfunc void __bpf_trap(void)
{
}

/*
 * Kfuncs for string operations.
 *
 * Since strings are not necessarily %NUL-terminated, we cannot directly call
 * in-kernel implementations. Instead, we open-code the implementations using
 * __get_kernel_nofault instead of plain dereference to make them safe.
 */

static int __bpf_strncasecmp(const char *s1, const char *s2, bool ignore_case, size_t len)
{
	char c1, c2;
	int i;

	if (!copy_from_kernel_nofault_allowed(s1, 1) ||
	    !copy_from_kernel_nofault_allowed(s2, 1)) {
		return -ERANGE;
	}

	guard(pagefault)();
	for (i = 0; i < len && i < XATTR_SIZE_MAX; i++) {
		__get_kernel_nofault(&c1, s1, char, err_out);
		__get_kernel_nofault(&c2, s2, char, err_out);
		if (ignore_case) {
			c1 = tolower(c1);
			c2 = tolower(c2);
		}
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (c1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return i == XATTR_SIZE_MAX ? -E2BIG : 0;
err_out:
	return -EFAULT;
}