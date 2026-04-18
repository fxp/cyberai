// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 59/86



	kvm_pmu_ops_update(ops->pmu_ops);
}

static int kvm_x86_check_processor_compatibility(void)
{
	int cpu = smp_processor_id();
	struct cpuinfo_x86 *c = &cpu_data(cpu);

	/*
	 * Compatibility checks are done when loading KVM and when enabling
	 * hardware, e.g. during CPU hotplug, to ensure all online CPUs are
	 * compatible, i.e. KVM should never perform a compatibility check on
	 * an offline CPU.
	 */
	WARN_ON(!cpu_online(cpu));

	if (__cr4_reserved_bits(cpu_has, c) !=
	    __cr4_reserved_bits(cpu_has, &boot_cpu_data))
		return -EIO;

	return kvm_x86_call(check_processor_compatibility)();
}

static void kvm_x86_check_cpu_compat(void *ret)
{
	*(int *)ret = kvm_x86_check_processor_compatibility();
}

int kvm_x86_vendor_init(struct kvm_x86_init_ops *ops)
{
	u64 host_pat;
	int r, cpu;

	guard(mutex)(&vendor_module_lock);

	if (kvm_x86_ops.enable_virtualization_cpu) {
		pr_err("already loaded vendor module '%s'\n", kvm_x86_ops.name);
		return -EEXIST;
	}

	/*
	 * KVM explicitly assumes that the guest has an FPU and
	 * FXSAVE/FXRSTOR. For example, the KVM_GET_FPU explicitly casts the
	 * vCPU's FPU state as a fxregs_state struct.
	 */
	if (!boot_cpu_has(X86_FEATURE_FPU) || !boot_cpu_has(X86_FEATURE_FXSR)) {
		pr_err("inadequate fpu\n");
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_PREEMPT_RT) && !boot_cpu_has(X86_FEATURE_CONSTANT_TSC)) {
		pr_err("RT requires X86_FEATURE_CONSTANT_TSC\n");
		return -EOPNOTSUPP;
	}

	/*
	 * KVM assumes that PAT entry '0' encodes WB memtype and simply zeroes
	 * the PAT bits in SPTEs.  Bail if PAT[0] is programmed to something
	 * other than WB.  Note, EPT doesn't utilize the PAT, but don't bother
	 * with an exception.  PAT[0] is set to WB on RESET and also by the
	 * kernel, i.e. failure indicates a kernel bug or broken firmware.
	 */
	if (rdmsrq_safe(MSR_IA32_CR_PAT, &host_pat) ||
	    (host_pat & GENMASK(2, 0)) != 6) {
		pr_err("host PAT[0] is not WB\n");
		return -EIO;
	}

	if (boot_cpu_has(X86_FEATURE_SHSTK) || boot_cpu_has(X86_FEATURE_IBT)) {
		rdmsrq(MSR_IA32_S_CET, kvm_host.s_cet);
		/*
		 * Linux doesn't yet support supervisor shadow stacks (SSS), so
		 * KVM doesn't save/restore the associated MSRs, i.e. KVM may
		 * clobber the host values.  Yell and refuse to load if SSS is
		 * unexpectedly enabled, e.g. to avoid crashing the host.
		 */
		if (WARN_ON_ONCE(kvm_host.s_cet & CET_SHSTK_EN))
			return -EIO;
	}

	memset(&kvm_caps, 0, sizeof(kvm_caps));

	x86_emulator_cache = kvm_alloc_emulator_cache();
	if (!x86_emulator_cache) {
		pr_err("failed to allocate cache for x86 emulator\n");
		return -ENOMEM;
	}

	r = kvm_mmu_vendor_module_init();
	if (r)
		goto out_free_x86_emulator_cache;

	kvm_caps.supported_vm_types = BIT(KVM_X86_DEFAULT_VM);
	kvm_caps.supported_mce_cap = MCG_CTL_P | MCG_SER_P;

	if (boot_cpu_has(X86_FEATURE_XSAVE)) {
		kvm_host.xcr0 = xgetbv(XCR_XFEATURE_ENABLED_MASK);
		kvm_caps.supported_xcr0 = kvm_host.xcr0 & KVM_SUPPORTED_XCR0;
	}

	if (boot_cpu_has(X86_FEATURE_XSAVES)) {
		rdmsrq(MSR_IA32_XSS, kvm_host.xss);
		kvm_caps.supported_xss = kvm_host.xss & KVM_SUPPORTED_XSS;
	}

	kvm_caps.supported_quirks = KVM_X86_VALID_QUIRKS;
	kvm_caps.inapplicable_quirks = KVM_X86_CONDITIONAL_QUIRKS;

	rdmsrq_safe(MSR_EFER, &kvm_host.efer);

	kvm_init_pmu_capability(ops->pmu_ops);

	if (boot_cpu_has(X86_FEATURE_ARCH_CAPABILITIES))
		rdmsrq(MSR_IA32_ARCH_CAPABILITIES, kvm_host.arch_capabilities);

	WARN_ON_ONCE(kvm_nr_uret_msrs);

	r = ops->hardware_setup();
	if (r != 0)
		goto out_mmu_exit;

	kvm_setup_efer_caps();

	enable_device_posted_irqs &= enable_apicv &&
				     irq_remapping_cap(IRQ_POSTING_CAP);

	kvm_ops_update(ops);

	for_each_online_cpu(cpu) {
		smp_call_function_single(cpu, kvm_x86_check_cpu_compat, &r, 1);
		if (r < 0)
			goto out_unwind_ops;
	}

	/*
	 * Point of no return!  DO NOT add error paths below this point unless
	 * absolutely necessary, as most operations from this point forward
	 * require unwinding.
	 */
	kvm_timer_init();

	if (pi_inject_timer == -1)
		pi_inject_timer = housekeeping_enabled(HK_TYPE_TIMER);
#ifdef CONFIG_X86_64
	pvclock_gtod_register_notifier(&pvclock_gtod_notifier);

	if (hypervisor_is_type(X86_HYPER_MS_HYPERV))
		set_hv_tscchange_cb(kvm_hyperv_tsc_notifier);
#endif

	__kvm_register_perf_callbacks(ops->handle_intel_pt_intr,
				      enable_mediated_pmu ? kvm_handle_guest_mediated_pmi : NULL);

	if (IS_ENABLED(CONFIG_KVM_SW_PROTECTED_VM) && tdp_mmu_enabled)
		kvm_caps.supported_vm_types |= BIT(KVM_X86_SW_PROTECTED_VM);

	/* KVM always ignores guest PAT for shadow paging.  */
	if (!tdp_enabled)
		kvm_caps.supported_quirks &= ~KVM_X86_QUIRK_IGNORE_GUEST_PAT;