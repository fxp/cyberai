// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 30/56



	/*
	 * RDPID is also gated by ENABLE_RDTSCP, turn on the control if either
	 * feature is exposed to the guest.  This creates a virtualization hole
	 * if both are supported in hardware but only one is exposed to the
	 * guest, but letting the guest execute RDTSCP or RDPID when either one
	 * is advertised is preferable to emulating the advertised instruction
	 * in KVM on #UD, and obviously better than incorrectly injecting #UD.
	 */
	if (cpu_has_vmx_rdtscp()) {
		bool rdpid_or_rdtscp_enabled =
			guest_cpu_cap_has(vcpu, X86_FEATURE_RDTSCP) ||
			guest_cpu_cap_has(vcpu, X86_FEATURE_RDPID);

		vmx_adjust_secondary_exec_control(vmx, &exec_control,
						  SECONDARY_EXEC_ENABLE_RDTSCP,
						  rdpid_or_rdtscp_enabled, false);
	}

	vmx_adjust_sec_exec_feature(vmx, &exec_control, invpcid, INVPCID);

	vmx_adjust_sec_exec_exiting(vmx, &exec_control, rdrand, RDRAND);
	vmx_adjust_sec_exec_exiting(vmx, &exec_control, rdseed, RDSEED);

	vmx_adjust_sec_exec_control(vmx, &exec_control, waitpkg, WAITPKG,
				    ENABLE_USR_WAIT_PAUSE, false);

	if (!vcpu->kvm->arch.bus_lock_detection_enabled)
		exec_control &= ~SECONDARY_EXEC_BUS_LOCK_DETECTION;

	if (!kvm_notify_vmexit_enabled(vcpu->kvm))
		exec_control &= ~SECONDARY_EXEC_NOTIFY_VM_EXITING;

	return exec_control;
}

static inline int vmx_get_pid_table_order(struct kvm *kvm)
{
	return get_order(kvm->arch.max_vcpu_ids * sizeof(*to_kvm_vmx(kvm)->pid_table));
}

static int vmx_alloc_ipiv_pid_table(struct kvm *kvm)
{
	struct page *pages;
	struct kvm_vmx *kvm_vmx = to_kvm_vmx(kvm);

	if (!irqchip_in_kernel(kvm) || !enable_ipiv)
		return 0;

	if (kvm_vmx->pid_table)
		return 0;

	pages = alloc_pages(GFP_KERNEL_ACCOUNT | __GFP_ZERO,
			    vmx_get_pid_table_order(kvm));
	if (!pages)
		return -ENOMEM;

	kvm_vmx->pid_table = (void *)page_address(pages);
	return 0;
}

int vmx_vcpu_precreate(struct kvm *kvm)
{
	return vmx_alloc_ipiv_pid_table(kvm);
}

#define VMX_XSS_EXIT_BITMAP 0

