// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 44/56



	if (unlikely(!enable_vnmi &&
		     vmx->loaded_vmcs->soft_vnmi_blocked)) {
		if (!vmx_interrupt_blocked(vcpu)) {
			vmx->loaded_vmcs->soft_vnmi_blocked = 0;
		} else if (vmx->loaded_vmcs->vnmi_blocked_time > 1000000000LL &&
			   vcpu->arch.nmi_pending) {
			/*
			 * This CPU don't support us in finding the end of an
			 * NMI-blocked window if the guest runs with IRQs
			 * disabled. So we pull the trigger after 1 s of
			 * futile waiting, but inform the user about this.
			 */
			printk(KERN_WARNING "%s: Breaking out of NMI-blocked "
			       "state on VCPU %d after 1 s timeout\n",
			       __func__, vcpu->vcpu_id);
			vmx->loaded_vmcs->soft_vnmi_blocked = 0;
		}
	}

	if (exit_fastpath != EXIT_FASTPATH_NONE)
		return 1;

	if (exit_reason.basic >= kvm_vmx_max_exit_handlers)
		goto unexpected_vmexit;
#ifdef CONFIG_MITIGATION_RETPOLINE
	if (exit_reason.basic == EXIT_REASON_MSR_WRITE)
		return kvm_emulate_wrmsr(vcpu);
	else if (exit_reason.basic == EXIT_REASON_MSR_WRITE_IMM)
		return handle_wrmsr_imm(vcpu);
	else if (exit_reason.basic == EXIT_REASON_PREEMPTION_TIMER)
		return handle_preemption_timer(vcpu);
	else if (exit_reason.basic == EXIT_REASON_INTERRUPT_WINDOW)
		return handle_interrupt_window(vcpu);
	else if (exit_reason.basic == EXIT_REASON_EXTERNAL_INTERRUPT)
		return handle_external_interrupt(vcpu);
	else if (exit_reason.basic == EXIT_REASON_HLT)
		return kvm_emulate_halt(vcpu);
	else if (exit_reason.basic == EXIT_REASON_EPT_MISCONFIG)
		return handle_ept_misconfig(vcpu);
#endif

	exit_handler_index = array_index_nospec((u16)exit_reason.basic,
						kvm_vmx_max_exit_handlers);
	if (!kvm_vmx_exit_handlers[exit_handler_index])
		goto unexpected_vmexit;

	return kvm_vmx_exit_handlers[exit_handler_index](vcpu);

unexpected_vmexit:
	dump_vmcs(vcpu);
	kvm_prepare_unexpected_reason_exit(vcpu, exit_reason.full);
	return 0;
}

int vmx_handle_exit(struct kvm_vcpu *vcpu, fastpath_t exit_fastpath)
{
	int ret = __vmx_handle_exit(vcpu, exit_fastpath);

	/*
	 * Exit to user space when bus lock detected to inform that there is
	 * a bus lock in guest.
	 */
	if (vmx_get_exit_reason(vcpu).bus_lock_detected) {
		if (ret > 0)
			vcpu->run->exit_reason = KVM_EXIT_X86_BUS_LOCK;

		vcpu->run->flags |= KVM_RUN_X86_BUS_LOCK;
		return 0;
	}
	return ret;
}

void vmx_update_cr8_intercept(struct kvm_vcpu *vcpu, int tpr, int irr)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
	int tpr_threshold;

	if (is_guest_mode(vcpu) &&
		nested_cpu_has(vmcs12, CPU_BASED_TPR_SHADOW))
		return;

	guard(vmx_vmcs01)(vcpu);

	tpr_threshold = (irr == -1 || tpr < irr) ? 0 : irr;
	vmcs_write32(TPR_THRESHOLD, tpr_threshold);
}

void vmx_set_virtual_apic_mode(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	u32 sec_exec_control;

	if (!lapic_in_kernel(vcpu))
		return;

	if (!flexpriority_enabled &&
	    !cpu_has_vmx_virtualize_x2apic_mode())
		return;

	guard(vmx_vmcs01)(vcpu);

	sec_exec_control = secondary_exec_controls_get(vmx);
	sec_exec_control &= ~(SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES |
			      SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE);

	switch (kvm_get_apic_mode(vcpu)) {
	case LAPIC_MODE_INVALID:
		WARN_ONCE(true, "Invalid local APIC state");
		break;
	case LAPIC_MODE_DISABLED:
		break;
	case LAPIC_MODE_XAPIC:
		if (flexpriority_enabled) {
			sec_exec_control |=
				SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES;
			kvm_make_request(KVM_REQ_APIC_PAGE_RELOAD, vcpu);

			/*
			 * Flush the TLB, reloading the APIC access page will
			 * only do so if its physical address has changed, but
			 * the guest may have inserted a non-APIC mapping into
			 * the TLB while the APIC access page was disabled.
			 *
			 * If L2 is active, immediately flush L1's TLB instead
			 * of requesting a flush of the current TLB, because
			 * the current TLB context is L2's.
			 */
			if (!is_guest_mode(vcpu))
				kvm_make_request(KVM_REQ_TLB_FLUSH_CURRENT, vcpu);
			else if (!enable_ept)
				vpid_sync_context(vmx->vpid);
			else if (VALID_PAGE(vcpu->arch.root_mmu.root.hpa))
				vmx_flush_tlb_ept_root(vcpu->arch.root_mmu.root.hpa);
		}
		break;
	case LAPIC_MODE_X2APIC:
		if (cpu_has_vmx_virtualize_x2apic_mode())
			sec_exec_control |=
				SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE;
		break;
	}
	secondary_exec_controls_set(vmx, sec_exec_control);

	vmx_update_msr_bitmap_x2apic(vcpu);
}

void vmx_set_apic_access_page_addr(struct kvm_vcpu *vcpu)
{
	const gfn_t gfn = APIC_DEFAULT_PHYS_BASE >> PAGE_SHIFT;
	struct kvm *kvm = vcpu->kvm;
	struct kvm_memslots *slots = kvm_memslots(kvm);
	struct kvm_memory_slot *slot;
	struct page *refcounted_page;
	unsigned long mmu_seq;
	kvm_pfn_t pfn;
	bool writable;

	/* Note, the VIRTUALIZE_APIC_ACCESSES check needs to query vmcs01. */
	guard(vmx_vmcs01)(vcpu);

	if (!(secondary_exec_controls_get(to_vmx(vcpu)) &
	    SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES))
		return;