// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 33/56



/*
 * If the host has split lock detection disabled, then #AC is
 * unconditionally injected into the guest, which is the pre split lock
 * detection behaviour.
 *
 * If the host has split lock detection enabled then #AC is
 * only injected into the guest when:
 *  - Guest CPL == 3 (user mode)
 *  - Guest has #AC detection enabled in CR0
 *  - Guest EFLAGS has AC bit set
 */
bool vmx_guest_inject_ac(struct kvm_vcpu *vcpu)
{
	if (!boot_cpu_has(X86_FEATURE_SPLIT_LOCK_DETECT))
		return true;

	return vmx_get_cpl(vcpu) == 3 && kvm_is_cr0_bit_set(vcpu, X86_CR0_AM) &&
	       (kvm_get_rflags(vcpu) & X86_EFLAGS_AC);
}

static bool is_xfd_nm_fault(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.guest_fpu.fpstate->xfd &&
	       !kvm_is_cr0_bit_set(vcpu, X86_CR0_TS);
}

static int vmx_handle_page_fault(struct kvm_vcpu *vcpu, u32 error_code)
{
	unsigned long cr2 = vmx_get_exit_qual(vcpu);

	if (vcpu->arch.apf.host_apf_flags)
		goto handle_pf;

	/* When using EPT, KVM intercepts #PF only to detect illegal GPAs. */
	WARN_ON_ONCE(enable_ept && !allow_smaller_maxphyaddr);

	/*
	 * On SGX2 hardware, EPCM violations are delivered as #PF with the SGX
	 * flag set in the error code (SGX1 hardware generates #GP(0)).  EPCM
	 * violations have nothing to do with shadow paging and can never be
	 * resolved by KVM; always reflect them into the guest.
	 */
	if (error_code & PFERR_SGX_MASK) {
		WARN_ON_ONCE(!IS_ENABLED(CONFIG_X86_SGX_KVM) ||
			     !cpu_feature_enabled(X86_FEATURE_SGX2));

		if (guest_cpu_cap_has(vcpu, X86_FEATURE_SGX2))
			kvm_fixup_and_inject_pf_error(vcpu, cr2, error_code);
		else
			kvm_inject_gp(vcpu, 0);
		return 1;
	}

	/*
	 * If EPT is enabled, fixup and inject the #PF.  KVM intercepts #PFs
	 * only to set PFERR_RSVD as appropriate (hardware won't set RSVD due
	 * to the GPA being legal with respect to host.MAXPHYADDR).
	 */
	if (enable_ept) {
		kvm_fixup_and_inject_pf_error(vcpu, cr2, error_code);
		return 1;
	}

handle_pf:
	return kvm_handle_page_fault(vcpu, error_code, cr2, NULL, 0);
}

static int handle_exception_nmi(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	struct kvm_run *kvm_run = vcpu->run;
	u32 intr_info, ex_no, error_code;
	unsigned long dr6;
	u32 vect_info;

	vect_info = vmx->idt_vectoring_info;
	intr_info = vmx_get_intr_info(vcpu);

	/*
	 * Machine checks are handled by handle_exception_irqoff(), or by
	 * vmx_vcpu_run() if a #MC occurs on VM-Entry.  NMIs are handled by
	 * vmx_vcpu_enter_exit().
	 */
	if (is_machine_check(intr_info) || is_nmi(intr_info))
		return 1;

	/*
	 * Queue the exception here instead of in handle_nm_fault_irqoff().
	 * This ensures the nested_vmx check is not skipped so vmexit can
	 * be reflected to L1 (when it intercepts #NM) before reaching this
	 * point.
	 */
	if (is_nm_fault(intr_info)) {
		kvm_queue_exception_p(vcpu, NM_VECTOR,
				      is_xfd_nm_fault(vcpu) ? vcpu->arch.guest_fpu.xfd_err : 0);
		return 1;
	}

	if (is_invalid_opcode(intr_info))
		return handle_ud(vcpu);

	if (WARN_ON_ONCE(is_ve_fault(intr_info))) {
		struct vmx_ve_information *ve_info = vmx->ve_info;

		WARN_ONCE(ve_info->exit_reason != EXIT_REASON_EPT_VIOLATION,
			  "Unexpected #VE on VM-Exit reason 0x%x", ve_info->exit_reason);
		dump_vmcs(vcpu);
		kvm_mmu_print_sptes(vcpu, ve_info->guest_physical_address, "#VE");
		return 1;
	}

	error_code = 0;
	if (intr_info & INTR_INFO_DELIVER_CODE_MASK)
		error_code = vmcs_read32(VM_EXIT_INTR_ERROR_CODE);

	if (!vmx->rmode.vm86_active && is_gp_fault(intr_info)) {
		WARN_ON_ONCE(!enable_vmware_backdoor);

		/*
		 * VMware backdoor emulation on #GP interception only handles
		 * IN{S}, OUT{S}, and RDPMC, none of which generate a non-zero
		 * error code on #GP.
		 */
		if (error_code) {
			kvm_queue_exception_e(vcpu, GP_VECTOR, error_code);
			return 1;
		}
		return kvm_emulate_instruction(vcpu, EMULTYPE_VMWARE_GP);
	}

	/*
	 * The #PF with PFEC.RSVD = 1 indicates the guest is accessing
	 * MMIO, it is better to report an internal error.
	 * See the comments in vmx_handle_exit.
	 */
	if ((vect_info & VECTORING_INFO_VALID_MASK) &&
	    !(is_page_fault(intr_info) && !(error_code & PFERR_RSVD_MASK))) {
		vcpu->run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
		vcpu->run->internal.suberror = KVM_INTERNAL_ERROR_SIMUL_EX;
		vcpu->run->internal.ndata = 4;
		vcpu->run->internal.data[0] = vect_info;
		vcpu->run->internal.data[1] = intr_info;
		vcpu->run->internal.data[2] = error_code;
		vcpu->run->internal.data[3] = vcpu->arch.last_vmentry_cpu;
		return 0;
	}

	if (is_page_fault(intr_info))
		return vmx_handle_page_fault(vcpu, error_code);

	ex_no = intr_info & INTR_INFO_VECTOR_MASK;

	if (vmx->rmode.vm86_active && rmode_exception(vcpu, ex_no))
		return handle_rmode_exception(vcpu, ex_no, error_code);