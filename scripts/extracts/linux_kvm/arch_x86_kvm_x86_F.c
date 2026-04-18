// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 38/86



long kvm_arch_vcpu_ioctl(struct file *filp,
			 unsigned int ioctl, unsigned long arg)
{
	struct kvm_vcpu *vcpu = filp->private_data;
	void __user *argp = (void __user *)arg;
	int r;
	union {
		struct kvm_sregs2 *sregs2;
		struct kvm_lapic_state *lapic;
		struct kvm_xsave *xsave;
		struct kvm_xcrs *xcrs;
		void *buffer;
	} u;

	vcpu_load(vcpu);

	u.buffer = NULL;
	switch (ioctl) {
	case KVM_GET_LAPIC: {
		r = -EINVAL;
		if (!lapic_in_kernel(vcpu))
			goto out;
		u.lapic = kzalloc_obj(struct kvm_lapic_state);

		r = -ENOMEM;
		if (!u.lapic)
			goto out;
		r = kvm_vcpu_ioctl_get_lapic(vcpu, u.lapic);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, u.lapic, sizeof(struct kvm_lapic_state)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_LAPIC: {
		r = -EINVAL;
		if (!lapic_in_kernel(vcpu))
			goto out;
		u.lapic = memdup_user(argp, sizeof(*u.lapic));
		if (IS_ERR(u.lapic)) {
			r = PTR_ERR(u.lapic);
			goto out_nofree;
		}

		r = kvm_vcpu_ioctl_set_lapic(vcpu, u.lapic);
		break;
	}
	case KVM_INTERRUPT: {
		struct kvm_interrupt irq;

		r = -EFAULT;
		if (copy_from_user(&irq, argp, sizeof(irq)))
			goto out;
		r = kvm_vcpu_ioctl_interrupt(vcpu, &irq);
		break;
	}
	case KVM_NMI: {
		r = kvm_vcpu_ioctl_nmi(vcpu);
		break;
	}
	case KVM_SMI: {
		r = kvm_inject_smi(vcpu);
		break;
	}
	case KVM_SET_CPUID: {
		struct kvm_cpuid __user *cpuid_arg = argp;
		struct kvm_cpuid cpuid;

		r = -EFAULT;
		if (copy_from_user(&cpuid, cpuid_arg, sizeof(cpuid)))
			goto out;
		r = kvm_vcpu_ioctl_set_cpuid(vcpu, &cpuid, cpuid_arg->entries);
		break;
	}
	case KVM_SET_CPUID2: {
		struct kvm_cpuid2 __user *cpuid_arg = argp;
		struct kvm_cpuid2 cpuid;

		r = -EFAULT;
		if (copy_from_user(&cpuid, cpuid_arg, sizeof(cpuid)))
			goto out;
		r = kvm_vcpu_ioctl_set_cpuid2(vcpu, &cpuid,
					      cpuid_arg->entries);
		break;
	}
	case KVM_GET_CPUID2: {
		struct kvm_cpuid2 __user *cpuid_arg = argp;
		struct kvm_cpuid2 cpuid;

		r = -EFAULT;
		if (copy_from_user(&cpuid, cpuid_arg, sizeof(cpuid)))
			goto out;
		r = kvm_vcpu_ioctl_get_cpuid2(vcpu, &cpuid,
					      cpuid_arg->entries);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(cpuid_arg, &cpuid, sizeof(cpuid)))
			goto out;
		r = 0;
		break;
	}
	case KVM_GET_MSRS: {
		int idx = srcu_read_lock(&vcpu->kvm->srcu);
		r = msr_io(vcpu, argp, do_get_msr, 1);
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
		break;
	}
	case KVM_SET_MSRS: {
		int idx = srcu_read_lock(&vcpu->kvm->srcu);
		r = msr_io(vcpu, argp, do_set_msr, 0);
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
		break;
	}
	case KVM_GET_ONE_REG:
	case KVM_SET_ONE_REG:
		r = kvm_get_set_one_reg(vcpu, ioctl, argp);
		break;
	case KVM_GET_REG_LIST:
		r = kvm_get_reg_list(vcpu, argp);
		break;
	case KVM_TPR_ACCESS_REPORTING: {
		struct kvm_tpr_access_ctl tac;

		r = -EFAULT;
		if (copy_from_user(&tac, argp, sizeof(tac)))
			goto out;
		r = vcpu_ioctl_tpr_access_reporting(vcpu, &tac);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, &tac, sizeof(tac)))
			goto out;
		r = 0;
		break;
	};
	case KVM_SET_VAPIC_ADDR: {
		struct kvm_vapic_addr va;
		int idx;

		r = -EINVAL;
		if (!lapic_in_kernel(vcpu))
			goto out;
		r = -EFAULT;
		if (copy_from_user(&va, argp, sizeof(va)))
			goto out;
		idx = srcu_read_lock(&vcpu->kvm->srcu);
		r = kvm_lapic_set_vapic_addr(vcpu, va.vapic_addr);
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
		break;
	}
	case KVM_X86_SETUP_MCE: {
		u64 mcg_cap;

		r = -EFAULT;
		if (copy_from_user(&mcg_cap, argp, sizeof(mcg_cap)))
			goto out;
		r = kvm_vcpu_ioctl_x86_setup_mce(vcpu, mcg_cap);
		break;
	}
	case KVM_X86_SET_MCE: {
		struct kvm_x86_mce mce;

		r = -EFAULT;
		if (copy_from_user(&mce, argp, sizeof(mce)))
			goto out;
		r = kvm_vcpu_ioctl_x86_set_mce(vcpu, &mce);
		break;
	}
	case KVM_GET_VCPU_EVENTS: {
		struct kvm_vcpu_events events;

		kvm_vcpu_ioctl_x86_get_vcpu_events(vcpu, &events);

		r = -EFAULT;
		if (copy_to_user(argp, &events, sizeof(struct kvm_vcpu_events)))
			break;
		r = 0;
		break;
	}
	case KVM_SET_VCPU_EVENTS: {
		struct kvm_vcpu_events events;

		r = -EFAULT;
		if (copy_from_user(&events, argp, sizeof(struct kvm_vcpu_events)))
			break;

		kvm_vcpu_srcu_read_lock(vcpu);
		r = kvm_vcpu_ioctl_x86_set_vcpu_events(vcpu, &events);
		kvm_vcpu_srcu_read_unlock(vcpu);
		break;
	}
	case KVM_GET_DEBUGREGS: {
		struct kvm_debugregs dbgregs;

		r = kvm_vcpu_ioctl_x86_get_debugregs(vcpu, &dbgregs);
		if (r < 0)
			break;

		r = -EFAULT;
		if (copy_to_user(argp, &dbgregs,
				 sizeof(struct kvm_debugregs)))
			break;
		r = 0;
		break;
	}
	case KVM_SET_DEBUGREGS: {
		struct kvm_debugregs dbgregs;

		r = -EFAULT;
		if (copy_from_user(&dbgregs, argp,
				   sizeof(struct kvm_debugregs)))
			break;

		r = kvm_vcpu_ioctl_x86_set_debugregs(vcpu, &dbgregs);
		break;
	}
	case KVM_GET_XSAVE: {
		r = -EINVAL;
		if (vcpu->arch.guest_fpu.uabi_size > sizeof(struct kvm_xsave))
			break;

		u.xsave = kzalloc_obj(struct kvm_xsave);
		r = -ENOMEM;
		if (!u.xsave)
			break;