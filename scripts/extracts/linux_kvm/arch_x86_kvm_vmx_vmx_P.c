// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 48/56



	switch (vmx_get_exit_reason(vcpu).basic) {
	case EXIT_REASON_MSR_WRITE:
		return handle_fastpath_wrmsr(vcpu);
	case EXIT_REASON_MSR_WRITE_IMM:
		return handle_fastpath_wrmsr_imm(vcpu, vmx_get_exit_qual(vcpu),
						 vmx_get_msr_imm_reg(vcpu));
	case EXIT_REASON_PREEMPTION_TIMER:
		return handle_fastpath_preemption_timer(vcpu, force_immediate_exit);
	case EXIT_REASON_HLT:
		return handle_fastpath_hlt(vcpu);
	case EXIT_REASON_INVD:
		return handle_fastpath_invd(vcpu);
	default:
		return EXIT_FASTPATH_NONE;
	}
}

noinstr void vmx_handle_nmi(struct kvm_vcpu *vcpu)
{
	if ((u16)vmx_get_exit_reason(vcpu).basic != EXIT_REASON_EXCEPTION_NMI ||
	    !is_nmi(vmx_get_intr_info(vcpu)))
		return;

	kvm_before_interrupt(vcpu, KVM_HANDLING_NMI);
	if (cpu_feature_enabled(X86_FEATURE_FRED))
		fred_entry_from_kvm(EVENT_TYPE_NMI, NMI_VECTOR);
	else
		vmx_do_nmi_irqoff();
	kvm_after_interrupt(vcpu);
}

static noinstr void vmx_vcpu_enter_exit(struct kvm_vcpu *vcpu,
					unsigned int flags)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	guest_state_enter_irqoff();

	vmx_l1d_flush(vcpu);

	vmx_disable_fb_clear(vmx);

	if (vcpu->arch.cr2 != native_read_cr2())
		native_write_cr2(vcpu->arch.cr2);

	vmx->fail = __vmx_vcpu_run(vmx, (unsigned long *)&vcpu->arch.regs,
				   flags);

	vcpu->arch.cr2 = native_read_cr2();
	vcpu->arch.regs_avail &= ~VMX_REGS_LAZY_LOAD_SET;

	vmx->idt_vectoring_info = 0;

	vmx_enable_fb_clear(vmx);

	if (unlikely(vmx->fail)) {
		vmx->vt.exit_reason.full = 0xdead;
		goto out;
	}

	vmx->vt.exit_reason.full = vmcs_read32(VM_EXIT_REASON);
	if (likely(!vmx_get_exit_reason(vcpu).failed_vmentry))
		vmx->idt_vectoring_info = vmcs_read32(IDT_VECTORING_INFO_FIELD);

	vmx_handle_nmi(vcpu);

out:
	guest_state_exit_irqoff();
}

fastpath_t vmx_vcpu_run(struct kvm_vcpu *vcpu, u64 run_flags)
{
	bool force_immediate_exit = run_flags & KVM_RUN_FORCE_IMMEDIATE_EXIT;
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	unsigned long cr3, cr4;

	/* Record the guest's net vcpu time for enforced NMI injections. */
	if (unlikely(!enable_vnmi &&
		     vmx->loaded_vmcs->soft_vnmi_blocked))
		vmx->loaded_vmcs->entry_time = ktime_get();

	/*
	 * Don't enter VMX if guest state is invalid, let the exit handler
	 * start emulation until we arrive back to a valid state.  Synthesize a
	 * consistency check VM-Exit due to invalid guest state and bail.
	 */
	if (unlikely(vmx->vt.emulation_required)) {
		vmx->fail = 0;

		vmx->vt.exit_reason.full = EXIT_REASON_INVALID_STATE;
		vmx->vt.exit_reason.failed_vmentry = 1;
		kvm_register_mark_available(vcpu, VCPU_EXREG_EXIT_INFO_1);
		vmx->vt.exit_qualification = ENTRY_FAIL_DEFAULT;
		kvm_register_mark_available(vcpu, VCPU_EXREG_EXIT_INFO_2);
		vmx->vt.exit_intr_info = 0;
		return EXIT_FASTPATH_NONE;
	}

	trace_kvm_entry(vcpu, force_immediate_exit);

	if (vmx->ple_window_dirty) {
		vmx->ple_window_dirty = false;
		vmcs_write32(PLE_WINDOW, vmx->ple_window);
	}

	/*
	 * We did this in prepare_switch_to_guest, because it needs to
	 * be within srcu_read_lock.
	 */
	WARN_ON_ONCE(vmx->nested.need_vmcs12_to_shadow_sync);

	if (kvm_register_is_dirty(vcpu, VCPU_REGS_RSP))
		vmcs_writel(GUEST_RSP, vcpu->arch.regs[VCPU_REGS_RSP]);
	if (kvm_register_is_dirty(vcpu, VCPU_REGS_RIP))
		vmcs_writel(GUEST_RIP, vcpu->arch.regs[VCPU_REGS_RIP]);
	vcpu->arch.regs_dirty = 0;

	if (run_flags & KVM_RUN_LOAD_GUEST_DR6)
		set_debugreg(vcpu->arch.dr6, 6);

	if (run_flags & KVM_RUN_LOAD_DEBUGCTL)
		vmx_reload_guest_debugctl(vcpu);

	/*
	 * Refresh vmcs.HOST_CR3 if necessary.  This must be done immediately
	 * prior to VM-Enter, as the kernel may load a new ASID (PCID) any time
	 * it switches back to the current->mm, which can occur in KVM context
	 * when switching to a temporary mm to patch kernel code, e.g. if KVM
	 * toggles a static key while handling a VM-Exit.
	 */
	cr3 = __get_current_cr3_fast();
	if (unlikely(cr3 != vmx->loaded_vmcs->host_state.cr3)) {
		vmcs_writel(HOST_CR3, cr3);
		vmx->loaded_vmcs->host_state.cr3 = cr3;
	}

	cr4 = cr4_read_shadow();
	if (unlikely(cr4 != vmx->loaded_vmcs->host_state.cr4)) {
		vmcs_writel(HOST_CR4, cr4);
		vmx->loaded_vmcs->host_state.cr4 = cr4;
	}

	/* When single-stepping over STI and MOV SS, we must clear the
	 * corresponding interruptibility bits in the guest state. Otherwise
	 * vmentry fails as it then expects bit 14 (BS) in pending debug
	 * exceptions being set, but that's not correct for the guest debugging
	 * case. */
	if (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)
		vmx_set_interrupt_shadow(vcpu, 0);

	pt_guest_enter(vmx);

	atomic_switch_perf_msrs(vmx);
	if (intel_pmu_lbr_is_enabled(vcpu))
		vmx_passthrough_lbr_msrs(vcpu);

	if (enable_preemption_timer)
		vmx_update_hv_timer(vcpu, force_immediate_exit);
	else if (force_immediate_exit)
		smp_send_reschedule(vcpu->cpu);

	kvm_wait_lapic_expire(vcpu);

	/* The actual VMENTER/EXIT is in the .noinstr.text section. */
	vmx_vcpu_enter_exit(vcpu, __vmx_vcpu_run_flags(vmx));