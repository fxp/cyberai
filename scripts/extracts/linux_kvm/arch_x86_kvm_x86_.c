// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 76/86



	if (kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_STUFF_FEATURE_MSRS)) {
		vcpu->arch.arch_capabilities = kvm_get_arch_capabilities();
		vcpu->arch.msr_platform_info = MSR_PLATFORM_INFO_CPUID_FAULT;
		vcpu->arch.perf_capabilities = kvm_caps.supported_perf_cap;
	}
	kvm_pmu_init(vcpu);

	vcpu->arch.pending_external_vector = -1;
	vcpu->arch.preempted_in_kernel = false;

#if IS_ENABLED(CONFIG_HYPERV)
	vcpu->arch.hv_root_tdp = INVALID_PAGE;
#endif

	r = kvm_x86_call(vcpu_create)(vcpu);
	if (r)
		goto free_guest_fpu;

	kvm_xen_init_vcpu(vcpu);
	vcpu_load(vcpu);
	kvm_vcpu_after_set_cpuid(vcpu);
	kvm_set_tsc_khz(vcpu, vcpu->kvm->arch.default_tsc_khz);
	kvm_vcpu_reset(vcpu, false);
	kvm_init_mmu(vcpu);
	vcpu_put(vcpu);
	return 0;

free_guest_fpu:
	fpu_free_guest_fpstate(&vcpu->arch.guest_fpu);
free_emulate_ctxt:
	kmem_cache_free(x86_emulator_cache, vcpu->arch.emulate_ctxt);
free_wbinvd_dirty_mask:
	free_cpumask_var(vcpu->arch.wbinvd_dirty_mask);
fail_free_mce_banks:
	kfree(vcpu->arch.mce_banks);
	kfree(vcpu->arch.mci_ctl2_banks);
	free_page((unsigned long)vcpu->arch.pio_data);
fail_free_lapic:
	kvm_free_lapic(vcpu);
fail_mmu_destroy:
	kvm_mmu_destroy(vcpu);
	return r;
}

void kvm_arch_vcpu_postcreate(struct kvm_vcpu *vcpu)
{
	if (mutex_lock_killable(&vcpu->mutex))
		return;
	vcpu_load(vcpu);
	kvm_synchronize_tsc(vcpu, NULL);
	vcpu_put(vcpu);

	/* poll control enabled by default */
	vcpu->arch.msr_kvm_poll_control = 1;

	mutex_unlock(&vcpu->mutex);
}

void kvm_arch_vcpu_destroy(struct kvm_vcpu *vcpu)
{
	int idx, cpu;

	kvm_clear_async_pf_completion_queue(vcpu);
	kvm_mmu_unload(vcpu);

	kvmclock_reset(vcpu);

	for_each_possible_cpu(cpu)
		cmpxchg(per_cpu_ptr(&last_vcpu, cpu), vcpu, NULL);

	kvm_x86_call(vcpu_free)(vcpu);

	kmem_cache_free(x86_emulator_cache, vcpu->arch.emulate_ctxt);
	free_cpumask_var(vcpu->arch.wbinvd_dirty_mask);
	fpu_free_guest_fpstate(&vcpu->arch.guest_fpu);

	kvm_xen_destroy_vcpu(vcpu);
	kvm_hv_vcpu_uninit(vcpu);
	kvm_pmu_destroy(vcpu);
	kfree(vcpu->arch.mce_banks);
	kfree(vcpu->arch.mci_ctl2_banks);
	kvm_free_lapic(vcpu);
	idx = srcu_read_lock(&vcpu->kvm->srcu);
	kvm_mmu_destroy(vcpu);
	srcu_read_unlock(&vcpu->kvm->srcu, idx);
	free_page((unsigned long)vcpu->arch.pio_data);
	kvfree(vcpu->arch.cpuid_entries);
}

static void kvm_xstate_reset(struct kvm_vcpu *vcpu, bool init_event)
{
	struct fpstate *fpstate = vcpu->arch.guest_fpu.fpstate;
	u64 xfeatures_mask;
	bool fpu_in_use;
	int i;

	/*
	 * Guest FPU state is zero allocated and so doesn't need to be manually
	 * cleared on RESET, i.e. during vCPU creation.
	 */
	if (!init_event || !fpstate)
		return;

	/*
	 * On INIT, only select XSTATE components are zeroed, most components
	 * are unchanged.  Currently, the only components that are zeroed and
	 * supported by KVM are MPX and CET related.
	 */
	xfeatures_mask = (kvm_caps.supported_xcr0 | kvm_caps.supported_xss) &
			 (XFEATURE_MASK_BNDREGS | XFEATURE_MASK_BNDCSR |
			  XFEATURE_MASK_CET_ALL);
	if (!xfeatures_mask)
		return;

	BUILD_BUG_ON(sizeof(xfeatures_mask) * BITS_PER_BYTE <= XFEATURE_MAX);

	/*
	 * Unload guest FPU state (if necessary) before zeroing XSTATE fields
	 * as the kernel can only modify the state when its resident in memory,
	 * i.e. when it's not loaded into hardware.
	 *
	 * WARN if the vCPU's desire to run, i.e. whether or not its in KVM_RUN,
	 * doesn't match the loaded/in-use state of the FPU, as KVM_RUN is the
	 * only path that can trigger INIT emulation _and_ loads FPU state, and
	 * KVM_RUN should _always_ load FPU state.
	 */
	WARN_ON_ONCE(vcpu->wants_to_run != fpstate->in_use);
	fpu_in_use = fpstate->in_use;
	if (fpu_in_use)
		kvm_put_guest_fpu(vcpu);
	for_each_set_bit(i, (unsigned long *)&xfeatures_mask, XFEATURE_MAX)
		fpstate_clear_xstate_component(fpstate, i);
	if (fpu_in_use)
		kvm_load_guest_fpu(vcpu);
}

void kvm_vcpu_reset(struct kvm_vcpu *vcpu, bool init_event)
{
	struct kvm_cpuid_entry2 *cpuid_0x1;
	unsigned long old_cr0 = kvm_read_cr0(vcpu);
	unsigned long new_cr0;

	/*
	 * Several of the "set" flows, e.g. ->set_cr0(), read other registers
	 * to handle side effects.  RESET emulation hits those flows and relies
	 * on emulated/virtualized registers, including those that are loaded
	 * into hardware, to be zeroed at vCPU creation.  Use CRs as a sentinel
	 * to detect improper or missing initialization.
	 */
	WARN_ON_ONCE(!init_event &&
		     (old_cr0 || kvm_read_cr3(vcpu) || kvm_read_cr4(vcpu)));

	/*
	 * SVM doesn't unconditionally VM-Exit on INIT and SHUTDOWN, thus it's
	 * possible to INIT the vCPU while L2 is active.  Force the vCPU back
	 * into L1 as EFER.SVME is cleared on INIT (along with all other EFER
	 * bits), i.e. virtualization is disabled.
	 */
	if (is_guest_mode(vcpu))
		kvm_leave_nested(vcpu);

	kvm_lapic_reset(vcpu, init_event);

	WARN_ON_ONCE(is_guest_mode(vcpu) || is_smm(vcpu));
	vcpu->arch.hflags = 0;