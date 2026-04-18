// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 55/56



	/*
	 * On pre-MKTME system, boot_cpu_data.x86_phys_bits equals to
	 * kvm_host.maxphyaddr.  On MKTME and/or TDX capable systems,
	 * boot_cpu_data.x86_phys_bits holds the actual physical address
	 * w/o the KeyID bits, and kvm_host.maxphyaddr equals to
	 * MAXPHYADDR reported by CPUID.  Those bits between are KeyID bits.
	 */
	if (boot_cpu_data.x86_phys_bits != kvm_host.maxphyaddr)
		me_mask = rsvd_bits(boot_cpu_data.x86_phys_bits,
				    kvm_host.maxphyaddr - 1);

	/*
	 * Unlike SME, host kernel doesn't support setting up any
	 * MKTME KeyID on Intel platforms.  No memory encryption
	 * bits should be included into the SPTE.
	 */
	kvm_mmu_set_me_spte_mask(0, me_mask);
}

__init int vmx_hardware_setup(void)
{
	unsigned long host_bndcfgs;
	struct desc_ptr dt;
	int r;

	store_idt(&dt);
	host_idt_base = dt.address;

	vmx_setup_user_return_msrs();

	if (boot_cpu_has(X86_FEATURE_MPX)) {
		rdmsrq(MSR_IA32_BNDCFGS, host_bndcfgs);
		WARN_ONCE(host_bndcfgs, "BNDCFGS in host will be lost");
	}

	if (!cpu_has_vmx_mpx())
		kvm_caps.supported_xcr0 &= ~(XFEATURE_MASK_BNDREGS |
					     XFEATURE_MASK_BNDCSR);

	if (!cpu_has_vmx_vpid() || !cpu_has_vmx_invvpid() ||
	    !(cpu_has_vmx_invvpid_single() || cpu_has_vmx_invvpid_global()))
		enable_vpid = 0;

	if (!cpu_has_vmx_ept() ||
	    !cpu_has_vmx_ept_4levels() ||
	    !cpu_has_vmx_ept_mt_wb() ||
	    !cpu_has_vmx_invept_global())
		enable_ept = 0;

	/* NX support is required for shadow paging. */
	if (!enable_ept && !boot_cpu_has(X86_FEATURE_NX)) {
		pr_err_ratelimited("NX (Execute Disable) not supported\n");
		return -EOPNOTSUPP;
	}

	/*
	 * Shadow paging doesn't have a (further) performance penalty
	 * from GUEST_MAXPHYADDR < HOST_MAXPHYADDR so enable it
	 * by default
	 */
	if (!enable_ept)
		allow_smaller_maxphyaddr = true;

	if (!cpu_has_vmx_ept_ad_bits() || !enable_ept)
		enable_ept_ad_bits = 0;

	if (!cpu_has_vmx_unrestricted_guest() || !enable_ept)
		enable_unrestricted_guest = 0;

	if (!cpu_has_vmx_flexpriority())
		flexpriority_enabled = 0;

	if (!cpu_has_virtual_nmis())
		enable_vnmi = 0;

#ifdef CONFIG_X86_SGX_KVM
	if (!cpu_has_vmx_encls_vmexit())
		enable_sgx = false;
#endif

	/*
	 * set_apic_access_page_addr() is used to reload apic access
	 * page upon invalidation.  No need to do anything if not
	 * using the APIC_ACCESS_ADDR VMCS field.
	 */
	if (!flexpriority_enabled)
		vt_x86_ops.set_apic_access_page_addr = NULL;

	if (!cpu_has_vmx_tpr_shadow())
		vt_x86_ops.update_cr8_intercept = NULL;

#if IS_ENABLED(CONFIG_HYPERV)
	if (ms_hyperv.nested_features & HV_X64_NESTED_GUEST_MAPPING_FLUSH
	    && enable_ept) {
		vt_x86_ops.flush_remote_tlbs = hv_flush_remote_tlbs;
		vt_x86_ops.flush_remote_tlbs_range = hv_flush_remote_tlbs_range;
	}
#endif

	if (!cpu_has_vmx_ple()) {
		ple_gap = 0;
		ple_window = 0;
		ple_window_grow = 0;
		ple_window_max = 0;
		ple_window_shrink = 0;
	}

	if (!cpu_has_vmx_apicv())
		enable_apicv = 0;
	if (!enable_apicv)
		vt_x86_ops.sync_pir_to_irr = NULL;

	if (!enable_apicv || !cpu_has_vmx_ipiv())
		enable_ipiv = false;

	if (cpu_has_vmx_tsc_scaling())
		kvm_caps.has_tsc_control = true;

	kvm_caps.max_tsc_scaling_ratio = KVM_VMX_TSC_MULTIPLIER_MAX;
	kvm_caps.tsc_scaling_ratio_frac_bits = 48;
	kvm_caps.has_bus_lock_exit = cpu_has_vmx_bus_lock_detection();
	kvm_caps.has_notify_vmexit = cpu_has_notify_vmexit();

	set_bit(0, vmx_vpid_bitmap); /* 0 is reserved for host */

	if (enable_ept)
		kvm_mmu_set_ept_masks(enable_ept_ad_bits,
				      cpu_has_vmx_ept_execute_only());
	else
		vt_x86_ops.get_mt_mask = NULL;

	/*
	 * Setup shadow_me_value/shadow_me_mask to include MKTME KeyID
	 * bits to shadow_zero_check.
	 */
	vmx_setup_me_spte_mask();

	kvm_configure_mmu(enable_ept, 0, vmx_get_max_ept_level(),
			  ept_caps_to_lpage_level(vmx_capability.ept));

	/*
	 * Only enable PML when hardware supports PML feature, and both EPT
	 * and EPT A/D bit features are enabled -- PML depends on them to work.
	 */
	if (!enable_ept || !enable_ept_ad_bits || !cpu_has_vmx_pml())
		enable_pml = 0;

	if (!cpu_has_vmx_preemption_timer())
		enable_preemption_timer = false;

	if (enable_preemption_timer) {
		u64 use_timer_freq = 5000ULL * 1000 * 1000;

		cpu_preemption_timer_multi =
			vmx_misc_preemption_timer_rate(vmcs_config.misc);

		if (tsc_khz)
			use_timer_freq = (u64)tsc_khz * 1000;
		use_timer_freq >>= cpu_preemption_timer_multi;

		/*
		 * KVM "disables" the preemption timer by setting it to its max
		 * value.  Don't use the timer if it might cause spurious exits
		 * at a rate faster than 0.1 Hz (of uninterrupted guest time).
		 */
		if (use_timer_freq > 0xffffffffu / 10)
			enable_preemption_timer = false;
	}

	if (!enable_preemption_timer) {
		vt_x86_ops.set_hv_timer = NULL;
		vt_x86_ops.cancel_hv_timer = NULL;
	}

	kvm_caps.supported_mce_cap |= MCG_LMCE_P;
	kvm_caps.supported_mce_cap |= MCG_CMCI_P;