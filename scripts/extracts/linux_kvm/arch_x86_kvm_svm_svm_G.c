// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 7/36



static bool svm_has_pending_gif_event(struct vcpu_svm *svm)
{
	return svm->vcpu.arch.smi_pending ||
	       svm->vcpu.arch.nmi_pending ||
	       kvm_cpu_has_injectable_intr(&svm->vcpu) ||
	       kvm_apic_has_pending_init_or_sipi(&svm->vcpu);
}

/* Evaluate instruction intercepts that depend on guest CPUID features. */
static void svm_recalc_instruction_intercepts(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * Intercept INVPCID if shadow paging is enabled to sync/free shadow
	 * roots, or if INVPCID is disabled in the guest to inject #UD.
	 */
	if (kvm_cpu_cap_has(X86_FEATURE_INVPCID)) {
		if (!npt_enabled ||
		    !guest_cpu_cap_has(&svm->vcpu, X86_FEATURE_INVPCID))
			svm_set_intercept(svm, INTERCEPT_INVPCID);
		else
			svm_clr_intercept(svm, INTERCEPT_INVPCID);
	}

	if (kvm_cpu_cap_has(X86_FEATURE_RDTSCP)) {
		if (guest_cpu_cap_has(vcpu, X86_FEATURE_RDTSCP))
			svm_clr_intercept(svm, INTERCEPT_RDTSCP);
		else
			svm_set_intercept(svm, INTERCEPT_RDTSCP);
	}

	/*
	 * Intercept instructions that #UD if EFER.SVME=0, as SVME must be set
	 * even when running the guest, i.e. hardware will only ever see
	 * EFER.SVME=1.
	 *
	 * No need to toggle any of the vgif/vls/etc. enable bits here, as they
	 * are set when the VMCB is initialized and never cleared (if the
	 * relevant intercepts are set, the enablements are meaningless anyway).
	 *
	 * FIXME: When #GP is not intercepted, a #GP on these instructions (e.g.
	 * due to CPL > 0) could be injected by hardware before the instruction
	 * is intercepted, leading to #GP taking precedence over #UD from the
	 * guest's perspective.
	 */
	if (!(vcpu->arch.efer & EFER_SVME)) {
		svm_set_intercept(svm, INTERCEPT_VMLOAD);
		svm_set_intercept(svm, INTERCEPT_VMSAVE);
		svm_set_intercept(svm, INTERCEPT_CLGI);
		svm_set_intercept(svm, INTERCEPT_STGI);
	} else {
		/*
		 * If hardware supports Virtual VMLOAD VMSAVE then enable it
		 * in VMCB and clear intercepts to avoid #VMEXIT.
		 */
		if (guest_cpuid_is_intel_compatible(vcpu)) {
			svm_set_intercept(svm, INTERCEPT_VMLOAD);
			svm_set_intercept(svm, INTERCEPT_VMSAVE);
		} else if (vls) {
			svm_clr_intercept(svm, INTERCEPT_VMLOAD);
			svm_clr_intercept(svm, INTERCEPT_VMSAVE);
		}

		/*
		 * Process pending events when clearing STGI/CLGI intercepts if
		 * there's at least one pending event that is masked by GIF, so
		 * that KVM re-evaluates if the intercept needs to be set again
		 * to track when GIF is re-enabled (e.g. for NMI injection).
		 */
		if (vgif) {
			svm_clr_intercept(svm, INTERCEPT_CLGI);
			svm_clr_intercept(svm, INTERCEPT_STGI);

			if (svm_has_pending_gif_event(svm))
				kvm_make_request(KVM_REQ_EVENT, &svm->vcpu);
		}
	}

	if (kvm_need_rdpmc_intercept(vcpu))
		svm_set_intercept(svm, INTERCEPT_RDPMC);
	else
		svm_clr_intercept(svm, INTERCEPT_RDPMC);
}

static void svm_recalc_intercepts(struct kvm_vcpu *vcpu)
{
	svm_recalc_instruction_intercepts(vcpu);
	svm_recalc_msr_intercepts(vcpu);
}

static void init_vmcb(struct kvm_vcpu *vcpu, bool init_event)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb *vmcb = svm->vmcb01.ptr;
	struct vmcb_control_area *control = &vmcb->control;
	struct vmcb_save_area *save = &vmcb->save;

	svm_set_intercept(svm, INTERCEPT_CR0_READ);
	svm_set_intercept(svm, INTERCEPT_CR3_READ);
	svm_set_intercept(svm, INTERCEPT_CR4_READ);
	svm_set_intercept(svm, INTERCEPT_CR0_WRITE);
	svm_set_intercept(svm, INTERCEPT_CR3_WRITE);
	svm_set_intercept(svm, INTERCEPT_CR4_WRITE);
	svm_set_intercept(svm, INTERCEPT_CR8_WRITE);

	set_dr_intercepts(svm);

	set_exception_intercept(svm, PF_VECTOR);
	set_exception_intercept(svm, UD_VECTOR);
	set_exception_intercept(svm, MC_VECTOR);
	set_exception_intercept(svm, AC_VECTOR);
	set_exception_intercept(svm, DB_VECTOR);
	/*
	 * Guest access to VMware backdoor ports could legitimately
	 * trigger #GP because of TSS I/O permission bitmap.
	 * We intercept those #GP and allow access to them anyway
	 * as VMware does.
	 */
	if (enable_vmware_backdoor)
		set_exception_intercept(svm, GP_VECTOR);

	svm_set_intercept(svm, INTERCEPT_INTR);
	svm_set_intercept(svm, INTERCEPT_NMI);

	if (intercept_smi)
		svm_set_intercept(svm, INTERCEPT_SMI);