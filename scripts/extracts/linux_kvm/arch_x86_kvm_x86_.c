// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 63/86



	/*
	 * Re-inject exceptions and events *especially* if immediate entry+exit
	 * to/from L2 is needed, as any event that has already been injected
	 * into L2 needs to complete its lifecycle before injecting a new event.
	 *
	 * Don't re-inject an NMI or interrupt if there is a pending exception.
	 * This collision arises if an exception occurred while vectoring the
	 * injected event, KVM intercepted said exception, and KVM ultimately
	 * determined the fault belongs to the guest and queues the exception
	 * for injection back into the guest.
	 *
	 * "Injected" interrupts can also collide with pending exceptions if
	 * userspace ignores the "ready for injection" flag and blindly queues
	 * an interrupt.  In that case, prioritizing the exception is correct,
	 * as the exception "occurred" before the exit to userspace.  Trap-like
	 * exceptions, e.g. most #DBs, have higher priority than interrupts.
	 * And while fault-like exceptions, e.g. #GP and #PF, are the lowest
	 * priority, they're only generated (pended) during instruction
	 * execution, and interrupts are recognized at instruction boundaries.
	 * Thus a pending fault-like exception means the fault occurred on the
	 * *previous* instruction and must be serviced prior to recognizing any
	 * new events in order to fully complete the previous instruction.
	 */
	if (vcpu->arch.exception.injected)
		kvm_inject_exception(vcpu);
	else if (kvm_is_exception_pending(vcpu))
		; /* see above */
	else if (vcpu->arch.nmi_injected)
		kvm_x86_call(inject_nmi)(vcpu);
	else if (vcpu->arch.interrupt.injected)
		kvm_x86_call(inject_irq)(vcpu, true);

	/*
	 * Exceptions that morph to VM-Exits are handled above, and pending
	 * exceptions on top of injected exceptions that do not VM-Exit should
	 * either morph to #DF or, sadly, override the injected exception.
	 */
	WARN_ON_ONCE(vcpu->arch.exception.injected &&
		     vcpu->arch.exception.pending);

	/*
	 * Bail if immediate entry+exit to/from the guest is needed to complete
	 * nested VM-Enter or event re-injection so that a different pending
	 * event can be serviced (or if KVM needs to exit to userspace).
	 *
	 * Otherwise, continue processing events even if VM-Exit occurred.  The
	 * VM-Exit will have cleared exceptions that were meant for L2, but
	 * there may now be events that can be injected into L1.
	 */
	if (r < 0)
		goto out;

	/*
	 * A pending exception VM-Exit should either result in nested VM-Exit
	 * or force an immediate re-entry and exit to/from L2, and exception
	 * VM-Exits cannot be injected (flag should _never_ be set).
	 */
	WARN_ON_ONCE(vcpu->arch.exception_vmexit.injected ||
		     vcpu->arch.exception_vmexit.pending);

	/*
	 * New events, other than exceptions, cannot be injected if KVM needs
	 * to re-inject a previous event.  See above comments on re-injecting
	 * for why pending exceptions get priority.
	 */
	can_inject = !kvm_event_needs_reinjection(vcpu);

	if (vcpu->arch.exception.pending) {
		/*
		 * Fault-class exceptions, except #DBs, set RF=1 in the RFLAGS
		 * value pushed on the stack.  Trap-like exception and all #DBs
		 * leave RF as-is (KVM follows Intel's behavior in this regard;
		 * AMD states that code breakpoint #DBs excplitly clear RF=0).
		 *
		 * Note, most versions of Intel's SDM and AMD's APM incorrectly
		 * describe the behavior of General Detect #DBs, which are
		 * fault-like.  They do _not_ set RF, a la code breakpoints.
		 */
		if (exception_type(vcpu->arch.exception.vector) == EXCPT_FAULT)
			__kvm_set_rflags(vcpu, kvm_get_rflags(vcpu) |
					     X86_EFLAGS_RF);

		if (vcpu->arch.exception.vector == DB_VECTOR &&
		    vcpu->arch.dr7 & DR7_GD) {
			vcpu->arch.dr7 &= ~DR7_GD;
			kvm_update_dr7(vcpu);
		}

		kvm_inject_exception(vcpu);

		vcpu->arch.exception.pending = false;
		vcpu->arch.exception.injected = true;

		can_inject = false;
	}

	/* Don't inject interrupts if the user asked to avoid doing so */
	if (vcpu->guest_debug & KVM_GUESTDBG_BLOCKIRQ)
		return 0;

	/*
	 * Finally, inject interrupt events.  If an event cannot be injected
	 * due to architectural conditions (e.g. IF=0) a window-open exit
	 * will re-request KVM_REQ_EVENT.  Sometimes however an event is pending
	 * and can architecturally be injected, but we cannot do it right now:
	 * an interrupt could have arrived just now and we have to inject it
	 * as a vmexit, or there could already an event in the queue, which is
	 * indicated by can_inject.  In that case we request an immediate exit
	 * in order to make progress and get back here for another iteration.
	 * The kvm_x86_ops hooks communicate this by returning -EBUSY.
	 */
#ifdef CONFIG_KVM_SMM
	if (vcpu->arch.smi_pending) {
		r = can_inject ? kvm_x86_call(smi_allowed)(vcpu, true) :
				 -EBUSY;
		if (r < 0)
			goto out;
		if (r) {
			vcpu->arch.smi_pending = false;
			++vcpu->arch.smi_count;
			enter_smm(vcpu);
			can_inject = false;
		} else
			kvm_x86_call(enable_smi_window)(vcpu);
	}
#endif