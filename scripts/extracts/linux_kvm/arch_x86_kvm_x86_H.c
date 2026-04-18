// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 40/86



		u.sregs2 = memdup_user(argp, sizeof(struct kvm_sregs2));
		if (IS_ERR(u.sregs2)) {
			r = PTR_ERR(u.sregs2);
			u.sregs2 = NULL;
			goto out;
		}
		r = __set_sregs2(vcpu, u.sregs2);
		break;
	}
	case KVM_HAS_DEVICE_ATTR:
	case KVM_GET_DEVICE_ATTR:
	case KVM_SET_DEVICE_ATTR:
		r = kvm_vcpu_ioctl_device_attr(vcpu, ioctl, argp);
		break;
	case KVM_MEMORY_ENCRYPT_OP:
		r = -ENOTTY;
		if (!kvm_x86_ops.vcpu_mem_enc_ioctl)
			goto out;
		r = kvm_x86_ops.vcpu_mem_enc_ioctl(vcpu, argp);
		break;
	default:
		r = -EINVAL;
	}
out:
	kfree(u.buffer);
out_nofree:
	vcpu_put(vcpu);
	return r;
}

vm_fault_t kvm_arch_vcpu_fault(struct kvm_vcpu *vcpu, struct vm_fault *vmf)
{
	return VM_FAULT_SIGBUS;
}

static int kvm_vm_ioctl_set_tss_addr(struct kvm *kvm, unsigned long addr)
{
	int ret;

	if (addr > (unsigned int)(-3 * PAGE_SIZE))
		return -EINVAL;
	ret = kvm_x86_call(set_tss_addr)(kvm, addr);
	return ret;
}

static int kvm_vm_ioctl_set_identity_map_addr(struct kvm *kvm,
					      u64 ident_addr)
{
	return kvm_x86_call(set_identity_map_addr)(kvm, ident_addr);
}

static int kvm_vm_ioctl_set_nr_mmu_pages(struct kvm *kvm,
					 unsigned long kvm_nr_mmu_pages)
{
	if (kvm_nr_mmu_pages < KVM_MIN_ALLOC_MMU_PAGES)
		return -EINVAL;

	mutex_lock(&kvm->slots_lock);

	kvm_mmu_change_mmu_pages(kvm, kvm_nr_mmu_pages);
	kvm->arch.n_requested_mmu_pages = kvm_nr_mmu_pages;

	mutex_unlock(&kvm->slots_lock);
	return 0;
}

void kvm_arch_sync_dirty_log(struct kvm *kvm, struct kvm_memory_slot *memslot)
{

	/*
	 * Flush all CPUs' dirty log buffers to the  dirty_bitmap.  Called
	 * before reporting dirty_bitmap to userspace.  KVM flushes the buffers
	 * on all VM-Exits, thus we only need to kick running vCPUs to force a
	 * VM-Exit.
	 */
	struct kvm_vcpu *vcpu;
	unsigned long i;

	if (!kvm->arch.cpu_dirty_log_size)
		return;

	kvm_for_each_vcpu(i, vcpu, kvm)
		kvm_vcpu_kick(vcpu);
}

int kvm_vm_ioctl_enable_cap(struct kvm *kvm,
			    struct kvm_enable_cap *cap)
{
	int r;

	if (cap->flags)
		return -EINVAL;

	switch (cap->cap) {
	case KVM_CAP_DISABLE_QUIRKS2:
		r = -EINVAL;
		if (cap->args[0] & ~kvm_caps.supported_quirks)
			break;
		fallthrough;
	case KVM_CAP_DISABLE_QUIRKS:
		kvm->arch.disabled_quirks |= cap->args[0] & kvm_caps.supported_quirks;
		r = 0;
		break;
	case KVM_CAP_SPLIT_IRQCHIP: {
		mutex_lock(&kvm->lock);
		r = -EINVAL;
		if (cap->args[0] > KVM_MAX_IRQ_ROUTES)
			goto split_irqchip_unlock;
		r = -EEXIST;
		if (irqchip_in_kernel(kvm))
			goto split_irqchip_unlock;
		if (kvm->created_vcpus)
			goto split_irqchip_unlock;
		/* Pairs with irqchip_in_kernel. */
		smp_wmb();
		kvm->arch.irqchip_mode = KVM_IRQCHIP_SPLIT;
		kvm->arch.nr_reserved_ioapic_pins = cap->args[0];
		kvm_clear_apicv_inhibit(kvm, APICV_INHIBIT_REASON_ABSENT);
		r = 0;
split_irqchip_unlock:
		mutex_unlock(&kvm->lock);
		break;
	}
	case KVM_CAP_X2APIC_API:
		r = -EINVAL;
		if (cap->args[0] & ~KVM_X2APIC_API_VALID_FLAGS)
			break;

		if ((cap->args[0] & KVM_X2APIC_ENABLE_SUPPRESS_EOI_BROADCAST) &&
		    (cap->args[0] & KVM_X2APIC_DISABLE_SUPPRESS_EOI_BROADCAST))
			break;

		if ((cap->args[0] & KVM_X2APIC_ENABLE_SUPPRESS_EOI_BROADCAST) &&
		    !irqchip_split(kvm))
			break;

		if (cap->args[0] & KVM_X2APIC_API_USE_32BIT_IDS)
			kvm->arch.x2apic_format = true;
		if (cap->args[0] & KVM_X2APIC_API_DISABLE_BROADCAST_QUIRK)
			kvm->arch.x2apic_broadcast_quirk_disabled = true;

		if (cap->args[0] & KVM_X2APIC_ENABLE_SUPPRESS_EOI_BROADCAST)
			kvm->arch.suppress_eoi_broadcast_mode = KVM_SUPPRESS_EOI_BROADCAST_ENABLED;
		if (cap->args[0] & KVM_X2APIC_DISABLE_SUPPRESS_EOI_BROADCAST)
			kvm->arch.suppress_eoi_broadcast_mode = KVM_SUPPRESS_EOI_BROADCAST_DISABLED;

		r = 0;
		break;
	case KVM_CAP_X86_DISABLE_EXITS:
		r = -EINVAL;
		if (cap->args[0] & ~kvm_get_allowed_disable_exits())
			break;

		mutex_lock(&kvm->lock);
		if (kvm->created_vcpus)
			goto disable_exits_unlock;

#define SMT_RSB_MSG "This processor is affected by the Cross-Thread Return Predictions vulnerability. " \
		    "KVM_CAP_X86_DISABLE_EXITS should only be used with SMT disabled or trusted guests."

		if (!mitigate_smt_rsb && boot_cpu_has_bug(X86_BUG_SMT_RSB) &&
		    cpu_smt_possible() &&
		    (cap->args[0] & ~(KVM_X86_DISABLE_EXITS_PAUSE |
				      KVM_X86_DISABLE_EXITS_APERFMPERF)))
			pr_warn_once(SMT_RSB_MSG);