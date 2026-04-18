// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 26/56



	for (i = 0; i < pmu->nr_arch_gp_counters; i++) {
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PERFCTR0 + i,
					  MSR_TYPE_RW, intercept);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PMC0 + i, MSR_TYPE_RW,
					  intercept || !fw_writes_is_enabled(vcpu));
	}
	for ( ; i < kvm_pmu_cap.num_counters_gp; i++) {
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PERFCTR0 + i,
					  MSR_TYPE_RW, true);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PMC0 + i,
					  MSR_TYPE_RW, true);
	}

	for (i = 0; i < pmu->nr_arch_fixed_counters; i++)
		vmx_set_intercept_for_msr(vcpu, MSR_CORE_PERF_FIXED_CTR0 + i,
					  MSR_TYPE_RW, intercept);
	for ( ; i < kvm_pmu_cap.num_counters_fixed; i++)
		vmx_set_intercept_for_msr(vcpu, MSR_CORE_PERF_FIXED_CTR0 + i,
					  MSR_TYPE_RW, true);

	intercept = kvm_need_perf_global_ctrl_intercept(vcpu);
	vmx_set_intercept_for_msr(vcpu, MSR_CORE_PERF_GLOBAL_STATUS,
				  MSR_TYPE_RW, intercept);
	vmx_set_intercept_for_msr(vcpu, MSR_CORE_PERF_GLOBAL_CTRL,
				  MSR_TYPE_RW, intercept);
	vmx_set_intercept_for_msr(vcpu, MSR_CORE_PERF_GLOBAL_OVF_CTRL,
				  MSR_TYPE_RW, intercept);
}

static void vmx_recalc_msr_intercepts(struct kvm_vcpu *vcpu)
{
	bool intercept;

	if (!cpu_has_vmx_msr_bitmap())
		return;

	vmx_disable_intercept_for_msr(vcpu, MSR_IA32_TSC, MSR_TYPE_R);
#ifdef CONFIG_X86_64
	vmx_disable_intercept_for_msr(vcpu, MSR_FS_BASE, MSR_TYPE_RW);
	vmx_disable_intercept_for_msr(vcpu, MSR_GS_BASE, MSR_TYPE_RW);
	vmx_disable_intercept_for_msr(vcpu, MSR_KERNEL_GS_BASE, MSR_TYPE_RW);
#endif
	vmx_disable_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_CS, MSR_TYPE_RW);
	vmx_disable_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_ESP, MSR_TYPE_RW);
	vmx_disable_intercept_for_msr(vcpu, MSR_IA32_SYSENTER_EIP, MSR_TYPE_RW);
	if (kvm_cstate_in_guest(vcpu->kvm)) {
		vmx_disable_intercept_for_msr(vcpu, MSR_CORE_C1_RES, MSR_TYPE_R);
		vmx_disable_intercept_for_msr(vcpu, MSR_CORE_C3_RESIDENCY, MSR_TYPE_R);
		vmx_disable_intercept_for_msr(vcpu, MSR_CORE_C6_RESIDENCY, MSR_TYPE_R);
		vmx_disable_intercept_for_msr(vcpu, MSR_CORE_C7_RESIDENCY, MSR_TYPE_R);
	}
	if (kvm_aperfmperf_in_guest(vcpu->kvm)) {
		vmx_disable_intercept_for_msr(vcpu, MSR_IA32_APERF, MSR_TYPE_R);
		vmx_disable_intercept_for_msr(vcpu, MSR_IA32_MPERF, MSR_TYPE_R);
	}

	/* PT MSRs can be passed through iff PT is exposed to the guest. */
	if (vmx_pt_mode_is_host_guest())
		pt_update_intercept_for_msr(vcpu);

	if (vcpu->arch.xfd_no_write_intercept)
		vmx_disable_intercept_for_msr(vcpu, MSR_IA32_XFD, MSR_TYPE_RW);

	vmx_set_intercept_for_msr(vcpu, MSR_IA32_SPEC_CTRL, MSR_TYPE_RW,
				  !to_vmx(vcpu)->spec_ctrl);

	if (kvm_cpu_cap_has(X86_FEATURE_XFD))
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_XFD_ERR, MSR_TYPE_R,
					  !guest_cpu_cap_has(vcpu, X86_FEATURE_XFD));

	if (cpu_feature_enabled(X86_FEATURE_IBPB))
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PRED_CMD, MSR_TYPE_W,
					  !guest_has_pred_cmd_msr(vcpu));

	if (cpu_feature_enabled(X86_FEATURE_FLUSH_L1D))
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_FLUSH_CMD, MSR_TYPE_W,
					  !guest_cpu_cap_has(vcpu, X86_FEATURE_FLUSH_L1D));

	if (kvm_cpu_cap_has(X86_FEATURE_SHSTK)) {
		intercept = !guest_cpu_cap_has(vcpu, X86_FEATURE_SHSTK);

		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PL0_SSP, MSR_TYPE_RW, intercept);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PL1_SSP, MSR_TYPE_RW, intercept);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PL2_SSP, MSR_TYPE_RW, intercept);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_PL3_SSP, MSR_TYPE_RW, intercept);
	}

	if (kvm_cpu_cap_has(X86_FEATURE_SHSTK) || kvm_cpu_cap_has(X86_FEATURE_IBT)) {
		intercept = !guest_cpu_cap_has(vcpu, X86_FEATURE_IBT) &&
			    !guest_cpu_cap_has(vcpu, X86_FEATURE_SHSTK);

		vmx_set_intercept_for_msr(vcpu, MSR_IA32_U_CET, MSR_TYPE_RW, intercept);
		vmx_set_intercept_for_msr(vcpu, MSR_IA32_S_CET, MSR_TYPE_RW, intercept);
	}

	vmx_recalc_pmu_msr_intercepts(vcpu);

	/*
	 * x2APIC and LBR MSR intercepts are modified on-demand and cannot be
	 * filtered by userspace.
	 */
}

static void vmx_recalc_instruction_intercepts(struct kvm_vcpu *vcpu)
{
	exec_controls_changebit(to_vmx(vcpu), CPU_BASED_RDPMC_EXITING,
				kvm_need_rdpmc_intercept(vcpu));
}

void vmx_recalc_intercepts(struct kvm_vcpu *vcpu)
{
	vmx_recalc_instruction_intercepts(vcpu);
	vmx_recalc_msr_intercepts(vcpu);
}

static int vmx_deliver_nested_posted_interrupt(struct kvm_vcpu *vcpu,
						int vector)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);