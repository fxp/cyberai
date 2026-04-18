// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 29/86



	for (i = 0; i < msrs->nmsrs; ++i) {
		/*
		 * If userspace is accessing one or more XSTATE-managed MSRs,
		 * temporarily load the guest's FPU state so that the guest's
		 * MSR value(s) is resident in hardware and thus can be accessed
		 * via RDMSR/WRMSR.
		 */
		if (!fpu_loaded && is_xstate_managed_msr(vcpu, entries[i].index)) {
			kvm_load_guest_fpu(vcpu);
			fpu_loaded = true;
		}
		if (do_msr(vcpu, entries[i].index, &entries[i].data))
			break;
	}
	if (fpu_loaded)
		kvm_put_guest_fpu(vcpu);

	return i;
}

/*
 * Read or write a bunch of msrs. Parameters are user addresses.
 *
 * @return number of msrs set successfully.
 */
static int msr_io(struct kvm_vcpu *vcpu, struct kvm_msrs __user *user_msrs,
		  int (*do_msr)(struct kvm_vcpu *vcpu,
				unsigned index, u64 *data),
		  int writeback)
{
	struct kvm_msrs msrs;
	struct kvm_msr_entry *entries;
	unsigned size;
	int r;

	r = -EFAULT;
	if (copy_from_user(&msrs, user_msrs, sizeof(msrs)))
		goto out;

	r = -E2BIG;
	if (msrs.nmsrs >= MAX_IO_MSRS)
		goto out;

	size = sizeof(struct kvm_msr_entry) * msrs.nmsrs;
	entries = memdup_user(user_msrs->entries, size);
	if (IS_ERR(entries)) {
		r = PTR_ERR(entries);
		goto out;
	}

	r = __msr_io(vcpu, &msrs, entries, do_msr);

	if (writeback && copy_to_user(user_msrs->entries, entries, size))
		r = -EFAULT;

	kfree(entries);
out:
	return r;
}

static inline bool kvm_can_mwait_in_guest(void)
{
	return boot_cpu_has(X86_FEATURE_MWAIT) &&
		!boot_cpu_has_bug(X86_BUG_MONITOR) &&
		boot_cpu_has(X86_FEATURE_ARAT);
}

static u64 kvm_get_allowed_disable_exits(void)
{
	u64 r = KVM_X86_DISABLE_EXITS_PAUSE;

	if (boot_cpu_has(X86_FEATURE_APERFMPERF))
		r |= KVM_X86_DISABLE_EXITS_APERFMPERF;

	if (!mitigate_smt_rsb) {
		r |= KVM_X86_DISABLE_EXITS_HLT |
			KVM_X86_DISABLE_EXITS_CSTATE;

		if (kvm_can_mwait_in_guest())
			r |= KVM_X86_DISABLE_EXITS_MWAIT;
	}
	return r;
}

#ifdef CONFIG_KVM_HYPERV
static int kvm_ioctl_get_supported_hv_cpuid(struct kvm_vcpu *vcpu,
					    struct kvm_cpuid2 __user *cpuid_arg)
{
	struct kvm_cpuid2 cpuid;
	int r;

	r = -EFAULT;
	if (copy_from_user(&cpuid, cpuid_arg, sizeof(cpuid)))
		return r;

	r = kvm_get_hv_cpuid(vcpu, &cpuid, cpuid_arg->entries);
	if (r)
		return r;

	r = -EFAULT;
	if (copy_to_user(cpuid_arg, &cpuid, sizeof(cpuid)))
		return r;

	return 0;
}
#endif

static bool kvm_is_vm_type_supported(unsigned long type)
{
	return type < 32 && (kvm_caps.supported_vm_types & BIT(type));
}

static inline u64 kvm_sync_valid_fields(struct kvm *kvm)
{
	return kvm && kvm->arch.has_protected_state ? 0 : KVM_SYNC_X86_VALID_FIELDS;
}

int kvm_vm_ioctl_check_extension(struct kvm *kvm, long ext)
{
	int r = 0;

	switch (ext) {
	case KVM_CAP_IRQCHIP:
	case KVM_CAP_HLT:
	case KVM_CAP_MMU_SHADOW_CACHE_CONTROL:
	case KVM_CAP_SET_TSS_ADDR:
	case KVM_CAP_EXT_CPUID:
	case KVM_CAP_EXT_EMUL_CPUID:
	case KVM_CAP_CLOCKSOURCE:
#ifdef CONFIG_KVM_IOAPIC
	case KVM_CAP_PIT:
	case KVM_CAP_PIT2:
	case KVM_CAP_PIT_STATE2:
	case KVM_CAP_REINJECT_CONTROL:
#endif
	case KVM_CAP_NOP_IO_DELAY:
	case KVM_CAP_MP_STATE:
	case KVM_CAP_USER_NMI:
	case KVM_CAP_IRQ_INJECT_STATUS:
	case KVM_CAP_IOEVENTFD:
	case KVM_CAP_IOEVENTFD_NO_LENGTH: