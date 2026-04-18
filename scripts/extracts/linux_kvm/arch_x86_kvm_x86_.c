// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 69/86



	/*
	 * Consume any pending interrupts, including the possible source of
	 * VM-Exit on SVM and any ticks that occur between VM-Exit and now.
	 * An instruction is required after local_irq_enable() to fully unblock
	 * interrupts on processors that implement an interrupt shadow, the
	 * stat.exits increment will do nicely.
	 */
	kvm_before_interrupt(vcpu, KVM_HANDLING_IRQ);
	local_irq_enable();
	++vcpu->stat.exits;
	local_irq_disable();
	kvm_after_interrupt(vcpu);

	/*
	 * Wait until after servicing IRQs to account guest time so that any
	 * ticks that occurred while running the guest are properly accounted
	 * to the guest.  Waiting until IRQs are enabled degrades the accuracy
	 * of accounting via context tracking, but the loss of accuracy is
	 * acceptable for all known use cases.
	 */
	guest_timing_exit_irqoff();

	local_irq_enable();
	preempt_enable();

	kvm_vcpu_srcu_read_lock(vcpu);

	/*
	 * Call this to ensure WC buffers in guest are evicted after each VM
	 * Exit, so that the evicted WC writes can be snooped across all cpus
	 */
	smp_mb__after_srcu_read_lock();

	/*
	 * Profile KVM exit RIPs:
	 */
	if (unlikely(prof_on == KVM_PROFILING &&
		     !vcpu->arch.guest_state_protected)) {
		unsigned long rip = kvm_rip_read(vcpu);
		profile_hit(KVM_PROFILING, (void *)rip);
	}

	if (unlikely(vcpu->arch.tsc_always_catchup))
		kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);

	if (vcpu->arch.apic_attention)
		kvm_lapic_sync_from_vapic(vcpu);

	if (unlikely(exit_fastpath == EXIT_FASTPATH_EXIT_USERSPACE))
		return 0;

	r = kvm_x86_call(handle_exit)(vcpu, exit_fastpath);
	return r;

cancel_injection:
	if (req_immediate_exit)
		kvm_make_request(KVM_REQ_EVENT, vcpu);
	kvm_x86_call(cancel_injection)(vcpu);
	if (unlikely(vcpu->arch.apic_attention))
		kvm_lapic_sync_from_vapic(vcpu);
out:
	return r;
}

static bool kvm_vcpu_running(struct kvm_vcpu *vcpu)
{
	return (vcpu->arch.mp_state == KVM_MP_STATE_RUNNABLE &&
		!vcpu->arch.apf.halted);
}

bool kvm_vcpu_has_events(struct kvm_vcpu *vcpu)
{
	if (!list_empty_careful(&vcpu->async_pf.done))
		return true;

	if (kvm_apic_has_pending_init_or_sipi(vcpu) &&
	    kvm_apic_init_sipi_allowed(vcpu))
		return true;

	if (kvm_is_exception_pending(vcpu))
		return true;

	if (kvm_test_request(KVM_REQ_NMI, vcpu) ||
	    (vcpu->arch.nmi_pending &&
	     kvm_x86_call(nmi_allowed)(vcpu, false)))
		return true;

#ifdef CONFIG_KVM_SMM
	if (kvm_test_request(KVM_REQ_SMI, vcpu) ||
	    (vcpu->arch.smi_pending &&
	     kvm_x86_call(smi_allowed)(vcpu, false)))
		return true;
#endif

	if (kvm_test_request(KVM_REQ_PMI, vcpu))
		return true;

	if (kvm_test_request(KVM_REQ_UPDATE_PROTECTED_GUEST_STATE, vcpu))
		return true;

	if (kvm_arch_interrupt_allowed(vcpu) && kvm_cpu_has_interrupt(vcpu))
		return true;

	if (kvm_hv_has_stimer_pending(vcpu))
		return true;

	if (is_guest_mode(vcpu) &&
	    kvm_x86_ops.nested_ops->has_events &&
	    kvm_x86_ops.nested_ops->has_events(vcpu, false))
		return true;

	if (kvm_xen_has_pending_events(vcpu))
		return true;

	return false;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_has_events);

int kvm_arch_vcpu_runnable(struct kvm_vcpu *vcpu)
{
	return kvm_vcpu_running(vcpu) || vcpu->arch.pv.pv_unhalted ||
	       kvm_vcpu_has_events(vcpu);
}

/* Called within kvm->srcu read side.  */
static inline int vcpu_block(struct kvm_vcpu *vcpu)
{
	bool hv_timer;

	if (!kvm_arch_vcpu_runnable(vcpu)) {
		/*
		 * Switch to the software timer before halt-polling/blocking as
		 * the guest's timer may be a break event for the vCPU, and the
		 * hypervisor timer runs only when the CPU is in guest mode.
		 * Switch before halt-polling so that KVM recognizes an expired
		 * timer before blocking.
		 */
		hv_timer = kvm_lapic_hv_timer_in_use(vcpu);
		if (hv_timer)
			kvm_lapic_switch_to_sw_timer(vcpu);

		kvm_vcpu_srcu_read_unlock(vcpu);
		if (vcpu->arch.mp_state == KVM_MP_STATE_HALTED)
			kvm_vcpu_halt(vcpu);
		else
			kvm_vcpu_block(vcpu);
		kvm_vcpu_srcu_read_lock(vcpu);

		if (hv_timer)
			kvm_lapic_switch_to_hv_timer(vcpu);

		/*
		 * If the vCPU is not runnable, a signal or another host event
		 * of some kind is pending; service it without changing the
		 * vCPU's activity state.
		 */
		if (!kvm_arch_vcpu_runnable(vcpu))
			return 1;
	}

	/*
	 * Evaluate nested events before exiting the halted state.  This allows
	 * the halt state to be recorded properly in the VMCS12's activity
	 * state field (AMD does not have a similar field and a VM-Exit always
	 * causes a spurious wakeup from HLT).
	 */
	if (is_guest_mode(vcpu)) {
		int r = kvm_check_nested_events(vcpu);

		if (r < 0 && r != -EBUSY)
			return 0;
	}

	if (kvm_apic_accept_events(vcpu) < 0)
		return 0;
	switch(vcpu->arch.mp_state) {
	case KVM_MP_STATE_HALTED:
	case KVM_MP_STATE_AP_RESET_HOLD:
		kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);
		fallthrough;
	case KVM_MP_STATE_RUNNABLE:
		vcpu->arch.apf.halted = false;
		break;
	case KVM_MP_STATE_INIT_RECEIVED:
		break;
	default:
		WARN_ON_ONCE(1);
		break;
	}
	return 1;
}