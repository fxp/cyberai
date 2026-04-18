// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 38/56



	/*
	 * PML buffer FULL happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (!(to_vmx(vcpu)->idt_vectoring_info & VECTORING_INFO_VALID_MASK) &&
			enable_vnmi &&
			(exit_qualification & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
				GUEST_INTR_STATE_NMI);

	/*
	 * PML buffer already flushed at beginning of VMEXIT. Nothing to do
	 * here.., and there's no userspace involvement needed for PML.
	 */
	return 1;
}

static fastpath_t handle_fastpath_preemption_timer(struct kvm_vcpu *vcpu,
						   bool force_immediate_exit)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	/*
	 * In the *extremely* unlikely scenario that this is a spurious VM-Exit
	 * due to the timer expiring while it was "soft" disabled, just eat the
	 * exit and re-enter the guest.
	 */
	if (unlikely(vmx->loaded_vmcs->hv_timer_soft_disabled))
		return EXIT_FASTPATH_REENTER_GUEST;

	/*
	 * If the timer expired because KVM used it to force an immediate exit,
	 * then mission accomplished.
	 */
	if (force_immediate_exit)
		return EXIT_FASTPATH_EXIT_HANDLED;

	/*
	 * If L2 is active, go down the slow path as emulating the guest timer
	 * expiration likely requires synthesizing a nested VM-Exit.
	 */
	if (is_guest_mode(vcpu))
		return EXIT_FASTPATH_NONE;

	kvm_lapic_expired_hv_timer(vcpu);
	return EXIT_FASTPATH_REENTER_GUEST;
}

static int handle_preemption_timer(struct kvm_vcpu *vcpu)
{
	/*
	 * This non-fastpath handler is reached if and only if the preemption
	 * timer was being used to emulate a guest timer while L2 is active.
	 * All other scenarios are supposed to be handled in the fastpath.
	 */
	WARN_ON_ONCE(!is_guest_mode(vcpu));
	kvm_lapic_expired_hv_timer(vcpu);
	return 1;
}

/*
 * When nested=0, all VMX instruction VM Exits filter here.  The handlers
 * are overwritten by nested_vmx_hardware_setup() when nested=1.
 */
static int handle_vmx_instruction(struct kvm_vcpu *vcpu)
{
	kvm_queue_exception(vcpu, UD_VECTOR);
	return 1;
}

static int handle_tdx_instruction(struct kvm_vcpu *vcpu)
{
	kvm_queue_exception(vcpu, UD_VECTOR);
	return 1;
}

#ifndef CONFIG_X86_SGX_KVM
static int handle_encls(struct kvm_vcpu *vcpu)
{
	/*
	 * SGX virtualization is disabled.  There is no software enable bit for
	 * SGX, so KVM intercepts all ENCLS leafs and injects a #UD to prevent
	 * the guest from executing ENCLS (when SGX is supported by hardware).
	 */
	kvm_queue_exception(vcpu, UD_VECTOR);
	return 1;
}
#endif /* CONFIG_X86_SGX_KVM */

static int handle_bus_lock_vmexit(struct kvm_vcpu *vcpu)
{
	/*
	 * Hardware may or may not set the BUS_LOCK_DETECTED flag on BUS_LOCK
	 * VM-Exits. Unconditionally set the flag here and leave the handling to
	 * vmx_handle_exit().
	 */
	to_vt(vcpu)->exit_reason.bus_lock_detected = true;
	return 1;
}

static int handle_notify(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qual = vmx_get_exit_qual(vcpu);
	bool context_invalid = exit_qual & NOTIFY_VM_CONTEXT_INVALID;

	++vcpu->stat.notify_window_exits;

	/*
	 * Notify VM exit happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (enable_vnmi && (exit_qual & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
			      GUEST_INTR_STATE_NMI);

	if (vcpu->kvm->arch.notify_vmexit_flags & KVM_X86_NOTIFY_VMEXIT_USER ||
	    context_invalid) {
		vcpu->run->exit_reason = KVM_EXIT_NOTIFY;
		vcpu->run->notify.flags = context_invalid ?
					  KVM_NOTIFY_CONTEXT_INVALID : 0;
		return 0;
	}

	return 1;
}

static int vmx_get_msr_imm_reg(struct kvm_vcpu *vcpu)
{
	return vmx_get_instr_info_reg(vmcs_read32(VMX_INSTRUCTION_INFO));
}

static int handle_rdmsr_imm(struct kvm_vcpu *vcpu)
{
	return kvm_emulate_rdmsr_imm(vcpu, vmx_get_exit_qual(vcpu),
				     vmx_get_msr_imm_reg(vcpu));
}

static int handle_wrmsr_imm(struct kvm_vcpu *vcpu)
{
	return kvm_emulate_wrmsr_imm(vcpu, vmx_get_exit_qual(vcpu),
				     vmx_get_msr_imm_reg(vcpu));
}