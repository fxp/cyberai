// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 33/86



	mark_page_dirty_in_slot(vcpu->kvm, ghc->memslot, gpa_to_gfn(ghc->gpa));
}

void kvm_arch_vcpu_put(struct kvm_vcpu *vcpu)
{
	int idx;

	if (vcpu->preempted) {
		/*
		 * Assume protected guests are in-kernel.  Inefficient yielding
		 * due to false positives is preferable to never yielding due
		 * to false negatives.
		 */
		vcpu->arch.preempted_in_kernel = vcpu->arch.guest_state_protected ||
						 !kvm_x86_call(get_cpl_no_cache)(vcpu);

		/*
		 * Take the srcu lock as memslots will be accessed to check the gfn
		 * cache generation against the memslots generation.
		 */
		idx = srcu_read_lock(&vcpu->kvm->srcu);
		if (kvm_xen_msr_enabled(vcpu->kvm))
			kvm_xen_runstate_set_preempted(vcpu);
		else
			kvm_steal_time_set_preempted(vcpu);
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
	}

	kvm_x86_call(vcpu_put)(vcpu);
	vcpu->arch.last_host_tsc = rdtsc();
}

static int kvm_vcpu_ioctl_get_lapic(struct kvm_vcpu *vcpu,
				    struct kvm_lapic_state *s)
{
	if (vcpu->arch.apic->guest_apic_protected)
		return -EINVAL;

	kvm_x86_call(sync_pir_to_irr)(vcpu);

	return kvm_apic_get_state(vcpu, s);
}

static int kvm_vcpu_ioctl_set_lapic(struct kvm_vcpu *vcpu,
				    struct kvm_lapic_state *s)
{
	int r;

	if (vcpu->arch.apic->guest_apic_protected)
		return -EINVAL;

	r = kvm_apic_set_state(vcpu, s);
	if (r)
		return r;
	update_cr8_intercept(vcpu);

	return 0;
}

static int kvm_cpu_accept_dm_intr(struct kvm_vcpu *vcpu)
{
	/*
	 * We can accept userspace's request for interrupt injection
	 * as long as we have a place to store the interrupt number.
	 * The actual injection will happen when the CPU is able to
	 * deliver the interrupt.
	 */
	if (kvm_cpu_has_extint(vcpu))
		return false;

	/* Acknowledging ExtINT does not happen if LINT0 is masked.  */
	return (!lapic_in_kernel(vcpu) ||
		kvm_apic_accept_pic_intr(vcpu));
}

static int kvm_vcpu_ready_for_interrupt_injection(struct kvm_vcpu *vcpu)
{
	/*
	 * Do not cause an interrupt window exit if an exception
	 * is pending or an event needs reinjection; userspace
	 * might want to inject the interrupt manually using KVM_SET_REGS
	 * or KVM_SET_SREGS.  For that to work, we must be at an
	 * instruction boundary and with no events half-injected.
	 */
	return (kvm_arch_interrupt_allowed(vcpu) &&
		kvm_cpu_accept_dm_intr(vcpu) &&
		!kvm_event_needs_reinjection(vcpu) &&
		!kvm_is_exception_pending(vcpu));
}

static int kvm_vcpu_ioctl_interrupt(struct kvm_vcpu *vcpu,
				    struct kvm_interrupt *irq)
{
	if (irq->irq >= KVM_NR_INTERRUPTS)
		return -EINVAL;

	if (!irqchip_in_kernel(vcpu->kvm)) {
		kvm_queue_interrupt(vcpu, irq->irq, false);
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		return 0;
	}

	/*
	 * With in-kernel LAPIC, we only use this to inject EXTINT, so
	 * fail for in-kernel 8259.
	 */
	if (pic_in_kernel(vcpu->kvm))
		return -ENXIO;

	if (vcpu->arch.pending_external_vector != -1)
		return -EEXIST;

	vcpu->arch.pending_external_vector = irq->irq;
	kvm_make_request(KVM_REQ_EVENT, vcpu);
	return 0;
}

static int kvm_vcpu_ioctl_nmi(struct kvm_vcpu *vcpu)
{
	kvm_inject_nmi(vcpu);

	return 0;
}

static int vcpu_ioctl_tpr_access_reporting(struct kvm_vcpu *vcpu,
					   struct kvm_tpr_access_ctl *tac)
{
	if (tac->flags)
		return -EINVAL;
	vcpu->arch.tpr_access_reporting = !!tac->enabled;
	return 0;
}

static int kvm_vcpu_ioctl_x86_setup_mce(struct kvm_vcpu *vcpu,
					u64 mcg_cap)
{
	int r;
	unsigned bank_num = mcg_cap & 0xff, bank;

	r = -EINVAL;
	if (!bank_num || bank_num > KVM_MAX_MCE_BANKS)
		goto out;
	if (mcg_cap & ~(kvm_caps.supported_mce_cap | 0xff | 0xff0000))
		goto out;
	r = 0;
	vcpu->arch.mcg_cap = mcg_cap;
	/* Init IA32_MCG_CTL to all 1s */
	if (mcg_cap & MCG_CTL_P)
		vcpu->arch.mcg_ctl = ~(u64)0;
	/* Init IA32_MCi_CTL to all 1s, IA32_MCi_CTL2 to all 0s */
	for (bank = 0; bank < bank_num; bank++) {
		vcpu->arch.mce_banks[bank*4] = ~(u64)0;
		if (mcg_cap & MCG_CMCI_P)
			vcpu->arch.mci_ctl2_banks[bank] = 0;
	}

	kvm_apic_after_set_mcg_cap(vcpu);

	kvm_x86_call(setup_mce)(vcpu);
out:
	return r;
}

/*
 * Validate this is an UCNA (uncorrectable no action) error by checking the
 * MCG_STATUS and MCi_STATUS registers:
 * - none of the bits for Machine Check Exceptions are set
 * - both the VAL (valid) and UC (uncorrectable) bits are set
 * MCI_STATUS_PCC - Processor Context Corrupted
 * MCI_STATUS_S - Signaled as a Machine Check Exception
 * MCI_STATUS_AR - Software recoverable Action Required
 */
static bool is_ucna(struct kvm_x86_mce *mce)
{
	return	!mce->mcg_status &&
		!(mce->status & (MCI_STATUS_PCC | MCI_STATUS_S | MCI_STATUS_AR)) &&
		(mce->status & MCI_STATUS_VAL) &&
		(mce->status & MCI_STATUS_UC);
}

static int kvm_vcpu_x86_set_ucna(struct kvm_vcpu *vcpu, struct kvm_x86_mce *mce, u64* banks)
{
	u64 mcg_cap = vcpu->arch.mcg_cap;

	banks[1] = mce->status;
	banks[2] = mce->addr;
	banks[3] = mce->misc;
	vcpu->arch.mcg_status = mce->mcg_status;

	if (!(mcg_cap & MCG_CMCI_P) ||
	    !(vcpu->arch.mci_ctl2_banks[mce->bank] & MCI_CTL2_CMCI_EN))
		return 0;