// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 44/86



		r = -ENXIO;
		if (!irqchip_full(kvm))
			goto set_irqchip_out;
		r = kvm_vm_ioctl_set_irqchip(kvm, chip);
	set_irqchip_out:
		kfree(chip);
		break;
	}
	case KVM_GET_PIT: {
		r = -EFAULT;
		if (copy_from_user(&u.ps, argp, sizeof(struct kvm_pit_state)))
			goto out;
		r = -ENXIO;
		if (!kvm->arch.vpit)
			goto out;
		r = kvm_vm_ioctl_get_pit(kvm, &u.ps);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, &u.ps, sizeof(struct kvm_pit_state)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_PIT: {
		r = -EFAULT;
		if (copy_from_user(&u.ps, argp, sizeof(u.ps)))
			goto out;
		mutex_lock(&kvm->lock);
		r = -ENXIO;
		if (!kvm->arch.vpit)
			goto set_pit_out;
		r = kvm_vm_ioctl_set_pit(kvm, &u.ps);
set_pit_out:
		mutex_unlock(&kvm->lock);
		break;
	}
	case KVM_GET_PIT2: {
		r = -ENXIO;
		if (!kvm->arch.vpit)
			goto out;
		r = kvm_vm_ioctl_get_pit2(kvm, &u.ps2);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, &u.ps2, sizeof(u.ps2)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_PIT2: {
		r = -EFAULT;
		if (copy_from_user(&u.ps2, argp, sizeof(u.ps2)))
			goto out;
		mutex_lock(&kvm->lock);
		r = -ENXIO;
		if (!kvm->arch.vpit)
			goto set_pit2_out;
		r = kvm_vm_ioctl_set_pit2(kvm, &u.ps2);
set_pit2_out:
		mutex_unlock(&kvm->lock);
		break;
	}
	case KVM_REINJECT_CONTROL: {
		struct kvm_reinject_control control;
		r =  -EFAULT;
		if (copy_from_user(&control, argp, sizeof(control)))
			goto out;
		r = -ENXIO;
		if (!kvm->arch.vpit)
			goto out;
		r = kvm_vm_ioctl_reinject(kvm, &control);
		break;
	}
#endif
	case KVM_SET_BOOT_CPU_ID:
		r = 0;
		mutex_lock(&kvm->lock);
		if (kvm->created_vcpus)
			r = -EBUSY;
		else if (arg > KVM_MAX_VCPU_IDS ||
			 (kvm->arch.max_vcpu_ids && arg > kvm->arch.max_vcpu_ids))
			r = -EINVAL;
		else
			kvm->arch.bsp_vcpu_id = arg;
		mutex_unlock(&kvm->lock);
		break;
#ifdef CONFIG_KVM_XEN
	case KVM_XEN_HVM_CONFIG: {
		struct kvm_xen_hvm_config xhc;
		r = -EFAULT;
		if (copy_from_user(&xhc, argp, sizeof(xhc)))
			goto out;
		r = kvm_xen_hvm_config(kvm, &xhc);
		break;
	}
	case KVM_XEN_HVM_GET_ATTR: {
		struct kvm_xen_hvm_attr xha;

		r = -EFAULT;
		if (copy_from_user(&xha, argp, sizeof(xha)))
			goto out;
		r = kvm_xen_hvm_get_attr(kvm, &xha);
		if (!r && copy_to_user(argp, &xha, sizeof(xha)))
			r = -EFAULT;
		break;
	}
	case KVM_XEN_HVM_SET_ATTR: {
		struct kvm_xen_hvm_attr xha;

		r = -EFAULT;
		if (copy_from_user(&xha, argp, sizeof(xha)))
			goto out;
		r = kvm_xen_hvm_set_attr(kvm, &xha);
		break;
	}
	case KVM_XEN_HVM_EVTCHN_SEND: {
		struct kvm_irq_routing_xen_evtchn uxe;

		r = -EFAULT;
		if (copy_from_user(&uxe, argp, sizeof(uxe)))
			goto out;
		r = kvm_xen_hvm_evtchn_send(kvm, &uxe);
		break;
	}
#endif
	case KVM_SET_CLOCK:
		r = kvm_vm_ioctl_set_clock(kvm, argp);
		break;
	case KVM_GET_CLOCK:
		r = kvm_vm_ioctl_get_clock(kvm, argp);
		break;
	case KVM_SET_TSC_KHZ: {
		u32 user_tsc_khz;

		r = -EINVAL;
		user_tsc_khz = (u32)arg;

		if (kvm_caps.has_tsc_control &&
		    user_tsc_khz >= kvm_caps.max_guest_tsc_khz)
			goto out;

		if (user_tsc_khz == 0)
			user_tsc_khz = tsc_khz;

		mutex_lock(&kvm->lock);
		if (!kvm->created_vcpus) {
			WRITE_ONCE(kvm->arch.default_tsc_khz, user_tsc_khz);
			r = 0;
		}
		mutex_unlock(&kvm->lock);
		goto out;
	}
	case KVM_GET_TSC_KHZ: {
		r = READ_ONCE(kvm->arch.default_tsc_khz);
		goto out;
	}
	case KVM_MEMORY_ENCRYPT_OP:
		r = -ENOTTY;
		if (!kvm_x86_ops.mem_enc_ioctl)
			goto out;

		r = kvm_x86_call(mem_enc_ioctl)(kvm, argp);
		break;
	case KVM_MEMORY_ENCRYPT_REG_REGION: {
		struct kvm_enc_region region;

		r = -EFAULT;
		if (copy_from_user(&region, argp, sizeof(region)))
			goto out;

		r = -ENOTTY;
		if (!kvm_x86_ops.mem_enc_register_region)
			goto out;

		r = kvm_x86_call(mem_enc_register_region)(kvm, &region);
		break;
	}
	case KVM_MEMORY_ENCRYPT_UNREG_REGION: {
		struct kvm_enc_region region;

		r = -EFAULT;
		if (copy_from_user(&region, argp, sizeof(region)))
			goto out;

		r = -ENOTTY;
		if (!kvm_x86_ops.mem_enc_unregister_region)
			goto out;

		r = kvm_x86_call(mem_enc_unregister_region)(kvm, &region);
		break;
	}
#ifdef CONFIG_KVM_HYPERV
	case KVM_HYPERV_EVENTFD: {
		struct kvm_hyperv_eventfd hvevfd;

		r = -EFAULT;
		if (copy_from_user(&hvevfd, argp, sizeof(hvevfd)))
			goto out;
		r = kvm_vm_ioctl_hv_eventfd(kvm, &hvevfd);
		break;
	}
#endif
	case KVM_SET_PMU_EVENT_FILTER:
		r = kvm_vm_ioctl_set_pmu_event_filter(kvm, argp);
		break;
	case KVM_X86_SET_MSR_FILTER: {
		struct kvm_msr_filter __user *user_msr_filter = argp;
		struct kvm_msr_filter filter;

		if (copy_from_user(&filter, user_msr_filter, sizeof(filter)))
			return -EFAULT;

		r = kvm_vm_ioctl_set_msr_filter(kvm, &filter);
		break;
	}
	default:
		r = -ENOTTY;
	}
out:
	return r;
}

static void kvm_probe_feature_msr(u32 msr_index)
{
	u64 data;

	if (kvm_get_feature_msr(NULL, msr_index, &data, true))
		return;

	msr_based_features[num_msr_based_features++] = msr_index;
}

static void kvm_probe_msr_to_save(u32 msr_index)
{
	u32 dummy[2];