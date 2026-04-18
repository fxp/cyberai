// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 8/17



	switch (msr) {
	case HV_X64_MSR_GUEST_OS_ID:
	case HV_X64_MSR_HYPERCALL:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_HYPERCALL_AVAILABLE;
	case HV_X64_MSR_VP_RUNTIME:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_VP_RUNTIME_AVAILABLE;
	case HV_X64_MSR_TIME_REF_COUNT:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_TIME_REF_COUNT_AVAILABLE;
	case HV_X64_MSR_VP_INDEX:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_VP_INDEX_AVAILABLE;
	case HV_X64_MSR_RESET:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_RESET_AVAILABLE;
	case HV_X64_MSR_REFERENCE_TSC:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_REFERENCE_TSC_AVAILABLE;
	case HV_X64_MSR_SCONTROL:
	case HV_X64_MSR_SVERSION:
	case HV_X64_MSR_SIEFP:
	case HV_X64_MSR_SIMP:
	case HV_X64_MSR_EOM:
	case HV_X64_MSR_SINT0 ... HV_X64_MSR_SINT15:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_SYNIC_AVAILABLE;
	case HV_X64_MSR_STIMER0_CONFIG:
	case HV_X64_MSR_STIMER1_CONFIG:
	case HV_X64_MSR_STIMER2_CONFIG:
	case HV_X64_MSR_STIMER3_CONFIG:
	case HV_X64_MSR_STIMER0_COUNT:
	case HV_X64_MSR_STIMER1_COUNT:
	case HV_X64_MSR_STIMER2_COUNT:
	case HV_X64_MSR_STIMER3_COUNT:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_SYNTIMER_AVAILABLE;
	case HV_X64_MSR_EOI:
	case HV_X64_MSR_ICR:
	case HV_X64_MSR_TPR:
	case HV_X64_MSR_VP_ASSIST_PAGE:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_MSR_APIC_ACCESS_AVAILABLE;
	case HV_X64_MSR_TSC_FREQUENCY:
	case HV_X64_MSR_APIC_FREQUENCY:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_ACCESS_FREQUENCY_MSRS;
	case HV_X64_MSR_REENLIGHTENMENT_CONTROL:
	case HV_X64_MSR_TSC_EMULATION_CONTROL:
	case HV_X64_MSR_TSC_EMULATION_STATUS:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_ACCESS_REENLIGHTENMENT;
	case HV_X64_MSR_TSC_INVARIANT_CONTROL:
		return hv_vcpu->cpuid_cache.features_eax &
			HV_ACCESS_TSC_INVARIANT;
	case HV_X64_MSR_CRASH_P0 ... HV_X64_MSR_CRASH_P4:
	case HV_X64_MSR_CRASH_CTL:
		return hv_vcpu->cpuid_cache.features_edx &
			HV_FEATURE_GUEST_CRASH_MSR_AVAILABLE;
	case HV_X64_MSR_SYNDBG_OPTIONS:
	case HV_X64_MSR_SYNDBG_CONTROL ... HV_X64_MSR_SYNDBG_PENDING_BUFFER:
		return hv_vcpu->cpuid_cache.features_edx &
			HV_FEATURE_DEBUG_MSRS_AVAILABLE;
	default:
		break;
	}

	return false;
}

#define KVM_HV_WIN2016_GUEST_ID 0x1040a00003839
#define KVM_HV_WIN2016_GUEST_ID_MASK (~GENMASK_ULL(23, 16)) /* mask out the service version */

/*
 * Hyper-V enabled Windows Server 2016 SMP VMs fail to boot in !XSAVES && XSAVEC
 * configuration.
 * Such configuration can result from, for example, AMD Erratum 1386 workaround.
 *
 * Print a notice so users aren't left wondering what's suddenly gone wrong.
 */
static void __kvm_hv_xsaves_xsavec_maybe_warn(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;
	struct kvm_hv *hv = to_kvm_hv(kvm);

	/* Check again under the hv_lock.  */
	if (hv->xsaves_xsavec_checked)
		return;

	if ((hv->hv_guest_os_id & KVM_HV_WIN2016_GUEST_ID_MASK) !=
	    KVM_HV_WIN2016_GUEST_ID)
		return;

	hv->xsaves_xsavec_checked = true;

	/* UP configurations aren't affected */
	if (atomic_read(&kvm->online_vcpus) < 2)
		return;

	if (guest_cpuid_has(vcpu, X86_FEATURE_XSAVES) ||
	    !guest_cpu_cap_has(vcpu, X86_FEATURE_XSAVEC))
		return;

	pr_notice_ratelimited("Booting SMP Windows KVM VM with !XSAVES && XSAVEC. "
			      "If it fails to boot try disabling XSAVEC in the VM config.\n");
}

void kvm_hv_xsaves_xsavec_maybe_warn(struct kvm_vcpu *vcpu)
{
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);

	if (!vcpu->arch.hyperv_enabled ||
	    hv->xsaves_xsavec_checked)
		return;

	mutex_lock(&hv->hv_lock);
	__kvm_hv_xsaves_xsavec_maybe_warn(vcpu);
	mutex_unlock(&hv->hv_lock);
}

static int kvm_hv_set_msr_pw(struct kvm_vcpu *vcpu, u32 msr, u64 data,
			     bool host)
{
	struct kvm *kvm = vcpu->kvm;
	struct kvm_hv *hv = to_kvm_hv(kvm);

	if (unlikely(!host && !hv_check_msr_access(to_hv_vcpu(vcpu), msr)))
		return 1;

	switch (msr) {
	case HV_X64_MSR_GUEST_OS_ID:
		hv->hv_guest_os_id = data;
		/* setting guest os id to zero disables hypercall page */
		if (!hv->hv_guest_os_id)
			hv->hv_hypercall &= ~HV_X64_MSR_HYPERCALL_ENABLE;
		break;
	case HV_X64_MSR_HYPERCALL: {
		u8 instructions[9];
		int i = 0;
		u64 addr;

		/* if guest os id is not set hypercall should remain disabled */
		if (!hv->hv_guest_os_id)
			break;
		if (!(data & HV_X64_MSR_HYPERCALL_ENABLE)) {
			hv->hv_hypercall = data;
			break;
		}

		/*
		 * If Xen and Hyper-V hypercalls are both enabled, disambiguate
		 * the same way Xen itself does, by setting the bit 31 of EAX
		 * which is RsvdZ in the 32-bit Hyper-V hypercall ABI and just
		 * going to be clobbered on 64-bit.
		 */
		if (kvm_xen_hypercall_enabled(kvm)) {
			/* orl $0x80000000, %eax */
			instructions[i++] = 0x0d;
			instructions[i++] = 0x00;
			instructions[i++] = 0x00;
			instructions[i++] = 0x00;
			instructions[i++] = 0x80;
		}

		/* vmcall/vmmcall */
		kvm_x86_call(patch_hypercall)(vcpu, instructions + i);
		i += 3;