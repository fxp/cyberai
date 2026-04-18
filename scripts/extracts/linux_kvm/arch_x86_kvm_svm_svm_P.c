// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 16/36



	return kvm_complete_insn_gp(vcpu, ret);
}

static int dr_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int reg, dr;
	int err = 0;

	/*
	 * SEV-ES intercepts DR7 only to disable guest debugging and the guest issues a VMGEXIT
	 * for DR7 write only. KVM cannot change DR7 (always swapped as type 'A') so return early.
	 */
	if (is_sev_es_guest(vcpu))
		return 1;

	if (vcpu->guest_debug == 0) {
		/*
		 * No more DR vmexits; force a reload of the debug registers
		 * and reenter on this instruction.  The next vmexit will
		 * retrieve the full state of the debug registers.
		 */
		clr_dr_intercepts(svm);
		vcpu->arch.switch_db_regs |= KVM_DEBUGREG_WONT_EXIT;
		return 1;
	}

	if (!boot_cpu_has(X86_FEATURE_DECODEASSISTS))
		return emulate_on_interception(vcpu);

	reg = svm->vmcb->control.exit_info_1 & SVM_EXITINFO_REG_MASK;
	dr = svm->vmcb->control.exit_code - SVM_EXIT_READ_DR0;
	if (dr >= 16) { /* mov to DRn  */
		dr -= 16;
		err = kvm_set_dr(vcpu, dr, kvm_register_read(vcpu, reg));
	} else {
		kvm_register_write(vcpu, reg, kvm_get_dr(vcpu, dr));
	}

	return kvm_complete_insn_gp(vcpu, err);
}

static int cr8_write_interception(struct kvm_vcpu *vcpu)
{
	u8 cr8_prev = kvm_get_cr8(vcpu);
	int r;

	WARN_ON_ONCE(kvm_vcpu_apicv_active(vcpu));

	/* instruction emulation calls kvm_set_cr8() */
	r = cr_interception(vcpu);
	if (lapic_in_kernel(vcpu))
		return r;
	if (cr8_prev <= kvm_get_cr8(vcpu))
		return r;
	vcpu->run->exit_reason = KVM_EXIT_SET_TPR;
	return 0;
}

static int efer_trap(struct kvm_vcpu *vcpu)
{
	struct msr_data msr_info;
	int ret;

	/*
	 * Clear the EFER_SVME bit from EFER. The SVM code always sets this
	 * bit in svm_set_efer(), but __kvm_valid_efer() checks it against
	 * whether the guest has X86_FEATURE_SVM - this avoids a failure if
	 * the guest doesn't have X86_FEATURE_SVM.
	 */
	msr_info.host_initiated = false;
	msr_info.index = MSR_EFER;
	msr_info.data = to_svm(vcpu)->vmcb->control.exit_info_1 & ~EFER_SVME;
	ret = kvm_set_msr_common(vcpu, &msr_info);

	return kvm_complete_insn_gp(vcpu, ret);
}

static int svm_get_feature_msr(u32 msr, u64 *data)
{
	*data = 0;

	switch (msr) {
	case MSR_AMD64_DE_CFG:
		if (cpu_feature_enabled(X86_FEATURE_LFENCE_RDTSC))
			*data |= MSR_AMD64_DE_CFG_LFENCE_SERIALIZE;
		break;
	default:
		return KVM_MSR_RET_UNSUPPORTED;
	}

	return 0;
}

static u64 *svm_vmcb_lbr(struct vcpu_svm *svm, u32 msr)
{
	switch (msr) {
	case MSR_IA32_LASTBRANCHFROMIP:
		return &svm->vmcb->save.br_from;
	case MSR_IA32_LASTBRANCHTOIP:
		return &svm->vmcb->save.br_to;
	case MSR_IA32_LASTINTFROMIP:
		return &svm->vmcb->save.last_excp_from;
	case MSR_IA32_LASTINTTOIP:
		return &svm->vmcb->save.last_excp_to;
	default:
		break;
	}
	KVM_BUG_ON(1, svm->vcpu.kvm);
	return &svm->vmcb->save.br_from;
}

static bool sev_es_prevent_msr_access(struct kvm_vcpu *vcpu,
				      struct msr_data *msr_info)
{
	return is_sev_es_guest(vcpu) && vcpu->arch.guest_state_protected &&
	       msr_info->index != MSR_IA32_XSS &&
	       !msr_write_intercepted(vcpu, msr_info->index);
}

static int svm_get_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (sev_es_prevent_msr_access(vcpu, msr_info)) {
		msr_info->data = 0;
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;
	}