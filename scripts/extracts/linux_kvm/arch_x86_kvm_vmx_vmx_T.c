// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 52/56



		/*
		 * Disallow adaptive PEBS as it is functionally broken, can be
		 * used by the guest to read *host* LBRs, and can be used to
		 * bypass userspace event filters.  To correctly and safely
		 * support adaptive PEBS, KVM needs to:
		 *
		 * 1. Account for the ADAPTIVE flag when (re)programming fixed
		 *    counters.
		 *
		 * 2. Gain support from perf (or take direct control of counter
		 *    programming) to support events without adaptive PEBS
		 *    enabled for the hardware counter.
		 *
		 * 3. Ensure LBR MSRs cannot hold host data on VM-Entry with
		 *    adaptive PEBS enabled and MSR_PEBS_DATA_CFG.LBRS=1.
		 *
		 * 4. Document which PMU events are effectively exposed to the
		 *    guest via adaptive PEBS, and make adaptive PEBS mutually
		 *    exclusive with KVM_SET_PMU_EVENT_FILTER if necessary.
		 */
		perf_cap &= ~PERF_CAP_PEBS_BASELINE;
	}

	return perf_cap;
}

static __init void vmx_set_cpu_caps(void)
{
	kvm_initialize_cpu_caps();

	/* CPUID 0x1 */
	if (nested)
		kvm_cpu_cap_set(X86_FEATURE_VMX);

	/* CPUID 0x7 */
	if (kvm_mpx_supported())
		kvm_cpu_cap_check_and_set(X86_FEATURE_MPX);
	if (!cpu_has_vmx_invpcid())
		kvm_cpu_cap_clear(X86_FEATURE_INVPCID);
	if (vmx_pt_mode_is_host_guest())
		kvm_cpu_cap_check_and_set(X86_FEATURE_INTEL_PT);
	if (vmx_pebs_supported()) {
		kvm_cpu_cap_check_and_set(X86_FEATURE_DS);
		kvm_cpu_cap_check_and_set(X86_FEATURE_DTES64);
	}

	if (!enable_pmu)
		kvm_cpu_cap_clear(X86_FEATURE_PDCM);
	kvm_caps.supported_perf_cap = vmx_get_perf_capabilities();

	if (!enable_sgx) {
		kvm_cpu_cap_clear(X86_FEATURE_SGX);
		kvm_cpu_cap_clear(X86_FEATURE_SGX_LC);
		kvm_cpu_cap_clear(X86_FEATURE_SGX1);
		kvm_cpu_cap_clear(X86_FEATURE_SGX2);
		kvm_cpu_cap_clear(X86_FEATURE_SGX_EDECCSSA);
	}

	if (vmx_umip_emulated())
		kvm_cpu_cap_set(X86_FEATURE_UMIP);

	/* CPUID 0xD.1 */
	if (!cpu_has_vmx_xsaves())
		kvm_cpu_cap_clear(X86_FEATURE_XSAVES);

	/* CPUID 0x80000001 and 0x7 (RDPID) */
	if (!cpu_has_vmx_rdtscp()) {
		kvm_cpu_cap_clear(X86_FEATURE_RDTSCP);
		kvm_cpu_cap_clear(X86_FEATURE_RDPID);
	}

	if (cpu_has_vmx_waitpkg())
		kvm_cpu_cap_check_and_set(X86_FEATURE_WAITPKG);

	/*
	 * Disable CET if unrestricted_guest is unsupported as KVM doesn't
	 * enforce CET HW behaviors in emulator. On platforms with
	 * VMX_BASIC[bit56] == 0, inject #CP at VMX entry with error code
	 * fails, so disable CET in this case too.
	 */
	if (!cpu_has_load_cet_ctrl() || !enable_unrestricted_guest ||
	    !cpu_has_vmx_basic_no_hw_errcode_cc()) {
		kvm_cpu_cap_clear(X86_FEATURE_SHSTK);
		kvm_cpu_cap_clear(X86_FEATURE_IBT);
	}

	kvm_setup_xss_caps();
	kvm_finalize_cpu_caps();
}

static bool vmx_is_io_intercepted(struct kvm_vcpu *vcpu,
				  struct x86_instruction_info *info,
				  unsigned long *exit_qualification)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
	unsigned short port;
	int size;
	bool imm;

	/*
	 * If the 'use IO bitmaps' VM-execution control is 0, IO instruction
	 * VM-exits depend on the 'unconditional IO exiting' VM-execution
	 * control.
	 *
	 * Otherwise, IO instruction VM-exits are controlled by the IO bitmaps.
	 */
	if (!nested_cpu_has(vmcs12, CPU_BASED_USE_IO_BITMAPS))
		return nested_cpu_has(vmcs12, CPU_BASED_UNCOND_IO_EXITING);

	if (info->intercept == x86_intercept_in ||
	    info->intercept == x86_intercept_ins) {
		port = info->src_val;
		size = info->dst_bytes;
		imm  = info->src_type == OP_IMM;
	} else {
		port = info->dst_val;
		size = info->src_bytes;
		imm  = info->dst_type == OP_IMM;
	}


	*exit_qualification = ((unsigned long)port << 16) | (size - 1);

	if (info->intercept == x86_intercept_ins ||
	    info->intercept == x86_intercept_outs)
		*exit_qualification |= BIT(4);

	if (info->rep_prefix)
		*exit_qualification |= BIT(5);

	if (imm)
		*exit_qualification |= BIT(6);

	return nested_vmx_check_io_bitmaps(vcpu, port, size);
}

int vmx_check_intercept(struct kvm_vcpu *vcpu,
			struct x86_instruction_info *info,
			enum x86_intercept_stage stage,
			struct x86_exception *exception)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
	unsigned long exit_qualification = 0;
	u32 vm_exit_reason;
	u64 exit_insn_len;

	switch (info->intercept) {
	case x86_intercept_rdpid:
		/*
		 * RDPID causes #UD if not enabled through secondary execution
		 * controls (ENABLE_RDTSCP).  Note, the implicit MSR access to
		 * TSC_AUX is NOT subject to interception, i.e. checking only
		 * the dedicated execution control is architecturally correct.
		 */
		if (!nested_cpu_has2(vmcs12, SECONDARY_EXEC_ENABLE_RDTSCP)) {
			exception->vector = UD_VECTOR;
			exception->error_code_valid = false;
			return X86EMUL_PROPAGATE_FAULT;
		}
		return X86EMUL_CONTINUE;

	case x86_intercept_in:
	case x86_intercept_ins:
	case x86_intercept_out:
	case x86_intercept_outs:
		if (!vmx_is_io_intercepted(vcpu, info, &exit_qualification))
			return X86EMUL_CONTINUE;

		vm_exit_reason = EXIT_REASON_IO_INSTRUCTION;
		break;