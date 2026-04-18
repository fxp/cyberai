// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 84/86



int kvm_spec_ctrl_test_value(u64 value)
{
	/*
	 * test that setting IA32_SPEC_CTRL to given value
	 * is allowed by the host processor
	 */

	u64 saved_value;
	unsigned long flags;
	int ret = 0;

	local_irq_save(flags);

	if (rdmsrq_safe(MSR_IA32_SPEC_CTRL, &saved_value))
		ret = 1;
	else if (wrmsrq_safe(MSR_IA32_SPEC_CTRL, value))
		ret = 1;
	else
		wrmsrq(MSR_IA32_SPEC_CTRL, saved_value);

	local_irq_restore(flags);

	return ret;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_spec_ctrl_test_value);

void kvm_fixup_and_inject_pf_error(struct kvm_vcpu *vcpu, gva_t gva, u16 error_code)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;
	struct x86_exception fault;
	u64 access = error_code &
		(PFERR_WRITE_MASK | PFERR_FETCH_MASK | PFERR_USER_MASK);

	if (!(error_code & PFERR_PRESENT_MASK) ||
	    mmu->gva_to_gpa(vcpu, mmu, gva, access, &fault) != INVALID_GPA) {
		/*
		 * If vcpu->arch.walk_mmu->gva_to_gpa succeeded, the page
		 * tables probably do not match the TLB.  Just proceed
		 * with the error code that the processor gave.
		 */
		fault.vector = PF_VECTOR;
		fault.error_code_valid = true;
		fault.error_code = error_code;
		fault.nested_page_fault = false;
		fault.address = gva;
		fault.async_page_fault = false;
	}
	vcpu->arch.walk_mmu->inject_page_fault(vcpu, &fault);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_fixup_and_inject_pf_error);

/*
 * Handles kvm_read/write_guest_virt*() result and either injects #PF or returns
 * KVM_EXIT_INTERNAL_ERROR for cases not currently handled by KVM. Return value
 * indicates whether exit to userspace is needed.
 */
int kvm_handle_memory_failure(struct kvm_vcpu *vcpu, int r,
			      struct x86_exception *e)
{
	if (r == X86EMUL_PROPAGATE_FAULT) {
		if (KVM_BUG_ON(!e, vcpu->kvm))
			return -EIO;

		kvm_inject_emulated_page_fault(vcpu, e);
		return 1;
	}

	/*
	 * In case kvm_read/write_guest_virt*() failed with X86EMUL_IO_NEEDED
	 * while handling a VMX instruction KVM could've handled the request
	 * correctly by exiting to userspace and performing I/O but there
	 * doesn't seem to be a real use-case behind such requests, just return
	 * KVM_EXIT_INTERNAL_ERROR for now.
	 */
	kvm_prepare_emulation_failure_exit(vcpu);

	return 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_handle_memory_failure);

int kvm_handle_invpcid(struct kvm_vcpu *vcpu, unsigned long type, gva_t gva)
{
	bool pcid_enabled;
	struct x86_exception e;
	struct {
		u64 pcid;
		u64 gla;
	} operand;
	int r;

	r = kvm_read_guest_virt(vcpu, gva, &operand, sizeof(operand), &e);
	if (r != X86EMUL_CONTINUE)
		return kvm_handle_memory_failure(vcpu, r, &e);

	if (operand.pcid >> 12 != 0) {
		kvm_inject_gp(vcpu, 0);
		return 1;
	}

	pcid_enabled = kvm_is_cr4_bit_set(vcpu, X86_CR4_PCIDE);

	switch (type) {
	case INVPCID_TYPE_INDIV_ADDR:
		/*
		 * LAM doesn't apply to addresses that are inputs to TLB
		 * invalidation.
		 */
		if ((!pcid_enabled && (operand.pcid != 0)) ||
		    is_noncanonical_invlpg_address(operand.gla, vcpu)) {
			kvm_inject_gp(vcpu, 0);
			return 1;
		}
		kvm_mmu_invpcid_gva(vcpu, operand.gla, operand.pcid);
		return kvm_skip_emulated_instruction(vcpu);

	case INVPCID_TYPE_SINGLE_CTXT:
		if (!pcid_enabled && (operand.pcid != 0)) {
			kvm_inject_gp(vcpu, 0);
			return 1;
		}

		/*
		 * When ERAPS is supported, invalidating a specific PCID clears
		 * the RAP (Return Address Predicator).
		 */
		if (guest_cpu_cap_has(vcpu, X86_FEATURE_ERAPS))
			kvm_register_is_dirty(vcpu, VCPU_EXREG_ERAPS);

		kvm_invalidate_pcid(vcpu, operand.pcid);
		return kvm_skip_emulated_instruction(vcpu);

	case INVPCID_TYPE_ALL_NON_GLOBAL:
		/*
		 * Currently, KVM doesn't mark global entries in the shadow
		 * page tables, so a non-global flush just degenerates to a
		 * global flush. If needed, we could optimize this later by
		 * keeping track of global entries in shadow page tables.
		 */

		fallthrough;
	case INVPCID_TYPE_ALL_INCL_GLOBAL:
		/*
		 * Don't bother marking VCPU_EXREG_ERAPS dirty, SVM will take
		 * care of doing so when emulating the full guest TLB flush
		 * (the RAP is cleared on all implicit TLB flushes).
		 */
		kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
		return kvm_skip_emulated_instruction(vcpu);

	default:
		kvm_inject_gp(vcpu, 0);
		return 1;
	}
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_handle_invpcid);

static int complete_sev_es_emulated_mmio(struct kvm_vcpu *vcpu)
{
	struct kvm_run *run = vcpu->run;
	struct kvm_mmio_fragment *frag;
	unsigned int len;

	if (KVM_BUG_ON(!vcpu->mmio_needed, vcpu->kvm))
		return -EIO;

	/* Complete previous fragment */
	frag = &vcpu->mmio_fragments[vcpu->mmio_cur_fragment];
	len = min(8u, frag->len);
	if (!vcpu->mmio_is_write)
		memcpy(frag->data, run->mmio.data, len);

	if (frag->len <= 8) {
		/* Switch to the next fragment. */
		frag++;
		vcpu->mmio_cur_fragment++;
	} else {
		/* Go forward to the next mmio piece. */
		frag->data += len;
		frag->gpa += len;
		frag->len -= len;
	}

	if (vcpu->mmio_cur_fragment >= vcpu->mmio_nr_fragments) {
		vcpu->mmio_needed = 0;