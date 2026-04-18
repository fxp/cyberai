// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 73/86



	if ((vcpu->arch.mp_state == KVM_MP_STATE_HALTED ||
	     vcpu->arch.mp_state == KVM_MP_STATE_AP_RESET_HOLD) &&
	    vcpu->arch.pv.pv_unhalted)
		mp_state->mp_state = KVM_MP_STATE_RUNNABLE;
	else
		mp_state->mp_state = vcpu->arch.mp_state;

out:
	kvm_vcpu_srcu_read_unlock(vcpu);
	vcpu_put(vcpu);
	return r;
}

int kvm_arch_vcpu_ioctl_set_mpstate(struct kvm_vcpu *vcpu,
				    struct kvm_mp_state *mp_state)
{
	int ret = -EINVAL;

	vcpu_load(vcpu);

	switch (mp_state->mp_state) {
	case KVM_MP_STATE_UNINITIALIZED:
	case KVM_MP_STATE_HALTED:
	case KVM_MP_STATE_AP_RESET_HOLD:
	case KVM_MP_STATE_INIT_RECEIVED:
	case KVM_MP_STATE_SIPI_RECEIVED:
		if (!lapic_in_kernel(vcpu))
			goto out;
		break;

	case KVM_MP_STATE_RUNNABLE:
		break;

	default:
		goto out;
	}

	/*
	 * SIPI_RECEIVED is obsolete and no longer used internally; KVM instead
	 * leaves the vCPU in INIT_RECIEVED (Wait-For-SIPI) and pends the SIPI.
	 * Translate SIPI_RECEIVED as appropriate for backwards compatibility.
	 */
	if (mp_state->mp_state == KVM_MP_STATE_SIPI_RECEIVED) {
		mp_state->mp_state = KVM_MP_STATE_INIT_RECEIVED;
		set_bit(KVM_APIC_SIPI, &vcpu->arch.apic->pending_events);
	}

	kvm_set_mp_state(vcpu, mp_state->mp_state);
	kvm_make_request(KVM_REQ_EVENT, vcpu);

	ret = 0;
out:
	vcpu_put(vcpu);
	return ret;
}

int kvm_task_switch(struct kvm_vcpu *vcpu, u16 tss_selector, int idt_index,
		    int reason, bool has_error_code, u32 error_code)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	int ret;

	if (kvm_is_cr4_bit_set(vcpu, X86_CR4_CET)) {
		u64 u_cet, s_cet;

		/*
		 * Check both User and Supervisor on task switches as inter-
		 * privilege level task switches are impacted by CET at both
		 * the current privilege level and the new privilege level, and
		 * that information is not known at this time.  The expectation
		 * is that the guest won't require emulation of task switches
		 * while using IBT or Shadow Stacks.
		 */
		if (__kvm_emulate_msr_read(vcpu, MSR_IA32_U_CET, &u_cet) ||
		    __kvm_emulate_msr_read(vcpu, MSR_IA32_S_CET, &s_cet))
			goto unhandled_task_switch;

		if ((u_cet | s_cet) & (CET_ENDBR_EN | CET_SHSTK_EN))
			goto unhandled_task_switch;
	}

	init_emulate_ctxt(vcpu);

	ret = emulator_task_switch(ctxt, tss_selector, idt_index, reason,
				   has_error_code, error_code);

	/*
	 * Report an error userspace if MMIO is needed, as KVM doesn't support
	 * MMIO during a task switch (or any other complex operation).
	 */
	if (ret || vcpu->mmio_needed)
		goto unhandled_task_switch;

	kvm_rip_write(vcpu, ctxt->eip);
	kvm_set_rflags(vcpu, ctxt->eflags);
	return 1;

unhandled_task_switch:
	vcpu->mmio_needed = false;
	vcpu->run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
	vcpu->run->internal.suberror = KVM_INTERNAL_ERROR_EMULATION;
	vcpu->run->internal.ndata = 0;
	return 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_task_switch);

static bool kvm_is_valid_sregs(struct kvm_vcpu *vcpu, struct kvm_sregs *sregs)
{
	if ((sregs->efer & EFER_LME) && (sregs->cr0 & X86_CR0_PG)) {
		/*
		 * When EFER.LME and CR0.PG are set, the processor is in
		 * 64-bit mode (though maybe in a 32-bit code segment).
		 * CR4.PAE and EFER.LMA must be set.
		 */
		if (!(sregs->cr4 & X86_CR4_PAE) || !(sregs->efer & EFER_LMA))
			return false;
		if (!kvm_vcpu_is_legal_cr3(vcpu, sregs->cr3))
			return false;
	} else {
		/*
		 * Not in 64-bit mode: EFER.LMA is clear and the code
		 * segment cannot be 64-bit.
		 */
		if (sregs->efer & EFER_LMA || sregs->cs.l)
			return false;
	}

	return kvm_is_valid_cr4(vcpu, sregs->cr4) &&
	       kvm_is_valid_cr0(vcpu, sregs->cr0);
}

static int __set_sregs_common(struct kvm_vcpu *vcpu, struct kvm_sregs *sregs,
		int *mmu_reset_needed, bool update_pdptrs)
{
	int idx;
	struct desc_ptr dt;

	if (!kvm_is_valid_sregs(vcpu, sregs))
		return -EINVAL;

	if (kvm_apic_set_base(vcpu, sregs->apic_base, true))
		return -EINVAL;

	if (vcpu->arch.guest_state_protected)
		return 0;

	dt.size = sregs->idt.limit;
	dt.address = sregs->idt.base;
	kvm_x86_call(set_idt)(vcpu, &dt);
	dt.size = sregs->gdt.limit;
	dt.address = sregs->gdt.base;
	kvm_x86_call(set_gdt)(vcpu, &dt);

	vcpu->arch.cr2 = sregs->cr2;
	*mmu_reset_needed |= kvm_read_cr3(vcpu) != sregs->cr3;
	vcpu->arch.cr3 = sregs->cr3;
	kvm_register_mark_dirty(vcpu, VCPU_EXREG_CR3);
	kvm_x86_call(post_set_cr3)(vcpu, sregs->cr3);

	kvm_set_cr8(vcpu, sregs->cr8);

	*mmu_reset_needed |= vcpu->arch.efer != sregs->efer;
	kvm_x86_call(set_efer)(vcpu, sregs->efer);

	*mmu_reset_needed |= kvm_read_cr0(vcpu) != sregs->cr0;
	kvm_x86_call(set_cr0)(vcpu, sregs->cr0);

	*mmu_reset_needed |= kvm_read_cr4(vcpu) != sregs->cr4;
	kvm_x86_call(set_cr4)(vcpu, sregs->cr4);

	if (update_pdptrs) {
		idx = srcu_read_lock(&vcpu->kvm->srcu);
		if (is_pae_paging(vcpu)) {
			load_pdptrs(vcpu, kvm_read_cr3(vcpu));
			*mmu_reset_needed = 1;
		}
		srcu_read_unlock(&vcpu->kvm->srcu, idx);
	}