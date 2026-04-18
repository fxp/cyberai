// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 55/86



static bool is_vmware_backdoor_opcode(struct x86_emulate_ctxt *ctxt)
{
	switch (ctxt->opcode_len) {
	case 1:
		switch (ctxt->b) {
		case 0xe4:	/* IN */
		case 0xe5:
		case 0xec:
		case 0xed:
		case 0xe6:	/* OUT */
		case 0xe7:
		case 0xee:
		case 0xef:
		case 0x6c:	/* INS */
		case 0x6d:
		case 0x6e:	/* OUTS */
		case 0x6f:
			return true;
		}
		break;
	case 2:
		switch (ctxt->b) {
		case 0x33:	/* RDPMC */
			return true;
		}
		break;
	}

	return false;
}

static bool is_soft_int_instruction(struct x86_emulate_ctxt *ctxt,
				    int emulation_type)
{
	u8 vector = EMULTYPE_GET_SOFT_INT_VECTOR(emulation_type);

	switch (ctxt->b) {
	case 0xcc:
		return vector == BP_VECTOR;
	case 0xcd:
		return vector == ctxt->src.val;
	case 0xce:
		return vector == OF_VECTOR;
	default:
		return false;
	}
}

/*
 * Decode an instruction for emulation.  The caller is responsible for handling
 * code breakpoints.  Note, manually detecting code breakpoints is unnecessary
 * (and wrong) when emulating on an intercepted fault-like exception[*], as
 * code breakpoints have higher priority and thus have already been done by
 * hardware.
 *
 * [*] Except #MC, which is higher priority, but KVM should never emulate in
 *     response to a machine check.
 */
int x86_decode_emulated_instruction(struct kvm_vcpu *vcpu, int emulation_type,
				    void *insn, int insn_len)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	int r;

	init_emulate_ctxt(vcpu);

	r = x86_decode_insn(ctxt, insn, insn_len, emulation_type);

	trace_kvm_emulate_insn_start(vcpu);
	++vcpu->stat.insn_emulation;

	return r;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(x86_decode_emulated_instruction);

int x86_emulate_instruction(struct kvm_vcpu *vcpu, gpa_t cr2_or_gpa,
			    int emulation_type, void *insn, int insn_len)
{
	int r;
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	bool writeback = true;

	if ((emulation_type & EMULTYPE_ALLOW_RETRY_PF) &&
	    (WARN_ON_ONCE(is_guest_mode(vcpu)) ||
	     WARN_ON_ONCE(!(emulation_type & EMULTYPE_PF))))
		emulation_type &= ~EMULTYPE_ALLOW_RETRY_PF;

	r = kvm_check_emulate_insn(vcpu, emulation_type, insn, insn_len);
	if (r != X86EMUL_CONTINUE) {
		if (r == X86EMUL_RETRY_INSTR || r == X86EMUL_PROPAGATE_FAULT)
			return 1;

		if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
						       emulation_type))
			return 1;

		if (r == X86EMUL_UNHANDLEABLE_VECTORING) {
			kvm_prepare_event_vectoring_exit(vcpu, cr2_or_gpa);
			return 0;
		}

		WARN_ON_ONCE(r != X86EMUL_UNHANDLEABLE);
		return handle_emulation_failure(vcpu, emulation_type);
	}

	kvm_request_l1tf_flush_l1d();

	if (!(emulation_type & EMULTYPE_NO_DECODE)) {
		kvm_clear_exception_queue(vcpu);

		/*
		 * Return immediately if RIP hits a code breakpoint, such #DBs
		 * are fault-like and are higher priority than any faults on
		 * the code fetch itself.
		 */
		if (kvm_vcpu_check_code_breakpoint(vcpu, emulation_type, &r))
			return r;

		r = x86_decode_emulated_instruction(vcpu, emulation_type,
						    insn, insn_len);
		if (r != EMULATION_OK)  {
			if ((emulation_type & EMULTYPE_TRAP_UD) ||
			    (emulation_type & EMULTYPE_TRAP_UD_FORCED)) {
				kvm_queue_exception(vcpu, UD_VECTOR);
				return 1;
			}
			if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
							       emulation_type))
				return 1;

			if (ctxt->have_exception &&
			    !(emulation_type & EMULTYPE_SKIP)) {
				/*
				 * #UD should result in just EMULATION_FAILED, and trap-like
				 * exception should not be encountered during decode.
				 */
				WARN_ON_ONCE(ctxt->exception.vector == UD_VECTOR ||
					     exception_type(ctxt->exception.vector) == EXCPT_TRAP);
				inject_emulated_exception(vcpu);
				return 1;
			}
			return handle_emulation_failure(vcpu, emulation_type);
		}
	}

	if ((emulation_type & EMULTYPE_VMWARE_GP) &&
	    !is_vmware_backdoor_opcode(ctxt)) {
		kvm_queue_exception_e(vcpu, GP_VECTOR, 0);
		return 1;
	}

	/*
	 * EMULTYPE_SKIP without EMULTYPE_COMPLETE_USER_EXIT is intended for
	 * use *only* by vendor callbacks for kvm_skip_emulated_instruction().
	 * The caller is responsible for updating interruptibility state and
	 * injecting single-step #DBs.
	 */
	if (emulation_type & EMULTYPE_SKIP) {
		if (emulation_type & EMULTYPE_SKIP_SOFT_INT &&
		    !is_soft_int_instruction(ctxt, emulation_type))
			return 0;

		if (ctxt->mode != X86EMUL_MODE_PROT64)
			ctxt->eip = (u32)ctxt->_eip;
		else
			ctxt->eip = ctxt->_eip;

		if (emulation_type & EMULTYPE_COMPLETE_USER_EXIT) {
			r = 1;
			goto writeback;
		}

		kvm_rip_write(vcpu, ctxt->eip);
		if (ctxt->eflags & X86_EFLAGS_RF)
			kvm_set_rflags(vcpu, ctxt->eflags & ~X86_EFLAGS_RF);
		return 1;
	}