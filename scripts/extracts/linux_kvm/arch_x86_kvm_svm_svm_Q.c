// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 17/36



	switch (msr_info->index) {
	case MSR_AMD64_TSC_RATIO:
		if (!msr_info->host_initiated &&
		    !guest_cpu_cap_has(vcpu, X86_FEATURE_TSCRATEMSR))
			return 1;
		msr_info->data = svm->tsc_ratio_msr;
		break;
	case MSR_STAR:
		msr_info->data = svm->vmcb01.ptr->save.star;
		break;
#ifdef CONFIG_X86_64
	case MSR_LSTAR:
		msr_info->data = svm->vmcb01.ptr->save.lstar;
		break;
	case MSR_CSTAR:
		msr_info->data = svm->vmcb01.ptr->save.cstar;
		break;
	case MSR_GS_BASE:
		msr_info->data = svm->vmcb01.ptr->save.gs.base;
		break;
	case MSR_FS_BASE:
		msr_info->data = svm->vmcb01.ptr->save.fs.base;
		break;
	case MSR_KERNEL_GS_BASE:
		msr_info->data = svm->vmcb01.ptr->save.kernel_gs_base;
		break;
	case MSR_SYSCALL_MASK:
		msr_info->data = svm->vmcb01.ptr->save.sfmask;
		break;
#endif
	case MSR_IA32_SYSENTER_CS:
		msr_info->data = svm->vmcb01.ptr->save.sysenter_cs;
		break;
	case MSR_IA32_SYSENTER_EIP:
		msr_info->data = (u32)svm->vmcb01.ptr->save.sysenter_eip;
		if (guest_cpuid_is_intel_compatible(vcpu))
			msr_info->data |= (u64)svm->sysenter_eip_hi << 32;
		break;
	case MSR_IA32_SYSENTER_ESP:
		msr_info->data = svm->vmcb01.ptr->save.sysenter_esp;
		if (guest_cpuid_is_intel_compatible(vcpu))
			msr_info->data |= (u64)svm->sysenter_esp_hi << 32;
		break;
	case MSR_IA32_S_CET:
		msr_info->data = svm->vmcb->save.s_cet;
		break;
	case MSR_IA32_INT_SSP_TAB:
		msr_info->data = svm->vmcb->save.isst_addr;
		break;
	case MSR_KVM_INTERNAL_GUEST_SSP:
		msr_info->data = svm->vmcb->save.ssp;
		break;
	case MSR_TSC_AUX:
		msr_info->data = svm->tsc_aux;
		break;
	case MSR_IA32_DEBUGCTLMSR:
		msr_info->data = lbrv ? svm->vmcb->save.dbgctl : 0;
		break;
	case MSR_IA32_LASTBRANCHFROMIP:
	case MSR_IA32_LASTBRANCHTOIP:
	case MSR_IA32_LASTINTFROMIP:
	case MSR_IA32_LASTINTTOIP:
		msr_info->data = lbrv ? *svm_vmcb_lbr(svm, msr_info->index) : 0;
		break;
	case MSR_VM_HSAVE_PA:
		msr_info->data = svm->nested.hsave_msr;
		break;
	case MSR_VM_CR:
		msr_info->data = svm->nested.vm_cr_msr;
		break;
	case MSR_IA32_SPEC_CTRL:
		if (!msr_info->host_initiated &&
		    !guest_has_spec_ctrl_msr(vcpu))
			return 1;

		if (boot_cpu_has(X86_FEATURE_V_SPEC_CTRL))
			msr_info->data = svm->vmcb->save.spec_ctrl;
		else
			msr_info->data = svm->spec_ctrl;
		break;
	case MSR_AMD64_VIRT_SPEC_CTRL:
		if (!msr_info->host_initiated &&
		    !guest_cpu_cap_has(vcpu, X86_FEATURE_VIRT_SSBD))
			return 1;

		msr_info->data = svm->virt_spec_ctrl;
		break;
	case MSR_F15H_IC_CFG: {

		int family, model;

		family = guest_cpuid_family(vcpu);
		model  = guest_cpuid_model(vcpu);

		if (family < 0 || model < 0)
			return kvm_get_msr_common(vcpu, msr_info);

		msr_info->data = 0;

		if (family == 0x15 &&
		    (model >= 0x2 && model < 0x20))
			msr_info->data = 0x1E;
		}
		break;
	case MSR_AMD64_DE_CFG:
		msr_info->data = svm->msr_decfg;
		break;
	default:
		return kvm_get_msr_common(vcpu, msr_info);
	}
	return 0;
}

static int svm_complete_emulated_msr(struct kvm_vcpu *vcpu, int err)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	if (!err || !is_sev_es_guest(vcpu) || WARN_ON_ONCE(!svm->sev_es.ghcb))
		return kvm_complete_insn_gp(vcpu, err);

	svm_vmgexit_inject_exception(svm, X86_TRAP_GP);
	return 1;
}

static int svm_set_vm_cr(struct kvm_vcpu *vcpu, u64 data)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int svm_dis, chg_mask;

	if (data & ~SVM_VM_CR_VALID_MASK)
		return 1;

	chg_mask = SVM_VM_CR_VALID_MASK;

	if (svm->nested.vm_cr_msr & SVM_VM_CR_SVM_DIS_MASK)
		chg_mask &= ~(SVM_VM_CR_SVM_LOCK_MASK | SVM_VM_CR_SVM_DIS_MASK);

	svm->nested.vm_cr_msr &= ~chg_mask;
	svm->nested.vm_cr_msr |= (data & chg_mask);

	svm_dis = svm->nested.vm_cr_msr & SVM_VM_CR_SVM_DIS_MASK;

	/* check for svm_disable while efer.svme is set */
	if (svm_dis && (vcpu->arch.efer & EFER_SVME))
		return 1;

	return 0;
}

static int svm_set_msr(struct kvm_vcpu *vcpu, struct msr_data *msr)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int ret = 0;

	u32 ecx = msr->index;
	u64 data = msr->data;

	if (sev_es_prevent_msr_access(vcpu, msr))
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;

	switch (ecx) {
	case MSR_AMD64_TSC_RATIO:

		if (!guest_cpu_cap_has(vcpu, X86_FEATURE_TSCRATEMSR)) {

			if (!msr->host_initiated)
				return 1;
			/*
			 * In case TSC scaling is not enabled, always
			 * leave this MSR at the default value.
			 *
			 * Due to bug in qemu 6.2.0, it would try to set
			 * this msr to 0 if tsc scaling is not enabled.
			 * Ignore this value as well.
			 */
			if (data != 0 && data != svm->tsc_ratio_msr)
				return 1;
			break;
		}

		if (data & SVM_TSC_RATIO_RSVD)
			return 1;

		svm->tsc_ratio_msr = data;

		if (guest_cpu_cap_has(vcpu, X86_FEATURE_TSCRATEMSR) &&
		    is_guest_mode(vcpu))
			nested_svm_update_tsc_ratio_msr(vcpu);

		break;
	case MSR_IA32_CR_PAT:
		ret = kvm_set_msr_common(vcpu, msr);
		if (ret)
			break;