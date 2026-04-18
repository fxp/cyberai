// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 8/36



	svm_set_intercept(svm, INTERCEPT_SELECTIVE_CR0);
	svm_set_intercept(svm, INTERCEPT_RDPMC);
	svm_set_intercept(svm, INTERCEPT_CPUID);
	svm_set_intercept(svm, INTERCEPT_INVD);
	svm_set_intercept(svm, INTERCEPT_INVLPG);
	svm_set_intercept(svm, INTERCEPT_INVLPGA);
	svm_set_intercept(svm, INTERCEPT_IOIO_PROT);
	svm_set_intercept(svm, INTERCEPT_MSR_PROT);
	svm_set_intercept(svm, INTERCEPT_TASK_SWITCH);
	svm_set_intercept(svm, INTERCEPT_SHUTDOWN);
	svm_set_intercept(svm, INTERCEPT_VMRUN);
	svm_set_intercept(svm, INTERCEPT_VMMCALL);
	svm_set_intercept(svm, INTERCEPT_VMLOAD);
	svm_set_intercept(svm, INTERCEPT_VMSAVE);
	svm_set_intercept(svm, INTERCEPT_STGI);
	svm_set_intercept(svm, INTERCEPT_CLGI);
	svm_set_intercept(svm, INTERCEPT_SKINIT);
	svm_set_intercept(svm, INTERCEPT_WBINVD);
	svm_set_intercept(svm, INTERCEPT_XSETBV);
	svm_set_intercept(svm, INTERCEPT_RDPRU);
	svm_set_intercept(svm, INTERCEPT_RSM);

	if (!kvm_mwait_in_guest(vcpu->kvm)) {
		svm_set_intercept(svm, INTERCEPT_MONITOR);
		svm_set_intercept(svm, INTERCEPT_MWAIT);
	}

	if (!kvm_hlt_in_guest(vcpu->kvm)) {
		if (cpu_feature_enabled(X86_FEATURE_IDLE_HLT))
			svm_set_intercept(svm, INTERCEPT_IDLE_HLT);
		else
			svm_set_intercept(svm, INTERCEPT_HLT);
	}

	control->iopm_base_pa = iopm_base;
	control->msrpm_base_pa = __sme_set(__pa(svm->msrpm));
	control->int_ctl = V_INTR_MASKING_MASK;

	init_seg(&save->es);
	init_seg(&save->ss);
	init_seg(&save->ds);
	init_seg(&save->fs);
	init_seg(&save->gs);

	save->cs.selector = 0xf000;
	save->cs.base = 0xffff0000;
	/* Executable/Readable Code Segment */
	save->cs.attrib = SVM_SELECTOR_READ_MASK | SVM_SELECTOR_P_MASK |
		SVM_SELECTOR_S_MASK | SVM_SELECTOR_CODE_MASK;
	save->cs.limit = 0xffff;

	save->gdtr.base = 0;
	save->gdtr.limit = 0xffff;
	save->idtr.base = 0;
	save->idtr.limit = 0xffff;

	init_sys_seg(&save->ldtr, SEG_TYPE_LDT);
	init_sys_seg(&save->tr, SEG_TYPE_BUSY_TSS16);

	if (npt_enabled) {
		/* Setup VMCB for Nested Paging */
		control->misc_ctl |= SVM_MISC_ENABLE_NP;
		svm_clr_intercept(svm, INTERCEPT_INVLPG);
		clr_exception_intercept(svm, PF_VECTOR);
		svm_clr_intercept(svm, INTERCEPT_CR3_READ);
		svm_clr_intercept(svm, INTERCEPT_CR3_WRITE);
		save->g_pat = vcpu->arch.pat;
		save->cr3 = 0;
	}
	svm->current_vmcb->asid_generation = 0;
	svm->asid = 0;

	svm->nested.vmcb12_gpa = INVALID_GPA;
	svm->nested.last_vmcb12_gpa = INVALID_GPA;

	if (!kvm_pause_in_guest(vcpu->kvm)) {
		control->pause_filter_count = pause_filter_count;
		if (pause_filter_thresh)
			control->pause_filter_thresh = pause_filter_thresh;
		svm_set_intercept(svm, INTERCEPT_PAUSE);
	} else {
		svm_clr_intercept(svm, INTERCEPT_PAUSE);
	}

	if (guest_cpu_cap_has(vcpu, X86_FEATURE_ERAPS))
		svm->vmcb->control.erap_ctl |= ERAP_CONTROL_ALLOW_LARGER_RAP;

	if (enable_apicv && irqchip_in_kernel(vcpu->kvm))
		avic_init_vmcb(svm, vmcb);

	if (vnmi)
		svm->vmcb->control.int_ctl |= V_NMI_ENABLE_MASK;

	if (vgif)
		svm->vmcb->control.int_ctl |= V_GIF_ENABLE_MASK;

	if (vls)
		svm->vmcb->control.misc_ctl2 |= SVM_MISC2_ENABLE_V_VMLOAD_VMSAVE;

	if (vcpu->kvm->arch.bus_lock_detection_enabled)
		svm_set_intercept(svm, INTERCEPT_BUSLOCK);

	if (is_sev_guest(vcpu))
		sev_init_vmcb(svm, init_event);

	svm_hv_init_vmcb(vmcb);

	kvm_make_request(KVM_REQ_RECALC_INTERCEPTS, vcpu);

	vmcb_mark_all_dirty(vmcb);

	enable_gif(svm);
}

static void __svm_vcpu_reset(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm_init_osvw(vcpu);

	if (kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_STUFF_FEATURE_MSRS))
		vcpu->arch.microcode_version = 0x01000065;
	svm->tsc_ratio_msr = kvm_caps.default_tsc_scaling_ratio;

	svm->nmi_masked = false;
	svm->awaiting_iret_completion = false;
}

static void svm_vcpu_reset(struct kvm_vcpu *vcpu, bool init_event)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	svm->spec_ctrl = 0;
	svm->virt_spec_ctrl = 0;

	init_vmcb(vcpu, init_event);

	if (!init_event)
		__svm_vcpu_reset(vcpu);
}

void svm_switch_vmcb(struct vcpu_svm *svm, struct kvm_vmcb_info *target_vmcb)
{
	svm->current_vmcb = target_vmcb;
	svm->vmcb = target_vmcb->ptr;
}

static int svm_vcpu_precreate(struct kvm *kvm)
{
	return avic_alloc_physical_id_table(kvm);
}

static int svm_vcpu_create(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm;
	struct page *vmcb01_page;
	int err;

	BUILD_BUG_ON(offsetof(struct vcpu_svm, vcpu) != 0);
	svm = to_svm(vcpu);

	err = -ENOMEM;
	vmcb01_page = snp_safe_alloc_page();
	if (!vmcb01_page)
		goto out;

	err = sev_vcpu_create(vcpu);
	if (err)
		goto error_free_vmcb_page;

	err = avic_init_vcpu(svm);
	if (err)
		goto error_free_sev;

	svm->msrpm = svm_vcpu_alloc_msrpm();
	if (!svm->msrpm) {
		err = -ENOMEM;
		goto error_free_sev;
	}

	svm->x2avic_msrs_intercepted = true;
	svm->lbr_msrs_intercepted = true;

	svm->vmcb01.ptr = page_address(vmcb01_page);
	svm->vmcb01.pa = __sme_set(page_to_pfn(vmcb01_page) << PAGE_SHIFT);
	svm_switch_vmcb(svm, &svm->vmcb01);

	svm->guest_state_loaded = false;

	return 0;