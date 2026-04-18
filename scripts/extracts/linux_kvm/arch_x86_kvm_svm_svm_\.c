// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 28/36



		kvm_requeue_exception(vcpu, vector,
				      exitintinfo & SVM_EXITINTINFO_VALID_ERR,
				      error_code);
		break;
	}
	case SVM_EXITINTINFO_TYPE_INTR:
		kvm_queue_interrupt(vcpu, vector, false);
		break;
	case SVM_EXITINTINFO_TYPE_SOFT:
		kvm_queue_interrupt(vcpu, vector, true);
		break;
	default:
		break;
	}

}

static void svm_cancel_injection(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb_control_area *control = &svm->vmcb->control;

	control->exit_int_info = control->event_inj;
	control->exit_int_info_err = control->event_inj_err;
	control->event_inj = 0;
	svm_complete_interrupts(vcpu);
}

static int svm_vcpu_pre_run(struct kvm_vcpu *vcpu)
{
#ifdef CONFIG_KVM_AMD_SEV
	if (to_kvm_sev_info(vcpu->kvm)->need_init)
		return -EINVAL;
#endif

	return 1;
}

static fastpath_t svm_exit_handlers_fastpath(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb_control_area *control = &svm->vmcb->control;

	/*
	 * Next RIP must be provided as IRQs are disabled, and accessing guest
	 * memory to decode the instruction might fault, i.e. might sleep.
	 */
	if (!nrips || !control->next_rip)
		return EXIT_FASTPATH_NONE;

	if (is_guest_mode(vcpu))
		return EXIT_FASTPATH_NONE;

	switch (control->exit_code) {
	case SVM_EXIT_MSR:
		if (!control->exit_info_1)
			break;
		return handle_fastpath_wrmsr(vcpu);
	case SVM_EXIT_HLT:
		return handle_fastpath_hlt(vcpu);
	case SVM_EXIT_INVD:
		return handle_fastpath_invd(vcpu);
	default:
		break;
	}

	return EXIT_FASTPATH_NONE;
}

static noinstr void svm_vcpu_enter_exit(struct kvm_vcpu *vcpu, bool spec_ctrl_intercepted)
{
	struct svm_cpu_data *sd = per_cpu_ptr(&svm_data, vcpu->cpu);
	struct vcpu_svm *svm = to_svm(vcpu);

	guest_state_enter_irqoff();

	/*
	 * Set RFLAGS.IF prior to VMRUN, as the host's RFLAGS.IF at the time of
	 * VMRUN controls whether or not physical IRQs are masked (KVM always
	 * runs with V_INTR_MASKING_MASK).  Toggle RFLAGS.IF here to avoid the
	 * temptation to do STI+VMRUN+CLI, as AMD CPUs bleed the STI shadow
	 * into guest state if delivery of an event during VMRUN triggers a
	 * #VMEXIT, and the guest_state transitions already tell lockdep that
	 * IRQs are being enabled/disabled.  Note!  GIF=0 for the entirety of
	 * this path, so IRQs aren't actually unmasked while running host code.
	 */
	raw_local_irq_enable();

	amd_clear_divider();

	if (is_sev_es_guest(vcpu))
		__svm_sev_es_vcpu_run(svm, spec_ctrl_intercepted,
				      sev_es_host_save_area(sd));
	else
		__svm_vcpu_run(svm, spec_ctrl_intercepted);

	raw_local_irq_disable();

	guest_state_exit_irqoff();
}

static __no_kcsan fastpath_t svm_vcpu_run(struct kvm_vcpu *vcpu, u64 run_flags)
{
	bool force_immediate_exit = run_flags & KVM_RUN_FORCE_IMMEDIATE_EXIT;
	struct vcpu_svm *svm = to_svm(vcpu);
	bool spec_ctrl_intercepted = msr_write_intercepted(vcpu, MSR_IA32_SPEC_CTRL);

	trace_kvm_entry(vcpu, force_immediate_exit);

	svm->vmcb->save.rax = vcpu->arch.regs[VCPU_REGS_RAX];
	svm->vmcb->save.rsp = vcpu->arch.regs[VCPU_REGS_RSP];
	svm->vmcb->save.rip = vcpu->arch.regs[VCPU_REGS_RIP];

	/*
	 * Disable singlestep if we're injecting an interrupt/exception.
	 * We don't want our modified rflags to be pushed on the stack where
	 * we might not be able to easily reset them if we disabled NMI
	 * singlestep later.
	 */
	if (svm->nmi_singlestep && svm->vmcb->control.event_inj) {
		/*
		 * Event injection happens before external interrupts cause a
		 * vmexit and interrupts are disabled here, so smp_send_reschedule
		 * is enough to force an immediate vmexit.
		 */
		disable_nmi_singlestep(svm);
		force_immediate_exit = true;
	}

	if (force_immediate_exit)
		smp_send_reschedule(vcpu->cpu);

	if (pre_svm_run(vcpu)) {
		vcpu->run->exit_reason = KVM_EXIT_FAIL_ENTRY;
		vcpu->run->fail_entry.hardware_entry_failure_reason = SVM_EXIT_ERR;
		vcpu->run->fail_entry.cpu = vcpu->cpu;
		return EXIT_FASTPATH_EXIT_USERSPACE;
	}

	sync_lapic_to_cr8(vcpu);

	if (unlikely(svm->asid != svm->vmcb->control.asid)) {
		svm->vmcb->control.asid = svm->asid;
		vmcb_mark_dirty(svm->vmcb, VMCB_ASID);
	}
	svm->vmcb->save.cr2 = vcpu->arch.cr2;

	if (guest_cpu_cap_has(vcpu, X86_FEATURE_ERAPS) &&
	    kvm_register_is_dirty(vcpu, VCPU_EXREG_ERAPS))
		svm->vmcb->control.erap_ctl |= ERAP_CONTROL_CLEAR_RAP;

	svm_fixup_nested_rips(vcpu);

	svm_hv_update_vp_id(svm->vmcb, vcpu);

	/*
	 * Run with all-zero DR6 unless the guest can write DR6 freely, so that
	 * KVM can get the exact cause of a #DB.  Note, loading guest DR6 from
	 * KVM's snapshot is only necessary when DR accesses won't exit.
	 */
	if (unlikely(run_flags & KVM_RUN_LOAD_GUEST_DR6))
		svm_set_dr6(vcpu, vcpu->arch.dr6);
	else if (likely(!(vcpu->arch.switch_db_regs & KVM_DEBUGREG_WONT_EXIT)))
		svm_set_dr6(vcpu, DR6_ACTIVE_LOW);

	clgi();