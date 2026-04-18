// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 29/36



	if (cleared)
		kvm_flush_remote_tlbs(kvm);

	return cleared;
}

int __attribute__((weak)) kvm_vm_ioctl_enable_cap(struct kvm *kvm,
						  struct kvm_enable_cap *cap)
{
	return -EINVAL;
}

bool kvm_are_all_memslots_empty(struct kvm *kvm)
{
	int i;

	lockdep_assert_held(&kvm->slots_lock);

	for (i = 0; i < kvm_arch_nr_memslot_as_ids(kvm); i++) {
		if (!kvm_memslots_empty(__kvm_memslots(kvm, i)))
			return false;
	}

	return true;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_are_all_memslots_empty);

static int kvm_vm_ioctl_enable_cap_generic(struct kvm *kvm,
					   struct kvm_enable_cap *cap)
{
	switch (cap->cap) {
#ifdef CONFIG_KVM_GENERIC_DIRTYLOG_READ_PROTECT
	case KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2: {
		u64 allowed_options = KVM_DIRTY_LOG_MANUAL_PROTECT_ENABLE;

		if (cap->args[0] & KVM_DIRTY_LOG_MANUAL_PROTECT_ENABLE)
			allowed_options = KVM_DIRTY_LOG_MANUAL_CAPS;

		if (cap->flags || (cap->args[0] & ~allowed_options))
			return -EINVAL;
		kvm->manual_dirty_log_protect = cap->args[0];
		return 0;
	}
#endif
	case KVM_CAP_HALT_POLL: {
		if (cap->flags || cap->args[0] != (unsigned int)cap->args[0])
			return -EINVAL;

		kvm->max_halt_poll_ns = cap->args[0];

		/*
		 * Ensure kvm->override_halt_poll_ns does not become visible
		 * before kvm->max_halt_poll_ns.
		 *
		 * Pairs with the smp_rmb() in kvm_vcpu_max_halt_poll_ns().
		 */
		smp_wmb();
		kvm->override_halt_poll_ns = true;

		return 0;
	}
	case KVM_CAP_DIRTY_LOG_RING:
	case KVM_CAP_DIRTY_LOG_RING_ACQ_REL:
		if (!kvm_vm_ioctl_check_extension_generic(kvm, cap->cap))
			return -EINVAL;

		return kvm_vm_ioctl_enable_dirty_log_ring(kvm, cap->args[0]);
	case KVM_CAP_DIRTY_LOG_RING_WITH_BITMAP: {
		int r = -EINVAL;

		if (!IS_ENABLED(CONFIG_NEED_KVM_DIRTY_RING_WITH_BITMAP) ||
		    !kvm->dirty_ring_size || cap->flags)
			return r;

		mutex_lock(&kvm->slots_lock);

		/*
		 * For simplicity, allow enabling ring+bitmap if and only if
		 * there are no memslots, e.g. to ensure all memslots allocate
		 * a bitmap after the capability is enabled.
		 */
		if (kvm_are_all_memslots_empty(kvm)) {
			kvm->dirty_ring_with_bitmap = true;
			r = 0;
		}

		mutex_unlock(&kvm->slots_lock);

		return r;
	}
	default:
		return kvm_vm_ioctl_enable_cap(kvm, cap);
	}
}

static ssize_t kvm_vm_stats_read(struct file *file, char __user *user_buffer,
			      size_t size, loff_t *offset)
{
	struct kvm *kvm = file->private_data;

	return kvm_stats_read(kvm->stats_id, &kvm_vm_stats_header,
				&kvm_vm_stats_desc[0], &kvm->stat,
				sizeof(kvm->stat), user_buffer, size, offset);
}

static int kvm_vm_stats_release(struct inode *inode, struct file *file)
{
	struct kvm *kvm = file->private_data;

	kvm_put_kvm(kvm);
	return 0;
}

static const struct file_operations kvm_vm_stats_fops = {
	.owner = THIS_MODULE,
	.read = kvm_vm_stats_read,
	.release = kvm_vm_stats_release,
	.llseek = noop_llseek,
};

static int kvm_vm_ioctl_get_stats_fd(struct kvm *kvm)
{
	int fd;
	struct file *file;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return fd;

	file = anon_inode_getfile_fmode("kvm-vm-stats",
			&kvm_vm_stats_fops, kvm, O_RDONLY, FMODE_PREAD);
	if (IS_ERR(file)) {
		put_unused_fd(fd);
		return PTR_ERR(file);
	}

	kvm_get_kvm(kvm);
	fd_install(fd, file);

	return fd;
}

#define SANITY_CHECK_MEM_REGION_FIELD(field)					\
do {										\
	BUILD_BUG_ON(offsetof(struct kvm_userspace_memory_region, field) !=		\
		     offsetof(struct kvm_userspace_memory_region2, field));	\
	BUILD_BUG_ON(sizeof_field(struct kvm_userspace_memory_region, field) !=		\
		     sizeof_field(struct kvm_userspace_memory_region2, field));	\
} while (0)

static long kvm_vm_ioctl(struct file *filp,
			   unsigned int ioctl, unsigned long arg)
{
	struct kvm *kvm = filp->private_data;
	void __user *argp = (void __user *)arg;
	int r;

	if (kvm->mm != current->mm || kvm->vm_dead)
		return -EIO;
	switch (ioctl) {
	case KVM_CREATE_VCPU:
		r = kvm_vm_ioctl_create_vcpu(kvm, arg);
		break;
	case KVM_ENABLE_CAP: {
		struct kvm_enable_cap cap;

		r = -EFAULT;
		if (copy_from_user(&cap, argp, sizeof(cap)))
			goto out;
		r = kvm_vm_ioctl_enable_cap_generic(kvm, &cap);
		break;
	}
	case KVM_SET_USER_MEMORY_REGION2:
	case KVM_SET_USER_MEMORY_REGION: {
		struct kvm_userspace_memory_region2 mem;
		unsigned long size;

		if (ioctl == KVM_SET_USER_MEMORY_REGION) {
			/*
			 * Fields beyond struct kvm_userspace_memory_region shouldn't be
			 * accessed, but avoid leaking kernel memory in case of a bug.
			 */
			memset(&mem, 0, sizeof(mem));
			size = sizeof(struct kvm_userspace_memory_region);
		} else {
			size = sizeof(struct kvm_userspace_memory_region2);
		}

		/* Ensure the common parts of the two structs are identical. */
		SANITY_CHECK_MEM_REGION_FIELD(slot);
		SANITY_CHECK_MEM_REGION_FIELD(flags);
		SANITY_CHECK_MEM_REGION_FIELD(guest_phys_addr);
		SANITY_CHECK_MEM_REGION_FIELD(memory_size);
		SANITY_CHECK_MEM_REGION_FIELD(userspace_addr);

		r = -EFAULT;
		if (copy_from_user(&mem, argp, size))
			goto out;