// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 27/36



	svm_flush_tlb_asid(vcpu);
}

static void svm_flush_tlb_all(struct kvm_vcpu *vcpu)
{
	/*
	 * When running on Hyper-V with EnlightenedNptTlb enabled, remote TLB
	 * flushes should be routed to hv_flush_remote_tlbs() without requesting
	 * a "regular" remote flush.  Reaching this point means either there's
	 * a KVM bug or a prior hv_flush_remote_tlbs() call failed, both of
	 * which might be fatal to the guest.  Yell, but try to recover.
	 */
	if (WARN_ON_ONCE(svm_hv_is_enlightened_tlb_enabled(vcpu)))
		hv_flush_remote_tlbs(vcpu->kvm);

	svm_flush_tlb_asid(vcpu);
}

static void svm_flush_tlb_gva(struct kvm_vcpu *vcpu, gva_t gva)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	invlpga(gva, svm->vmcb->control.asid);
}

static void svm_flush_tlb_guest(struct kvm_vcpu *vcpu)
{
	kvm_register_mark_dirty(vcpu, VCPU_EXREG_ERAPS);

	svm_flush_tlb_asid(vcpu);
}

static inline void sync_cr8_to_lapic(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (nested_svm_virtualize_tpr(vcpu))
		return;

	if (!svm_is_intercept(svm, INTERCEPT_CR8_WRITE)) {
		int cr8 = svm->vmcb->control.int_ctl & V_TPR_MASK;
		kvm_set_cr8(vcpu, cr8);
	}
}

static inline void sync_lapic_to_cr8(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u64 cr8;

	if (nested_svm_virtualize_tpr(vcpu))
		return;

	cr8 = kvm_get_cr8(vcpu);
	svm->vmcb->control.int_ctl &= ~V_TPR_MASK;
	svm->vmcb->control.int_ctl |= cr8 & V_TPR_MASK;
}

static void svm_complete_soft_interrupt(struct kvm_vcpu *vcpu, u8 vector,
					int type)
{
	bool is_exception = (type == SVM_EXITINTINFO_TYPE_EXEPT);
	bool is_soft = (type == SVM_EXITINTINFO_TYPE_SOFT);
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * Initialize the soft int fields *before* reading them below if KVM
	 * aborted entry to the guest with a nested VMRUN pending.  To ensure
	 * KVM uses up-to-date values for RIP and CS base across save/restore,
	 * regardless of restore order, KVM waits to set the soft int fields
	 * until VMRUN is imminent.  But when canceling injection, KVM requeues
	 * the soft int and will reinject it via the standard injection flow,
	 * and so KVM needs to grab the state from the pending nested VMRUN.
	 */
	if (is_guest_mode(vcpu) && vcpu->arch.nested_run_pending)
		svm_set_nested_run_soft_int_state(vcpu);

	/*
	 * If NRIPS is enabled, KVM must snapshot the pre-VMRUN next_rip that's
	 * associated with the original soft exception/interrupt.  next_rip is
	 * cleared on all exits that can occur while vectoring an event, so KVM
	 * needs to manually set next_rip for re-injection.  Unlike the !nrips
	 * case below, this needs to be done if and only if KVM is re-injecting
	 * the same event, i.e. if the event is a soft exception/interrupt,
	 * otherwise next_rip is unused on VMRUN.
	 */
	if (nrips && (is_soft || (is_exception && kvm_exception_is_soft(vector))) &&
	    kvm_is_linear_rip(vcpu, svm->soft_int_old_rip + svm->soft_int_csbase))
		svm->vmcb->control.next_rip = svm->soft_int_next_rip;
	/*
	 * If NRIPS isn't enabled, KVM must manually advance RIP prior to
	 * injecting the soft exception/interrupt.  That advancement needs to
	 * be unwound if vectoring didn't complete.  Note, the new event may
	 * not be the injected event, e.g. if KVM injected an INTn, the INTn
	 * hit a #NP in the guest, and the #NP encountered a #PF, the #NP will
	 * be the reported vectored event, but RIP still needs to be unwound.
	 */
	else if (!nrips && (is_soft || is_exception) &&
		 kvm_is_linear_rip(vcpu, svm->soft_int_next_rip + svm->soft_int_csbase))
		kvm_rip_write(vcpu, svm->soft_int_old_rip);
}

static void svm_complete_interrupts(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u8 vector;
	int type;
	u32 exitintinfo = svm->vmcb->control.exit_int_info;
	bool nmi_l1_to_l2 = svm->nmi_l1_to_l2;
	bool soft_int_injected = svm->soft_int_injected;

	svm->nmi_l1_to_l2 = false;
	svm->soft_int_injected = false;

	/*
	 * If we've made progress since setting awaiting_iret_completion, we've
	 * executed an IRET and can allow NMI injection.
	 */
	if (svm->awaiting_iret_completion &&
	    kvm_rip_read(vcpu) != svm->nmi_iret_rip) {
		svm->awaiting_iret_completion = false;
		svm->nmi_masked = false;
		kvm_make_request(KVM_REQ_EVENT, vcpu);
	}

	vcpu->arch.nmi_injected = false;
	kvm_clear_exception_queue(vcpu);
	kvm_clear_interrupt_queue(vcpu);

	if (!(exitintinfo & SVM_EXITINTINFO_VALID))
		return;

	kvm_make_request(KVM_REQ_EVENT, vcpu);

	vector = exitintinfo & SVM_EXITINTINFO_VEC_MASK;
	type = exitintinfo & SVM_EXITINTINFO_TYPE_MASK;

	if (soft_int_injected)
		svm_complete_soft_interrupt(vcpu, vector, type);

	switch (type) {
	case SVM_EXITINTINFO_TYPE_NMI:
		vcpu->arch.nmi_injected = true;
		svm->nmi_l1_to_l2 = nmi_l1_to_l2;
		break;
	case SVM_EXITINTINFO_TYPE_EXEPT: {
		u32 error_code = 0;

		/*
		 * Never re-inject a #VC exception.
		 */
		if (vector == X86_TRAP_VC)
			break;

		if (exitintinfo & SVM_EXITINTINFO_VALID_ERR)
			error_code = svm->vmcb->control.exit_int_info_err;