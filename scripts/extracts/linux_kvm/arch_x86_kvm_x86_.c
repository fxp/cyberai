// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 68/86



	if (kvm_vcpu_exit_request(vcpu)) {
		vcpu->mode = OUTSIDE_GUEST_MODE;
		smp_wmb();
		local_irq_enable();
		preempt_enable();
		kvm_vcpu_srcu_read_lock(vcpu);
		r = 1;
		goto cancel_injection;
	}

	run_flags = 0;
	if (req_immediate_exit) {
		run_flags |= KVM_RUN_FORCE_IMMEDIATE_EXIT;
		kvm_make_request(KVM_REQ_EVENT, vcpu);
	}

	fpregs_assert_state_consistent();
	if (test_thread_flag(TIF_NEED_FPU_LOAD))
		switch_fpu_return();

	if (vcpu->arch.guest_fpu.xfd_err)
		wrmsrq(MSR_IA32_XFD_ERR, vcpu->arch.guest_fpu.xfd_err);

	kvm_load_xfeatures(vcpu, true);

	if (unlikely(vcpu->arch.switch_db_regs &&
		     !(vcpu->arch.switch_db_regs & KVM_DEBUGREG_AUTO_SWITCH))) {
		set_debugreg(DR7_FIXED_1, 7);
		set_debugreg(vcpu->arch.eff_db[0], 0);
		set_debugreg(vcpu->arch.eff_db[1], 1);
		set_debugreg(vcpu->arch.eff_db[2], 2);
		set_debugreg(vcpu->arch.eff_db[3], 3);
		/* When KVM_DEBUGREG_WONT_EXIT, dr6 is accessible in guest. */
		if (unlikely(vcpu->arch.switch_db_regs & KVM_DEBUGREG_WONT_EXIT))
			run_flags |= KVM_RUN_LOAD_GUEST_DR6;
	} else if (unlikely(hw_breakpoint_active())) {
		set_debugreg(DR7_FIXED_1, 7);
	}

	/*
	 * Refresh the host DEBUGCTL snapshot after disabling IRQs, as DEBUGCTL
	 * can be modified in IRQ context, e.g. via SMP function calls.  Inform
	 * vendor code if any host-owned bits were changed, e.g. so that the
	 * value loaded into hardware while running the guest can be updated.
	 */
	debug_ctl = get_debugctlmsr();
	if ((debug_ctl ^ vcpu->arch.host_debugctl) & kvm_x86_ops.HOST_OWNED_DEBUGCTL &&
	    !vcpu->arch.guest_state_protected)
		run_flags |= KVM_RUN_LOAD_DEBUGCTL;
	vcpu->arch.host_debugctl = debug_ctl;

	kvm_mediated_pmu_load(vcpu);

	guest_timing_enter_irqoff();

	/*
	 * Swap PKRU with hardware breakpoints disabled to minimize the number
	 * of flows where non-KVM code can run with guest state loaded.
	 */
	kvm_load_guest_pkru(vcpu);

	for (;;) {
		/*
		 * Assert that vCPU vs. VM APICv state is consistent.  An APICv
		 * update must kick and wait for all vCPUs before toggling the
		 * per-VM state, and responding vCPUs must wait for the update
		 * to complete before servicing KVM_REQ_APICV_UPDATE.
		 */
		WARN_ON_ONCE((kvm_vcpu_apicv_activated(vcpu) != kvm_vcpu_apicv_active(vcpu)) &&
			     (kvm_get_apic_mode(vcpu) != LAPIC_MODE_DISABLED));

		exit_fastpath = kvm_x86_call(vcpu_run)(vcpu, run_flags);
		if (likely(exit_fastpath != EXIT_FASTPATH_REENTER_GUEST))
			break;

		if (kvm_lapic_enabled(vcpu))
			kvm_x86_call(sync_pir_to_irr)(vcpu);

		if (unlikely(kvm_vcpu_exit_request(vcpu))) {
			exit_fastpath = EXIT_FASTPATH_EXIT_HANDLED;
			break;
		}

		run_flags = 0;

		/* Note, VM-Exits that go down the "slow" path are accounted below. */
		++vcpu->stat.exits;
	}

	kvm_load_host_pkru(vcpu);

	kvm_mediated_pmu_put(vcpu);

	/*
	 * Do this here before restoring debug registers on the host.  And
	 * since we do this before handling the vmexit, a DR access vmexit
	 * can (a) read the correct value of the debug registers, (b) set
	 * KVM_DEBUGREG_WONT_EXIT again.
	 */
	if (unlikely(vcpu->arch.switch_db_regs & KVM_DEBUGREG_WONT_EXIT)) {
		WARN_ON(vcpu->guest_debug & KVM_GUESTDBG_USE_HW_BP);
		WARN_ON(vcpu->arch.switch_db_regs & KVM_DEBUGREG_AUTO_SWITCH);
		kvm_x86_call(sync_dirty_debug_regs)(vcpu);
		kvm_update_dr0123(vcpu);
		kvm_update_dr7(vcpu);
	}

	/*
	 * If the guest has used debug registers, at least dr7
	 * will be disabled while returning to the host.
	 * If we don't have active breakpoints in the host, we don't
	 * care about the messed up debug address registers. But if
	 * we have some of them active, restore the old state.
	 */
	if (hw_breakpoint_active())
		hw_breakpoint_restore();

	vcpu->arch.last_vmentry_cpu = vcpu->cpu;
	vcpu->arch.last_guest_tsc = kvm_read_l1_tsc(vcpu, rdtsc());

	vcpu->mode = OUTSIDE_GUEST_MODE;
	smp_wmb();

	kvm_load_xfeatures(vcpu, false);

	/*
	 * Sync xfd before calling handle_exit_irqoff() which may
	 * rely on the fact that guest_fpu::xfd is up-to-date (e.g.
	 * in #NM irqoff handler).
	 */
	if (vcpu->arch.xfd_no_write_intercept)
		fpu_sync_guest_vmexit_xfd_state();

	kvm_x86_call(handle_exit_irqoff)(vcpu);

	if (vcpu->arch.guest_fpu.xfd_err)
		wrmsrq(MSR_IA32_XFD_ERR, 0);

	/*
	 * Mark this CPU as needing a branch predictor flush before running
	 * userspace. Must be done before enabling preemption to ensure it gets
	 * set for the CPU that actually ran the guest, and not the CPU that it
	 * may migrate to.
	 */
	if (cpu_feature_enabled(X86_FEATURE_IBPB_EXIT_TO_USER))
		this_cpu_write(x86_ibpb_exit_to_user, true);