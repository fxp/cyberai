// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 31/86



		/* SMBASE is usually relocated above 1M on modern chipsets,
		 * and SMM handlers might indeed rely on 4G segment limits,
		 * so do not report SMM to be available if real mode is
		 * emulated via vm86 mode.  Still, do not go to great lengths
		 * to avoid userspace's usage of the feature, because it is a
		 * fringe case that is not enabled except via specific settings
		 * of the module parameters.
		 */
		r = kvm_x86_call(has_emulated_msr)(kvm, MSR_IA32_SMBASE);
		break;
	case KVM_CAP_NR_VCPUS:
		r = min_t(unsigned int, num_online_cpus(), KVM_MAX_VCPUS);
		break;
	case KVM_CAP_MAX_VCPUS:
		r = KVM_MAX_VCPUS;
		if (kvm)
			r = kvm->max_vcpus;
		break;
	case KVM_CAP_MAX_VCPU_ID:
		r = KVM_MAX_VCPU_IDS;
		break;
	case KVM_CAP_PV_MMU:	/* obsolete */
		r = 0;
		break;
	case KVM_CAP_MCE:
		r = KVM_MAX_MCE_BANKS;
		break;
	case KVM_CAP_XCRS:
		r = boot_cpu_has(X86_FEATURE_XSAVE);
		break;
	case KVM_CAP_TSC_CONTROL:
	case KVM_CAP_VM_TSC_CONTROL:
		r = kvm_caps.has_tsc_control;
		break;
	case KVM_CAP_X2APIC_API:
		r = KVM_X2APIC_API_VALID_FLAGS;
		if (kvm && !irqchip_split(kvm))
			r &= ~KVM_X2APIC_ENABLE_SUPPRESS_EOI_BROADCAST;
		break;
	case KVM_CAP_NESTED_STATE:
		r = kvm_x86_ops.nested_ops->get_state ?
			kvm_x86_ops.nested_ops->get_state(NULL, NULL, 0) : 0;
		break;
#ifdef CONFIG_KVM_HYPERV
	case KVM_CAP_HYPERV_DIRECT_TLBFLUSH:
		r = kvm_x86_ops.enable_l2_tlb_flush != NULL;
		break;
	case KVM_CAP_HYPERV_ENLIGHTENED_VMCS:
		r = kvm_x86_ops.nested_ops->enable_evmcs != NULL;
		break;
#endif
	case KVM_CAP_SMALLER_MAXPHYADDR:
		r = (int) allow_smaller_maxphyaddr;
		break;
	case KVM_CAP_STEAL_TIME:
		r = sched_info_on();
		break;
	case KVM_CAP_X86_BUS_LOCK_EXIT:
		if (kvm_caps.has_bus_lock_exit)
			r = KVM_BUS_LOCK_DETECTION_OFF |
			    KVM_BUS_LOCK_DETECTION_EXIT;
		else
			r = 0;
		break;
	case KVM_CAP_XSAVE2: {
		r = xstate_required_size(kvm_get_filtered_xcr0(), false);
		if (r < sizeof(struct kvm_xsave))
			r = sizeof(struct kvm_xsave);
		break;
	}
	case KVM_CAP_PMU_CAPABILITY:
		r = enable_pmu ? KVM_CAP_PMU_VALID_MASK : 0;
		break;
	case KVM_CAP_DISABLE_QUIRKS2:
		r = kvm_caps.supported_quirks;
		break;
	case KVM_CAP_X86_NOTIFY_VMEXIT:
		r = kvm_caps.has_notify_vmexit;
		break;
	case KVM_CAP_VM_TYPES:
		r = kvm_caps.supported_vm_types;
		break;
	case KVM_CAP_READONLY_MEM:
		r = kvm ? kvm_arch_has_readonly_mem(kvm) : 1;
		break;
	default:
		break;
	}
	return r;
}

static int __kvm_x86_dev_get_attr(struct kvm_device_attr *attr, u64 *val)
{
	if (attr->group) {
		if (kvm_x86_ops.dev_get_attr)
			return kvm_x86_call(dev_get_attr)(attr->group, attr->attr, val);
		return -ENXIO;
	}

	switch (attr->attr) {
	case KVM_X86_XCOMP_GUEST_SUPP:
		*val = kvm_caps.supported_xcr0;
		return 0;
	default:
		return -ENXIO;
	}
}

static int kvm_x86_dev_get_attr(struct kvm_device_attr *attr)
{
	u64 __user *uaddr = u64_to_user_ptr(attr->addr);
	int r;
	u64 val;

	r = __kvm_x86_dev_get_attr(attr, &val);
	if (r < 0)
		return r;

	if (put_user(val, uaddr))
		return -EFAULT;

	return 0;
}

static int kvm_x86_dev_has_attr(struct kvm_device_attr *attr)
{
	u64 val;

	return __kvm_x86_dev_get_attr(attr, &val);
}

long kvm_arch_dev_ioctl(struct file *filp,
			unsigned int ioctl, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	long r;

	switch (ioctl) {
	case KVM_GET_MSR_INDEX_LIST: {
		struct kvm_msr_list __user *user_msr_list = argp;
		struct kvm_msr_list msr_list;
		unsigned n;

		r = -EFAULT;
		if (copy_from_user(&msr_list, user_msr_list, sizeof(msr_list)))
			goto out;
		n = msr_list.nmsrs;
		msr_list.nmsrs = num_msrs_to_save + num_emulated_msrs;
		if (copy_to_user(user_msr_list, &msr_list, sizeof(msr_list)))
			goto out;
		r = -E2BIG;
		if (n < msr_list.nmsrs)
			goto out;
		r = -EFAULT;
		if (copy_to_user(user_msr_list->indices, &msrs_to_save,
				 num_msrs_to_save * sizeof(u32)))
			goto out;
		if (copy_to_user(user_msr_list->indices + num_msrs_to_save,
				 &emulated_msrs,
				 num_emulated_msrs * sizeof(u32)))
			goto out;
		r = 0;
		break;
	}
	case KVM_GET_SUPPORTED_CPUID:
	case KVM_GET_EMULATED_CPUID: {
		struct kvm_cpuid2 __user *cpuid_arg = argp;
		struct kvm_cpuid2 cpuid;

		r = -EFAULT;
		if (copy_from_user(&cpuid, cpuid_arg, sizeof(cpuid)))
			goto out;

		r = kvm_dev_ioctl_get_cpuid(&cpuid, cpuid_arg->entries,
					    ioctl);
		if (r)
			goto out;

		r = -EFAULT;
		if (copy_to_user(cpuid_arg, &cpuid, sizeof(cpuid)))
			goto out;
		r = 0;
		break;
	}
	case KVM_X86_GET_MCE_CAP_SUPPORTED:
		r = -EFAULT;
		if (copy_to_user(argp, &kvm_caps.supported_mce_cap,
				 sizeof(kvm_caps.supported_mce_cap)))
			goto out;
		r = 0;
		break;
	case KVM_GET_MSR_FEATURE_INDEX_LIST: {
		struct kvm_msr_list __user *user_msr_list = argp;
		struct kvm_msr_list msr_list;
		unsigned int n;