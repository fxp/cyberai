// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 12/36



static void new_asid(struct vcpu_svm *svm, struct svm_cpu_data *sd)
{
	if (sd->next_asid > sd->max_asid) {
		++sd->asid_generation;
		sd->next_asid = sd->min_asid;
		svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ALL_ASID;
		vmcb_mark_dirty(svm->vmcb, VMCB_ASID);
	}

	svm->current_vmcb->asid_generation = sd->asid_generation;
	svm->asid = sd->next_asid++;
}

static void svm_set_dr6(struct kvm_vcpu *vcpu, unsigned long value)
{
	struct vmcb *vmcb = to_svm(vcpu)->vmcb;

	if (vcpu->arch.guest_state_protected)
		return;

	if (unlikely(value != vmcb->save.dr6)) {
		vmcb->save.dr6 = value;
		vmcb_mark_dirty(vmcb, VMCB_DR);
	}
}

static void svm_sync_dirty_debug_regs(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (WARN_ON_ONCE(is_sev_es_guest(vcpu)))
		return;

	get_debugreg(vcpu->arch.db[0], 0);
	get_debugreg(vcpu->arch.db[1], 1);
	get_debugreg(vcpu->arch.db[2], 2);
	get_debugreg(vcpu->arch.db[3], 3);
	/*
	 * We cannot reset svm->vmcb->save.dr6 to DR6_ACTIVE_LOW here,
	 * because db_interception might need it.  We can do it before vmentry.
	 */
	vcpu->arch.dr6 = svm->vmcb->save.dr6;
	vcpu->arch.dr7 = svm->vmcb->save.dr7;
	vcpu->arch.switch_db_regs &= ~KVM_DEBUGREG_WONT_EXIT;
	set_dr_intercepts(svm);
}

static void svm_set_dr7(struct kvm_vcpu *vcpu, unsigned long value)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (vcpu->arch.guest_state_protected)
		return;

	svm->vmcb->save.dr7 = value;
	vmcb_mark_dirty(svm->vmcb, VMCB_DR);
}

static int pf_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	u64 fault_address = svm->vmcb->control.exit_info_2;
	u64 error_code = svm->vmcb->control.exit_info_1;

	return kvm_handle_page_fault(vcpu, error_code, fault_address,
			static_cpu_has(X86_FEATURE_DECODEASSISTS) ?
			svm->vmcb->control.insn_bytes : NULL,
			svm->vmcb->control.insn_len);
}

static int svm_check_emulate_instruction(struct kvm_vcpu *vcpu, int emul_type,
					 void *insn, int insn_len);

static int npf_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int rc;

	u64 error_code = svm->vmcb->control.exit_info_1;
	gpa_t gpa = svm->vmcb->control.exit_info_2;

	/*
	 * WARN if hardware generates a fault with an error code that collides
	 * with KVM-defined sythentic flags.  Clear the flags and continue on,
	 * i.e. don't terminate the VM, as KVM can't possibly be relying on a
	 * flag that KVM doesn't know about.
	 */
	if (WARN_ON_ONCE(error_code & PFERR_SYNTHETIC_MASK))
		error_code &= ~PFERR_SYNTHETIC_MASK;

	/*
	 * Expedite fast MMIO kicks if the next RIP is known and KVM is allowed
	 * emulate a page fault, e.g. skipping the current instruction is wrong
	 * if the #NPF occurred while vectoring an event.
	 */
	if ((error_code & PFERR_RSVD_MASK) && !is_guest_mode(vcpu)) {
		const int emul_type = EMULTYPE_PF | EMULTYPE_NO_DECODE;

		if (svm_check_emulate_instruction(vcpu, emul_type, NULL, 0))
			return 1;

		if (nrips && svm->vmcb->control.next_rip &&
		    !kvm_io_bus_write(vcpu, KVM_FAST_MMIO_BUS, gpa, 0, NULL)) {
			trace_kvm_fast_mmio(gpa);
			return kvm_skip_emulated_instruction(vcpu);
		}
	}

	if (is_sev_snp_guest(vcpu) && (error_code & PFERR_GUEST_ENC_MASK))
		error_code |= PFERR_PRIVATE_ACCESS;

	trace_kvm_page_fault(vcpu, gpa, error_code);
	rc = kvm_mmu_page_fault(vcpu, gpa, error_code,
				static_cpu_has(X86_FEATURE_DECODEASSISTS) ?
				svm->vmcb->control.insn_bytes : NULL,
				svm->vmcb->control.insn_len);

	if (rc > 0 && error_code & PFERR_GUEST_RMP_MASK)
		sev_handle_rmp_fault(vcpu, gpa, error_code);

	return rc;
}

static int db_interception(struct kvm_vcpu *vcpu)
{
	struct kvm_run *kvm_run = vcpu->run;
	struct vcpu_svm *svm = to_svm(vcpu);

	if (!(vcpu->guest_debug &
	      (KVM_GUESTDBG_SINGLESTEP | KVM_GUESTDBG_USE_HW_BP)) &&
		!svm->nmi_singlestep) {
		u32 payload = svm->vmcb->save.dr6 ^ DR6_ACTIVE_LOW;
		kvm_queue_exception_p(vcpu, DB_VECTOR, payload);
		return 1;
	}

	if (svm->nmi_singlestep) {
		disable_nmi_singlestep(svm);
		/* Make sure we check for pending NMIs upon entry */
		kvm_make_request(KVM_REQ_EVENT, vcpu);
	}

	if (vcpu->guest_debug &
	    (KVM_GUESTDBG_SINGLESTEP | KVM_GUESTDBG_USE_HW_BP)) {
		kvm_run->exit_reason = KVM_EXIT_DEBUG;
		kvm_run->debug.arch.dr6 = svm->vmcb->save.dr6;
		kvm_run->debug.arch.dr7 = svm->vmcb->save.dr7;
		kvm_run->debug.arch.pc =
			svm->vmcb->save.cs.base + svm->vmcb->save.rip;
		kvm_run->debug.arch.exception = DB_VECTOR;
		return 0;
	}

	return 1;
}

static int bp_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct kvm_run *kvm_run = vcpu->run;

	kvm_run->exit_reason = KVM_EXIT_DEBUG;
	kvm_run->debug.arch.pc = svm->vmcb->save.cs.base + svm->vmcb->save.rip;
	kvm_run->debug.arch.exception = BP_VECTOR;
	return 0;
}

static int ud_interception(struct kvm_vcpu *vcpu)
{
	return handle_ud(vcpu);
}

static int ac_interception(struct kvm_vcpu *vcpu)
{
	kvm_queue_exception_e(vcpu, AC_VECTOR, 0);
	return 1;
}