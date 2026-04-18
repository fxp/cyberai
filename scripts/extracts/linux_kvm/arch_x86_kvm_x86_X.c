// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 56/86



	/*
	 * If emulation was caused by a write-protection #PF on a non-page_table
	 * writing instruction, try to unprotect the gfn, i.e. zap shadow pages,
	 * and retry the instruction, as the vCPU is likely no longer using the
	 * gfn as a page table.
	 */
	if ((emulation_type & EMULTYPE_ALLOW_RETRY_PF) &&
	    !x86_page_table_writing_insn(ctxt) &&
	    kvm_mmu_unprotect_gfn_and_retry(vcpu, cr2_or_gpa))
		return 1;

	/* this is needed for vmware backdoor interface to work since it
	   changes registers values  during IO operation */
	if (vcpu->arch.emulate_regs_need_sync_from_vcpu) {
		vcpu->arch.emulate_regs_need_sync_from_vcpu = false;
		emulator_invalidate_register_cache(ctxt);
	}

restart:
	if (emulation_type & EMULTYPE_PF) {
		/* Save the faulting GPA (cr2) in the address field */
		ctxt->exception.address = cr2_or_gpa;

		/* With shadow page tables, cr2 contains a GVA or nGPA. */
		if (vcpu->arch.mmu->root_role.direct) {
			ctxt->gpa_available = true;
			ctxt->gpa_val = cr2_or_gpa;
		}
	} else {
		/* Sanitize the address out of an abundance of paranoia. */
		ctxt->exception.address = 0;
	}

	/*
	 * Check L1's instruction intercepts when emulating instructions for
	 * L2, unless KVM is re-emulating a previously decoded instruction,
	 * e.g. to complete userspace I/O, in which case KVM has already
	 * checked the intercepts.
	 */
	r = x86_emulate_insn(ctxt, is_guest_mode(vcpu) &&
				   !(emulation_type & EMULTYPE_NO_DECODE));

	if (r == EMULATION_INTERCEPTED)
		return 1;

	if (r == EMULATION_FAILED) {
		if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
						       emulation_type))
			return 1;

		return handle_emulation_failure(vcpu, emulation_type);
	}

	if (ctxt->have_exception) {
		WARN_ON_ONCE(vcpu->mmio_needed && !vcpu->mmio_is_write);
		vcpu->mmio_needed = false;
		r = 1;
		inject_emulated_exception(vcpu);
	} else if (vcpu->arch.pio.count) {
		if (!vcpu->arch.pio.in) {
			/* FIXME: return into emulator if single-stepping.  */
			vcpu->arch.pio.count = 0;
		} else {
			writeback = false;
			vcpu->arch.complete_userspace_io = complete_emulated_pio;
		}
		r = 0;
	} else if (vcpu->mmio_needed) {
		++vcpu->stat.mmio_exits;

		if (!vcpu->mmio_is_write)
			writeback = false;
		r = 0;
		vcpu->arch.complete_userspace_io = complete_emulated_mmio;
	} else if (vcpu->arch.complete_userspace_io) {
		writeback = false;
		r = 0;
	} else if (r == EMULATION_RESTART)
		goto restart;
	else
		r = 1;

writeback:
	if (writeback) {
		unsigned long rflags = kvm_x86_call(get_rflags)(vcpu);
		toggle_interruptibility(vcpu, ctxt->interruptibility);
		vcpu->arch.emulate_regs_need_sync_to_vcpu = false;

		/*
		 * Note, EXCPT_DB is assumed to be fault-like as the emulator
		 * only supports code breakpoints and general detect #DB, both
		 * of which are fault-like.
		 */
		if (!ctxt->have_exception ||
		    exception_type(ctxt->exception.vector) == EXCPT_TRAP) {
			kvm_pmu_instruction_retired(vcpu);
			if (ctxt->is_branch)
				kvm_pmu_branch_retired(vcpu);
			kvm_rip_write(vcpu, ctxt->eip);
			if (r && (ctxt->tf || (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)))
				r = kvm_vcpu_do_singlestep(vcpu);
			kvm_x86_call(update_emulated_instruction)(vcpu);
			__kvm_set_rflags(vcpu, ctxt->eflags);
		}

		/*
		 * For STI, interrupts are shadowed; so KVM_REQ_EVENT will
		 * do nothing, and it will be requested again as soon as
		 * the shadow expires.  But we still need to check here,
		 * because POPF has no interrupt shadow.
		 */
		if (unlikely((ctxt->eflags & ~rflags) & X86_EFLAGS_IF))
			kvm_make_request(KVM_REQ_EVENT, vcpu);
	} else
		vcpu->arch.emulate_regs_need_sync_to_vcpu = true;

	return r;
}

int kvm_emulate_instruction(struct kvm_vcpu *vcpu, int emulation_type)
{
	return x86_emulate_instruction(vcpu, 0, emulation_type, NULL, 0);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_instruction);

int kvm_emulate_instruction_from_buffer(struct kvm_vcpu *vcpu,
					void *insn, int insn_len)
{
	return x86_emulate_instruction(vcpu, 0, 0, insn, insn_len);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_instruction_from_buffer);

static int complete_fast_pio_out_port_0x7e(struct kvm_vcpu *vcpu)
{
	vcpu->arch.pio.count = 0;
	return 1;
}

static int complete_fast_pio_out(struct kvm_vcpu *vcpu)
{
	vcpu->arch.pio.count = 0;

	if (unlikely(!kvm_is_linear_rip(vcpu, vcpu->arch.cui_linear_rip)))
		return 1;

	return kvm_skip_emulated_instruction(vcpu);
}

static int kvm_fast_pio_out(struct kvm_vcpu *vcpu, int size,
			    unsigned short port)
{
	unsigned long val = kvm_rax_read(vcpu);
	int ret = emulator_pio_out(vcpu, size, port, &val, 1);

	if (ret)
		return ret;