// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 47/56



	vector = idt_vectoring_info & VECTORING_INFO_VECTOR_MASK;
	type = idt_vectoring_info & VECTORING_INFO_TYPE_MASK;

	switch (type) {
	case INTR_TYPE_NMI_INTR:
		vcpu->arch.nmi_injected = true;
		/*
		 * SDM 3: 27.7.1.2 (September 2008)
		 * Clear bit "block by NMI" before VM entry if a NMI
		 * delivery faulted.
		 */
		vmx_set_nmi_mask(vcpu, false);
		break;
	case INTR_TYPE_SOFT_EXCEPTION:
		vcpu->arch.event_exit_inst_len = vmcs_read32(instr_len_field);
		fallthrough;
	case INTR_TYPE_HARD_EXCEPTION: {
		u32 error_code = 0;

		if (idt_vectoring_info & VECTORING_INFO_DELIVER_CODE_MASK)
			error_code = vmcs_read32(error_code_field);

		kvm_requeue_exception(vcpu, vector,
				      idt_vectoring_info & VECTORING_INFO_DELIVER_CODE_MASK,
				      error_code);
		break;
	}
	case INTR_TYPE_SOFT_INTR:
		vcpu->arch.event_exit_inst_len = vmcs_read32(instr_len_field);
		fallthrough;
	case INTR_TYPE_EXT_INTR:
		kvm_queue_interrupt(vcpu, vector, type == INTR_TYPE_SOFT_INTR);
		break;
	default:
		break;
	}
}

static void vmx_complete_interrupts(struct vcpu_vmx *vmx)
{
	__vmx_complete_interrupts(&vmx->vcpu, vmx->idt_vectoring_info,
				  VM_EXIT_INSTRUCTION_LEN,
				  IDT_VECTORING_ERROR_CODE);
}

void vmx_cancel_injection(struct kvm_vcpu *vcpu)
{
	__vmx_complete_interrupts(vcpu,
				  vmcs_read32(VM_ENTRY_INTR_INFO_FIELD),
				  VM_ENTRY_INSTRUCTION_LEN,
				  VM_ENTRY_EXCEPTION_ERROR_CODE);

	vmcs_write32(VM_ENTRY_INTR_INFO_FIELD, 0);
}

static void atomic_switch_perf_msrs(struct vcpu_vmx *vmx)
{
	int i, nr_msrs;
	struct perf_guest_switch_msr *msrs;
	struct kvm_pmu *pmu = vcpu_to_pmu(&vmx->vcpu);

	if (kvm_vcpu_has_mediated_pmu(&vmx->vcpu))
		return;

	pmu->host_cross_mapped_mask = 0;
	if (pmu->pebs_enable & pmu->global_ctrl)
		intel_pmu_cross_mapped_check(pmu);

	/* Note, nr_msrs may be garbage if perf_guest_get_msrs() returns NULL. */
	msrs = perf_guest_get_msrs(&nr_msrs, (void *)pmu);
	if (!msrs)
		return;

	for (i = 0; i < nr_msrs; i++)
		if (msrs[i].host == msrs[i].guest)
			clear_atomic_switch_msr(vmx, msrs[i].msr);
		else
			add_atomic_switch_msr(vmx, msrs[i].msr, msrs[i].guest,
					      msrs[i].host);
}

static void vmx_refresh_guest_perf_global_control(struct kvm_vcpu *vcpu)
{
	struct kvm_pmu *pmu = vcpu_to_pmu(vcpu);
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	if (msr_write_intercepted(vmx, MSR_CORE_PERF_GLOBAL_CTRL))
		return;

	if (!cpu_has_save_perf_global_ctrl()) {
		int slot = vmx_find_loadstore_msr_slot(&vmx->msr_autostore,
						       MSR_CORE_PERF_GLOBAL_CTRL);

		if (WARN_ON_ONCE(slot < 0))
			return;

		pmu->global_ctrl = vmx->msr_autostore.val[slot].value;
		vmcs_write64(GUEST_IA32_PERF_GLOBAL_CTRL, pmu->global_ctrl);
		return;
	}

	pmu->global_ctrl = vmcs_read64(GUEST_IA32_PERF_GLOBAL_CTRL);
}

static void vmx_update_hv_timer(struct kvm_vcpu *vcpu, bool force_immediate_exit)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	u64 tscl;
	u32 delta_tsc;

	if (force_immediate_exit) {
		vmcs_write32(VMX_PREEMPTION_TIMER_VALUE, 0);
		vmx->loaded_vmcs->hv_timer_soft_disabled = false;
	} else if (vmx->hv_deadline_tsc != -1) {
		tscl = rdtsc();
		if (vmx->hv_deadline_tsc > tscl)
			/* set_hv_timer ensures the delta fits in 32-bits */
			delta_tsc = (u32)((vmx->hv_deadline_tsc - tscl) >>
				cpu_preemption_timer_multi);
		else
			delta_tsc = 0;

		vmcs_write32(VMX_PREEMPTION_TIMER_VALUE, delta_tsc);
		vmx->loaded_vmcs->hv_timer_soft_disabled = false;
	} else if (!vmx->loaded_vmcs->hv_timer_soft_disabled) {
		vmcs_write32(VMX_PREEMPTION_TIMER_VALUE, -1);
		vmx->loaded_vmcs->hv_timer_soft_disabled = true;
	}
}

void noinstr vmx_update_host_rsp(struct vcpu_vmx *vmx, unsigned long host_rsp)
{
	if (unlikely(host_rsp != vmx->loaded_vmcs->host_state.rsp)) {
		vmx->loaded_vmcs->host_state.rsp = host_rsp;
		vmcs_writel(HOST_RSP, host_rsp);
	}
}

void noinstr vmx_spec_ctrl_restore_host(struct vcpu_vmx *vmx,
					unsigned int flags)
{
	u64 hostval = this_cpu_read(x86_spec_ctrl_current);

	if (!cpu_feature_enabled(X86_FEATURE_MSR_SPEC_CTRL))
		return;

	if (flags & VMX_RUN_SAVE_SPEC_CTRL)
		vmx->spec_ctrl = native_rdmsrq(MSR_IA32_SPEC_CTRL);

	/*
	 * If the guest/host SPEC_CTRL values differ, restore the host value.
	 *
	 * For legacy IBRS, the IBRS bit always needs to be written after
	 * transitioning from a less privileged predictor mode, regardless of
	 * whether the guest/host values differ.
	 */
	if (cpu_feature_enabled(X86_FEATURE_KERNEL_IBRS) ||
	    vmx->spec_ctrl != hostval)
		native_wrmsrq(MSR_IA32_SPEC_CTRL, hostval);

	barrier_nospec();
}

static fastpath_t vmx_exit_handlers_fastpath(struct kvm_vcpu *vcpu,
					     bool force_immediate_exit)
{
	/*
	 * If L2 is active, some VMX preemption timer exits can be handled in
	 * the fastpath even, all other exits must use the slow path.
	 */
	if (is_guest_mode(vcpu) &&
	    vmx_get_exit_reason(vcpu).basic != EXIT_REASON_PREEMPTION_TIMER)
		return EXIT_FASTPATH_NONE;