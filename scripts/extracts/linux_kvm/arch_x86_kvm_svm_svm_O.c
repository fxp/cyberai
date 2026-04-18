// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 15/36



	if (reason == TASK_SWITCH_GATE) {
		switch (type) {
		case SVM_EXITINTINFO_TYPE_NMI:
			vcpu->arch.nmi_injected = false;
			break;
		case SVM_EXITINTINFO_TYPE_EXEPT:
			if (svm->vmcb->control.exit_info_2 &
			    (1ULL << SVM_EXITINFOSHIFT_TS_HAS_ERROR_CODE)) {
				has_error_code = true;
				error_code =
					(u32)svm->vmcb->control.exit_info_2;
			}
			kvm_clear_exception_queue(vcpu);
			break;
		case SVM_EXITINTINFO_TYPE_INTR:
		case SVM_EXITINTINFO_TYPE_SOFT:
			kvm_clear_interrupt_queue(vcpu);
			break;
		default:
			break;
		}
	}

	if (reason != TASK_SWITCH_GATE ||
	    int_type == SVM_EXITINTINFO_TYPE_SOFT ||
	    (int_type == SVM_EXITINTINFO_TYPE_EXEPT &&
	     (int_vec == OF_VECTOR || int_vec == BP_VECTOR))) {
		if (!svm_skip_emulated_instruction(vcpu))
			return 0;
	}

	if (int_type != SVM_EXITINTINFO_TYPE_SOFT)
		int_vec = -1;

	return kvm_task_switch(vcpu, tss_selector, int_vec, reason,
			       has_error_code, error_code);
}

static void svm_clr_iret_intercept(struct vcpu_svm *svm)
{
	if (!is_sev_es_guest(&svm->vcpu))
		svm_clr_intercept(svm, INTERCEPT_IRET);
}

static void svm_set_iret_intercept(struct vcpu_svm *svm)
{
	if (!is_sev_es_guest(&svm->vcpu))
		svm_set_intercept(svm, INTERCEPT_IRET);
}

static int iret_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	WARN_ON_ONCE(is_sev_es_guest(vcpu));

	++vcpu->stat.nmi_window_exits;
	svm->awaiting_iret_completion = true;

	svm_clr_iret_intercept(svm);
	svm->nmi_iret_rip = kvm_rip_read(vcpu);

	kvm_make_request(KVM_REQ_EVENT, vcpu);
	return 1;
}

static int invlpg_interception(struct kvm_vcpu *vcpu)
{
	if (!static_cpu_has(X86_FEATURE_DECODEASSISTS))
		return kvm_emulate_instruction(vcpu, 0);

	kvm_mmu_invlpg(vcpu, to_svm(vcpu)->vmcb->control.exit_info_1);
	return kvm_skip_emulated_instruction(vcpu);
}

static int emulate_on_interception(struct kvm_vcpu *vcpu)
{
	return kvm_emulate_instruction(vcpu, 0);
}

static int rsm_interception(struct kvm_vcpu *vcpu)
{
	return kvm_emulate_instruction_from_buffer(vcpu, rsm_ins_bytes, 2);
}

static bool check_selective_cr0_intercepted(struct kvm_vcpu *vcpu,
					    unsigned long val)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	unsigned long cr0 = vcpu->arch.cr0;
	bool ret = false;

	if (!is_guest_mode(vcpu) ||
	    (!(vmcb12_is_intercept(&svm->nested.ctl, INTERCEPT_SELECTIVE_CR0))))
		return false;

	cr0 &= ~SVM_CR0_SELECTIVE_MASK;
	val &= ~SVM_CR0_SELECTIVE_MASK;

	if (cr0 ^ val) {
		svm->vmcb->control.exit_code = SVM_EXIT_CR0_SEL_WRITE;
		ret = (nested_svm_exit_handled(svm) == NESTED_EXIT_DONE);
	}

	return ret;
}

#define CR_VALID (1ULL << 63)

static int cr_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	int reg, cr;
	unsigned long val;
	int err;

	if (!static_cpu_has(X86_FEATURE_DECODEASSISTS))
		return emulate_on_interception(vcpu);

	if (unlikely((svm->vmcb->control.exit_info_1 & CR_VALID) == 0))
		return emulate_on_interception(vcpu);

	reg = svm->vmcb->control.exit_info_1 & SVM_EXITINFO_REG_MASK;
	if (svm->vmcb->control.exit_code == SVM_EXIT_CR0_SEL_WRITE)
		cr = SVM_EXIT_WRITE_CR0 - SVM_EXIT_READ_CR0;
	else
		cr = svm->vmcb->control.exit_code - SVM_EXIT_READ_CR0;

	err = 0;
	if (cr >= 16) { /* mov to cr */
		cr -= 16;
		val = kvm_register_read(vcpu, reg);
		trace_kvm_cr_write(cr, val);
		switch (cr) {
		case 0:
			if (!check_selective_cr0_intercepted(vcpu, val))
				err = kvm_set_cr0(vcpu, val);
			else
				return 1;

			break;
		case 3:
			err = kvm_set_cr3(vcpu, val);
			break;
		case 4:
			err = kvm_set_cr4(vcpu, val);
			break;
		case 8:
			err = kvm_set_cr8(vcpu, val);
			break;
		default:
			WARN(1, "unhandled write to CR%d", cr);
			kvm_queue_exception(vcpu, UD_VECTOR);
			return 1;
		}
	} else { /* mov from cr */
		switch (cr) {
		case 0:
			val = kvm_read_cr0(vcpu);
			break;
		case 2:
			val = vcpu->arch.cr2;
			break;
		case 3:
			val = kvm_read_cr3(vcpu);
			break;
		case 4:
			val = kvm_read_cr4(vcpu);
			break;
		case 8:
			val = kvm_get_cr8(vcpu);
			break;
		default:
			WARN(1, "unhandled read from CR%d", cr);
			kvm_queue_exception(vcpu, UD_VECTOR);
			return 1;
		}
		kvm_register_write(vcpu, reg, val);
		trace_kvm_cr_read(cr, val);
	}
	return kvm_complete_insn_gp(vcpu, err);
}

static int cr_trap(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	unsigned long old_value, new_value;
	unsigned int cr;
	int ret = 0;

	new_value = (unsigned long)svm->vmcb->control.exit_info_1;

	cr = svm->vmcb->control.exit_code - SVM_EXIT_CR0_WRITE_TRAP;
	switch (cr) {
	case 0:
		old_value = kvm_read_cr0(vcpu);
		svm_set_cr0(vcpu, new_value);

		kvm_post_set_cr0(vcpu, old_value, new_value);
		break;
	case 4:
		old_value = kvm_read_cr4(vcpu);
		svm_set_cr4(vcpu, new_value);

		kvm_post_set_cr4(vcpu, old_value, new_value);
		break;
	case 8:
		ret = kvm_set_cr8(vcpu, new_value);
		break;
	default:
		WARN(1, "unhandled CR%d write trap", cr);
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}