static void init_vmcs(struct vcpu_vmx *vmx)
{
	struct kvm *kvm = vmx->vcpu.kvm;
	struct kvm_vmx *kvm_vmx = to_kvm_vmx(kvm);

	if (nested)
		nested_vmx_set_vmcs_shadowing_bitmap();

	if (cpu_has_vmx_msr_bitmap())
		vmcs_write64(MSR_BITMAP, __pa(vmx->vmcs01.msr_bitmap));

	vmcs_write64(VMCS_LINK_POINTER, INVALID_GPA); /* 22.3.1.5 */

	/* Control */
	pin_controls_set(vmx, vmx_pin_based_exec_ctrl(vmx));

	exec_controls_set(vmx, vmx_exec_control(vmx));

	if (cpu_has_secondary_exec_ctrls()) {
		secondary_exec_controls_set(vmx, vmx_secondary_exec_control(vmx));
		if (vmx->ve_info)
			vmcs_write64(VE_INFORMATION_ADDRESS,
				     __pa(vmx->ve_info));
	}

	if (cpu_has_tertiary_exec_ctrls())
		tertiary_exec_controls_set(vmx, vmx_tertiary_exec_control(vmx));

	if (enable_apicv && lapic_in_kernel(&vmx->vcpu)) {
		vmcs_write64(EOI_EXIT_BITMAP0, 0);
		vmcs_write64(EOI_EXIT_BITMAP1, 0);
		vmcs_write64(EOI_EXIT_BITMAP2, 0);
		vmcs_write64(EOI_EXIT_BITMAP3, 0);

		vmcs_write16(GUEST_INTR_STATUS, 0);

		vmcs_write16(POSTED_INTR_NV, POSTED_INTR_VECTOR);
		vmcs_write64(POSTED_INTR_DESC_ADDR, __pa((&vmx->vt.pi_desc)));
	}

	if (vmx_can_use_ipiv(&vmx->vcpu)) {
		vmcs_write64(PID_POINTER_TABLE, __pa(kvm_vmx->pid_table));
		vmcs_write16(LAST_PID_POINTER_INDEX, kvm->arch.max_vcpu_ids - 1);
	}

	if (!kvm_pause_in_guest(kvm)) {
		vmcs_write32(PLE_GAP, ple_gap);
		vmx->ple_window = ple_window;
		vmx->ple_window_dirty = true;
	}

	if (kvm_notify_vmexit_enabled(kvm))
		vmcs_write32(NOTIFY_WINDOW, kvm->arch.notify_window);

	vmcs_write32(PAGE_FAULT_ERROR_CODE_MASK, 0);
	vmcs_write32(PAGE_FAULT_ERROR_CODE_MATCH, 0);
	vmcs_write32(CR3_TARGET_COUNT, 0);           /* 22.2.1 */

	vmcs_write16(HOST_FS_SELECTOR, 0);            /* 22.2.4 */
	vmcs_write16(HOST_GS_SELECTOR, 0);            /* 22.2.4 */
	vmx_set_constant_host_state(vmx);
	vmcs_writel(HOST_FS_BASE, 0); /* 22.2.4 */
	vmcs_writel(HOST_GS_BASE, 0); /* 22.2.4 */

	if (cpu_has_vmx_vmfunc())
		vmcs_write64(VM_FUNCTION_CONTROL, 0);

	vmcs_write32(VM_EXIT_MSR_STORE_COUNT, 0);
	vmcs_write64(VM_EXIT_MSR_STORE_ADDR, __pa(vmx->msr_autostore.val));
	vmcs_write32(VM_EXIT_MSR_LOAD_COUNT, 0);
	vmcs_write64(VM_EXIT_MSR_LOAD_ADDR, __pa(vmx->msr_autoload.host.val));
	vmcs_write32(VM_ENTRY_MSR_LOAD_COUNT, 0);
	vmcs_write64(VM_ENTRY_MSR_LOAD_ADDR, __pa(vmx->msr_autoload.guest.val));

	if (vmcs_config.vmentry_ctrl & VM_ENTRY_LOAD_IA32_PAT)
		vmcs_write64(GUEST_IA32_PAT, vmx->vcpu.arch.pat);

	vm_exit_controls_set(vmx, vmx_get_initial_vmexit_ctrl());

	/* 22.2.1, 20.8.1 */
	vm_entry_controls_set(vmx, vmx_get_initial_vmentry_ctrl());

	vmx->vcpu.arch.cr0_guest_owned_bits = vmx_l1_guest_owned_cr0_bits();
	vmcs_writel(CR0_GUEST_HOST_MASK, ~vmx->vcpu.arch.cr0_guest_owned_bits);

	set_cr4_guest_host_mask(vmx);

	if (vmx->vpid != 0)
		vmcs_write16(VIRTUAL_PROCESSOR_ID, vmx->vpid);

	if (cpu_has_vmx_xsaves())
		vmcs_write64(XSS_EXIT_BITMAP, VMX_XSS_EXIT_BITMAP);

	if (enable_pml) {
		vmcs_write64(PML_ADDRESS, page_to_phys(vmx->pml_pg));
		vmcs_write16(GUEST_PML_INDEX, PML_HEAD_INDEX);
	}