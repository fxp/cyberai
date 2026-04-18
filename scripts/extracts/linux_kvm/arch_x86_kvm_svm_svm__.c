// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 31/36



		/*
		 * Convert the exit_code to SVM_EXIT_CR0_SEL_WRITE if a
		 * selective CR0 intercept is triggered (the common logic will
		 * treat the selective intercept as being enabled).  Note, the
		 * unconditional intercept has higher priority, i.e. this is
		 * only relevant if *only* the selective intercept is enabled.
		 */
		if (vmcb12_is_intercept(&svm->nested.ctl, INTERCEPT_CR0_WRITE) ||
		    !(vmcb12_is_intercept(&svm->nested.ctl, INTERCEPT_SELECTIVE_CR0)))
			break;

		/* CLTS never triggers INTERCEPT_SELECTIVE_CR0 */
		if (info->intercept == x86_intercept_clts)
			break;

		/* LMSW always triggers INTERCEPT_SELECTIVE_CR0 */
		if (info->intercept == x86_intercept_lmsw) {
			icpt_info.exit_code = SVM_EXIT_CR0_SEL_WRITE;
			break;
		}

		/*
		 * MOV-to-CR0 only triggers INTERCEPT_SELECTIVE_CR0 if any bit
		 * other than SVM_CR0_SELECTIVE_MASK is changed.
		 */
		cr0 = vcpu->arch.cr0 & ~SVM_CR0_SELECTIVE_MASK;
		val = info->src_val  & ~SVM_CR0_SELECTIVE_MASK;
		if (cr0 ^ val)
			icpt_info.exit_code = SVM_EXIT_CR0_SEL_WRITE;
		break;
	}
	case SVM_EXIT_READ_DR0:
	case SVM_EXIT_WRITE_DR0:
		icpt_info.exit_code += info->modrm_reg;
		break;
	case SVM_EXIT_MSR:
		if (info->intercept == x86_intercept_wrmsr)
			vmcb->control.exit_info_1 = 1;
		else
			vmcb->control.exit_info_1 = 0;
		break;
	case SVM_EXIT_PAUSE:
		/*
		 * We get this for NOP only, but pause
		 * is rep not, check this here
		 */
		if (info->rep_prefix != REPE_PREFIX)
			goto out;
		break;
	case SVM_EXIT_IOIO: {
		u64 exit_info;
		u32 bytes;

		if (info->intercept == x86_intercept_in ||
		    info->intercept == x86_intercept_ins) {
			exit_info = ((info->src_val & 0xffff) << 16) |
				SVM_IOIO_TYPE_MASK;
			bytes = info->dst_bytes;
		} else {
			exit_info = (info->dst_val & 0xffff) << 16;
			bytes = info->src_bytes;
		}

		if (info->intercept == x86_intercept_outs ||
		    info->intercept == x86_intercept_ins)
			exit_info |= SVM_IOIO_STR_MASK;

		if (info->rep_prefix)
			exit_info |= SVM_IOIO_REP_MASK;

		bytes = min(bytes, 4u);

		exit_info |= bytes << SVM_IOIO_SIZE_SHIFT;

		exit_info |= (u32)info->ad_bytes << (SVM_IOIO_ASIZE_SHIFT - 1);

		vmcb->control.exit_info_1 = exit_info;
		vmcb->control.exit_info_2 = info->next_rip;

		break;
	}
	default:
		break;
	}

	/* TODO: Advertise NRIPS to guest hypervisor unconditionally */
	if (static_cpu_has(X86_FEATURE_NRIPS))
		vmcb->control.next_rip  = info->next_rip;
	vmcb->control.exit_code = icpt_info.exit_code;
	vmexit = nested_svm_exit_handled(svm);

	ret = (vmexit == NESTED_EXIT_DONE) ? X86EMUL_INTERCEPTED
					   : X86EMUL_CONTINUE;

out:
	return ret;
}

static void svm_handle_exit_irqoff(struct kvm_vcpu *vcpu)
{
	switch (to_svm(vcpu)->vmcb->control.exit_code) {
	case SVM_EXIT_EXCP_BASE + MC_VECTOR:
		svm_handle_mce(vcpu);
		break;
	case SVM_EXIT_INTR:
		vcpu->arch.at_instruction_boundary = true;
		break;
	default:
		break;
	}
}

static void svm_setup_mce(struct kvm_vcpu *vcpu)
{
	/* [63:9] are reserved. */
	vcpu->arch.mcg_cap &= 0x1ff;
}

#ifdef CONFIG_KVM_SMM
bool svm_smi_blocked(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/* Per APM Vol.2 15.22.2 "Response to SMI" */
	if (!gif_set(svm))
		return true;

	return is_smm(vcpu);
}

static int svm_smi_allowed(struct kvm_vcpu *vcpu, bool for_injection)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	if (vcpu->arch.nested_run_pending)
		return -EBUSY;

	if (svm_smi_blocked(vcpu))
		return 0;

	/* An SMI must not be injected into L2 if it's supposed to VM-Exit.  */
	if (for_injection && is_guest_mode(vcpu) && nested_exit_on_smi(svm))
		return -EBUSY;

	return 1;
}

static int svm_enter_smm(struct kvm_vcpu *vcpu, union kvm_smram *smram)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct kvm_host_map map_save;

	if (!is_guest_mode(vcpu))
		return 0;

	/*
	 * 32-bit SMRAM format doesn't preserve EFER and SVM state.  Userspace is
	 * responsible for ensuring nested SVM and SMIs are mutually exclusive.
	 */

	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_LM))
		return 1;

	smram->smram64.svm_guest_flag = 1;
	smram->smram64.svm_guest_vmcb_gpa = svm->nested.vmcb12_gpa;

	svm->vmcb->save.rax = vcpu->arch.regs[VCPU_REGS_RAX];
	svm->vmcb->save.rsp = vcpu->arch.regs[VCPU_REGS_RSP];
	svm->vmcb->save.rip = vcpu->arch.regs[VCPU_REGS_RIP];

	nested_svm_simple_vmexit(svm, SVM_EXIT_SW);

	/*
	 * KVM uses VMCB01 to store L1 host state while L2 runs but
	 * VMCB01 is going to be used during SMM and thus the state will
	 * be lost. Temporary save non-VMLOAD/VMSAVE state to the host save
	 * area pointed to by MSR_VM_HSAVE_PA. APM guarantees that the
	 * format of the area is identical to guest save area offsetted
	 * by 0x400 (matches the offset of 'struct vmcb_save_area'
	 * within 'struct vmcb'). Note: HSAVE area may also be used by
	 * L1 hypervisor to save additional host context (e.g. KVM does
	 * that, see svm_prepare_switch_to_guest()) which must be
	 * preserved.
	 */
	if (kvm_vcpu_map(vcpu, gpa_to_gfn(svm->nested.hsave_msr), &map_save))
		return 1;