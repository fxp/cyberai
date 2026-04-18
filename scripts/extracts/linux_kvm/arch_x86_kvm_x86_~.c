// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 62/86



	if (max_irr != -1)
		max_irr >>= 4;

	tpr = kvm_lapic_get_cr8(vcpu);

	kvm_x86_call(update_cr8_intercept)(vcpu, tpr, max_irr);
}


int kvm_check_nested_events(struct kvm_vcpu *vcpu)
{
	if (kvm_test_request(KVM_REQ_TRIPLE_FAULT, vcpu)) {
		kvm_x86_ops.nested_ops->triple_fault(vcpu);
		return 1;
	}

	return kvm_x86_ops.nested_ops->check_events(vcpu);
}

static void kvm_inject_exception(struct kvm_vcpu *vcpu)
{
	/*
	 * Suppress the error code if the vCPU is in Real Mode, as Real Mode
	 * exceptions don't report error codes.  The presence of an error code
	 * is carried with the exception and only stripped when the exception
	 * is injected as intercepted #PF VM-Exits for AMD's Paged Real Mode do
	 * report an error code despite the CPU being in Real Mode.
	 */
	vcpu->arch.exception.has_error_code &= is_protmode(vcpu);

	trace_kvm_inj_exception(vcpu->arch.exception.vector,
				vcpu->arch.exception.has_error_code,
				vcpu->arch.exception.error_code,
				vcpu->arch.exception.injected);

	kvm_x86_call(inject_exception)(vcpu);
}

/*
 * Check for any event (interrupt or exception) that is ready to be injected,
 * and if there is at least one event, inject the event with the highest
 * priority.  This handles both "pending" events, i.e. events that have never
 * been injected into the guest, and "injected" events, i.e. events that were
 * injected as part of a previous VM-Enter, but weren't successfully delivered
 * and need to be re-injected.
 *
 * Note, this is not guaranteed to be invoked on a guest instruction boundary,
 * i.e. doesn't guarantee that there's an event window in the guest.  KVM must
 * be able to inject exceptions in the "middle" of an instruction, and so must
 * also be able to re-inject NMIs and IRQs in the middle of an instruction.
 * I.e. for exceptions and re-injected events, NOT invoking this on instruction
 * boundaries is necessary and correct.
 *
 * For simplicity, KVM uses a single path to inject all events (except events
 * that are injected directly from L1 to L2) and doesn't explicitly track
 * instruction boundaries for asynchronous events.  However, because VM-Exits
 * that can occur during instruction execution typically result in KVM skipping
 * the instruction or injecting an exception, e.g. instruction and exception
 * intercepts, and because pending exceptions have higher priority than pending
 * interrupts, KVM still honors instruction boundaries in most scenarios.
 *
 * But, if a VM-Exit occurs during instruction execution, and KVM does NOT skip
 * the instruction or inject an exception, then KVM can incorrecty inject a new
 * asynchronous event if the event became pending after the CPU fetched the
 * instruction (in the guest).  E.g. if a page fault (#PF, #NPF, EPT violation)
 * occurs and is resolved by KVM, a coincident NMI, SMI, IRQ, etc... can be
 * injected on the restarted instruction instead of being deferred until the
 * instruction completes.
 *
 * In practice, this virtualization hole is unlikely to be observed by the
 * guest, and even less likely to cause functional problems.  To detect the
 * hole, the guest would have to trigger an event on a side effect of an early
 * phase of instruction execution, e.g. on the instruction fetch from memory.
 * And for it to be a functional problem, the guest would need to depend on the
 * ordering between that side effect, the instruction completing, _and_ the
 * delivery of the asynchronous event.
 */
static int kvm_check_and_inject_events(struct kvm_vcpu *vcpu,
				       bool *req_immediate_exit)
{
	bool can_inject;
	int r;

	/*
	 * Process nested events first, as nested VM-Exit supersedes event
	 * re-injection.  If there's an event queued for re-injection, it will
	 * be saved into the appropriate vmc{b,s}12 fields on nested VM-Exit.
	 */
	if (is_guest_mode(vcpu))
		r = kvm_check_nested_events(vcpu);
	else
		r = 0;