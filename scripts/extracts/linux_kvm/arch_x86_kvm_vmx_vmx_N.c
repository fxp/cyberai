// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 46/56



static void handle_nm_fault_irqoff(struct kvm_vcpu *vcpu)
{
	/*
	 * Save xfd_err to guest_fpu before interrupt is enabled, so the
	 * MSR value is not clobbered by the host activity before the guest
	 * has chance to consume it.
	 *
	 * Update the guest's XFD_ERR if and only if XFD is enabled, as the #NM
	 * interception may have been caused by L1 interception.  Per the SDM,
	 * XFD_ERR is not modified for non-XFD #NM, i.e. if CR0.TS=1.
	 *
	 * Note, XFD_ERR is updated _before_ the #NM interception check, i.e.
	 * unlike CR2 and DR6, the value is not a payload that is attached to
	 * the #NM exception.
	 */
	if (is_xfd_nm_fault(vcpu))
		rdmsrq(MSR_IA32_XFD_ERR, vcpu->arch.guest_fpu.xfd_err);
}

static void handle_exception_irqoff(struct kvm_vcpu *vcpu, u32 intr_info)
{
	/* if exit due to PF check for async PF */
	if (is_page_fault(intr_info))
		vcpu->arch.apf.host_apf_flags = kvm_read_and_reset_apf_flags();
	/* if exit due to NM, handle before interrupts are enabled */
	else if (is_nm_fault(intr_info))
		handle_nm_fault_irqoff(vcpu);
	/* Handle machine checks before interrupts are enabled */
	else if (is_machine_check(intr_info))
		kvm_machine_check();
}

static void handle_external_interrupt_irqoff(struct kvm_vcpu *vcpu,
					     u32 intr_info)
{
	unsigned int vector = intr_info & INTR_INFO_VECTOR_MASK;

	if (KVM_BUG(!is_external_intr(intr_info), vcpu->kvm,
	    "unexpected VM-Exit interrupt info: 0x%x", intr_info))
		return;

	/*
	 * Invoke the kernel's IRQ handler for the vector.  Use the FRED path
	 * when it's available even if FRED isn't fully enabled, e.g. even if
	 * FRED isn't supported in hardware, in order to avoid the indirect
	 * CALL in the non-FRED path.
	 */
	kvm_before_interrupt(vcpu, KVM_HANDLING_IRQ);
	if (IS_ENABLED(CONFIG_X86_FRED))
		fred_entry_from_kvm(EVENT_TYPE_EXTINT, vector);
	else
		vmx_do_interrupt_irqoff(gate_offset((gate_desc *)host_idt_base + vector));
	kvm_after_interrupt(vcpu);

	vcpu->arch.at_instruction_boundary = true;
}

void vmx_handle_exit_irqoff(struct kvm_vcpu *vcpu)
{
	if (to_vt(vcpu)->emulation_required)
		return;

	switch (vmx_get_exit_reason(vcpu).basic) {
	case EXIT_REASON_EXTERNAL_INTERRUPT:
		handle_external_interrupt_irqoff(vcpu, vmx_get_intr_info(vcpu));
		break;
	case EXIT_REASON_EXCEPTION_NMI:
		handle_exception_irqoff(vcpu, vmx_get_intr_info(vcpu));
		break;
	case EXIT_REASON_MCE_DURING_VMENTRY:
		kvm_machine_check();
		break;
	default:
		break;
	}
}

/*
 * The kvm parameter can be NULL (module initialization, or invocation before
 * VM creation). Be sure to check the kvm parameter before using it.
 */
bool vmx_has_emulated_msr(struct kvm *kvm, u32 index)
{
	switch (index) {
	case MSR_IA32_SMBASE:
		if (!IS_ENABLED(CONFIG_KVM_SMM))
			return false;
		/*
		 * We cannot do SMM unless we can run the guest in big
		 * real mode.
		 */
		return enable_unrestricted_guest || emulate_invalid_guest_state;
	case KVM_FIRST_EMULATED_VMX_MSR ... KVM_LAST_EMULATED_VMX_MSR:
		return nested;
	case MSR_AMD64_VIRT_SPEC_CTRL:
	case MSR_AMD64_TSC_RATIO:
		/* This is AMD only.  */
		return false;
	default:
		return true;
	}
}

static void vmx_recover_nmi_blocking(struct vcpu_vmx *vmx)
{
	u32 exit_intr_info;
	bool unblock_nmi;
	u8 vector;
	bool idtv_info_valid;

	idtv_info_valid = vmx->idt_vectoring_info & VECTORING_INFO_VALID_MASK;

	if (enable_vnmi) {
		if (vmx->loaded_vmcs->nmi_known_unmasked)
			return;

		exit_intr_info = vmx_get_intr_info(&vmx->vcpu);
		unblock_nmi = (exit_intr_info & INTR_INFO_UNBLOCK_NMI) != 0;
		vector = exit_intr_info & INTR_INFO_VECTOR_MASK;
		/*
		 * SDM 3: 27.7.1.2 (September 2008)
		 * Re-set bit "block by NMI" before VM entry if vmexit caused by
		 * a guest IRET fault.
		 * SDM 3: 23.2.2 (September 2008)
		 * Bit 12 is undefined in any of the following cases:
		 *  If the VM exit sets the valid bit in the IDT-vectoring
		 *   information field.
		 *  If the VM exit is due to a double fault.
		 */
		if ((exit_intr_info & INTR_INFO_VALID_MASK) && unblock_nmi &&
		    vector != DF_VECTOR && !idtv_info_valid)
			vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
				      GUEST_INTR_STATE_NMI);
		else
			vmx->loaded_vmcs->nmi_known_unmasked =
				!(vmcs_read32(GUEST_INTERRUPTIBILITY_INFO)
				  & GUEST_INTR_STATE_NMI);
	} else if (unlikely(vmx->loaded_vmcs->soft_vnmi_blocked))
		vmx->loaded_vmcs->vnmi_blocked_time +=
			ktime_to_ns(ktime_sub(ktime_get(),
					      vmx->loaded_vmcs->entry_time));
}

static void __vmx_complete_interrupts(struct kvm_vcpu *vcpu,
				      u32 idt_vectoring_info,
				      int instr_len_field,
				      int error_code_field)
{
	u8 vector;
	int type;
	bool idtv_info_valid;

	idtv_info_valid = idt_vectoring_info & VECTORING_INFO_VALID_MASK;

	vcpu->arch.nmi_injected = false;
	kvm_clear_exception_queue(vcpu);
	kvm_clear_interrupt_queue(vcpu);

	if (!idtv_info_valid)
		return;

	kvm_make_request(KVM_REQ_EVENT, vcpu);