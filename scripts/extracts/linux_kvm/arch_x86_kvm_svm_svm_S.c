// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 19/36



static int msr_interception(struct kvm_vcpu *vcpu)
{
	if (to_svm(vcpu)->vmcb->control.exit_info_1)
		return kvm_emulate_wrmsr(vcpu);
	else
		return kvm_emulate_rdmsr(vcpu);
}

static int interrupt_window_interception(struct kvm_vcpu *vcpu)
{
	kvm_make_request(KVM_REQ_EVENT, vcpu);
	svm_clear_vintr(to_svm(vcpu));

	++vcpu->stat.irq_window_exits;
	return 1;
}

static int pause_interception(struct kvm_vcpu *vcpu)
{
	bool in_kernel;
	/*
	 * CPL is not made available for an SEV-ES guest, therefore
	 * vcpu->arch.preempted_in_kernel can never be true.  Just
	 * set in_kernel to false as well.
	 */
	in_kernel = !is_sev_es_guest(vcpu) && svm_get_cpl(vcpu) == 0;

	grow_ple_window(vcpu);

	kvm_vcpu_on_spin(vcpu, in_kernel);
	return kvm_skip_emulated_instruction(vcpu);
}

static int invpcid_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	unsigned long type;
	gva_t gva;

	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_INVPCID)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

	/*
	 * For an INVPCID intercept:
	 * EXITINFO1 provides the linear address of the memory operand.
	 * EXITINFO2 provides the contents of the register operand.
	 */
	type = svm->vmcb->control.exit_info_2;
	gva = svm->vmcb->control.exit_info_1;

	/*
	 * FIXME: Perform segment checks for 32-bit mode, and inject #SS if the
	 *        stack segment is used.  The intercept takes priority over all
	 *        #GP checks except CPL>0, but somehow still generates a linear
	 *        address?  The APM is sorely lacking.
	 */
	if (is_noncanonical_address(gva, vcpu, 0)) {
		kvm_queue_exception_e(vcpu, GP_VECTOR, 0);
		return 1;
	}

	return kvm_handle_invpcid(vcpu, type, gva);
}

static inline int complete_userspace_buslock(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * If userspace has NOT changed RIP, then KVM's ABI is to let the guest
	 * execute the bus-locking instruction.  Set the bus lock counter to '1'
	 * to effectively step past the bus lock.
	 */
	if (kvm_is_linear_rip(vcpu, vcpu->arch.cui_linear_rip))
		svm->vmcb->control.bus_lock_counter = 1;

	return 1;
}

static int bus_lock_exit(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	vcpu->run->exit_reason = KVM_EXIT_X86_BUS_LOCK;
	vcpu->run->flags |= KVM_RUN_X86_BUS_LOCK;

	vcpu->arch.cui_linear_rip = kvm_get_linear_rip(vcpu);
	vcpu->arch.complete_userspace_io = complete_userspace_buslock;

	if (is_guest_mode(vcpu))
		svm->nested.last_bus_lock_rip = vcpu->arch.cui_linear_rip;

	return 0;
}

static int vmmcall_interception(struct kvm_vcpu *vcpu)
{
	/*
	 * Inject a #UD if L2 is active and the VMMCALL isn't a Hyper-V TLB
	 * hypercall, as VMMCALL #UDs if it's not intercepted, and this path is
	 * reachable if and only if L1 doesn't want to intercept VMMCALL or has
	 * enabled L0 (KVM) handling of Hyper-V L2 TLB flush hypercalls.
	 */
	if (is_guest_mode(vcpu) && !nested_svm_is_l2_tlb_flush_hcall(vcpu)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

	return kvm_emulate_hypercall(vcpu);
}