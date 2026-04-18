// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 25/56



static void vmx_msr_bitmap_l01_changed(struct vcpu_vmx *vmx)
{
	/*
	 * When KVM is a nested hypervisor on top of Hyper-V and uses
	 * 'Enlightened MSR Bitmap' feature L0 needs to know that MSR
	 * bitmap has changed.
	 */
	if (kvm_is_using_evmcs()) {
		struct hv_enlightened_vmcs *evmcs = (void *)vmx->vmcs01.vmcs;

		if (evmcs->hv_enlightenments_control.msr_bitmap)
			evmcs->hv_clean_fields &=
				~HV_VMX_ENLIGHTENED_CLEAN_FIELD_MSR_BITMAP;
	}

	vmx->nested.force_msr_bitmap_recalc = true;
}

void vmx_set_intercept_for_msr(struct kvm_vcpu *vcpu, u32 msr, int type, bool set)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	unsigned long *msr_bitmap = vmx->vmcs01.msr_bitmap;

	if (!cpu_has_vmx_msr_bitmap())
		return;

	vmx_msr_bitmap_l01_changed(vmx);

	if (type & MSR_TYPE_R) {
		if (!set && kvm_msr_allowed(vcpu, msr, KVM_MSR_FILTER_READ))
			vmx_clear_msr_bitmap_read(msr_bitmap, msr);
		else
			vmx_set_msr_bitmap_read(msr_bitmap, msr);
	}

	if (type & MSR_TYPE_W) {
		if (!set && kvm_msr_allowed(vcpu, msr, KVM_MSR_FILTER_WRITE))
			vmx_clear_msr_bitmap_write(msr_bitmap, msr);
		else
			vmx_set_msr_bitmap_write(msr_bitmap, msr);
	}
}

static void vmx_update_msr_bitmap_x2apic(struct kvm_vcpu *vcpu)
{
	/*
	 * x2APIC indices for 64-bit accesses into the RDMSR and WRMSR halves
	 * of the MSR bitmap.  KVM emulates APIC registers up through 0x3f0,
	 * i.e. MSR 0x83f, and so only needs to dynamically manipulate 64 bits.
	 */
	const int read_idx = APIC_BASE_MSR / BITS_PER_LONG_LONG;
	const int write_idx = read_idx + (0x800 / sizeof(u64));
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	u64 *msr_bitmap = (u64 *)vmx->vmcs01.msr_bitmap;
	u8 mode;

	if (!cpu_has_vmx_msr_bitmap() || WARN_ON_ONCE(!lapic_in_kernel(vcpu)))
		return;

	if (cpu_has_secondary_exec_ctrls() &&
	    (secondary_exec_controls_get(vmx) &
	     SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE)) {
		mode = MSR_BITMAP_MODE_X2APIC;
		if (enable_apicv && kvm_vcpu_apicv_active(vcpu))
			mode |= MSR_BITMAP_MODE_X2APIC_APICV;
	} else {
		mode = 0;
	}

	if (mode == vmx->x2apic_msr_bitmap_mode)
		return;

	vmx->x2apic_msr_bitmap_mode = mode;

	/*
	 * Reset the bitmap for MSRs 0x800 - 0x83f.  Leave AMD's uber-extended
	 * registers (0x840 and above) intercepted, KVM doesn't support them.
	 * Intercept all writes by default and poke holes as needed.  Pass
	 * through reads for all valid registers by default in x2APIC+APICv
	 * mode, only the current timer count needs on-demand emulation by KVM.
	 */
	if (mode & MSR_BITMAP_MODE_X2APIC_APICV)
		msr_bitmap[read_idx] = ~kvm_lapic_readable_reg_mask(vcpu->arch.apic);
	else
		msr_bitmap[read_idx] = ~0ull;
	msr_bitmap[write_idx] = ~0ull;

	/*
	 * TPR reads and writes can be virtualized even if virtual interrupt
	 * delivery is not in use.
	 */
	vmx_set_intercept_for_msr(vcpu, X2APIC_MSR(APIC_TASKPRI), MSR_TYPE_RW,
				  !(mode & MSR_BITMAP_MODE_X2APIC));

	if (mode & MSR_BITMAP_MODE_X2APIC_APICV) {
		vmx_enable_intercept_for_msr(vcpu, X2APIC_MSR(APIC_TMCCT), MSR_TYPE_RW);
		vmx_disable_intercept_for_msr(vcpu, X2APIC_MSR(APIC_EOI), MSR_TYPE_W);
		vmx_disable_intercept_for_msr(vcpu, X2APIC_MSR(APIC_SELF_IPI), MSR_TYPE_W);
		if (enable_ipiv)
			vmx_disable_intercept_for_msr(vcpu, X2APIC_MSR(APIC_ICR), MSR_TYPE_RW);
	}
}

void pt_update_intercept_for_msr(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	bool flag = !(vmx->pt_desc.guest.ctl & RTIT_CTL_TRACEEN);
	u32 i;

	vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_STATUS, MSR_TYPE_RW, flag);
	vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_OUTPUT_BASE, MSR_TYPE_RW, flag);
	vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_OUTPUT_MASK, MSR_TYPE_RW, flag);
	vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_CR3_MATCH, MSR_TYPE_RW, flag);
	for (i = 0; i < vmx->pt_desc.num_address_ranges; i++) {
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_ADDR0_A + i * 2, MSR_TYPE_RW, flag);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_RTIT_ADDR0_B + i * 2, MSR_TYPE_RW, flag);
	}
}

static void vmx_recalc_pmu_msr_intercepts(struct kvm_vcpu *vcpu)
{
	u64 vm_exit_controls_bits = VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL |
				    VM_EXIT_SAVE_IA32_PERF_GLOBAL_CTRL;
	bool has_mediated_pmu = kvm_vcpu_has_mediated_pmu(vcpu);
	struct kvm_pmu *pmu = vcpu_to_pmu(vcpu);
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	bool intercept = !has_mediated_pmu;
	int i;

	if (!enable_mediated_pmu)
		return;

	if (!cpu_has_save_perf_global_ctrl()) {
		vm_exit_controls_bits &= ~VM_EXIT_SAVE_IA32_PERF_GLOBAL_CTRL;

		if (has_mediated_pmu)
			vmx_add_autostore_msr(vmx, MSR_CORE_PERF_GLOBAL_CTRL);
		else
			vmx_remove_autostore_msr(vmx, MSR_CORE_PERF_GLOBAL_CTRL);
	}

	vm_entry_controls_changebit(vmx, VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL,
				    has_mediated_pmu);

	vm_exit_controls_changebit(vmx, vm_exit_controls_bits, has_mediated_pmu);