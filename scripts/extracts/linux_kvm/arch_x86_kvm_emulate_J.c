// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 10/33



	if (ctxt->mode == X86EMUL_MODE_REAL) {
		/* set real mode segment descriptor (keep limit etc. for
		 * unreal mode) */
		ctxt->ops->get_segment(ctxt, &dummy, &seg_desc, NULL, seg);
		set_desc_base(&seg_desc, selector << 4);
		goto load;
	} else if (seg <= VCPU_SREG_GS && ctxt->mode == X86EMUL_MODE_VM86) {
		/* VM86 needs a clean new segment descriptor */
		set_desc_base(&seg_desc, selector << 4);
		set_desc_limit(&seg_desc, 0xffff);
		seg_desc.type = 3;
		seg_desc.p = 1;
		seg_desc.s = 1;
		seg_desc.dpl = 3;
		goto load;
	}

	rpl = selector & 3;

	/* TR should be in GDT only */
	if (seg == VCPU_SREG_TR && (selector & (1 << 2)))
		goto exception;

	/* NULL selector is not valid for TR, CS and (except for long mode) SS */
	if (null_selector) {
		if (seg == VCPU_SREG_CS || seg == VCPU_SREG_TR)
			goto exception;

		if (seg == VCPU_SREG_SS) {
			if (ctxt->mode != X86EMUL_MODE_PROT64 || rpl != cpl)
				goto exception;

			/*
			 * ctxt->ops->set_segment expects the CPL to be in
			 * SS.DPL, so fake an expand-up 32-bit data segment.
			 */
			seg_desc.type = 3;
			seg_desc.p = 1;
			seg_desc.s = 1;
			seg_desc.dpl = cpl;
			seg_desc.d = 1;
			seg_desc.g = 1;
		}

		/* Skip all following checks */
		goto load;
	}

	ret = read_segment_descriptor(ctxt, selector, &seg_desc, &desc_addr);
	if (ret != X86EMUL_CONTINUE)
		return ret;

	err_code = selector & 0xfffc;
	err_vec = (transfer == X86_TRANSFER_TASK_SWITCH) ? TS_VECTOR :
							   GP_VECTOR;

	/* can't load system descriptor into segment selector */
	if (seg <= VCPU_SREG_GS && !seg_desc.s) {
		if (transfer == X86_TRANSFER_CALL_JMP)
			return X86EMUL_UNHANDLEABLE;
		goto exception;
	}

	dpl = seg_desc.dpl;

	switch (seg) {
	case VCPU_SREG_SS:
		/*
		 * segment is not a writable data segment or segment
		 * selector's RPL != CPL or DPL != CPL
		 */
		if (rpl != cpl || (seg_desc.type & 0xa) != 0x2 || dpl != cpl)
			goto exception;
		break;
	case VCPU_SREG_CS:
		/*
		 * KVM uses "none" when loading CS as part of emulating Real
		 * Mode exceptions and IRET (handled above).  In all other
		 * cases, loading CS without a control transfer is a KVM bug.
		 */
		if (WARN_ON_ONCE(transfer == X86_TRANSFER_NONE))
			goto exception;

		if (!(seg_desc.type & 8))
			goto exception;

		if (transfer == X86_TRANSFER_RET) {
			/* RET can never return to an inner privilege level. */
			if (rpl < cpl)
				goto exception;
			/* Outer-privilege level return is not implemented */
			if (rpl > cpl)
				return X86EMUL_UNHANDLEABLE;
		}
		if (transfer == X86_TRANSFER_RET || transfer == X86_TRANSFER_TASK_SWITCH) {
			if (seg_desc.type & 4) {
				/* conforming */
				if (dpl > rpl)
					goto exception;
			} else {
				/* nonconforming */
				if (dpl != rpl)
					goto exception;
			}
		} else { /* X86_TRANSFER_CALL_JMP */
			if (seg_desc.type & 4) {
				/* conforming */
				if (dpl > cpl)
					goto exception;
			} else {
				/* nonconforming */
				if (rpl > cpl || dpl != cpl)
					goto exception;
			}
		}
		/* in long-mode d/b must be clear if l is set */
		if (seg_desc.d && seg_desc.l) {
			u64 efer = 0;

			ctxt->ops->get_msr(ctxt, MSR_EFER, &efer);
			if (efer & EFER_LMA)
				goto exception;
		}
		if (!seg_desc.l && emulator_is_ssp_invalid(ctxt, cpl)) {
			err_code = 0;
			goto exception;
		}

		/* CS(RPL) <- CPL */
		selector = (selector & 0xfffc) | cpl;
		break;
	case VCPU_SREG_TR:
		if (seg_desc.s || (seg_desc.type != 1 && seg_desc.type != 9))
			goto exception;
		break;
	case VCPU_SREG_LDTR:
		if (seg_desc.s || seg_desc.type != 2)
			goto exception;
		break;
	default: /*  DS, ES, FS, or GS */
		/*
		 * segment is not a data or readable code segment or
		 * ((segment is a data or nonconforming code segment)
		 * and ((RPL > DPL) or (CPL > DPL)))
		 */
		if ((seg_desc.type & 0xa) == 0x8 ||
		    (((seg_desc.type & 0xc) != 0xc) &&
		     (rpl > dpl || cpl > dpl)))
			goto exception;
		break;
	}

	if (!seg_desc.p) {
		err_vec = (seg == VCPU_SREG_SS) ? SS_VECTOR : NP_VECTOR;
		goto exception;
	}

	if (seg_desc.s) {
		/* mark segment as accessed */
		if (!(seg_desc.type & 1)) {
			seg_desc.type |= 1;
			ret = write_segment_descriptor(ctxt, selector,
						       &seg_desc);
			if (ret != X86EMUL_CONTINUE)
				return ret;
		}
	} else if (ctxt->mode == X86EMUL_MODE_PROT64) {
		ret = linear_read_system(ctxt, desc_addr+8, &base3, sizeof(base3));
		if (ret != X86EMUL_CONTINUE)
			return ret;
		if (emul_is_noncanonical_address(get_desc_base(&seg_desc) |
						 ((u64)base3 << 32), ctxt,
						 X86EMUL_F_DT_LOAD))
			return emulate_gp(ctxt, err_code);
	}