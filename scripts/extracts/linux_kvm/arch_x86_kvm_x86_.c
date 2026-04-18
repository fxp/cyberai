// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 74/86



	kvm_set_segment(vcpu, &sregs->cs, VCPU_SREG_CS);
	kvm_set_segment(vcpu, &sregs->ds, VCPU_SREG_DS);
	kvm_set_segment(vcpu, &sregs->es, VCPU_SREG_ES);
	kvm_set_segment(vcpu, &sregs->fs, VCPU_SREG_FS);
	kvm_set_segment(vcpu, &sregs->gs, VCPU_SREG_GS);
	kvm_set_segment(vcpu, &sregs->ss, VCPU_SREG_SS);

	kvm_set_segment(vcpu, &sregs->tr, VCPU_SREG_TR);
	kvm_set_segment(vcpu, &sregs->ldt, VCPU_SREG_LDTR);

	update_cr8_intercept(vcpu);

	/* Older userspace won't unhalt the vcpu on reset. */
	if (kvm_vcpu_is_bsp(vcpu) && kvm_rip_read(vcpu) == 0xfff0 &&
	    sregs->cs.selector == 0xf000 && sregs->cs.base == 0xffff0000 &&
	    !is_protmode(vcpu))
		kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);

	return 0;
}

static int __set_sregs(struct kvm_vcpu *vcpu, struct kvm_sregs *sregs)
{
	int pending_vec, max_bits;
	int mmu_reset_needed = 0;
	int ret = __set_sregs_common(vcpu, sregs, &mmu_reset_needed, true);

	if (ret)
		return ret;

	if (mmu_reset_needed) {
		kvm_mmu_reset_context(vcpu);
		kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
	}

	max_bits = KVM_NR_INTERRUPTS;
	pending_vec = find_first_bit(
		(const unsigned long *)sregs->interrupt_bitmap, max_bits);

	if (pending_vec < max_bits) {
		kvm_queue_interrupt(vcpu, pending_vec, false);
		pr_debug("Set back pending irq %d\n", pending_vec);
		kvm_make_request(KVM_REQ_EVENT, vcpu);
	}
	return 0;
}

static int __set_sregs2(struct kvm_vcpu *vcpu, struct kvm_sregs2 *sregs2)
{
	int mmu_reset_needed = 0;
	bool valid_pdptrs = sregs2->flags & KVM_SREGS2_FLAGS_PDPTRS_VALID;
	bool pae = (sregs2->cr0 & X86_CR0_PG) && (sregs2->cr4 & X86_CR4_PAE) &&
		!(sregs2->efer & EFER_LMA);
	int i, ret;

	if (sregs2->flags & ~KVM_SREGS2_FLAGS_PDPTRS_VALID)
		return -EINVAL;

	if (valid_pdptrs && (!pae || vcpu->arch.guest_state_protected))
		return -EINVAL;

	ret = __set_sregs_common(vcpu, (struct kvm_sregs *)sregs2,
				 &mmu_reset_needed, !valid_pdptrs);
	if (ret)
		return ret;

	if (valid_pdptrs) {
		for (i = 0; i < 4 ; i++)
			kvm_pdptr_write(vcpu, i, sregs2->pdptrs[i]);

		kvm_register_mark_dirty(vcpu, VCPU_EXREG_PDPTR);
		mmu_reset_needed = 1;
		vcpu->arch.pdptrs_from_userspace = true;
	}
	if (mmu_reset_needed) {
		kvm_mmu_reset_context(vcpu);
		kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
	}
	return 0;
}

int kvm_arch_vcpu_ioctl_set_sregs(struct kvm_vcpu *vcpu,
				  struct kvm_sregs *sregs)
{
	int ret;

	if (vcpu->kvm->arch.has_protected_state &&
	    vcpu->arch.guest_state_protected)
		return -EINVAL;

	vcpu_load(vcpu);
	ret = __set_sregs(vcpu, sregs);
	vcpu_put(vcpu);
	return ret;
}

static void kvm_arch_vcpu_guestdbg_update_apicv_inhibit(struct kvm *kvm)
{
	bool set = false;
	struct kvm_vcpu *vcpu;
	unsigned long i;

	if (!enable_apicv)
		return;

	down_write(&kvm->arch.apicv_update_lock);

	kvm_for_each_vcpu(i, vcpu, kvm) {
		if (vcpu->guest_debug & KVM_GUESTDBG_BLOCKIRQ) {
			set = true;
			break;
		}
	}
	__kvm_set_or_clear_apicv_inhibit(kvm, APICV_INHIBIT_REASON_BLOCKIRQ, set);
	up_write(&kvm->arch.apicv_update_lock);
}

int kvm_arch_vcpu_ioctl_set_guest_debug(struct kvm_vcpu *vcpu,
					struct kvm_guest_debug *dbg)
{
	unsigned long rflags;
	int i, r;

	if (vcpu->arch.guest_state_protected)
		return -EINVAL;

	vcpu_load(vcpu);

	if (dbg->control & (KVM_GUESTDBG_INJECT_DB | KVM_GUESTDBG_INJECT_BP)) {
		r = -EBUSY;
		if (kvm_is_exception_pending(vcpu) || vcpu->arch.exception.injected)
			goto out;
		if (dbg->control & KVM_GUESTDBG_INJECT_DB)
			kvm_queue_exception(vcpu, DB_VECTOR);
		else
			kvm_queue_exception(vcpu, BP_VECTOR);
	}

	/*
	 * Read rflags as long as potentially injected trace flags are still
	 * filtered out.
	 */
	rflags = kvm_get_rflags(vcpu);

	vcpu->guest_debug = dbg->control;
	if (!(vcpu->guest_debug & KVM_GUESTDBG_ENABLE))
		vcpu->guest_debug = 0;

	if (vcpu->guest_debug & KVM_GUESTDBG_USE_HW_BP) {
		for (i = 0; i < KVM_NR_DB_REGS; ++i)
			vcpu->arch.eff_db[i] = dbg->arch.debugreg[i];
		vcpu->arch.guest_debug_dr7 = dbg->arch.debugreg[7];
	} else {
		for (i = 0; i < KVM_NR_DB_REGS; i++)
			vcpu->arch.eff_db[i] = vcpu->arch.db[i];
	}
	kvm_update_dr7(vcpu);

	if (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)
		vcpu->arch.singlestep_rip = kvm_get_linear_rip(vcpu);

	/*
	 * Trigger an rflags update that will inject or remove the trace
	 * flags.
	 */
	kvm_set_rflags(vcpu, rflags);

	kvm_x86_call(update_exception_bitmap)(vcpu);

	kvm_arch_vcpu_guestdbg_update_apicv_inhibit(vcpu->kvm);

	r = 0;

out:
	vcpu_put(vcpu);
	return r;
}

/*
 * Translate a guest virtual address to a guest physical address.
 */
int kvm_arch_vcpu_ioctl_translate(struct kvm_vcpu *vcpu,
				    struct kvm_translation *tr)
{
	unsigned long vaddr = tr->linear_address;
	gpa_t gpa;
	int idx;

	vcpu_load(vcpu);

	idx = srcu_read_lock(&vcpu->kvm->srcu);
	gpa = kvm_mmu_gva_to_gpa_system(vcpu, vaddr, NULL);
	srcu_read_unlock(&vcpu->kvm->srcu, idx);
	tr->physical_address = gpa;
	tr->valid = gpa != INVALID_GPA;
	tr->writeable = 1;
	tr->usermode = 0;

	vcpu_put(vcpu);
	return 0;
}