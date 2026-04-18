// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 23/36



		pr_err("%-15s %016llx %-13s %016llx\n",
		       "rax:", vmsa->rax, "rbx:", vmsa->rbx);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "rcx:", vmsa->rcx, "rdx:", vmsa->rdx);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "rsi:", vmsa->rsi, "rdi:", vmsa->rdi);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "rbp:", vmsa->rbp, "rsp:", vmsa->rsp);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "r8:", vmsa->r8, "r9:", vmsa->r9);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "r10:", vmsa->r10, "r11:", vmsa->r11);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "r12:", vmsa->r12, "r13:", vmsa->r13);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "r14:", vmsa->r14, "r15:", vmsa->r15);
		pr_err("%-15s %016llx %-13s %016llx\n",
		       "xcr0:", vmsa->xcr0, "xss:", vmsa->xss);
	} else {
		pr_err("%-15s %016llx %-13s %016lx\n",
		       "rax:", save->rax, "rbx:",
		       vcpu->arch.regs[VCPU_REGS_RBX]);
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "rcx:", vcpu->arch.regs[VCPU_REGS_RCX],
		       "rdx:", vcpu->arch.regs[VCPU_REGS_RDX]);
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "rsi:", vcpu->arch.regs[VCPU_REGS_RSI],
		       "rdi:", vcpu->arch.regs[VCPU_REGS_RDI]);
		pr_err("%-15s %016lx %-13s %016llx\n",
		       "rbp:", vcpu->arch.regs[VCPU_REGS_RBP],
		       "rsp:", save->rsp);
#ifdef CONFIG_X86_64
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "r8:", vcpu->arch.regs[VCPU_REGS_R8],
		       "r9:", vcpu->arch.regs[VCPU_REGS_R9]);
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "r10:", vcpu->arch.regs[VCPU_REGS_R10],
		       "r11:", vcpu->arch.regs[VCPU_REGS_R11]);
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "r12:", vcpu->arch.regs[VCPU_REGS_R12],
		       "r13:", vcpu->arch.regs[VCPU_REGS_R13]);
		pr_err("%-15s %016lx %-13s %016lx\n",
		       "r14:", vcpu->arch.regs[VCPU_REGS_R14],
		       "r15:", vcpu->arch.regs[VCPU_REGS_R15]);
#endif
	}

no_vmsa:
	if (is_sev_es_guest(vcpu))
		sev_free_decrypted_vmsa(vcpu, save);
}

int svm_invoke_exit_handler(struct kvm_vcpu *vcpu, u64 __exit_code)
{
	u32 exit_code = __exit_code;

	/*
	 * SVM uses negative values, i.e. 64-bit values, to indicate that VMRUN
	 * failed.  Report all such errors to userspace (note, VMEXIT_INVALID,
	 * a.k.a. SVM_EXIT_ERR, is special cased by svm_handle_exit()).  Skip
	 * the check when running as a VM, as KVM has historically left garbage
	 * in bits 63:32, i.e. running KVM-on-KVM would hit false positives if
	 * the underlying kernel is buggy.
	 */
	if (!cpu_feature_enabled(X86_FEATURE_HYPERVISOR) &&
	    (u64)exit_code != __exit_code)
		goto unexpected_vmexit;

#ifdef CONFIG_MITIGATION_RETPOLINE
	if (exit_code == SVM_EXIT_MSR)
		return msr_interception(vcpu);
	else if (exit_code == SVM_EXIT_VINTR)
		return interrupt_window_interception(vcpu);
	else if (exit_code == SVM_EXIT_INTR)
		return intr_interception(vcpu);
	else if (exit_code == SVM_EXIT_HLT || exit_code == SVM_EXIT_IDLE_HLT)
		return kvm_emulate_halt(vcpu);
	else if (exit_code == SVM_EXIT_NPF)
		return npf_interception(vcpu);
#ifdef CONFIG_KVM_AMD_SEV
	else if (exit_code == SVM_EXIT_VMGEXIT)
		return sev_handle_vmgexit(vcpu);
#endif
#endif
	if (exit_code >= ARRAY_SIZE(svm_exit_handlers))
		goto unexpected_vmexit;

	exit_code = array_index_nospec(exit_code, ARRAY_SIZE(svm_exit_handlers));
	if (!svm_exit_handlers[exit_code])
		goto unexpected_vmexit;

	return svm_exit_handlers[exit_code](vcpu);

unexpected_vmexit:
	dump_vmcb(vcpu);
	kvm_prepare_unexpected_reason_exit(vcpu, __exit_code);
	return 0;
}

static void svm_get_exit_info(struct kvm_vcpu *vcpu, u32 *reason,
			      u64 *info1, u64 *info2,
			      u32 *intr_info, u32 *error_code)
{
	struct vmcb_control_area *control = &to_svm(vcpu)->vmcb->control;

	*reason = control->exit_code;
	*info1 = control->exit_info_1;
	*info2 = control->exit_info_2;
	*intr_info = control->exit_int_info;
	if ((*intr_info & SVM_EXITINTINFO_VALID) &&
	    (*intr_info & SVM_EXITINTINFO_VALID_ERR))
		*error_code = control->exit_int_info_err;
	else
		*error_code = 0;
}

static void svm_get_entry_info(struct kvm_vcpu *vcpu, u32 *intr_info,
			       u32 *error_code)
{
	struct vmcb_control_area *control = &to_svm(vcpu)->vmcb->control;

	*intr_info = control->event_inj;

	if ((*intr_info & SVM_EXITINTINFO_VALID) &&
	    (*intr_info & SVM_EXITINTINFO_VALID_ERR))
		*error_code = control->event_inj_err;
	else
		*error_code = 0;

}

static int svm_handle_exit(struct kvm_vcpu *vcpu, fastpath_t exit_fastpath)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct kvm_run *kvm_run = vcpu->run;

	/* SEV-ES guests must use the CR write traps to track CR registers. */
	if (!is_sev_es_guest(vcpu)) {
		if (!svm_is_intercept(svm, INTERCEPT_CR0_WRITE))
			vcpu->arch.cr0 = svm->vmcb->save.cr0;
		if (npt_enabled)
			vcpu->arch.cr3 = svm->vmcb->save.cr3;
	}

	if (is_guest_mode(vcpu)) {
		int vmexit;

		trace_kvm_nested_vmexit(vcpu, KVM_ISA_SVM);

		vmexit = nested_svm_exit_special(svm);