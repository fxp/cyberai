// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 45/86



	if (rdmsr_safe(msr_index, &dummy[0], &dummy[1]))
		return;

	/*
	 * Even MSRs that are valid in the host may not be exposed to guests in
	 * some cases.
	 */
	switch (msr_index) {
	case MSR_IA32_BNDCFGS:
		if (!kvm_mpx_supported())
			return;
		break;
	case MSR_TSC_AUX:
		if (!kvm_cpu_cap_has(X86_FEATURE_RDTSCP) &&
		    !kvm_cpu_cap_has(X86_FEATURE_RDPID))
			return;
		break;
	case MSR_IA32_UMWAIT_CONTROL:
		if (!kvm_cpu_cap_has(X86_FEATURE_WAITPKG))
			return;
		break;
	case MSR_IA32_RTIT_CTL:
	case MSR_IA32_RTIT_STATUS:
		if (!kvm_cpu_cap_has(X86_FEATURE_INTEL_PT))
			return;
		break;
	case MSR_IA32_RTIT_CR3_MATCH:
		if (!kvm_cpu_cap_has(X86_FEATURE_INTEL_PT) ||
		    !intel_pt_validate_hw_cap(PT_CAP_cr3_filtering))
			return;
		break;
	case MSR_IA32_RTIT_OUTPUT_BASE:
	case MSR_IA32_RTIT_OUTPUT_MASK:
		if (!kvm_cpu_cap_has(X86_FEATURE_INTEL_PT) ||
		    (!intel_pt_validate_hw_cap(PT_CAP_topa_output) &&
		     !intel_pt_validate_hw_cap(PT_CAP_single_range_output)))
			return;
		break;
	case MSR_IA32_RTIT_ADDR0_A ... MSR_IA32_RTIT_ADDR3_B:
		if (!kvm_cpu_cap_has(X86_FEATURE_INTEL_PT) ||
		    (msr_index - MSR_IA32_RTIT_ADDR0_A >=
		     intel_pt_validate_hw_cap(PT_CAP_num_address_ranges) * 2))
			return;
		break;
	case MSR_ARCH_PERFMON_PERFCTR0 ...
	     MSR_ARCH_PERFMON_PERFCTR0 + KVM_MAX_NR_GP_COUNTERS - 1:
		if (msr_index - MSR_ARCH_PERFMON_PERFCTR0 >=
		    kvm_pmu_cap.num_counters_gp)
			return;
		break;
	case MSR_ARCH_PERFMON_EVENTSEL0 ...
	     MSR_ARCH_PERFMON_EVENTSEL0 + KVM_MAX_NR_GP_COUNTERS - 1:
		if (msr_index - MSR_ARCH_PERFMON_EVENTSEL0 >=
		    kvm_pmu_cap.num_counters_gp)
			return;
		break;
	case MSR_ARCH_PERFMON_FIXED_CTR0 ...
	     MSR_ARCH_PERFMON_FIXED_CTR0 + KVM_MAX_NR_FIXED_COUNTERS - 1:
		if (msr_index - MSR_ARCH_PERFMON_FIXED_CTR0 >=
		    kvm_pmu_cap.num_counters_fixed)
			return;
		break;
	case MSR_AMD64_PERF_CNTR_GLOBAL_CTL:
	case MSR_AMD64_PERF_CNTR_GLOBAL_STATUS:
	case MSR_AMD64_PERF_CNTR_GLOBAL_STATUS_CLR:
	case MSR_AMD64_PERF_CNTR_GLOBAL_STATUS_SET:
		if (!kvm_cpu_cap_has(X86_FEATURE_PERFMON_V2))
			return;
		break;
	case MSR_IA32_XFD:
	case MSR_IA32_XFD_ERR:
		if (!kvm_cpu_cap_has(X86_FEATURE_XFD))
			return;
		break;
	case MSR_IA32_TSX_CTRL:
		if (!(kvm_get_arch_capabilities() & ARCH_CAP_TSX_CTRL_MSR))
			return;
		break;
	case MSR_IA32_XSS:
		if (!kvm_caps.supported_xss)
			return;
		break;
	case MSR_IA32_U_CET:
	case MSR_IA32_S_CET:
		if (!kvm_cpu_cap_has(X86_FEATURE_SHSTK) &&
		    !kvm_cpu_cap_has(X86_FEATURE_IBT))
			return;
		break;
	case MSR_IA32_INT_SSP_TAB:
		if (!kvm_cpu_cap_has(X86_FEATURE_LM))
			return;
		fallthrough;
	case MSR_IA32_PL0_SSP ... MSR_IA32_PL3_SSP:
		if (!kvm_cpu_cap_has(X86_FEATURE_SHSTK))
			return;
		break;
	default:
		break;
	}

	msrs_to_save[num_msrs_to_save++] = msr_index;
}

