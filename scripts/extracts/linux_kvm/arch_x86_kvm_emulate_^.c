// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 30/33



done_modrm:
	ctxt->d = opcode.flags;
	while (ctxt->d & GroupMask) {
		switch (ctxt->d & GroupMask) {
		case Group:
			goffset = (ctxt->modrm >> 3) & 7;
			opcode = opcode.u.group[goffset];
			break;
		case GroupDual:
			goffset = (ctxt->modrm >> 3) & 7;
			if ((ctxt->modrm >> 6) == 3)
				opcode = opcode.u.gdual->mod3[goffset];
			else
				opcode = opcode.u.gdual->mod012[goffset];
			break;
		case RMExt:
			goffset = ctxt->modrm & 7;
			opcode = opcode.u.group[goffset];
			break;
		case Prefix:
			if (ctxt->rep_prefix && ctxt->op_prefix)
				return EMULATION_FAILED;
			simd_prefix = ctxt->op_prefix ? 0x66 : ctxt->rep_prefix;
			switch (simd_prefix) {
			case 0x00: opcode = opcode.u.gprefix->pfx_no; break;
			case 0x66: opcode = opcode.u.gprefix->pfx_66; break;
			case 0xf2: opcode = opcode.u.gprefix->pfx_f2; break;
			case 0xf3: opcode = opcode.u.gprefix->pfx_f3; break;
			}
			break;
		case Escape:
			if (ctxt->modrm > 0xbf) {
				size_t size = ARRAY_SIZE(opcode.u.esc->high);
				u32 index = array_index_nospec(
					ctxt->modrm - 0xc0, size);

				opcode = opcode.u.esc->high[index];
			} else {
				opcode = opcode.u.esc->op[(ctxt->modrm >> 3) & 7];
			}
			break;
		case InstrDual:
			if ((ctxt->modrm >> 6) == 3)
				opcode = opcode.u.idual->mod3;
			else
				opcode = opcode.u.idual->mod012;
			break;
		case ModeDual:
			if (ctxt->mode == X86EMUL_MODE_PROT64)
				opcode = opcode.u.mdual->mode64;
			else
				opcode = opcode.u.mdual->mode32;
			break;
		default:
			return EMULATION_FAILED;
		}

		ctxt->d &= ~(u64)GroupMask;
		ctxt->d |= opcode.flags;
	}

	ctxt->is_branch = opcode.flags & IsBranch;

	/* Unrecognised? */
	if (ctxt->d == 0)
		return EMULATION_FAILED;

	if (unlikely(vex_prefix)) {
		/*
		 * Only specifically marked instructions support VEX.  Since many
		 * instructions support it but are not annotated, return not implemented
		 * rather than #UD.
		 */
		if (!(ctxt->d & Avx))
			return EMULATION_FAILED;

		if (!(ctxt->d & AlignMask))
			ctxt->d |= Unaligned;
	}

	ctxt->execute = opcode.u.execute;

	/*
	 * Reject emulation if KVM might need to emulate shadow stack updates
	 * and/or indirect branch tracking enforcement, which the emulator
	 * doesn't support.
	 */
	if ((is_ibt_instruction(ctxt) || is_shstk_instruction(ctxt)) &&
	    ctxt->ops->get_cr(ctxt, 4) & X86_CR4_CET) {
		u64 u_cet = 0, s_cet = 0;

		/*
		 * Check both User and Supervisor on far transfers as inter-
		 * privilege level transfers are impacted by CET at the target
		 * privilege level, and that is not known at this time.  The
		 * expectation is that the guest will not require emulation of
		 * any CET-affected instructions at any privilege level.
		 */
		if (!(ctxt->d & NearBranch))
			u_cet = s_cet = CET_SHSTK_EN | CET_ENDBR_EN;
		else if (ctxt->ops->cpl(ctxt) == 3)
			u_cet = CET_SHSTK_EN | CET_ENDBR_EN;
		else
			s_cet = CET_SHSTK_EN | CET_ENDBR_EN;

		if ((u_cet && ctxt->ops->get_msr(ctxt, MSR_IA32_U_CET, &u_cet)) ||
		    (s_cet && ctxt->ops->get_msr(ctxt, MSR_IA32_S_CET, &s_cet)))
			return EMULATION_FAILED;

		if ((u_cet | s_cet) & CET_SHSTK_EN && is_shstk_instruction(ctxt))
			return EMULATION_FAILED;

		if ((u_cet | s_cet) & CET_ENDBR_EN && is_ibt_instruction(ctxt))
			return EMULATION_FAILED;
	}

	if (unlikely(emulation_type & EMULTYPE_TRAP_UD) &&
	    likely(!(ctxt->d & EmulateOnUD)))
		return EMULATION_FAILED;

	if (unlikely(ctxt->d &
	    (NotImpl|Stack|Op3264|Sse|Mmx|Intercept|CheckPerm|NearBranch|
	     No16))) {
		/*
		 * These are copied unconditionally here, and checked unconditionally
		 * in x86_emulate_insn.
		 */
		ctxt->check_perm = opcode.check_perm;
		ctxt->intercept = opcode.intercept;

		if (ctxt->d & NotImpl)
			return EMULATION_FAILED;

		if (mode == X86EMUL_MODE_PROT64) {
			if (ctxt->op_bytes == 4 && (ctxt->d & Stack))
				ctxt->op_bytes = 8;
			else if (ctxt->d & NearBranch)
				ctxt->op_bytes = 8;
		}

		if (ctxt->d & Op3264) {
			if (mode == X86EMUL_MODE_PROT64)
				ctxt->op_bytes = 8;
			else
				ctxt->op_bytes = 4;
		}

		if ((ctxt->d & No16) && ctxt->op_bytes == 2)
			ctxt->op_bytes = 4;

		if (vex_prefix)
			;
		else if (ctxt->d & Sse)
			ctxt->op_bytes = 16, ctxt->d &= ~Avx;
		else if (ctxt->d & Mmx)
			ctxt->op_bytes = 8;
	}

	/* ModRM and SIB bytes. */
	if (ctxt->d & ModRM) {
		rc = decode_modrm(ctxt, &ctxt->memop);
		if (!has_seg_override) {
			has_seg_override = true;
			ctxt->seg_override = ctxt->modrm_seg;
		}
	} else if (ctxt->d & MemAbs)
		rc = decode_abs(ctxt, &ctxt->memop);
	if (rc != X86EMUL_CONTINUE)
		goto done;

	if (!has_seg_override)
		ctxt->seg_override = VCPU_SREG_DS;

	ctxt->memop.addr.mem.seg = ctxt->seg_override;

	/*
	 * Decode and fetch the source operand: register, memory
	 * or immediate.
	 */
	rc = decode_operand(ctxt, &ctxt->src, (ctxt->d >> SrcShift) & OpMask);
	if (rc != X86EMUL_CONTINUE)
		goto done;