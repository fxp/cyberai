// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 9/36



error_free_sev:
	sev_free_vcpu(vcpu);
error_free_vmcb_page:
	__free_page(vmcb01_page);
out:
	return err;
}

static void svm_vcpu_free(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	WARN_ON_ONCE(!list_empty(&svm->ir_list));

	svm_leave_nested(vcpu);
	svm_free_nested(svm);

	sev_free_vcpu(vcpu);

	__free_page(__sme_pa_to_page(svm->vmcb01.pa));
	svm_vcpu_free_msrpm(svm->msrpm);
}

#ifdef CONFIG_CPU_MITIGATIONS
static DEFINE_SPINLOCK(srso_lock);
static atomic_t srso_nr_vms;

static void svm_srso_clear_bp_spec_reduce(void *ign)
{
	struct svm_cpu_data *sd = this_cpu_ptr(&svm_data);

	if (!sd->bp_spec_reduce_set)
		return;

	msr_clear_bit(MSR_ZEN4_BP_CFG, MSR_ZEN4_BP_CFG_BP_SPEC_REDUCE_BIT);
	sd->bp_spec_reduce_set = false;
}

static void svm_srso_vm_destroy(void)
{
	if (!cpu_feature_enabled(X86_FEATURE_SRSO_BP_SPEC_REDUCE))
		return;

	if (atomic_dec_return(&srso_nr_vms))
		return;

	guard(spinlock)(&srso_lock);

	/*
	 * Verify a new VM didn't come along, acquire the lock, and increment
	 * the count before this task acquired the lock.
	 */
	if (atomic_read(&srso_nr_vms))
		return;

	on_each_cpu(svm_srso_clear_bp_spec_reduce, NULL, 1);
}

static void svm_srso_vm_init(void)
{
	if (!cpu_feature_enabled(X86_FEATURE_SRSO_BP_SPEC_REDUCE))
		return;

	/*
	 * Acquire the lock on 0 => 1 transitions to ensure a potential 1 => 0
	 * transition, i.e. destroying the last VM, is fully complete, e.g. so
	 * that a delayed IPI doesn't clear BP_SPEC_REDUCE after a vCPU runs.
	 */
	if (atomic_inc_not_zero(&srso_nr_vms))
		return;

	guard(spinlock)(&srso_lock);

	atomic_inc(&srso_nr_vms);
}
#else
static void svm_srso_vm_init(void) { }
static void svm_srso_vm_destroy(void) { }
#endif

static void svm_prepare_switch_to_guest(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct svm_cpu_data *sd = per_cpu_ptr(&svm_data, vcpu->cpu);

	if (is_sev_es_guest(vcpu))
		sev_es_unmap_ghcb(svm);

	if (svm->guest_state_loaded)
		return;

	/*
	 * Save additional host state that will be restored on VMEXIT (sev-es)
	 * or subsequent vmload of host save area.
	 */
	vmsave(sd->save_area_pa);
	if (is_sev_es_guest(vcpu))
		sev_es_prepare_switch_to_guest(svm, sev_es_host_save_area(sd));

	if (tsc_scaling)
		__svm_write_tsc_multiplier(vcpu->arch.tsc_scaling_ratio);

	/*
	 * TSC_AUX is always virtualized (context switched by hardware) for
	 * SEV-ES guests when the feature is available.  For non-SEV-ES guests,
	 * context switch TSC_AUX via the user_return MSR infrastructure (not
	 * all CPUs support TSC_AUX virtualization).
	 */
	if (likely(tsc_aux_uret_slot >= 0) &&
	    (!boot_cpu_has(X86_FEATURE_V_TSC_AUX) || !is_sev_es_guest(vcpu)))
		kvm_set_user_return_msr(tsc_aux_uret_slot, svm->tsc_aux, -1ull);

	if (cpu_feature_enabled(X86_FEATURE_SRSO_BP_SPEC_REDUCE) &&
	    !sd->bp_spec_reduce_set) {
		sd->bp_spec_reduce_set = true;
		msr_set_bit(MSR_ZEN4_BP_CFG, MSR_ZEN4_BP_CFG_BP_SPEC_REDUCE_BIT);
	}
	svm->guest_state_loaded = true;
}

static void svm_prepare_host_switch(struct kvm_vcpu *vcpu)
{
	to_svm(vcpu)->guest_state_loaded = false;
}

static void svm_vcpu_load(struct kvm_vcpu *vcpu, int cpu)
{
	if (vcpu->scheduled_out && !kvm_pause_in_guest(vcpu->kvm))
		shrink_ple_window(vcpu);

	if (kvm_vcpu_apicv_active(vcpu))
		avic_vcpu_load(vcpu, cpu);
}

static void svm_vcpu_put(struct kvm_vcpu *vcpu)
{
	if (kvm_vcpu_apicv_active(vcpu))
		avic_vcpu_put(vcpu);

	svm_prepare_host_switch(vcpu);

	++vcpu->stat.host_state_reload;
}

static unsigned long svm_get_rflags(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	unsigned long rflags = svm->vmcb->save.rflags;

	if (svm->nmi_singlestep) {
		/* Hide our flags if they were not set by the guest */
		if (!(svm->nmi_singlestep_guest_rflags & X86_EFLAGS_TF))
			rflags &= ~X86_EFLAGS_TF;
		if (!(svm->nmi_singlestep_guest_rflags & X86_EFLAGS_RF))
			rflags &= ~X86_EFLAGS_RF;
	}
	return rflags;
}

static void svm_set_rflags(struct kvm_vcpu *vcpu, unsigned long rflags)
{
	if (to_svm(vcpu)->nmi_singlestep)
		rflags |= (X86_EFLAGS_TF | X86_EFLAGS_RF);

       /*
        * Any change of EFLAGS.VM is accompanied by a reload of SS
        * (caused by either a task switch or an inter-privilege IRET),
        * so we do not need to update the CPL here.
        */
	to_svm(vcpu)->vmcb->save.rflags = rflags;
}

static bool svm_get_if_flag(struct kvm_vcpu *vcpu)
{
	struct vmcb *vmcb = to_svm(vcpu)->vmcb;

	return is_sev_es_guest(vcpu)
		? vmcb->control.int_state & SVM_GUEST_INTERRUPT_MASK
		: kvm_get_rflags(vcpu) & X86_EFLAGS_IF;
}

static void svm_cache_reg(struct kvm_vcpu *vcpu, enum kvm_reg reg)
{
	kvm_register_mark_available(vcpu, reg);

	switch (reg) {
	case VCPU_EXREG_PDPTR:
		/*
		 * When !npt_enabled, mmu->pdptrs[] is already available since
		 * it is always updated per SDM when moving to CRs.
		 */
		if (npt_enabled)
			load_pdptrs(vcpu, kvm_read_cr3(vcpu));
		break;
	default:
		KVM_BUG_ON(1, vcpu->kvm);
	}
}