static void kvm_init_msr_lists(void)
{
	unsigned i;

	BUILD_BUG_ON_MSG(KVM_MAX_NR_FIXED_COUNTERS != 3,
			 "Please update the fixed PMCs in msrs_to_save_pmu[]");

	num_msrs_to_save = 0;
	num_emulated_msrs = 0;
	num_msr_based_features = 0;

	for (i = 0; i < ARRAY_SIZE(msrs_to_save_base); i++)
		kvm_probe_msr_to_save(msrs_to_save_base[i]);

	if (enable_pmu) {
		for (i = 0; i < ARRAY_SIZE(msrs_to_save_pmu); i++)
			kvm_probe_msr_to_save(msrs_to_save_pmu[i]);
	}

	for (i = 0; i < ARRAY_SIZE(emulated_msrs_all); i++) {
		if (!kvm_x86_call(has_emulated_msr)(NULL,
						    emulated_msrs_all[i]))
			continue;

		emulated_msrs[num_emulated_msrs++] = emulated_msrs_all[i];
	}

	for (i = KVM_FIRST_EMULATED_VMX_MSR; i <= KVM_LAST_EMULATED_VMX_MSR; i++)
		kvm_probe_feature_msr(i);

	for (i = 0; i < ARRAY_SIZE(msr_based_features_all_except_vmx); i++)
		kvm_probe_feature_msr(msr_based_features_all_except_vmx[i]);
}

static int vcpu_mmio_write(struct kvm_vcpu *vcpu, gpa_t addr, int len,
			   void *__v)
{
	const void *v = __v;
	int handled = 0;
	int n;

	trace_kvm_mmio(KVM_TRACE_MMIO_WRITE, len, addr, __v);

	do {
		n = min(len, 8);
		if (!(lapic_in_kernel(vcpu) &&
		      !kvm_iodevice_write(vcpu, &vcpu->arch.apic->dev, addr, n, v))
		    && kvm_io_bus_write(vcpu, KVM_MMIO_BUS, addr, n, v))
			break;
		handled += n;
		addr += n;
		len -= n;
		v += n;
	} while (len);

	return handled;
}

static int vcpu_mmio_read(struct kvm_vcpu *vcpu, gpa_t addr, int len, void *v)
{
	int handled = 0;
	int n;

	do {
		n = min(len, 8);
		if (!(lapic_in_kernel(vcpu) &&
		      !kvm_iodevice_read(vcpu, &vcpu->arch.apic->dev,
					 addr, n, v))
		    && kvm_io_bus_read(vcpu, KVM_MMIO_BUS, addr, n, v))
			break;
		trace_kvm_mmio(KVM_TRACE_MMIO_READ, n, addr, v);
		handled += n;
		addr += n;
		len -= n;
		v += n;
	} while (len);

	if (len)
		trace_kvm_mmio(KVM_TRACE_MMIO_READ_UNSATISFIED, len, addr, NULL);

	return handled;
}

void kvm_set_segment(struct kvm_vcpu *vcpu,
		     struct kvm_segment *var, int seg)
{
	kvm_x86_call(set_segment)(vcpu, var, seg);
}

void kvm_get_segment(struct kvm_vcpu *vcpu,
		     struct kvm_segment *var, int seg)
{
	kvm_x86_call(get_segment)(vcpu, var, seg);
}