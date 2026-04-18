// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 32/33



		if (ctxt->rep_prefix && (ctxt->d & String)) {
			/* All REP prefixes have the same first termination condition */
			if (address_mask(ctxt, reg_read(ctxt, VCPU_REGS_RCX)) == 0) {
				string_registers_quirk(ctxt);
				ctxt->eip = ctxt->_eip;
				ctxt->eflags &= ~X86_EFLAGS_RF;
				goto done;
			}
		}
	}

	if ((ctxt->src.type == OP_MEM) && !(ctxt->d & NoAccess)) {
		rc = segmented_read(ctxt, ctxt->src.addr.mem,
				    ctxt->src.valptr, ctxt->src.bytes);
		if (rc != X86EMUL_CONTINUE)
			goto done;
		ctxt->src.orig_val64 = ctxt->src.val64;
	}

	if (ctxt->src2.type == OP_MEM) {
		rc = segmented_read(ctxt, ctxt->src2.addr.mem,
				    &ctxt->src2.val, ctxt->src2.bytes);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	}

	if ((ctxt->d & DstMask) == ImplicitOps)
		goto special_insn;


	if ((ctxt->dst.type == OP_MEM) && !(ctxt->d & Mov)) {
		/* optimisation - avoid slow emulated read if Mov */
		rc = segmented_read(ctxt, ctxt->dst.addr.mem,
				   &ctxt->dst.val, ctxt->dst.bytes);
		if (rc != X86EMUL_CONTINUE) {
			if (!(ctxt->d & NoWrite) &&
			    rc == X86EMUL_PROPAGATE_FAULT &&
			    ctxt->exception.vector == PF_VECTOR)
				ctxt->exception.error_code |= PFERR_WRITE_MASK;
			goto done;
		}
	}
	/* Copy full 64-bit value for CMPXCHG8B.  */
	ctxt->dst.orig_val64 = ctxt->dst.val64;

special_insn:

	if (unlikely(check_intercepts) && (ctxt->d & Intercept)) {
		rc = emulator_check_intercept(ctxt, ctxt->intercept,
					      X86_ICPT_POST_MEMACCESS);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	}

	if (ctxt->rep_prefix && (ctxt->d & String))
		ctxt->eflags |= X86_EFLAGS_RF;
	else
		ctxt->eflags &= ~X86_EFLAGS_RF;

	if (ctxt->execute) {
		rc = ctxt->execute(ctxt);
		if (rc != X86EMUL_CONTINUE)
			goto done;
		goto writeback;
	}

	if (ctxt->opcode_len == 2)
		goto twobyte_insn;
	else if (ctxt->opcode_len == 3)
		goto threebyte_insn;

	switch (ctxt->b) {
	case 0x70 ... 0x7f: /* jcc (short) */
		if (test_cc(ctxt->b, ctxt->eflags))
			rc = jmp_rel(ctxt, ctxt->src.val);
		break;
	case 0x8d: /* lea r16/r32, m */
		ctxt->dst.val = ctxt->src.addr.mem.ea;
		break;
	case 0x90 ... 0x97: /* nop / xchg reg, rax */
		if (ctxt->dst.addr.reg == reg_rmw(ctxt, VCPU_REGS_RAX))
			ctxt->dst.type = OP_NONE;
		else
			rc = em_xchg(ctxt);
		break;
	case 0x98: /* cbw/cwde/cdqe */
		switch (ctxt->op_bytes) {
		case 2: ctxt->dst.val = (s8)ctxt->dst.val; break;
		case 4: ctxt->dst.val = (s16)ctxt->dst.val; break;
		case 8: ctxt->dst.val = (s32)ctxt->dst.val; break;
		}
		break;
	case 0xcc:		/* int3 */
		rc = emulate_int(ctxt, 3);
		break;
	case 0xcd:		/* int n */
		rc = emulate_int(ctxt, ctxt->src.val);
		break;
	case 0xce:		/* into */
		if (ctxt->eflags & X86_EFLAGS_OF)
			rc = emulate_int(ctxt, 4);
		break;
	case 0xe9: /* jmp rel */
	case 0xeb: /* jmp rel short */
		rc = jmp_rel(ctxt, ctxt->src.val);
		ctxt->dst.type = OP_NONE; /* Disable writeback. */
		break;
	case 0xf4:              /* hlt */
		ctxt->ops->halt(ctxt);
		break;
	case 0xf5:	/* cmc */
		/* complement carry flag from eflags reg */
		ctxt->eflags ^= X86_EFLAGS_CF;
		break;
	case 0xf8: /* clc */
		ctxt->eflags &= ~X86_EFLAGS_CF;
		break;
	case 0xf9: /* stc */
		ctxt->eflags |= X86_EFLAGS_CF;
		break;
	case 0xfc: /* cld */
		ctxt->eflags &= ~X86_EFLAGS_DF;
		break;
	case 0xfd: /* std */
		ctxt->eflags |= X86_EFLAGS_DF;
		break;
	default:
		goto cannot_emulate;
	}

	if (rc != X86EMUL_CONTINUE)
		goto done;

writeback:
	if (ctxt->d & SrcWrite) {
		BUG_ON(ctxt->src.type == OP_MEM || ctxt->src.type == OP_MEM_STR);
		rc = writeback(ctxt, &ctxt->src);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	}
	if (!(ctxt->d & NoWrite)) {
		rc = writeback(ctxt, &ctxt->dst);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	}

	/*
	 * restore dst type in case the decoding will be reused
	 * (happens for string instruction )
	 */
	ctxt->dst.type = saved_dst_type;

	if ((ctxt->d & SrcMask) == SrcSI)
		string_addr_inc(ctxt, VCPU_REGS_RSI, &ctxt->src);

	if ((ctxt->d & DstMask) == DstDI)
		string_addr_inc(ctxt, VCPU_REGS_RDI, &ctxt->dst);

	if (ctxt->rep_prefix && (ctxt->d & String)) {
		unsigned int count;
		struct read_cache *r = &ctxt->io_read;
		if ((ctxt->d & SrcMask) == SrcSI)
			count = ctxt->src.count;
		else
			count = ctxt->dst.count;
		register_address_increment(ctxt, VCPU_REGS_RCX, -count);

		if (!string_insn_completed(ctxt)) {
			/*
			 * Re-enter guest when pio read ahead buffer is empty
			 * or, if it is not used, after each 1024 iteration.
			 */
			if ((r->end != 0 || reg_read(ctxt, VCPU_REGS_RCX) & 0x3ff) &&
			    (r->end == 0 || r->end != r->pos)) {
				/*
				 * Reset read cache. Usually happens before
				 * decode, but since instruction is restarted
				 * we have to do it here.
				 */
				ctxt->mem_read.end = 0;
				writeback_registers(ctxt);
				return EMULATION_RESTART;
			}
			goto done; /* skip rip writeback */
		}
		ctxt->eflags &= ~X86_EFLAGS_RF;
	}

	ctxt->eip = ctxt->_eip;
	if (ctxt->mode != X86EMUL_MODE_PROT64)
		ctxt->eip = (u32)ctxt->_eip;