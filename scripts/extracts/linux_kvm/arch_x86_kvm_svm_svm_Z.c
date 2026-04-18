// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 26/36



	/*
	 * In case GIF=0 we can't rely on the CPU to tell us when GIF becomes
	 * 1, because that's a separate STGI/VMRUN intercept.  The next time we
	 * get that intercept, this function will be called again though and
	 * we'll get the vintr intercept. However, if the vGIF feature is
	 * enabled, the STGI interception will not occur. Enable the irq
	 * window under the assumption that the hardware will set the GIF.
	 */
	if (vgif || gif_set(svm)) {
		/*
		 * KVM only enables IRQ windows when AVIC is enabled if there's
		 * pending ExtINT since it cannot be injected via AVIC (ExtINT
		 * bypasses the local APIC).  V_IRQ is ignored by hardware when
		 * AVIC is enabled, and so KVM needs to temporarily disable
		 * AVIC in order to detect when it's ok to inject the ExtINT.
		 *
		 * If running nested, AVIC is already locally inhibited on this
		 * vCPU (L2 vCPUs use a different MMU that never maps the AVIC
		 * backing page), therefore there is no need to increment the
		 * VM-wide AVIC inhibit.  KVM will re-evaluate events when the
		 * vCPU exits to L1 and enable an IRQ window if the ExtINT is
		 * still pending.
		 *
		 * Note, the IRQ window inhibit needs to be updated even if
		 * AVIC is inhibited for a different reason, as KVM needs to
		 * keep AVIC inhibited if the other reason is cleared and there
		 * is still an injectable interrupt pending.
		 */
		if (enable_apicv && !svm->avic_irq_window && !is_guest_mode(vcpu)) {
			svm->avic_irq_window = true;
			kvm_inc_apicv_irq_window_req(vcpu->kvm);
		}

		svm_set_vintr(svm);
	}
}

static void svm_enable_nmi_window(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * If NMIs are outright masked, i.e. the vCPU is already handling an
	 * NMI, and KVM has not yet intercepted an IRET, then there is nothing
	 * more to do at this time as KVM has already enabled IRET intercepts.
	 * If KVM has already intercepted IRET, then single-step over the IRET,
	 * as NMIs aren't architecturally unmasked until the IRET completes.
	 *
	 * If vNMI is enabled, KVM should never request an NMI window if NMIs
	 * are masked, as KVM allows at most one to-be-injected NMI and one
	 * pending NMI.  If two NMIs arrive simultaneously, KVM will inject one
	 * NMI and set V_NMI_PENDING for the other, but if and only if NMIs are
	 * unmasked.  KVM _will_ request an NMI window in some situations, e.g.
	 * if the vCPU is in an STI shadow or if GIF=0, KVM can't immediately
	 * inject the NMI.  In those situations, KVM needs to single-step over
	 * the STI shadow or intercept STGI.
	 */
	if (svm_get_nmi_mask(vcpu)) {
		WARN_ON_ONCE(is_vnmi_enabled(svm));

		if (!svm->awaiting_iret_completion)
			return; /* IRET will cause a vm exit */
	}

	/*
	 * SEV-ES guests are responsible for signaling when a vCPU is ready to
	 * receive a new NMI, as SEV-ES guests can't be single-stepped, i.e.
	 * KVM can't intercept and single-step IRET to detect when NMIs are
	 * unblocked (architecturally speaking).  See SVM_VMGEXIT_NMI_COMPLETE.
	 *
	 * Note, GIF is guaranteed to be '1' for SEV-ES guests as hardware
	 * ignores SEV-ES guest writes to EFER.SVME *and* CLGI/STGI are not
	 * supported NAEs in the GHCB protocol.
	 */
	if (is_sev_es_guest(vcpu))
		return;

	if (!gif_set(svm)) {
		if (vgif)
			svm_set_intercept(svm, INTERCEPT_STGI);
		return; /* STGI will cause a vm exit */
	}

	/*
	 * Something prevents NMI from been injected. Single step over possible
	 * problem (IRET or exception injection or interrupt shadow)
	 */
	svm->nmi_singlestep_guest_rflags = svm_get_rflags(vcpu);
	svm->nmi_singlestep = true;
	svm->vmcb->save.rflags |= (X86_EFLAGS_TF | X86_EFLAGS_RF);
}

static void svm_flush_tlb_asid(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	/*
	 * Unlike VMX, SVM doesn't provide a way to flush only NPT TLB entries.
	 * A TLB flush for the current ASID flushes both "host" and "guest" TLB
	 * entries, and thus is a superset of Hyper-V's fine grained flushing.
	 */
	kvm_hv_vcpu_purge_flush_tlb(vcpu);

	/*
	 * Flush only the current ASID even if the TLB flush was invoked via
	 * kvm_flush_remote_tlbs().  Although flushing remote TLBs requires all
	 * ASIDs to be flushed, KVM uses a single ASID for L1 and L2, and
	 * unconditionally does a TLB flush on both nested VM-Enter and nested
	 * VM-Exit (via kvm_mmu_reset_context()).
	 */
	if (static_cpu_has(X86_FEATURE_FLUSHBYASID))
		svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ASID;
	else
		svm->current_vmcb->asid_generation--;
}

static void svm_flush_tlb_current(struct kvm_vcpu *vcpu)
{
	hpa_t root_tdp = vcpu->arch.mmu->root.hpa;

	/*
	 * When running on Hyper-V with EnlightenedNptTlb enabled, explicitly
	 * flush the NPT mappings via hypercall as flushing the ASID only
	 * affects virtual to physical mappings, it does not invalidate guest
	 * physical to host physical mappings.
	 */
	if (svm_hv_is_enlightened_tlb_enabled(vcpu) && VALID_PAGE(root_tdp))
		hyperv_flush_guest_mapping(root_tdp);