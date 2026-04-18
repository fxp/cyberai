// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 56/56



	if (pt_mode != PT_MODE_SYSTEM && pt_mode != PT_MODE_HOST_GUEST)
		return -EINVAL;
	if (!enable_ept || !enable_pmu || !cpu_has_vmx_intel_pt())
		pt_mode = PT_MODE_SYSTEM;
	if (pt_mode == PT_MODE_HOST_GUEST)
		vt_init_ops.handle_intel_pt_intr = vmx_handle_intel_pt_intr;
	else
		vt_init_ops.handle_intel_pt_intr = NULL;

	setup_default_sgx_lepubkeyhash();

	vmx_set_cpu_caps();

	/*
	 * Configure nested capabilities after core CPU capabilities so that
	 * nested support can be conditional on base support, e.g. so that KVM
	 * can hide/show features based on kvm_cpu_cap_has().
	 */
	if (nested) {
		r = nested_vmx_hardware_setup(kvm_vmx_exit_handlers);
		if (r)
			return r;
	}

	kvm_set_posted_intr_wakeup_handler(pi_wakeup_handler);

	/*
	 * On Intel CPUs that lack self-snoop feature, letting the guest control
	 * memory types may result in unexpected behavior. So always ignore guest
	 * PAT on those CPUs and map VM as writeback, not allowing userspace to
	 * disable the quirk.
	 *
	 * On certain Intel CPUs (e.g. SPR, ICX), though self-snoop feature is
	 * supported, UC is slow enough to cause issues with some older guests (e.g.
	 * an old version of bochs driver uses ioremap() instead of ioremap_wc() to
	 * map the video RAM, causing wayland desktop to fail to get started
	 * correctly). To avoid breaking those older guests that rely on KVM to force
	 * memory type to WB, provide KVM_X86_QUIRK_IGNORE_GUEST_PAT to preserve the
	 * safer (for performance) default behavior.
	 *
	 * On top of this, non-coherent DMA devices need the guest to flush CPU
	 * caches properly.  This also requires honoring guest PAT, and is forced
	 * independent of the quirk in vmx_ignore_guest_pat().
	 */
	if (!static_cpu_has(X86_FEATURE_SELFSNOOP))
		kvm_caps.supported_quirks &= ~KVM_X86_QUIRK_IGNORE_GUEST_PAT;

	kvm_caps.inapplicable_quirks &= ~KVM_X86_QUIRK_IGNORE_GUEST_PAT;

	return 0;
}

void vmx_exit(void)
{
	allow_smaller_maxphyaddr = false;

	vmx_cleanup_l1d_flush();

	kvm_x86_vendor_exit();
}

int __init vmx_init(void)
{
	int r, cpu;

	KVM_SANITY_CHECK_VM_STRUCT_SIZE(kvm_vmx);

	if (!kvm_is_vmx_supported())
		return -EOPNOTSUPP;

	/*
	 * Note, VMCS and eVMCS configuration only touch VMX knobs/variables,
	 * i.e. there's nothing to unwind if a later step fails.
	 */
	hv_init_evmcs();

	/*
	 * Parse the VMCS config and VMX capabilities before anything else, so
	 * that the information is available to all setup flows.
	 */
	if (setup_vmcs_config(&vmcs_config, &vmx_capability) < 0)
		return -EIO;

	r = kvm_x86_vendor_init(&vt_init_ops);
	if (r)
		return r;

	/* Must be called after common x86 init so enable_ept is setup. */
	r = vmx_setup_l1d_flush();
	if (r)
		goto err_l1d_flush;

	for_each_possible_cpu(cpu) {
		INIT_LIST_HEAD(&per_cpu(loaded_vmcss_on_cpu, cpu));

		pi_init_cpu(cpu);
	}

	vmx_check_vmcs12_offsets();

	return 0;

err_l1d_flush:
	kvm_x86_vendor_exit();
	return r;
}
