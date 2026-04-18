// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 45/56



	/*
	 * Explicitly grab the memslot using KVM's internal slot ID to ensure
	 * KVM doesn't unintentionally grab a userspace memslot.  It _should_
	 * be impossible for userspace to create a memslot for the APIC when
	 * APICv is enabled, but paranoia won't hurt in this case.
	 */
	slot = id_to_memslot(slots, APIC_ACCESS_PAGE_PRIVATE_MEMSLOT);
	if (!slot || slot->flags & KVM_MEMSLOT_INVALID)
		return;

	/*
	 * Ensure that the mmu_notifier sequence count is read before KVM
	 * retrieves the pfn from the primary MMU.  Note, the memslot is
	 * protected by SRCU, not the mmu_notifier.  Pairs with the smp_wmb()
	 * in kvm_mmu_invalidate_end().
	 */
	mmu_seq = kvm->mmu_invalidate_seq;
	smp_rmb();

	/*
	 * No need to retry if the memslot does not exist or is invalid.  KVM
	 * controls the APIC-access page memslot, and only deletes the memslot
	 * if APICv is permanently inhibited, i.e. the memslot won't reappear.
	 */
	pfn = __kvm_faultin_pfn(slot, gfn, FOLL_WRITE, &writable, &refcounted_page);
	if (is_error_noslot_pfn(pfn))
		return;

	read_lock(&vcpu->kvm->mmu_lock);
	if (mmu_invalidate_retry_gfn(kvm, mmu_seq, gfn))
		kvm_make_request(KVM_REQ_APIC_PAGE_RELOAD, vcpu);
	else
		vmcs_write64(APIC_ACCESS_ADDR, pfn_to_hpa(pfn));

	/*
	 * Do not pin the APIC access page in memory so that it can be freely
	 * migrated, the MMU notifier will call us again if it is migrated or
	 * swapped out.  KVM backs the memslot with anonymous memory, the pfn
	 * should always point at a refcounted page (if the pfn is valid).
	 */
	if (!WARN_ON_ONCE(!refcounted_page))
		kvm_release_page_clean(refcounted_page);

	/*
	 * No need for a manual TLB flush at this point, KVM has already done a
	 * flush if there were SPTEs pointing at the previous page.
	 */
	read_unlock(&vcpu->kvm->mmu_lock);
}

void vmx_hwapic_isr_update(struct kvm_vcpu *vcpu, int max_isr)
{
	u16 status;
	u8 old;

	if (max_isr == -1)
		max_isr = 0;

	/*
	 * Always update SVI in vmcs01, as SVI is only relevant for L2 if and
	 * only if Virtual Interrupt Delivery is enabled in vmcs12, and if VID
	 * is enabled then L2 EOIs affect L2's vAPIC, not L1's vAPIC.
	 */
	guard(vmx_vmcs01)(vcpu);

	status = vmcs_read16(GUEST_INTR_STATUS);
	old = status >> 8;
	if (max_isr != old) {
		status &= 0xff;
		status |= max_isr << 8;
		vmcs_write16(GUEST_INTR_STATUS, status);
	}
}

static void vmx_set_rvi(int vector)
{
	u16 status;
	u8 old;

	if (vector == -1)
		vector = 0;

	status = vmcs_read16(GUEST_INTR_STATUS);
	old = (u8)status & 0xff;
	if ((u8)vector != old) {
		status &= ~0xff;
		status |= (u8)vector;
		vmcs_write16(GUEST_INTR_STATUS, status);
	}
}

int vmx_sync_pir_to_irr(struct kvm_vcpu *vcpu)
{
	struct vcpu_vt *vt = to_vt(vcpu);
	int max_irr;
	bool got_posted_interrupt;

	if (KVM_BUG_ON(!enable_apicv, vcpu->kvm))
		return -EIO;

	if (pi_test_on(&vt->pi_desc)) {
		pi_clear_on(&vt->pi_desc);
		/*
		 * IOMMU can write to PID.ON, so the barrier matters even on UP.
		 * But on x86 this is just a compiler barrier anyway.
		 */
		smp_mb__after_atomic();
		got_posted_interrupt =
			kvm_apic_update_irr(vcpu, vt->pi_desc.pir, &max_irr);
	} else {
		max_irr = kvm_lapic_find_highest_irr(vcpu);
		got_posted_interrupt = false;
	}

	/*
	 * Newly recognized interrupts are injected via either virtual interrupt
	 * delivery (RVI) or KVM_REQ_EVENT.  Virtual interrupt delivery is
	 * disabled in two cases:
	 *
	 * 1) If L2 is running and the vCPU has a new pending interrupt.  If L1
	 * wants to exit on interrupts, KVM_REQ_EVENT is needed to synthesize a
	 * VM-Exit to L1.  If L1 doesn't want to exit, the interrupt is injected
	 * into L2, but KVM doesn't use virtual interrupt delivery to inject
	 * interrupts into L2, and so KVM_REQ_EVENT is again needed.
	 *
	 * 2) If APICv is disabled for this vCPU, assigned devices may still
	 * attempt to post interrupts.  The posted interrupt vector will cause
	 * a VM-Exit and the subsequent entry will call sync_pir_to_irr.
	 */
	if (!is_guest_mode(vcpu) && kvm_vcpu_apicv_active(vcpu))
		vmx_set_rvi(max_irr);
	else if (got_posted_interrupt)
		kvm_make_request(KVM_REQ_EVENT, vcpu);

	return max_irr;
}

void vmx_load_eoi_exitmap(struct kvm_vcpu *vcpu, u64 *eoi_exit_bitmap)
{
	if (!kvm_vcpu_apicv_active(vcpu))
		return;

	vmcs_write64(EOI_EXIT_BITMAP0, eoi_exit_bitmap[0]);
	vmcs_write64(EOI_EXIT_BITMAP1, eoi_exit_bitmap[1]);
	vmcs_write64(EOI_EXIT_BITMAP2, eoi_exit_bitmap[2]);
	vmcs_write64(EOI_EXIT_BITMAP3, eoi_exit_bitmap[3]);
}

void vmx_do_interrupt_irqoff(unsigned long entry);
void vmx_do_nmi_irqoff(void);