// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 24/36



		if (vmexit == NESTED_EXIT_CONTINUE)
			vmexit = nested_svm_exit_handled(svm);

		if (vmexit == NESTED_EXIT_DONE)
			return 1;
	}

	if (svm_is_vmrun_failure(svm->vmcb->control.exit_code)) {
		kvm_run->exit_reason = KVM_EXIT_FAIL_ENTRY;
		kvm_run->fail_entry.hardware_entry_failure_reason
			= svm->vmcb->control.exit_code;
		kvm_run->fail_entry.cpu = vcpu->arch.last_vmentry_cpu;
		dump_vmcb(vcpu);
		return 0;
	}

	if (exit_fastpath != EXIT_FASTPATH_NONE)
		return 1;

	return svm_invoke_exit_handler(vcpu, svm->vmcb->control.exit_code);
}

static void svm_set_nested_run_soft_int_state(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->soft_int_csbase = svm->vmcb->save.cs.base;
	svm->soft_int_old_rip = kvm_rip_read(vcpu);
	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_NRIPS))
		svm->soft_int_next_rip = kvm_rip_read(vcpu);
}

static int pre_svm_run(struct kvm_vcpu *vcpu)
{
	struct svm_cpu_data *sd = per_cpu_ptr(&svm_data, vcpu->cpu);
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * If the previous vmrun of the vmcb occurred on a different physical
	 * cpu, then mark the vmcb dirty and assign a new asid.  Hardware's
	 * vmcb clean bits are per logical CPU, as are KVM's asid assignments.
	 */
	if (unlikely(svm->current_vmcb->cpu != vcpu->cpu)) {
		svm->current_vmcb->asid_generation = 0;
		vmcb_mark_all_dirty(svm->vmcb);
		svm->current_vmcb->cpu = vcpu->cpu;
        }

	if (is_sev_guest(vcpu))
		return pre_sev_run(svm, vcpu->cpu);

	/* FIXME: handle wraparound of asid_generation */
	if (svm->current_vmcb->asid_generation != sd->asid_generation)
		new_asid(svm, sd);

	return 0;
}

static void svm_inject_nmi(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->vmcb->control.event_inj = SVM_EVTINJ_VALID | SVM_EVTINJ_TYPE_NMI;

	if (svm->nmi_l1_to_l2)
		return;

	/*
	 * No need to manually track NMI masking when vNMI is enabled, hardware
	 * automatically sets V_NMI_BLOCKING_MASK as appropriate, including the
	 * case where software directly injects an NMI.
	 */
	if (!is_vnmi_enabled(svm)) {
		svm->nmi_masked = true;
		svm_set_iret_intercept(svm);
	}
	++vcpu->stat.nmi_injections;
}

static bool svm_is_vnmi_pending(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (!is_vnmi_enabled(svm))
		return false;

	return !!(svm->vmcb->control.int_ctl & V_NMI_PENDING_MASK);
}

static bool svm_set_vnmi_pending(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (!is_vnmi_enabled(svm))
		return false;

	if (svm->vmcb->control.int_ctl & V_NMI_PENDING_MASK)
		return false;

	svm->vmcb->control.int_ctl |= V_NMI_PENDING_MASK;
	vmcb_mark_dirty(svm->vmcb, VMCB_INTR);

	/*
	 * Because the pending NMI is serviced by hardware, KVM can't know when
	 * the NMI is "injected", but for all intents and purposes, passing the
	 * NMI off to hardware counts as injection.
	 */
	++vcpu->stat.nmi_injections;

	return true;
}

static void svm_inject_irq(struct kvm_vcpu *vcpu, bool reinjected)
{
	struct kvm_queued_interrupt *intr = &vcpu->arch.interrupt;
	struct vcpu_svm *svm = to_svm(vcpu);
	u32 type;

	if (intr->soft) {
		if (svm_update_soft_interrupt_rip(vcpu, intr->nr))
			return;

		type = SVM_EVTINJ_TYPE_SOFT;
	} else {
		type = SVM_EVTINJ_TYPE_INTR;
	}

	/*
	 * If AVIC was inhibited in order to detect an IRQ window, and there's
	 * no other injectable interrupts pending or L2 is active (see below),
	 * then drop the inhibit as the window has served its purpose.
	 *
	 * If L2 is active, this path is reachable if L1 is not intercepting
	 * IRQs, i.e. if KVM is injecting L1 IRQs into L2.  AVIC is locally
	 * inhibited while L2 is active; drop the VM-wide inhibit to optimize
	 * the case in which the interrupt window was requested while L1 was
	 * active (the vCPU was not running nested).
	 */
	if (svm->avic_irq_window &&
	    (!kvm_cpu_has_injectable_intr(vcpu) || is_guest_mode(vcpu))) {
		svm->avic_irq_window = false;
		kvm_dec_apicv_irq_window_req(svm->vcpu.kvm);
	}

	trace_kvm_inj_virq(intr->nr, intr->soft, reinjected);
	++vcpu->stat.irq_injections;

	svm->vmcb->control.event_inj = intr->nr | SVM_EVTINJ_VALID | type;
}

static void svm_fixup_nested_rips(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (!is_guest_mode(vcpu) || !vcpu->arch.nested_run_pending)
		return;

	/*
	 * If nrips is supported in hardware but not exposed to L1, stuff the
	 * actual L2 RIP to emulate what a nrips=0 CPU would do (L1 is
	 * responsible for advancing RIP prior to injecting the event). Once L2
	 * runs after L1 executes VMRUN, NextRIP is updated by the CPU and/or
	 * KVM, and this is no longer needed.
	 *
	 * This is done here (as opposed to when preparing vmcb02) to use the
	 * most up-to-date value of RIP regardless of the order of restoring
	 * registers and nested state in the vCPU save+restore path.
	 */
	if (boot_cpu_has(X86_FEATURE_NRIPS) &&
	    !guest_cpu_cap_has(vcpu, X86_FEATURE_NRIPS))
		svm->vmcb->control.next_rip = kvm_rip_read(vcpu);