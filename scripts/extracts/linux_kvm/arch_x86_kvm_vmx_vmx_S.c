// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 51/56



	/* Get the number of configurable Address Ranges for filtering */
	vmx->pt_desc.num_address_ranges = intel_pt_validate_cap(vmx->pt_desc.caps,
						PT_CAP_num_address_ranges);

	/* Initialize and clear the no dependency bits */
	vmx->pt_desc.ctl_bitmask = ~(RTIT_CTL_TRACEEN | RTIT_CTL_OS |
			RTIT_CTL_USR | RTIT_CTL_TSC_EN | RTIT_CTL_DISRETC |
			RTIT_CTL_BRANCH_EN);

	/*
	 * If CPUID.(EAX=14H,ECX=0):EBX[0]=1 CR3Filter can be set otherwise
	 * will inject an #GP
	 */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_cr3_filtering))
		vmx->pt_desc.ctl_bitmask &= ~RTIT_CTL_CR3EN;

	/*
	 * If CPUID.(EAX=14H,ECX=0):EBX[1]=1 CYCEn, CycThresh and
	 * PSBFreq can be set
	 */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_psb_cyc))
		vmx->pt_desc.ctl_bitmask &= ~(RTIT_CTL_CYCLEACC |
				RTIT_CTL_CYC_THRESH | RTIT_CTL_PSB_FREQ);

	/*
	 * If CPUID.(EAX=14H,ECX=0):EBX[3]=1 MTCEn and MTCFreq can be set
	 */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_mtc))
		vmx->pt_desc.ctl_bitmask &= ~(RTIT_CTL_MTC_EN |
					      RTIT_CTL_MTC_RANGE);

	/* If CPUID.(EAX=14H,ECX=0):EBX[4]=1 FUPonPTW and PTWEn can be set */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_ptwrite))
		vmx->pt_desc.ctl_bitmask &= ~(RTIT_CTL_FUP_ON_PTW |
							RTIT_CTL_PTW_EN);

	/* If CPUID.(EAX=14H,ECX=0):EBX[5]=1 PwrEvEn can be set */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_power_event_trace))
		vmx->pt_desc.ctl_bitmask &= ~RTIT_CTL_PWR_EVT_EN;

	/* If CPUID.(EAX=14H,ECX=0):ECX[0]=1 ToPA can be set */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_topa_output))
		vmx->pt_desc.ctl_bitmask &= ~RTIT_CTL_TOPA;

	/* If CPUID.(EAX=14H,ECX=0):ECX[3]=1 FabricEn can be set */
	if (intel_pt_validate_cap(vmx->pt_desc.caps, PT_CAP_output_subsys))
		vmx->pt_desc.ctl_bitmask &= ~RTIT_CTL_FABRIC_EN;

	/* unmask address range configure area */
	for (i = 0; i < vmx->pt_desc.num_address_ranges; i++)
		vmx->pt_desc.ctl_bitmask &= ~(0xfULL << (32 + i * 4));
}

void vmx_vcpu_after_set_cpuid(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	/*
	 * XSAVES is effectively enabled if and only if XSAVE is also exposed
	 * to the guest.  XSAVES depends on CR4.OSXSAVE, and CR4.OSXSAVE can be
	 * set if and only if XSAVE is supported.
	 */
	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_XSAVE))
		guest_cpu_cap_clear(vcpu, X86_FEATURE_XSAVES);

	vmx_setup_uret_msrs(vmx);

	if (cpu_has_secondary_exec_ctrls())
		vmcs_set_secondary_exec_control(vmx,
						vmx_secondary_exec_control(vmx));

	if (guest_cpu_cap_has(vcpu, X86_FEATURE_VMX))
		vmx->msr_ia32_feature_control_valid_bits |=
			FEAT_CTL_VMX_ENABLED_INSIDE_SMX |
			FEAT_CTL_VMX_ENABLED_OUTSIDE_SMX;
	else
		vmx->msr_ia32_feature_control_valid_bits &=
			~(FEAT_CTL_VMX_ENABLED_INSIDE_SMX |
			  FEAT_CTL_VMX_ENABLED_OUTSIDE_SMX);

	if (guest_cpu_cap_has(vcpu, X86_FEATURE_VMX))
		nested_vmx_cr_fixed1_bits_update(vcpu);

	if (boot_cpu_has(X86_FEATURE_INTEL_PT) &&
			guest_cpu_cap_has(vcpu, X86_FEATURE_INTEL_PT))
		update_intel_pt_cfg(vcpu);

	if (boot_cpu_has(X86_FEATURE_RTM)) {
		struct vmx_uret_msr *msr;
		msr = vmx_find_uret_msr(vmx, MSR_IA32_TSX_CTRL);
		if (msr) {
			bool enabled = guest_cpu_cap_has(vcpu, X86_FEATURE_RTM);
			vmx_set_guest_uret_msr(vmx, msr, enabled ? 0 : TSX_CTRL_RTM_DISABLE);
		}
	}

	set_cr4_guest_host_mask(vmx);

	vmx_write_encls_bitmap(vcpu, NULL);
	if (guest_cpu_cap_has(vcpu, X86_FEATURE_SGX))
		vmx->msr_ia32_feature_control_valid_bits |= FEAT_CTL_SGX_ENABLED;
	else
		vmx->msr_ia32_feature_control_valid_bits &= ~FEAT_CTL_SGX_ENABLED;

	if (guest_cpu_cap_has(vcpu, X86_FEATURE_SGX_LC))
		vmx->msr_ia32_feature_control_valid_bits |=
			FEAT_CTL_SGX_LC_ENABLED;
	else
		vmx->msr_ia32_feature_control_valid_bits &=
			~FEAT_CTL_SGX_LC_ENABLED;

	/* Refresh #PF interception to account for MAXPHYADDR changes. */
	vmx_update_exception_bitmap(vcpu);
}

static __init u64 vmx_get_perf_capabilities(void)
{
	u64 perf_cap = PERF_CAP_FW_WRITES;
	u64 host_perf_cap = 0;

	if (!enable_pmu)
		return 0;

	if (boot_cpu_has(X86_FEATURE_PDCM))
		rdmsrq(MSR_IA32_PERF_CAPABILITIES, host_perf_cap);

	if (!cpu_feature_enabled(X86_FEATURE_ARCH_LBR) &&
	    !enable_mediated_pmu) {
		x86_perf_get_lbr(&vmx_lbr_caps);

		/*
		 * KVM requires LBR callstack support, as the overhead due to
		 * context switching LBRs without said support is too high.
		 * See intel_pmu_create_guest_lbr_event() for more info.
		 */
		if (!vmx_lbr_caps.has_callstack)
			memset(&vmx_lbr_caps, 0, sizeof(vmx_lbr_caps));
		else if (vmx_lbr_caps.nr)
			perf_cap |= host_perf_cap & PERF_CAP_LBR_FMT;
	}

	if (vmx_pebs_supported()) {
		perf_cap |= host_perf_cap & PERF_CAP_PEBS_MASK;