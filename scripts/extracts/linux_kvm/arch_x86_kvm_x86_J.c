// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 42/86



static void kvm_free_msr_filter(struct kvm_x86_msr_filter *msr_filter)
{
	u32 i;

	if (!msr_filter)
		return;

	for (i = 0; i < msr_filter->count; i++)
		kfree(msr_filter->ranges[i].bitmap);

	kfree(msr_filter);
}

static int kvm_add_msr_filter(struct kvm_x86_msr_filter *msr_filter,
			      struct kvm_msr_filter_range *user_range)
{
	unsigned long *bitmap;
	size_t bitmap_size;

	if (!user_range->nmsrs)
		return 0;

	if (user_range->flags & ~KVM_MSR_FILTER_RANGE_VALID_MASK)
		return -EINVAL;

	if (!user_range->flags)
		return -EINVAL;

	bitmap_size = BITS_TO_LONGS(user_range->nmsrs) * sizeof(long);
	if (!bitmap_size || bitmap_size > KVM_MSR_FILTER_MAX_BITMAP_SIZE)
		return -EINVAL;

	bitmap = memdup_user((__user u8*)user_range->bitmap, bitmap_size);
	if (IS_ERR(bitmap))
		return PTR_ERR(bitmap);

	msr_filter->ranges[msr_filter->count] = (struct msr_bitmap_range) {
		.flags = user_range->flags,
		.base = user_range->base,
		.nmsrs = user_range->nmsrs,
		.bitmap = bitmap,
	};

	msr_filter->count++;
	return 0;
}

static int kvm_vm_ioctl_set_msr_filter(struct kvm *kvm,
				       struct kvm_msr_filter *filter)
{
	struct kvm_x86_msr_filter *new_filter, *old_filter;
	bool default_allow;
	bool empty = true;
	int r;
	u32 i;

	if (filter->flags & ~KVM_MSR_FILTER_VALID_MASK)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(filter->ranges); i++)
		empty &= !filter->ranges[i].nmsrs;

	default_allow = !(filter->flags & KVM_MSR_FILTER_DEFAULT_DENY);
	if (empty && !default_allow)
		return -EINVAL;

	new_filter = kvm_alloc_msr_filter(default_allow);
	if (!new_filter)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(filter->ranges); i++) {
		r = kvm_add_msr_filter(new_filter, &filter->ranges[i]);
		if (r) {
			kvm_free_msr_filter(new_filter);
			return r;
		}
	}

	mutex_lock(&kvm->lock);
	old_filter = rcu_replace_pointer(kvm->arch.msr_filter, new_filter,
					 mutex_is_locked(&kvm->lock));
	mutex_unlock(&kvm->lock);
	synchronize_srcu(&kvm->srcu);

	kvm_free_msr_filter(old_filter);

	/*
	 * Recalc MSR intercepts as userspace may want to intercept accesses to
	 * MSRs that KVM would otherwise pass through to the guest.
	 */
	kvm_make_all_cpus_request(kvm, KVM_REQ_RECALC_INTERCEPTS);

	return 0;
}

#ifdef CONFIG_KVM_COMPAT
/* for KVM_X86_SET_MSR_FILTER */
struct kvm_msr_filter_range_compat {
	__u32 flags;
	__u32 nmsrs;
	__u32 base;
	__u32 bitmap;
};

struct kvm_msr_filter_compat {
	__u32 flags;
	struct kvm_msr_filter_range_compat ranges[KVM_MSR_FILTER_MAX_RANGES];
};

#define KVM_X86_SET_MSR_FILTER_COMPAT _IOW(KVMIO, 0xc6, struct kvm_msr_filter_compat)

long kvm_arch_vm_compat_ioctl(struct file *filp, unsigned int ioctl,
			      unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct kvm *kvm = filp->private_data;
	long r = -ENOTTY;

	switch (ioctl) {
	case KVM_X86_SET_MSR_FILTER_COMPAT: {
		struct kvm_msr_filter __user *user_msr_filter = argp;
		struct kvm_msr_filter_compat filter_compat;
		struct kvm_msr_filter filter;
		int i;

		if (copy_from_user(&filter_compat, user_msr_filter,
				   sizeof(filter_compat)))
			return -EFAULT;

		filter.flags = filter_compat.flags;
		for (i = 0; i < ARRAY_SIZE(filter.ranges); i++) {
			struct kvm_msr_filter_range_compat *cr;

			cr = &filter_compat.ranges[i];
			filter.ranges[i] = (struct kvm_msr_filter_range) {
				.flags = cr->flags,
				.nmsrs = cr->nmsrs,
				.base = cr->base,
				.bitmap = (__u8 *)(ulong)cr->bitmap,
			};
		}

		r = kvm_vm_ioctl_set_msr_filter(kvm, &filter);
		break;
	}
	}

	return r;
}
#endif

#ifdef CONFIG_HAVE_KVM_PM_NOTIFIER
static int kvm_arch_suspend_notifier(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;

	/*
	 * Ignore the return, marking the guest paused only "fails" if the vCPU
	 * isn't using kvmclock; continuing on is correct and desirable.
	 */
	kvm_for_each_vcpu(i, vcpu, kvm)
		(void)kvm_set_guest_paused(vcpu);

	return NOTIFY_DONE;
}

int kvm_arch_pm_notifier(struct kvm *kvm, unsigned long state)
{
	switch (state) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		return kvm_arch_suspend_notifier(kvm);
	}

	return NOTIFY_DONE;
}
#endif /* CONFIG_HAVE_KVM_PM_NOTIFIER */

static int kvm_vm_ioctl_get_clock(struct kvm *kvm, void __user *argp)
{
	struct kvm_clock_data data = { 0 };

	get_kvmclock(kvm, &data);
	if (copy_to_user(argp, &data, sizeof(data)))
		return -EFAULT;

	return 0;
}

static int kvm_vm_ioctl_set_clock(struct kvm *kvm, void __user *argp)
{
	struct kvm_arch *ka = &kvm->arch;
	struct kvm_clock_data data;
	u64 now_raw_ns;

	if (copy_from_user(&data, argp, sizeof(data)))
		return -EFAULT;

	/*
	 * Only KVM_CLOCK_REALTIME is used, but allow passing the
	 * result of KVM_GET_CLOCK back to KVM_SET_CLOCK.
	 */
	if (data.flags & ~KVM_CLOCK_VALID_FLAGS)
		return -EINVAL;

	kvm_hv_request_tsc_page_update(kvm);
	kvm_start_pvclock_update(kvm);
	pvclock_update_vm_gtod_copy(kvm);