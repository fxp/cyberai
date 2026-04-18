// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 11/36



static void svm_set_idt(struct kvm_vcpu *vcpu, struct desc_ptr *dt)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->vmcb->save.idtr.limit = dt->size;
	svm->vmcb->save.idtr.base = dt->address ;
	vmcb_mark_dirty(svm->vmcb, VMCB_DT);
}

static void svm_get_gdt(struct kvm_vcpu *vcpu, struct desc_ptr *dt)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	dt->size = svm->vmcb->save.gdtr.limit;
	dt->address = svm->vmcb->save.gdtr.base;
}

static void svm_set_gdt(struct kvm_vcpu *vcpu, struct desc_ptr *dt)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->vmcb->save.gdtr.limit = dt->size;
	svm->vmcb->save.gdtr.base = dt->address ;
	vmcb_mark_dirty(svm->vmcb, VMCB_DT);
}

static void sev_post_set_cr3(struct kvm_vcpu *vcpu, unsigned long cr3)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * For guests that don't set guest_state_protected, the cr3 update is
	 * handled via kvm_mmu_load() while entering the guest. For guests
	 * that do (SEV-ES/SEV-SNP), the cr3 update needs to be written to
	 * VMCB save area now, since the save area will become the initial
	 * contents of the VMSA, and future VMCB save area updates won't be
	 * seen.
	 */
	if (is_sev_es_guest(vcpu)) {
		svm->vmcb->save.cr3 = cr3;
		vmcb_mark_dirty(svm->vmcb, VMCB_CR);
	}
}

static bool svm_is_valid_cr0(struct kvm_vcpu *vcpu, unsigned long cr0)
{
	return true;
}

void svm_set_cr0(struct kvm_vcpu *vcpu, unsigned long cr0)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u64 hcr0 = cr0;
	bool old_paging = is_paging(vcpu);

#ifdef CONFIG_X86_64
	if (vcpu->arch.efer & EFER_LME) {
		if (!is_paging(vcpu) && (cr0 & X86_CR0_PG)) {
			vcpu->arch.efer |= EFER_LMA;
			if (!vcpu->arch.guest_state_protected)
				svm->vmcb->save.efer |= EFER_LMA | EFER_LME;
		}

		if (is_paging(vcpu) && !(cr0 & X86_CR0_PG)) {
			vcpu->arch.efer &= ~EFER_LMA;
			if (!vcpu->arch.guest_state_protected)
				svm->vmcb->save.efer &= ~(EFER_LMA | EFER_LME);
		}
	}
#endif
	vcpu->arch.cr0 = cr0;

	if (!npt_enabled) {
		hcr0 |= X86_CR0_PG | X86_CR0_WP;
		if (old_paging != is_paging(vcpu))
			svm_set_cr4(vcpu, kvm_read_cr4(vcpu));
	}

	/*
	 * re-enable caching here because the QEMU bios
	 * does not do it - this results in some delay at
	 * reboot
	 */
	if (kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_CD_NW_CLEARED))
		hcr0 &= ~(X86_CR0_CD | X86_CR0_NW);

	svm->vmcb->save.cr0 = hcr0;
	vmcb_mark_dirty(svm->vmcb, VMCB_CR);

	/*
	 * SEV-ES guests must always keep the CR intercepts cleared. CR
	 * tracking is done using the CR write traps.
	 */
	if (is_sev_es_guest(vcpu))
		return;

	if (hcr0 == cr0) {
		/* Selective CR0 write remains on.  */
		svm_clr_intercept(svm, INTERCEPT_CR0_READ);
		svm_clr_intercept(svm, INTERCEPT_CR0_WRITE);
	} else {
		svm_set_intercept(svm, INTERCEPT_CR0_READ);
		svm_set_intercept(svm, INTERCEPT_CR0_WRITE);
	}
}

static bool svm_is_valid_cr4(struct kvm_vcpu *vcpu, unsigned long cr4)
{
	return true;
}

void svm_set_cr4(struct kvm_vcpu *vcpu, unsigned long cr4)
{
	unsigned long host_cr4_mce = cr4_read_shadow() & X86_CR4_MCE;
	unsigned long old_cr4 = vcpu->arch.cr4;

	vcpu->arch.cr4 = cr4;
	if (!npt_enabled) {
		cr4 |= X86_CR4_PAE;

		if (!is_paging(vcpu))
			cr4 &= ~(X86_CR4_SMEP | X86_CR4_SMAP | X86_CR4_PKE);
	}
	cr4 |= host_cr4_mce;
	to_svm(vcpu)->vmcb->save.cr4 = cr4;
	vmcb_mark_dirty(to_svm(vcpu)->vmcb, VMCB_CR);

	if ((cr4 ^ old_cr4) & (X86_CR4_OSXSAVE | X86_CR4_PKE))
		vcpu->arch.cpuid_dynamic_bits_dirty = true;
}

static void svm_set_segment(struct kvm_vcpu *vcpu,
			    struct kvm_segment *var, int seg)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb_seg *s = svm_seg(vcpu, seg);

	s->base = var->base;
	s->limit = var->limit;
	s->selector = var->selector;
	s->attrib = (var->type & SVM_SELECTOR_TYPE_MASK);
	s->attrib |= (var->s & 1) << SVM_SELECTOR_S_SHIFT;
	s->attrib |= (var->dpl & 3) << SVM_SELECTOR_DPL_SHIFT;
	s->attrib |= ((var->present & 1) && !var->unusable) << SVM_SELECTOR_P_SHIFT;
	s->attrib |= (var->avl & 1) << SVM_SELECTOR_AVL_SHIFT;
	s->attrib |= (var->l & 1) << SVM_SELECTOR_L_SHIFT;
	s->attrib |= (var->db & 1) << SVM_SELECTOR_DB_SHIFT;
	s->attrib |= (var->g & 1) << SVM_SELECTOR_G_SHIFT;

	/*
	 * This is always accurate, except if SYSRET returned to a segment
	 * with SS.DPL != 3.  Intel does not have this quirk, and always
	 * forces SS.DPL to 3 on sysret, so we ignore that case; fixing it
	 * would entail passing the CPL to userspace and back.
	 */
	if (seg == VCPU_SREG_SS)
		/* This is symmetric with svm_get_segment() */
		svm->vmcb->save.cpl = (var->dpl & 3);

	vmcb_mark_dirty(svm->vmcb, VMCB_SEG);
}

static void svm_update_exception_bitmap(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	clr_exception_intercept(svm, BP_VECTOR);

	if (vcpu->guest_debug & KVM_GUESTDBG_ENABLE) {
		if (vcpu->guest_debug & KVM_GUESTDBG_USE_SW_BP)
			set_exception_intercept(svm, BP_VECTOR);
	}
}