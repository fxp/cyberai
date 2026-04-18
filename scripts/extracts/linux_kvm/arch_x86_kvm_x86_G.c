// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 39/86



		r = kvm_vcpu_ioctl_x86_get_xsave(vcpu, u.xsave);
		if (r < 0)
			break;

		r = -EFAULT;
		if (copy_to_user(argp, u.xsave, sizeof(struct kvm_xsave)))
			break;
		r = 0;
		break;
	}
	case KVM_SET_XSAVE: {
		int size = vcpu->arch.guest_fpu.uabi_size;

		u.xsave = memdup_user(argp, size);
		if (IS_ERR(u.xsave)) {
			r = PTR_ERR(u.xsave);
			goto out_nofree;
		}

		r = kvm_vcpu_ioctl_x86_set_xsave(vcpu, u.xsave);
		break;
	}

	case KVM_GET_XSAVE2: {
		int size = vcpu->arch.guest_fpu.uabi_size;

		u.xsave = kzalloc(size, GFP_KERNEL);
		r = -ENOMEM;
		if (!u.xsave)
			break;

		r = kvm_vcpu_ioctl_x86_get_xsave2(vcpu, u.buffer, size);
		if (r < 0)
			break;

		r = -EFAULT;
		if (copy_to_user(argp, u.xsave, size))
			break;

		r = 0;
		break;
	}

	case KVM_GET_XCRS: {
		u.xcrs = kzalloc_obj(struct kvm_xcrs);
		r = -ENOMEM;
		if (!u.xcrs)
			break;

		r = kvm_vcpu_ioctl_x86_get_xcrs(vcpu, u.xcrs);
		if (r < 0)
			break;

		r = -EFAULT;
		if (copy_to_user(argp, u.xcrs,
				 sizeof(struct kvm_xcrs)))
			break;
		r = 0;
		break;
	}
	case KVM_SET_XCRS: {
		u.xcrs = memdup_user(argp, sizeof(*u.xcrs));
		if (IS_ERR(u.xcrs)) {
			r = PTR_ERR(u.xcrs);
			goto out_nofree;
		}

		r = kvm_vcpu_ioctl_x86_set_xcrs(vcpu, u.xcrs);
		break;
	}
	case KVM_SET_TSC_KHZ: {
		u32 user_tsc_khz;

		r = -EINVAL;

		if (vcpu->arch.guest_tsc_protected)
			goto out;

		user_tsc_khz = (u32)arg;

		if (kvm_caps.has_tsc_control &&
		    user_tsc_khz >= kvm_caps.max_guest_tsc_khz)
			goto out;

		if (user_tsc_khz == 0)
			user_tsc_khz = tsc_khz;

		if (!kvm_set_tsc_khz(vcpu, user_tsc_khz))
			r = 0;

		goto out;
	}
	case KVM_GET_TSC_KHZ: {
		r = vcpu->arch.virtual_tsc_khz;
		goto out;
	}
	case KVM_KVMCLOCK_CTRL: {
		r = kvm_set_guest_paused(vcpu);
		goto out;
	}
	case KVM_ENABLE_CAP: {
		struct kvm_enable_cap cap;

		r = -EFAULT;
		if (copy_from_user(&cap, argp, sizeof(cap)))
			goto out;
		r = kvm_vcpu_ioctl_enable_cap(vcpu, &cap);
		break;
	}
	case KVM_GET_NESTED_STATE: {
		struct kvm_nested_state __user *user_kvm_nested_state = argp;
		u32 user_data_size;

		r = -EINVAL;
		if (!kvm_x86_ops.nested_ops->get_state)
			break;

		BUILD_BUG_ON(sizeof(user_data_size) != sizeof(user_kvm_nested_state->size));
		r = -EFAULT;
		if (get_user(user_data_size, &user_kvm_nested_state->size))
			break;

		r = kvm_x86_ops.nested_ops->get_state(vcpu, user_kvm_nested_state,
						     user_data_size);
		if (r < 0)
			break;

		if (r > user_data_size) {
			if (put_user(r, &user_kvm_nested_state->size))
				r = -EFAULT;
			else
				r = -E2BIG;
			break;
		}

		r = 0;
		break;
	}
	case KVM_SET_NESTED_STATE: {
		struct kvm_nested_state __user *user_kvm_nested_state = argp;
		struct kvm_nested_state kvm_state;
		int idx;

		r = -EINVAL;
		if (!kvm_x86_ops.nested_ops->set_state)
			break;

		r = -EFAULT;
		if (copy_from_user(&kvm_state, user_kvm_nested_state, sizeof(kvm_state)))
			break;

		r = -EINVAL;
		if (kvm_state.size < sizeof(kvm_state))
			break;

		if (kvm_state.flags &
		    ~(KVM_STATE_NESTED_RUN_PENDING | KVM_STATE_NESTED_GUEST_MODE
		      | KVM_STATE_NESTED_EVMCS | KVM_STATE_NESTED_MTF_PENDING
		      | KVM_STATE_NESTED_GIF_SET))
			break;

		/* nested_run_pending implies guest_mode.  */
		if ((kvm_state.flags & KVM_STATE_NESTED_RUN_PENDING)
		    && !(kvm_state.flags & KVM_STATE_NESTED_GUEST_MODE))
			break;

		idx = srcu_read_lock(&vcpu->kvm->srcu);
		r = kvm_x86_ops.nested_ops->set_state(vcpu, user_kvm_nested_state, &kvm_state);
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
		break;
	}
#ifdef CONFIG_KVM_HYPERV
	case KVM_GET_SUPPORTED_HV_CPUID:
		r = kvm_ioctl_get_supported_hv_cpuid(vcpu, argp);
		break;
#endif
#ifdef CONFIG_KVM_XEN
	case KVM_XEN_VCPU_GET_ATTR: {
		struct kvm_xen_vcpu_attr xva;

		r = -EFAULT;
		if (copy_from_user(&xva, argp, sizeof(xva)))
			goto out;
		r = kvm_xen_vcpu_get_attr(vcpu, &xva);
		if (!r && copy_to_user(argp, &xva, sizeof(xva)))
			r = -EFAULT;
		break;
	}
	case KVM_XEN_VCPU_SET_ATTR: {
		struct kvm_xen_vcpu_attr xva;

		r = -EFAULT;
		if (copy_from_user(&xva, argp, sizeof(xva)))
			goto out;
		r = kvm_xen_vcpu_set_attr(vcpu, &xva);
		break;
	}
#endif
	case KVM_GET_SREGS2: {
		r = -EINVAL;
		if (vcpu->kvm->arch.has_protected_state &&
		    vcpu->arch.guest_state_protected)
			goto out;

		u.sregs2 = kzalloc_obj(struct kvm_sregs2);
		r = -ENOMEM;
		if (!u.sregs2)
			goto out;
		__get_sregs2(vcpu, u.sregs2);
		r = -EFAULT;
		if (copy_to_user(argp, u.sregs2, sizeof(struct kvm_sregs2)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_SREGS2: {
		r = -EINVAL;
		if (vcpu->kvm->arch.has_protected_state &&
		    vcpu->arch.guest_state_protected)
			goto out;