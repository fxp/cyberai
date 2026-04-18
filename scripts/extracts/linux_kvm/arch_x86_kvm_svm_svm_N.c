// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 14/36



/*
 * #GP handling code. Note that #GP can be triggered under the following two
 * cases:
 *   1) SVM VM-related instructions (VMRUN/VMSAVE/VMLOAD) that trigger #GP on
 *      some AMD CPUs when EAX of these instructions are in the reserved memory
 *      regions (e.g. SMM memory on host).
 *   2) VMware backdoor
 */
static int gp_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u32 error_code = svm->vmcb->control.exit_info_1;
	u64 svm_exit_code;

	/* Both #GP cases have zero error_code */
	if (error_code)
		goto reinject;

	/* Decode the instruction for usage later */
	if (x86_decode_emulated_instruction(vcpu, 0, NULL, 0) != EMULATION_OK)
		goto reinject;

	/* FIXME: Handle SVM instructions through the emulator */
	svm_exit_code = svm_get_decoded_instr_exit_code(vcpu);
	if (svm_exit_code) {
		if (!is_guest_mode(vcpu))
			return svm_invoke_exit_handler(vcpu, svm_exit_code);

		if (nested_svm_check_permissions(vcpu))
			return 1;

		if (!page_address_valid(vcpu, kvm_register_read(vcpu, VCPU_REGS_RAX)))
			goto reinject;

		/*
		 * FIXME: Only synthesize a #VMEXIT if L1 sets the intercept,
		 * but only after the VMLOAD/VMSAVE exit handlers can properly
		 * handle VMLOAD/VMSAVE from L2 with VLS enabled in L1 (i.e.
		 * RAX is an L2 GPA that needs translation through L1's NPT).
		 */
		nested_svm_simple_vmexit(svm, svm_exit_code);
		return 1;
	}

	/*
	 * VMware backdoor emulation on #GP interception only handles
	 * IN{S}, OUT{S}, and RDPMC, and only for L1.
	 */
	if (!enable_vmware_backdoor || is_guest_mode(vcpu))
		goto reinject;

	return kvm_emulate_instruction(vcpu, EMULTYPE_VMWARE_GP | EMULTYPE_NO_DECODE);

reinject:
	kvm_queue_exception_e(vcpu, GP_VECTOR, error_code);
	return 1;
}

void svm_set_gif(struct vcpu_svm *svm, bool value)
{
	if (value) {
		/*
		 * If VGIF is enabled, the STGI intercept is only added to
		 * detect the opening of the SMI/NMI window; remove it now.
		 * Likewise, clear the VINTR intercept, we will set it
		 * again while processing KVM_REQ_EVENT if needed.
		 */
		if (vgif)
			svm_clr_intercept(svm, INTERCEPT_STGI);
		if (svm_is_intercept(svm, INTERCEPT_VINTR))
			svm_clear_vintr(svm);

		enable_gif(svm);
		if (svm_has_pending_gif_event(svm))
			kvm_make_request(KVM_REQ_EVENT, &svm->vcpu);
	} else {
		disable_gif(svm);

		/*
		 * After a CLGI no interrupts should come.  But if vGIF is
		 * in use, we still rely on the VINTR intercept (rather than
		 * STGI) to detect an open interrupt window.
		*/
		if (!vgif)
			svm_clear_vintr(svm);
	}
}

static int stgi_interception(struct kvm_vcpu *vcpu)
{
	int ret;

	if (nested_svm_check_permissions(vcpu))
		return 1;

	ret = kvm_skip_emulated_instruction(vcpu);
	svm_set_gif(to_svm(vcpu), true);
	return ret;
}

static int clgi_interception(struct kvm_vcpu *vcpu)
{
	int ret;

	if (nested_svm_check_permissions(vcpu))
		return 1;

	ret = kvm_skip_emulated_instruction(vcpu);
	svm_set_gif(to_svm(vcpu), false);
	return ret;
}

static int invlpga_interception(struct kvm_vcpu *vcpu)
{
	gva_t gva = kvm_rax_read(vcpu);
	u32 asid = kvm_rcx_read(vcpu);

	if (nested_svm_check_permissions(vcpu))
		return 1;

	/* FIXME: Handle an address size prefix. */
	if (!is_long_mode(vcpu))
		gva = (u32)gva;

	trace_kvm_invlpga(to_svm(vcpu)->vmcb->save.rip, asid, gva);

	/* Let's treat INVLPGA the same as INVLPG (can be optimized!) */
	kvm_mmu_invlpg(vcpu, gva);

	return kvm_skip_emulated_instruction(vcpu);
}

static int skinit_interception(struct kvm_vcpu *vcpu)
{
	trace_kvm_skinit(to_svm(vcpu)->vmcb->save.rip, kvm_rax_read(vcpu));

	kvm_queue_exception(vcpu, UD_VECTOR);
	return 1;
}

static int task_switch_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u16 tss_selector;
	int reason;
	int int_type = svm->vmcb->control.exit_int_info &
		SVM_EXITINTINFO_TYPE_MASK;
	int int_vec = svm->vmcb->control.exit_int_info & SVM_EVTINJ_VEC_MASK;
	uint32_t type =
		svm->vmcb->control.exit_int_info & SVM_EXITINTINFO_TYPE_MASK;
	uint32_t idt_v =
		svm->vmcb->control.exit_int_info & SVM_EXITINTINFO_VALID;
	bool has_error_code = false;
	u32 error_code = 0;

	tss_selector = (u16)svm->vmcb->control.exit_info_1;

	if (svm->vmcb->control.exit_info_2 &
	    (1ULL << SVM_EXITINFOSHIFT_TS_REASON_IRET))
		reason = TASK_SWITCH_IRET;
	else if (svm->vmcb->control.exit_info_2 &
		 (1ULL << SVM_EXITINFOSHIFT_TS_REASON_JMP))
		reason = TASK_SWITCH_JMP;
	else if (idt_v)
		reason = TASK_SWITCH_GATE;
	else
		reason = TASK_SWITCH_CALL;