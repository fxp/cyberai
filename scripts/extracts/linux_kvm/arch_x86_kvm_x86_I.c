// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 41/86



		kvm_disable_exits(kvm, cap->args[0]);
		r = 0;
disable_exits_unlock:
		mutex_unlock(&kvm->lock);
		break;
	case KVM_CAP_MSR_PLATFORM_INFO:
		kvm->arch.guest_can_read_msr_platform_info = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_EXCEPTION_PAYLOAD:
		kvm->arch.exception_payload_enabled = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_X86_TRIPLE_FAULT_EVENT:
		kvm->arch.triple_fault_event = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_X86_USER_SPACE_MSR:
		r = -EINVAL;
		if (cap->args[0] & ~KVM_MSR_EXIT_REASON_VALID_MASK)
			break;
		kvm->arch.user_space_msr_mask = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_X86_BUS_LOCK_EXIT:
		r = -EINVAL;
		if (cap->args[0] & ~KVM_BUS_LOCK_DETECTION_VALID_MODE)
			break;

		if ((cap->args[0] & KVM_BUS_LOCK_DETECTION_OFF) &&
		    (cap->args[0] & KVM_BUS_LOCK_DETECTION_EXIT))
			break;

		if (kvm_caps.has_bus_lock_exit &&
		    cap->args[0] & KVM_BUS_LOCK_DETECTION_EXIT)
			kvm->arch.bus_lock_detection_enabled = true;
		r = 0;
		break;
#ifdef CONFIG_X86_SGX_KVM
	case KVM_CAP_SGX_ATTRIBUTE: {
		unsigned long allowed_attributes = 0;

		r = sgx_set_attribute(&allowed_attributes, cap->args[0]);
		if (r)
			break;

		/* KVM only supports the PROVISIONKEY privileged attribute. */
		if ((allowed_attributes & SGX_ATTR_PROVISIONKEY) &&
		    !(allowed_attributes & ~SGX_ATTR_PROVISIONKEY))
			kvm->arch.sgx_provisioning_allowed = true;
		else
			r = -EINVAL;
		break;
	}
#endif
	case KVM_CAP_VM_COPY_ENC_CONTEXT_FROM:
		r = -EINVAL;
		if (!kvm_x86_ops.vm_copy_enc_context_from)
			break;

		r = kvm_x86_call(vm_copy_enc_context_from)(kvm, cap->args[0]);
		break;
	case KVM_CAP_VM_MOVE_ENC_CONTEXT_FROM:
		r = -EINVAL;
		if (!kvm_x86_ops.vm_move_enc_context_from)
			break;

		r = kvm_x86_call(vm_move_enc_context_from)(kvm, cap->args[0]);
		break;
	case KVM_CAP_EXIT_HYPERCALL:
		if (cap->args[0] & ~KVM_EXIT_HYPERCALL_VALID_MASK) {
			r = -EINVAL;
			break;
		}
		kvm->arch.hypercall_exit_enabled = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_EXIT_ON_EMULATION_FAILURE:
		r = -EINVAL;
		if (cap->args[0] & ~1)
			break;
		kvm->arch.exit_on_emulation_error = cap->args[0];
		r = 0;
		break;
	case KVM_CAP_PMU_CAPABILITY:
		r = -EINVAL;
		if (!enable_pmu || (cap->args[0] & ~KVM_CAP_PMU_VALID_MASK))
			break;

		mutex_lock(&kvm->lock);
		if (!kvm->created_vcpus && !kvm->arch.created_mediated_pmu) {
			kvm->arch.enable_pmu = !(cap->args[0] & KVM_PMU_CAP_DISABLE);
			r = 0;
		}
		mutex_unlock(&kvm->lock);
		break;
	case KVM_CAP_MAX_VCPU_ID:
		r = -EINVAL;
		if (cap->args[0] > KVM_MAX_VCPU_IDS)
			break;

		mutex_lock(&kvm->lock);
		if (kvm->arch.bsp_vcpu_id > cap->args[0]) {
			;
		} else if (kvm->arch.max_vcpu_ids == cap->args[0]) {
			r = 0;
		} else if (!kvm->arch.max_vcpu_ids) {
			kvm->arch.max_vcpu_ids = cap->args[0];
			r = 0;
		}
		mutex_unlock(&kvm->lock);
		break;
	case KVM_CAP_X86_NOTIFY_VMEXIT:
		r = -EINVAL;
		if ((u32)cap->args[0] & ~KVM_X86_NOTIFY_VMEXIT_VALID_BITS)
			break;
		if (!kvm_caps.has_notify_vmexit)
			break;
		if (!((u32)cap->args[0] & KVM_X86_NOTIFY_VMEXIT_ENABLED))
			break;
		mutex_lock(&kvm->lock);
		if (!kvm->created_vcpus) {
			kvm->arch.notify_window = cap->args[0] >> 32;
			kvm->arch.notify_vmexit_flags = (u32)cap->args[0];
			r = 0;
		}
		mutex_unlock(&kvm->lock);
		break;
	case KVM_CAP_VM_DISABLE_NX_HUGE_PAGES:
		r = -EINVAL;

		/*
		 * Since the risk of disabling NX hugepages is a guest crashing
		 * the system, ensure the userspace process has permission to
		 * reboot the system.
		 *
		 * Note that unlike the reboot() syscall, the process must have
		 * this capability in the root namespace because exposing
		 * /dev/kvm into a container does not limit the scope of the
		 * iTLB multihit bug to that container. In other words,
		 * this must use capable(), not ns_capable().
		 */
		if (!capable(CAP_SYS_BOOT)) {
			r = -EPERM;
			break;
		}

		if (cap->args[0])
			break;

		mutex_lock(&kvm->lock);
		if (!kvm->created_vcpus) {
			kvm->arch.disable_nx_huge_pages = true;
			r = 0;
		}
		mutex_unlock(&kvm->lock);
		break;
	case KVM_CAP_X86_APIC_BUS_CYCLES_NS: {
		u64 bus_cycle_ns = cap->args[0];
		u64 unused;

		/*
		 * Guard against overflow in tmict_to_ns(). 128 is the highest
		 * divide value that can be programmed in APIC_TDCR.
		 */
		r = -EINVAL;
		if (!bus_cycle_ns ||
		    check_mul_overflow((u64)U32_MAX * 128, bus_cycle_ns, &unused))
			break;

		r = 0;
		mutex_lock(&kvm->lock);
		if (!irqchip_in_kernel(kvm))
			r = -ENXIO;
		else if (kvm->created_vcpus)
			r = -EINVAL;
		else
			kvm->arch.apic_bus_cycle_ns = bus_cycle_ns;
		mutex_unlock(&kvm->lock);
		break;
	}
	default:
		r = -EINVAL;
		break;
	}
	return r;
}

static struct kvm_x86_msr_filter *kvm_alloc_msr_filter(bool default_allow)
{
	struct kvm_x86_msr_filter *msr_filter;

	msr_filter = kzalloc_obj(*msr_filter, GFP_KERNEL_ACCOUNT);
	if (!msr_filter)
		return NULL;

	msr_filter->default_allow = default_allow;
	return msr_filter;
}