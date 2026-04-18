// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 35/36



	/*
	 * If the mask bit location is below 52, then some bits above the
	 * physical addressing limit will always be reserved, so use the
	 * rsvd_bits() function to generate the mask. This mask, along with
	 * the present bit, will be used to generate a page fault with
	 * PFER.RSV = 1.
	 *
	 * If the mask bit location is 52 (or above), then clear the mask.
	 */
	mask = (mask_bit < 52) ? rsvd_bits(mask_bit, 51) | PT_PRESENT_MASK : 0;

	kvm_mmu_set_mmio_spte_mask(mask, mask, PT_WRITABLE_MASK | PT_USER_MASK);
}

static __init void svm_set_cpu_caps(void)
{
	kvm_initialize_cpu_caps();

	kvm_caps.supported_perf_cap = 0;

	kvm_cpu_cap_clear(X86_FEATURE_IBT);

	/* CPUID 0x80000001 and 0x8000000A (SVM features) */
	if (nested) {
		kvm_cpu_cap_set(X86_FEATURE_SVM);
		kvm_cpu_cap_set(X86_FEATURE_VMCBCLEAN);

		/*
		 * KVM currently flushes TLBs on *every* nested SVM transition,
		 * and so for all intents and purposes KVM supports flushing by
		 * ASID, i.e. KVM is guaranteed to honor every L1 ASID flush.
		 */
		kvm_cpu_cap_set(X86_FEATURE_FLUSHBYASID);

		if (nrips)
			kvm_cpu_cap_set(X86_FEATURE_NRIPS);

		if (npt_enabled)
			kvm_cpu_cap_set(X86_FEATURE_NPT);

		if (tsc_scaling)
			kvm_cpu_cap_set(X86_FEATURE_TSCRATEMSR);

		if (vls)
			kvm_cpu_cap_set(X86_FEATURE_V_VMSAVE_VMLOAD);
		if (lbrv)
			kvm_cpu_cap_set(X86_FEATURE_LBRV);

		if (boot_cpu_has(X86_FEATURE_PAUSEFILTER))
			kvm_cpu_cap_set(X86_FEATURE_PAUSEFILTER);

		if (boot_cpu_has(X86_FEATURE_PFTHRESHOLD))
			kvm_cpu_cap_set(X86_FEATURE_PFTHRESHOLD);

		if (vgif)
			kvm_cpu_cap_set(X86_FEATURE_VGIF);

		if (vnmi)
			kvm_cpu_cap_set(X86_FEATURE_VNMI);

		/* Nested VM can receive #VMEXIT instead of triggering #GP */
		kvm_cpu_cap_set(X86_FEATURE_SVME_ADDR_CHK);
	}

	if (cpu_feature_enabled(X86_FEATURE_BUS_LOCK_THRESHOLD))
		kvm_caps.has_bus_lock_exit = true;

	/* CPUID 0x80000008 */
	if (boot_cpu_has(X86_FEATURE_LS_CFG_SSBD) ||
	    boot_cpu_has(X86_FEATURE_AMD_SSBD))
		kvm_cpu_cap_set(X86_FEATURE_VIRT_SSBD);

	if (enable_pmu) {
		/*
		 * Enumerate support for PERFCTR_CORE if and only if KVM has
		 * access to enough counters to virtualize "core" support,
		 * otherwise limit vPMU support to the legacy number of counters.
		 */
		if (kvm_pmu_cap.num_counters_gp < AMD64_NUM_COUNTERS_CORE)
			kvm_pmu_cap.num_counters_gp = min(AMD64_NUM_COUNTERS,
							  kvm_pmu_cap.num_counters_gp);
		else
			kvm_cpu_cap_check_and_set(X86_FEATURE_PERFCTR_CORE);

		if (kvm_pmu_cap.version != 2 ||
		    !kvm_cpu_cap_has(X86_FEATURE_PERFCTR_CORE))
			kvm_cpu_cap_clear(X86_FEATURE_PERFMON_V2);
	}

	/* CPUID 0x8000001F (SME/SEV features) */
	sev_set_cpu_caps();

	/*
	 * Clear capabilities that are automatically configured by common code,
	 * but that require explicit SVM support (that isn't yet implemented).
	 */
	kvm_cpu_cap_clear(X86_FEATURE_BUS_LOCK_DETECT);
	kvm_cpu_cap_clear(X86_FEATURE_MSR_IMM);

	kvm_setup_xss_caps();
	kvm_finalize_cpu_caps();
}

static __init int svm_hardware_setup(void)
{
	void *iopm_va;
	int cpu, r;

	/*
	 * NX is required for shadow paging and for NPT if the NX huge pages
	 * mitigation is enabled.
	 */
	if (!boot_cpu_has(X86_FEATURE_NX)) {
		pr_err_ratelimited("NX (Execute Disable) not supported\n");
		return -EOPNOTSUPP;
	}

	kvm_caps.supported_xcr0 &= ~(XFEATURE_MASK_BNDREGS |
				     XFEATURE_MASK_BNDCSR);

	if (tsc_scaling) {
		if (!boot_cpu_has(X86_FEATURE_TSCRATEMSR)) {
			tsc_scaling = false;
		} else {
			pr_info("TSC scaling supported\n");
			kvm_caps.has_tsc_control = true;
		}
	}
	kvm_caps.max_tsc_scaling_ratio = SVM_TSC_RATIO_MAX;
	kvm_caps.tsc_scaling_ratio_frac_bits = 32;

	tsc_aux_uret_slot = kvm_add_user_return_msr(MSR_TSC_AUX);

	/* Check for pause filtering support */
	if (!boot_cpu_has(X86_FEATURE_PAUSEFILTER)) {
		pause_filter_count = 0;
		pause_filter_thresh = 0;
	} else if (!boot_cpu_has(X86_FEATURE_PFTHRESHOLD)) {
		pause_filter_thresh = 0;
	}

	if (nested) {
		pr_info("Nested Virtualization enabled\n");
		kvm_enable_efer_bits(EFER_SVME);
		if (!boot_cpu_has(X86_FEATURE_EFER_LMSLE_MBZ))
			kvm_enable_efer_bits(EFER_LMSLE);

		r = nested_svm_init_msrpm_merge_offsets();
		if (r)
			return r;
	}

	/*
	 * KVM's MMU doesn't support using 2-level paging for itself, and thus
	 * NPT isn't supported if the host is using 2-level paging since host
	 * CR4 is unchanged on VMRUN.
	 */
	if (!IS_ENABLED(CONFIG_X86_64) && !IS_ENABLED(CONFIG_X86_PAE))
		npt_enabled = false;

	if (!boot_cpu_has(X86_FEATURE_NPT))
		npt_enabled = false;

	/* Force VM NPT level equal to the host's paging level */
	kvm_configure_mmu(npt_enabled, get_npt_level(),
			  get_npt_level(), PG_LEVEL_1G);
	pr_info("Nested Paging %s\n", str_enabled_disabled(npt_enabled));