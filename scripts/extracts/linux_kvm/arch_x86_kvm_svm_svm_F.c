// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 6/36



		svm_set_intercept_for_msr(vcpu, MSR_IA32_U_CET, MSR_TYPE_RW, !shstk_enabled);
		svm_set_intercept_for_msr(vcpu, MSR_IA32_S_CET, MSR_TYPE_RW, !shstk_enabled);
		svm_set_intercept_for_msr(vcpu, MSR_IA32_PL0_SSP, MSR_TYPE_RW, !shstk_enabled);
		svm_set_intercept_for_msr(vcpu, MSR_IA32_PL1_SSP, MSR_TYPE_RW, !shstk_enabled);
		svm_set_intercept_for_msr(vcpu, MSR_IA32_PL2_SSP, MSR_TYPE_RW, !shstk_enabled);
		svm_set_intercept_for_msr(vcpu, MSR_IA32_PL3_SSP, MSR_TYPE_RW, !shstk_enabled);
	}

	if (is_sev_es_guest(vcpu))
		sev_es_recalc_msr_intercepts(vcpu);

	svm_recalc_pmu_msr_intercepts(vcpu);

	/*
	 * x2APIC intercepts are modified on-demand and cannot be filtered by
	 * userspace.
	 */
}

static void __svm_enable_lbrv(struct kvm_vcpu *vcpu)
{
	to_svm(vcpu)->vmcb->control.misc_ctl2 |= SVM_MISC2_ENABLE_V_LBR;
}

void svm_enable_lbrv(struct kvm_vcpu *vcpu)
{
	__svm_enable_lbrv(vcpu);
	svm_recalc_lbr_msr_intercepts(vcpu);
}

static void __svm_disable_lbrv(struct kvm_vcpu *vcpu)
{
	KVM_BUG_ON(is_sev_es_guest(vcpu), vcpu->kvm);
	to_svm(vcpu)->vmcb->control.misc_ctl2 &= ~SVM_MISC2_ENABLE_V_LBR;
}

void svm_update_lbrv(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	bool current_enable_lbrv = svm->vmcb->control.misc_ctl2 & SVM_MISC2_ENABLE_V_LBR;
	bool enable_lbrv = (svm->vmcb->save.dbgctl & DEBUGCTLMSR_LBR) ||
			    (is_guest_mode(vcpu) && guest_cpu_cap_has(vcpu, X86_FEATURE_LBRV) &&
			    (svm->nested.ctl.misc_ctl2 & SVM_MISC2_ENABLE_V_LBR));

	if (enable_lbrv && !current_enable_lbrv)
		__svm_enable_lbrv(vcpu);
	else if (!enable_lbrv && current_enable_lbrv)
		__svm_disable_lbrv(vcpu);

	/*
	 * During nested transitions, it is possible that the current VMCB has
	 * LBR_CTL set, but the previous LBR_CTL had it cleared (or vice versa).
	 * In this case, even though LBR_CTL does not need an update, intercepts
	 * do, so always recalculate the intercepts here.
	 */
	svm_recalc_lbr_msr_intercepts(vcpu);
}

void disable_nmi_singlestep(struct vcpu_svm *svm)
{
	svm->nmi_singlestep = false;

	if (!(svm->vcpu.guest_debug & KVM_GUESTDBG_SINGLESTEP)) {
		/* Clear our flags if they were not set by the guest */
		if (!(svm->nmi_singlestep_guest_rflags & X86_EFLAGS_TF))
			svm->vmcb->save.rflags &= ~X86_EFLAGS_TF;
		if (!(svm->nmi_singlestep_guest_rflags & X86_EFLAGS_RF))
			svm->vmcb->save.rflags &= ~X86_EFLAGS_RF;
	}
}

static void grow_ple_window(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb_control_area *control = &svm->vmcb->control;
	int old = control->pause_filter_count;

	if (kvm_pause_in_guest(vcpu->kvm))
		return;

	control->pause_filter_count = __grow_ple_window(old,
							pause_filter_count,
							pause_filter_count_grow,
							pause_filter_count_max);

	if (control->pause_filter_count != old) {
		vmcb_mark_dirty(svm->vmcb, VMCB_INTERCEPTS);
		trace_kvm_ple_window_update(vcpu->vcpu_id,
					    control->pause_filter_count, old);
	}
}

static void shrink_ple_window(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb_control_area *control = &svm->vmcb->control;
	int old = control->pause_filter_count;

	if (kvm_pause_in_guest(vcpu->kvm))
		return;

	control->pause_filter_count =
				__shrink_ple_window(old,
						    pause_filter_count,
						    pause_filter_count_shrink,
						    pause_filter_count);
	if (control->pause_filter_count != old) {
		vmcb_mark_dirty(svm->vmcb, VMCB_INTERCEPTS);
		trace_kvm_ple_window_update(vcpu->vcpu_id,
					    control->pause_filter_count, old);
	}
}

static void svm_hardware_unsetup(void)
{
	int cpu;

	avic_hardware_unsetup();

	sev_hardware_unsetup();

	for_each_possible_cpu(cpu)
		svm_cpu_uninit(cpu);

	__free_pages(__sme_pa_to_page(iopm_base), get_order(IOPM_SIZE));
	iopm_base = 0;
}

static void init_seg(struct vmcb_seg *seg)
{
	seg->selector = 0;
	seg->attrib = SVM_SELECTOR_P_MASK | SVM_SELECTOR_S_MASK |
		      SVM_SELECTOR_WRITE_MASK; /* Read/Write Data Segment */
	seg->limit = 0xffff;
	seg->base = 0;
}

static void init_sys_seg(struct vmcb_seg *seg, uint32_t type)
{
	seg->selector = 0;
	seg->attrib = SVM_SELECTOR_P_MASK | type;
	seg->limit = 0xffff;
	seg->base = 0;
}

static u64 svm_get_l2_tsc_offset(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	return svm->nested.ctl.tsc_offset;
}

static u64 svm_get_l2_tsc_multiplier(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	return svm->tsc_ratio_msr;
}

static void svm_write_tsc_offset(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->vmcb01.ptr->control.tsc_offset = vcpu->arch.l1_tsc_offset;
	svm->vmcb->control.tsc_offset = vcpu->arch.tsc_offset;
	vmcb_mark_dirty(svm->vmcb, VMCB_INTERCEPTS);
}

void svm_write_tsc_multiplier(struct kvm_vcpu *vcpu)
{
	preempt_disable();
	if (to_svm(vcpu)->guest_state_loaded)
		__svm_write_tsc_multiplier(vcpu->arch.tsc_scaling_ratio);
	preempt_enable();
}