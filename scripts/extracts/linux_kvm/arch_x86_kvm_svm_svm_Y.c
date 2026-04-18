// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 25/36



	/*
	 * Simiarly, initialize the soft int metadata here to use the most
	 * up-to-date values of RIP and CS base, regardless of restore order.
	 */
	if (svm->soft_int_injected)
		svm_set_nested_run_soft_int_state(vcpu);
}

void svm_complete_interrupt_delivery(struct kvm_vcpu *vcpu, int delivery_mode,
				     int trig_mode, int vector)
{
	/*
	 * apic->apicv_active must be read after vcpu->mode.
	 * Pairs with smp_store_release in vcpu_enter_guest.
	 */
	bool in_guest_mode = (smp_load_acquire(&vcpu->mode) == IN_GUEST_MODE);

	/* Note, this is called iff the local APIC is in-kernel. */
	if (!READ_ONCE(vcpu->arch.apic->apicv_active)) {
		/* Process the interrupt via kvm_check_and_inject_events(). */
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
		return;
	}

	trace_kvm_apicv_accept_irq(vcpu->vcpu_id, delivery_mode, trig_mode, vector);
	if (in_guest_mode) {
		/*
		 * Signal the doorbell to tell hardware to inject the IRQ.  If
		 * the vCPU exits the guest before the doorbell chimes, hardware
		 * will automatically process AVIC interrupts at the next VMRUN.
		 */
		avic_ring_doorbell(vcpu);
	} else {
		/*
		 * Wake the vCPU if it was blocking.  KVM will then detect the
		 * pending IRQ when checking if the vCPU has a wake event.
		 */
		kvm_vcpu_wake_up(vcpu);
	}
}

static void svm_deliver_interrupt(struct kvm_lapic *apic,  int delivery_mode,
				  int trig_mode, int vector)
{
	kvm_lapic_set_irr(vector, apic);

	/*
	 * Pairs with the smp_mb_*() after setting vcpu->guest_mode in
	 * vcpu_enter_guest() to ensure the write to the vIRR is ordered before
	 * the read of guest_mode.  This guarantees that either VMRUN will see
	 * and process the new vIRR entry, or that svm_complete_interrupt_delivery
	 * will signal the doorbell if the CPU has already entered the guest.
	 */
	smp_mb__after_atomic();
	svm_complete_interrupt_delivery(apic->vcpu, delivery_mode, trig_mode, vector);
}

static void svm_update_cr8_intercept(struct kvm_vcpu *vcpu, int tpr, int irr)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * SEV-ES guests must always keep the CR intercepts cleared. CR
	 * tracking is done using the CR write traps.
	 */
	if (is_sev_es_guest(vcpu))
		return;

	if (nested_svm_virtualize_tpr(vcpu))
		return;

	svm_clr_intercept(svm, INTERCEPT_CR8_WRITE);

	if (irr == -1)
		return;

	if (tpr >= irr)
		svm_set_intercept(svm, INTERCEPT_CR8_WRITE);
}

static bool svm_get_nmi_mask(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (is_vnmi_enabled(svm))
		return svm->vmcb->control.int_ctl & V_NMI_BLOCKING_MASK;
	else
		return svm->nmi_masked;
}

static void svm_set_nmi_mask(struct kvm_vcpu *vcpu, bool masked)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (is_vnmi_enabled(svm)) {
		if (masked)
			svm->vmcb->control.int_ctl |= V_NMI_BLOCKING_MASK;
		else
			svm->vmcb->control.int_ctl &= ~V_NMI_BLOCKING_MASK;

	} else {
		svm->nmi_masked = masked;
		if (masked)
			svm_set_iret_intercept(svm);
		else
			svm_clr_iret_intercept(svm);
	}
}

bool svm_nmi_blocked(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb *vmcb = svm->vmcb;

	if (!gif_set(svm))
		return true;

	if (is_guest_mode(vcpu) && nested_exit_on_nmi(svm))
		return false;

	if (svm_get_nmi_mask(vcpu))
		return true;

	return vmcb->control.int_state & SVM_INTERRUPT_SHADOW_MASK;
}

static int svm_nmi_allowed(struct kvm_vcpu *vcpu, bool for_injection)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	if (vcpu->arch.nested_run_pending)
		return -EBUSY;

	if (svm_nmi_blocked(vcpu))
		return 0;

	/* An NMI must not be injected into L2 if it's supposed to VM-Exit.  */
	if (for_injection && is_guest_mode(vcpu) && nested_exit_on_nmi(svm))
		return -EBUSY;
	return 1;
}

bool svm_interrupt_blocked(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb *vmcb = svm->vmcb;

	if (!gif_set(svm))
		return true;

	if (is_guest_mode(vcpu)) {
		/* As long as interrupts are being delivered...  */
		if ((svm->nested.ctl.int_ctl & V_INTR_MASKING_MASK)
		    ? !(svm->vmcb01.ptr->save.rflags & X86_EFLAGS_IF)
		    : !(kvm_get_rflags(vcpu) & X86_EFLAGS_IF))
			return true;

		/* ... vmexits aren't blocked by the interrupt shadow  */
		if (nested_exit_on_intr(svm))
			return false;
	} else {
		if (!svm_get_if_flag(vcpu))
			return true;
	}

	return (vmcb->control.int_state & SVM_INTERRUPT_SHADOW_MASK);
}

static int svm_interrupt_allowed(struct kvm_vcpu *vcpu, bool for_injection)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (vcpu->arch.nested_run_pending)
		return -EBUSY;

	if (svm_interrupt_blocked(vcpu))
		return 0;

	/*
	 * An IRQ must not be injected into L2 if it's supposed to VM-Exit,
	 * e.g. if the IRQ arrived asynchronously after checking nested events.
	 */
	if (for_injection && is_guest_mode(vcpu) && nested_exit_on_intr(svm))
		return -EBUSY;

	return 1;
}

static void svm_enable_irq_window(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);