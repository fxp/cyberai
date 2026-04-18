// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 36/36



	/*
	 * It seems that on AMD processors PTE's accessed bit is
	 * being set by the CPU hardware before the NPF vmexit.
	 * This is not expected behaviour and our tests fail because
	 * of it.
	 * A workaround here is to disable support for
	 * GUEST_MAXPHYADDR < HOST_MAXPHYADDR if NPT is enabled.
	 * In this case userspace can know if there is support using
	 * KVM_CAP_SMALLER_MAXPHYADDR extension and decide how to handle
	 * it
	 * If future AMD CPU models change the behaviour described above,
	 * this variable can be changed accordingly
	 */
	allow_smaller_maxphyaddr = !npt_enabled;

	/* Setup shadow_me_value and shadow_me_mask */
	kvm_mmu_set_me_spte_mask(sme_me_mask, sme_me_mask);

	svm_adjust_mmio_mask();

	nrips = nrips && boot_cpu_has(X86_FEATURE_NRIPS);

	if (lbrv) {
		if (!boot_cpu_has(X86_FEATURE_LBRV))
			lbrv = false;
		else
			pr_info("LBR virtualization supported\n");
	}

	iopm_va = svm_alloc_permissions_map(IOPM_SIZE, GFP_KERNEL);
	if (!iopm_va)
		return -ENOMEM;

	iopm_base = __sme_set(__pa(iopm_va));

	/*
	 * Note, SEV setup consumes npt_enabled and enable_mmio_caching (which
	 * may be modified by svm_adjust_mmio_mask()), as well as nrips.
	 */
	sev_hardware_setup();

	svm_hv_hardware_setup();

	enable_apicv = avic_hardware_setup();
	if (!enable_apicv) {
		enable_ipiv = false;
		svm_x86_ops.vcpu_blocking = NULL;
		svm_x86_ops.vcpu_unblocking = NULL;
		svm_x86_ops.vcpu_get_apicv_inhibit_reasons = NULL;
	}

	if (vls) {
		if (!npt_enabled ||
		    !boot_cpu_has(X86_FEATURE_V_VMSAVE_VMLOAD) ||
		    !IS_ENABLED(CONFIG_X86_64)) {
			vls = false;
		} else {
			pr_info("Virtual VMLOAD VMSAVE supported\n");
		}
	}

	if (boot_cpu_has(X86_FEATURE_SVME_ADDR_CHK))
		svm_gp_erratum_intercept = false;

	if (vgif) {
		if (!boot_cpu_has(X86_FEATURE_VGIF))
			vgif = false;
		else
			pr_info("Virtual GIF supported\n");
	}

	vnmi = vgif && vnmi && boot_cpu_has(X86_FEATURE_VNMI);
	if (vnmi)
		pr_info("Virtual NMI enabled\n");

	if (!vnmi) {
		svm_x86_ops.is_vnmi_pending = NULL;
		svm_x86_ops.set_vnmi_pending = NULL;
	}

	if (!enable_pmu)
		pr_info("PMU virtualization is disabled\n");

	svm_set_cpu_caps();

	kvm_caps.inapplicable_quirks &= ~KVM_X86_QUIRK_CD_NW_CLEARED;

	for_each_possible_cpu(cpu) {
		r = svm_cpu_init(cpu);
		if (r)
			goto err;
	}

	return 0;

err:
	svm_hardware_unsetup();
	return r;
}


static struct kvm_x86_init_ops svm_init_ops __initdata = {
	.hardware_setup = svm_hardware_setup,

	.runtime_ops = &svm_x86_ops,
	.pmu_ops = &amd_pmu_ops,
};

static void __svm_exit(void)
{
	kvm_x86_vendor_exit();
}

static int __init svm_init(void)
{
	int r;

	KVM_SANITY_CHECK_VM_STRUCT_SIZE(kvm_svm);

	__unused_size_checks();

	if (!kvm_is_svm_supported())
		return -EOPNOTSUPP;

	r = kvm_x86_vendor_init(&svm_init_ops);
	if (r)
		return r;

	/*
	 * Common KVM initialization _must_ come last, after this, /dev/kvm is
	 * exposed to userspace!
	 */
	r = kvm_init(sizeof(struct vcpu_svm), __alignof__(struct vcpu_svm),
		     THIS_MODULE);
	if (r)
		goto err_kvm_init;

	return 0;

err_kvm_init:
	__svm_exit();
	return r;
}

static void __exit svm_exit(void)
{
	kvm_exit();
	__svm_exit();
}

module_init(svm_init)
module_exit(svm_exit)
