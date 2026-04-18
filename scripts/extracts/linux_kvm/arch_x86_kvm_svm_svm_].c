// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 29/36



	/*
	 * Hardware only context switches DEBUGCTL if LBR virtualization is
	 * enabled.  Manually load DEBUGCTL if necessary (and restore it after
	 * VM-Exit), as running with the host's DEBUGCTL can negatively affect
	 * guest state and can even be fatal, e.g. due to Bus Lock Detect.
	 */
	if (!(svm->vmcb->control.misc_ctl2 & SVM_MISC2_ENABLE_V_LBR) &&
	    vcpu->arch.host_debugctl != svm->vmcb->save.dbgctl)
		update_debugctlmsr(svm->vmcb->save.dbgctl);

	kvm_wait_lapic_expire(vcpu);

	/*
	 * If this vCPU has touched SPEC_CTRL, restore the guest's value if
	 * it's non-zero. Since vmentry is serialising on affected CPUs, there
	 * is no need to worry about the conditional branch over the wrmsr
	 * being speculatively taken.
	 */
	if (!static_cpu_has(X86_FEATURE_V_SPEC_CTRL))
		x86_spec_ctrl_set_guest(svm->virt_spec_ctrl);

	svm_vcpu_enter_exit(vcpu, spec_ctrl_intercepted);

	if (!static_cpu_has(X86_FEATURE_V_SPEC_CTRL))
		x86_spec_ctrl_restore_host(svm->virt_spec_ctrl);

	if (!is_sev_es_guest(vcpu)) {
		vcpu->arch.cr2 = svm->vmcb->save.cr2;
		vcpu->arch.regs[VCPU_REGS_RAX] = svm->vmcb->save.rax;
		vcpu->arch.regs[VCPU_REGS_RSP] = svm->vmcb->save.rsp;
		vcpu->arch.regs[VCPU_REGS_RIP] = svm->vmcb->save.rip;
	}
	vcpu->arch.regs_dirty = 0;

	if (unlikely(svm->vmcb->control.exit_code == SVM_EXIT_NMI))
		kvm_before_interrupt(vcpu, KVM_HANDLING_NMI);

	if (!(svm->vmcb->control.misc_ctl2 & SVM_MISC2_ENABLE_V_LBR) &&
	    vcpu->arch.host_debugctl != svm->vmcb->save.dbgctl)
		update_debugctlmsr(vcpu->arch.host_debugctl);

	stgi();

	/* Any pending NMI will happen here */

	if (unlikely(svm->vmcb->control.exit_code == SVM_EXIT_NMI))
		kvm_after_interrupt(vcpu);

	sync_cr8_to_lapic(vcpu);

	svm->next_rip = 0;
	if (is_guest_mode(vcpu)) {
		nested_sync_control_from_vmcb02(svm);

		/* Track VMRUNs that have made past consistency checking */
		if (vcpu->arch.nested_run_pending &&
		    !svm_is_vmrun_failure(svm->vmcb->control.exit_code))
                        ++vcpu->stat.nested_run;

		vcpu->arch.nested_run_pending = 0;
	}

	svm->vmcb->control.tlb_ctl = TLB_CONTROL_DO_NOTHING;

	/*
	 * Unconditionally mask off the CLEAR_RAP bit, the AND is just as cheap
	 * as the TEST+Jcc to avoid it.
	 */
	if (cpu_feature_enabled(X86_FEATURE_ERAPS))
		svm->vmcb->control.erap_ctl &= ~ERAP_CONTROL_CLEAR_RAP;

	vmcb_mark_all_clean(svm->vmcb);

	/* if exit due to PF check for async PF */
	if (svm->vmcb->control.exit_code == SVM_EXIT_EXCP_BASE + PF_VECTOR)
		vcpu->arch.apf.host_apf_flags =
			kvm_read_and_reset_apf_flags();

	vcpu->arch.regs_avail &= ~SVM_REGS_LAZY_LOAD_SET;

	if (!msr_write_intercepted(vcpu, MSR_AMD64_PERF_CNTR_GLOBAL_CTL))
		rdmsrq(MSR_AMD64_PERF_CNTR_GLOBAL_CTL, vcpu_to_pmu(vcpu)->global_ctrl);

	trace_kvm_exit(vcpu, KVM_ISA_SVM);

	svm_complete_interrupts(vcpu);

	/*
	 * Update the cache after completing interrupts to get an accurate
	 * NextRIP, e.g. when re-injecting a soft interrupt.
	 *
	 * FIXME: Rework svm_get_nested_state() to not pull data from the
	 *        cache (except for maybe int_ctl).
	 */
	if (is_guest_mode(vcpu))
		svm->nested.ctl.next_rip = svm->vmcb->control.next_rip;

	return svm_exit_handlers_fastpath(vcpu);
}

static void svm_load_mmu_pgd(struct kvm_vcpu *vcpu, hpa_t root_hpa,
			     int root_level)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	unsigned long cr3;

	if (npt_enabled) {
		svm->vmcb->control.nested_cr3 = __sme_set(root_hpa);
		vmcb_mark_dirty(svm->vmcb, VMCB_NPT);

		hv_track_root_tdp(vcpu, root_hpa);

		cr3 = vcpu->arch.cr3;
	} else if (root_level >= PT64_ROOT_4LEVEL) {
		cr3 = __sme_set(root_hpa) | kvm_get_active_pcid(vcpu);
	} else {
		/* PCID in the guest should be impossible with a 32-bit MMU. */
		WARN_ON_ONCE(kvm_get_active_pcid(vcpu));
		cr3 = root_hpa;
	}

	svm->vmcb->save.cr3 = cr3;
	vmcb_mark_dirty(svm->vmcb, VMCB_CR);
}

static void
svm_patch_hypercall(struct kvm_vcpu *vcpu, unsigned char *hypercall)
{
	/*
	 * Patch in the VMMCALL instruction:
	 */
	hypercall[0] = 0x0f;
	hypercall[1] = 0x01;
	hypercall[2] = 0xd9;
}

/*
 * The kvm parameter can be NULL (module initialization, or invocation before
 * VM creation). Be sure to check the kvm parameter before using it.
 */
static bool svm_has_emulated_msr(struct kvm *kvm, u32 index)
{
	switch (index) {
	case MSR_IA32_MCG_EXT_CTL:
	case KVM_FIRST_EMULATED_VMX_MSR ... KVM_LAST_EMULATED_VMX_MSR:
		return false;
	case MSR_IA32_SMBASE:
		if (!IS_ENABLED(CONFIG_KVM_SMM))
			return false;

#ifdef CONFIG_KVM_AMD_SEV
		/*
		 * KVM can't access register state to emulate SMM for SEV-ES
		 * guests.  Conusming stale data here is "fine", as KVM only
		 * checks for MSR_IA32_SMBASE support without a vCPU when
		 * userspace is querying KVM_CAP_X86_SMM.
		 */
		if (kvm && ____sev_es_guest(kvm))
			return false;
#endif
		break;
	default:
		break;
	}

	return true;
}

static void svm_vcpu_after_set_cpuid(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);