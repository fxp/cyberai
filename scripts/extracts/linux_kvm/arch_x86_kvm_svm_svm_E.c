// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 5/36



	if (type & MSR_TYPE_W) {
		if (!set && kvm_msr_allowed(vcpu, msr, KVM_MSR_FILTER_WRITE))
			svm_clear_msr_bitmap_write(msrpm, msr);
		else
			svm_set_msr_bitmap_write(msrpm, msr);
	}

	svm_hv_vmcb_dirty_nested_enlightenments(vcpu);
	svm->nested.force_msr_bitmap_recalc = true;
}

void *svm_alloc_permissions_map(unsigned long size, gfp_t gfp_mask)
{
	unsigned int order = get_order(size);
	struct page *pages = alloc_pages(gfp_mask, order);
	void *pm;

	if (!pages)
		return NULL;

	/*
	 * Set all bits in the permissions map so that all MSR and I/O accesses
	 * are intercepted by default.
	 */
	pm = page_address(pages);
	memset(pm, 0xff, PAGE_SIZE * (1 << order));

	return pm;
}

static void svm_recalc_lbr_msr_intercepts(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	bool intercept = !(svm->vmcb->control.misc_ctl2 & SVM_MISC2_ENABLE_V_LBR);

	if (intercept == svm->lbr_msrs_intercepted)
		return;

	svm_set_intercept_for_msr(vcpu, MSR_IA32_LASTBRANCHFROMIP, MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_IA32_LASTBRANCHTOIP, MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_IA32_LASTINTFROMIP, MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_IA32_LASTINTTOIP, MSR_TYPE_RW, intercept);

	if (is_sev_es_guest(vcpu))
		svm_set_intercept_for_msr(vcpu, MSR_IA32_DEBUGCTLMSR, MSR_TYPE_RW, intercept);

	svm->lbr_msrs_intercepted = intercept;
}

void svm_vcpu_free_msrpm(void *msrpm)
{
	__free_pages(virt_to_page(msrpm), get_order(MSRPM_SIZE));
}

static void svm_recalc_pmu_msr_intercepts(struct kvm_vcpu *vcpu)
{
	bool intercept = !kvm_vcpu_has_mediated_pmu(vcpu);
	struct kvm_pmu *pmu = vcpu_to_pmu(vcpu);
	int i;

	if (!enable_mediated_pmu)
		return;

	/* Legacy counters are always available for AMD CPUs with a PMU. */
	for (i = 0; i < min(pmu->nr_arch_gp_counters, AMD64_NUM_COUNTERS); i++)
		svm_set_intercept_for_msr(vcpu, MSR_K7_PERFCTR0 + i,
					  MSR_TYPE_RW, intercept);

	intercept |= !guest_cpu_cap_has(vcpu, X86_FEATURE_PERFCTR_CORE);
	for (i = 0; i < pmu->nr_arch_gp_counters; i++)
		svm_set_intercept_for_msr(vcpu, MSR_F15H_PERF_CTR + 2 * i,
					  MSR_TYPE_RW, intercept);

	for ( ; i < kvm_pmu_cap.num_counters_gp; i++)
		svm_enable_intercept_for_msr(vcpu, MSR_F15H_PERF_CTR + 2 * i,
					     MSR_TYPE_RW);

	intercept = kvm_need_perf_global_ctrl_intercept(vcpu);
	svm_set_intercept_for_msr(vcpu, MSR_AMD64_PERF_CNTR_GLOBAL_CTL,
				  MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_AMD64_PERF_CNTR_GLOBAL_STATUS,
				  MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_AMD64_PERF_CNTR_GLOBAL_STATUS_CLR,
				  MSR_TYPE_RW, intercept);
	svm_set_intercept_for_msr(vcpu, MSR_AMD64_PERF_CNTR_GLOBAL_STATUS_SET,
				  MSR_TYPE_RW, intercept);
}

static void svm_recalc_msr_intercepts(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm_disable_intercept_for_msr(vcpu, MSR_STAR, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_CS, MSR_TYPE_RW);

#ifdef CONFIG_X86_64
	svm_disable_intercept_for_msr(vcpu, MSR_GS_BASE, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_FS_BASE, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_KERNEL_GS_BASE, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_LSTAR, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_CSTAR, MSR_TYPE_RW);
	svm_disable_intercept_for_msr(vcpu, MSR_SYSCALL_MASK, MSR_TYPE_RW);
#endif

	if (lbrv)
		svm_recalc_lbr_msr_intercepts(vcpu);

	if (cpu_feature_enabled(X86_FEATURE_IBPB))
		svm_set_intercept_for_msr(vcpu, MSR_IA32_PRED_CMD, MSR_TYPE_W,
					  !guest_has_pred_cmd_msr(vcpu));

	if (cpu_feature_enabled(X86_FEATURE_FLUSH_L1D))
		svm_set_intercept_for_msr(vcpu, MSR_IA32_FLUSH_CMD, MSR_TYPE_W,
					  !guest_cpu_cap_has(vcpu, X86_FEATURE_FLUSH_L1D));

	/*
	 * Disable interception of SPEC_CTRL if KVM doesn't need to manually
	 * context switch the MSR (SPEC_CTRL is virtualized by the CPU), or if
	 * the guest has a non-zero SPEC_CTRL value, i.e. is likely actively
	 * using SPEC_CTRL.
	 */
	if (cpu_feature_enabled(X86_FEATURE_V_SPEC_CTRL))
		svm_set_intercept_for_msr(vcpu, MSR_IA32_SPEC_CTRL, MSR_TYPE_RW,
					  !guest_has_spec_ctrl_msr(vcpu));
	else
		svm_set_intercept_for_msr(vcpu, MSR_IA32_SPEC_CTRL, MSR_TYPE_RW,
					  !svm->spec_ctrl);

	/*
	 * Intercept SYSENTER_EIP and SYSENTER_ESP when emulating an Intel CPU,
	 * as AMD hardware only store 32 bits, whereas Intel CPUs track 64 bits.
	 */
	svm_set_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_EIP, MSR_TYPE_RW,
				  guest_cpuid_is_intel_compatible(vcpu));
	svm_set_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_ESP, MSR_TYPE_RW,
				  guest_cpuid_is_intel_compatible(vcpu));

	if (kvm_aperfmperf_in_guest(vcpu->kvm)) {
		svm_disable_intercept_for_msr(vcpu, MSR_IA32_APERF, MSR_TYPE_R);
		svm_disable_intercept_for_msr(vcpu, MSR_IA32_MPERF, MSR_TYPE_R);
	}

	if (kvm_cpu_cap_has(X86_FEATURE_SHSTK)) {
		bool shstk_enabled = guest_cpu_cap_has(vcpu, X86_FEATURE_SHSTK);