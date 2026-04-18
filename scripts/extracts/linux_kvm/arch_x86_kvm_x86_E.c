// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 37/86



		raw_spin_lock_irqsave(&kvm->arch.tsc_write_lock, flags);

		matched = (vcpu->arch.virtual_tsc_khz &&
			   kvm->arch.last_tsc_khz == vcpu->arch.virtual_tsc_khz &&
			   kvm->arch.last_tsc_offset == offset);

		tsc = kvm_scale_tsc(rdtsc(), vcpu->arch.l1_tsc_scaling_ratio) + offset;
		ns = get_kvmclock_base_ns();

		__kvm_synchronize_tsc(vcpu, offset, tsc, ns, matched, true);
		raw_spin_unlock_irqrestore(&kvm->arch.tsc_write_lock, flags);

		r = 0;
		break;
	}
	default:
		r = -ENXIO;
	}

	return r;
}

static int kvm_vcpu_ioctl_device_attr(struct kvm_vcpu *vcpu,
				      unsigned int ioctl,
				      void __user *argp)
{
	struct kvm_device_attr attr;
	int r;

	if (copy_from_user(&attr, argp, sizeof(attr)))
		return -EFAULT;

	if (attr.group != KVM_VCPU_TSC_CTRL)
		return -ENXIO;

	switch (ioctl) {
	case KVM_HAS_DEVICE_ATTR:
		r = kvm_arch_tsc_has_attr(vcpu, &attr);
		break;
	case KVM_GET_DEVICE_ATTR:
		r = kvm_arch_tsc_get_attr(vcpu, &attr);
		break;
	case KVM_SET_DEVICE_ATTR:
		r = kvm_arch_tsc_set_attr(vcpu, &attr);
		break;
	}

	return r;
}

static int kvm_vcpu_ioctl_enable_cap(struct kvm_vcpu *vcpu,
				     struct kvm_enable_cap *cap)
{
	if (cap->flags)
		return -EINVAL;

	switch (cap->cap) {
#ifdef CONFIG_KVM_HYPERV
	case KVM_CAP_HYPERV_SYNIC2:
		if (cap->args[0])
			return -EINVAL;
		fallthrough;

	case KVM_CAP_HYPERV_SYNIC:
		if (!irqchip_in_kernel(vcpu->kvm))
			return -EINVAL;
		return kvm_hv_activate_synic(vcpu, cap->cap ==
					     KVM_CAP_HYPERV_SYNIC2);
	case KVM_CAP_HYPERV_ENLIGHTENED_VMCS:
		{
			int r;
			uint16_t vmcs_version;
			void __user *user_ptr;

			if (!kvm_x86_ops.nested_ops->enable_evmcs)
				return -ENOTTY;
			r = kvm_x86_ops.nested_ops->enable_evmcs(vcpu, &vmcs_version);
			if (!r) {
				user_ptr = (void __user *)(uintptr_t)cap->args[0];
				if (copy_to_user(user_ptr, &vmcs_version,
						 sizeof(vmcs_version)))
					r = -EFAULT;
			}
			return r;
		}
	case KVM_CAP_HYPERV_DIRECT_TLBFLUSH:
		if (!kvm_x86_ops.enable_l2_tlb_flush)
			return -ENOTTY;

		return kvm_x86_call(enable_l2_tlb_flush)(vcpu);

	case KVM_CAP_HYPERV_ENFORCE_CPUID:
		return kvm_hv_set_enforce_cpuid(vcpu, cap->args[0]);
#endif

	case KVM_CAP_ENFORCE_PV_FEATURE_CPUID:
		vcpu->arch.pv_cpuid.enforce = cap->args[0];
		return 0;
	default:
		return -EINVAL;
	}
}

struct kvm_x86_reg_id {
	__u32 index;
	__u8  type;
	__u8  rsvd1;
	__u8  rsvd2:4;
	__u8  size:4;
	__u8  x86;
};

static int kvm_translate_kvm_reg(struct kvm_vcpu *vcpu,
				 struct kvm_x86_reg_id *reg)
{
	switch (reg->index) {
	case KVM_REG_GUEST_SSP:
		/*
		 * FIXME: If host-initiated accesses are ever exempted from
		 * ignore_msrs (in kvm_do_msr_access()), drop this manual check
		 * and rely on KVM's standard checks to reject accesses to regs
		 * that don't exist.
		 */
		if (!guest_cpu_cap_has(vcpu, X86_FEATURE_SHSTK))
			return -EINVAL;

		reg->type = KVM_X86_REG_TYPE_MSR;
		reg->index = MSR_KVM_INTERNAL_GUEST_SSP;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int kvm_get_one_msr(struct kvm_vcpu *vcpu, u32 msr, u64 __user *user_val)
{
	u64 val;

	if (do_get_msr(vcpu, msr, &val))
		return -EINVAL;

	if (put_user(val, user_val))
		return -EFAULT;

	return 0;
}

static int kvm_set_one_msr(struct kvm_vcpu *vcpu, u32 msr, u64 __user *user_val)
{
	u64 val;

	if (get_user(val, user_val))
		return -EFAULT;

	if (do_set_msr(vcpu, msr, &val))
		return -EINVAL;

	return 0;
}

static int kvm_get_set_one_reg(struct kvm_vcpu *vcpu, unsigned int ioctl,
			       void __user *argp)
{
	struct kvm_one_reg one_reg;
	struct kvm_x86_reg_id *reg;
	u64 __user *user_val;
	bool load_fpu;
	int r;

	if (copy_from_user(&one_reg, argp, sizeof(one_reg)))
		return -EFAULT;

	if ((one_reg.id & KVM_REG_ARCH_MASK) != KVM_REG_X86)
		return -EINVAL;

	reg = (struct kvm_x86_reg_id *)&one_reg.id;
	if (reg->rsvd1 || reg->rsvd2)
		return -EINVAL;

	if (reg->type == KVM_X86_REG_TYPE_KVM) {
		r = kvm_translate_kvm_reg(vcpu, reg);
		if (r)
			return r;
	}

	if (reg->type != KVM_X86_REG_TYPE_MSR)
		return -EINVAL;

	if ((one_reg.id & KVM_REG_SIZE_MASK) != KVM_REG_SIZE_U64)
		return -EINVAL;

	guard(srcu)(&vcpu->kvm->srcu);

	load_fpu = is_xstate_managed_msr(vcpu, reg->index);
	if (load_fpu)
		kvm_load_guest_fpu(vcpu);

	user_val = u64_to_user_ptr(one_reg.addr);
	if (ioctl == KVM_GET_ONE_REG)
		r = kvm_get_one_msr(vcpu, reg->index, user_val);
	else
		r = kvm_set_one_msr(vcpu, reg->index, user_val);

	if (load_fpu)
		kvm_put_guest_fpu(vcpu);
	return r;
}

static int kvm_get_reg_list(struct kvm_vcpu *vcpu,
			    struct kvm_reg_list __user *user_list)
{
	u64 nr_regs = guest_cpu_cap_has(vcpu, X86_FEATURE_SHSTK) ? 1 : 0;
	u64 user_nr_regs;

	if (get_user(user_nr_regs, &user_list->n))
		return -EFAULT;

	if (put_user(nr_regs, &user_list->n))
		return -EFAULT;

	if (user_nr_regs < nr_regs)
		return -E2BIG;

	if (nr_regs &&
	    put_user(KVM_X86_REG_KVM(KVM_REG_GUEST_SSP), &user_list->reg[0]))
		return -EFAULT;

	return 0;
}