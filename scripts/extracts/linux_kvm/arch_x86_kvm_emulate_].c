// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 29/33



	if (l)
		ctxt->op_bytes = 32;
	else
		ctxt->op_bytes = 16;

	switch (pp) {
	case 0: break;
	case 1: ctxt->op_prefix = true; break;
	case 2: ctxt->rep_prefix = 0xf3; break;
	case 3: ctxt->rep_prefix = 0xf2; break;
	}

done:
	return rc;
ud:
	*opcode = ud;
	return rc;
}

int x86_decode_insn(struct x86_emulate_ctxt *ctxt, void *insn, int insn_len, int emulation_type)
{
	int rc = X86EMUL_CONTINUE;
	int mode = ctxt->mode;
	int def_op_bytes, def_ad_bytes, goffset, simd_prefix;
	bool vex_prefix = false;
	bool has_seg_override = false;
	struct opcode opcode;
	u16 dummy;
	struct desc_struct desc;

	ctxt->memop.type = OP_NONE;
	ctxt->memopp = NULL;
	ctxt->_eip = ctxt->eip;
	ctxt->fetch.ptr = ctxt->fetch.data;
	ctxt->fetch.end = ctxt->fetch.data + insn_len;
	ctxt->opcode_len = 1;
	ctxt->intercept = x86_intercept_none;
	if (insn_len > 0)
		memcpy(ctxt->fetch.data, insn, insn_len);
	else {
		rc = __do_insn_fetch_bytes(ctxt, 1);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	}

	switch (mode) {
	case X86EMUL_MODE_REAL:
	case X86EMUL_MODE_VM86:
		def_op_bytes = def_ad_bytes = 2;
		ctxt->ops->get_segment(ctxt, &dummy, &desc, NULL, VCPU_SREG_CS);
		if (desc.d)
			def_op_bytes = def_ad_bytes = 4;
		break;
	case X86EMUL_MODE_PROT16:
		def_op_bytes = def_ad_bytes = 2;
		break;
	case X86EMUL_MODE_PROT32:
		def_op_bytes = def_ad_bytes = 4;
		break;
#ifdef CONFIG_X86_64
	case X86EMUL_MODE_PROT64:
		def_op_bytes = 4;
		def_ad_bytes = 8;
		break;
#endif
	default:
		return EMULATION_FAILED;
	}

	ctxt->op_bytes = def_op_bytes;
	ctxt->ad_bytes = def_ad_bytes;

	/* Legacy prefixes. */
	for (;;) {
		switch (ctxt->b = insn_fetch(u8, ctxt)) {
		case 0x66:	/* operand-size override */
			ctxt->op_prefix = true;
			/* switch between 2/4 bytes */
			ctxt->op_bytes = def_op_bytes ^ 6;
			break;
		case 0x67:	/* address-size override */
			if (mode == X86EMUL_MODE_PROT64)
				/* switch between 4/8 bytes */
				ctxt->ad_bytes = def_ad_bytes ^ 12;
			else
				/* switch between 2/4 bytes */
				ctxt->ad_bytes = def_ad_bytes ^ 6;
			break;
		case 0x26:	/* ES override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_ES;
			break;
		case 0x2e:	/* CS override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_CS;
			break;
		case 0x36:	/* SS override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_SS;
			break;
		case 0x3e:	/* DS override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_DS;
			break;
		case 0x64:	/* FS override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_FS;
			break;
		case 0x65:	/* GS override */
			has_seg_override = true;
			ctxt->seg_override = VCPU_SREG_GS;
			break;
		case 0x40 ... 0x4f: /* REX */
			if (mode != X86EMUL_MODE_PROT64)
				goto done_prefixes;
			ctxt->rex_prefix = REX_PREFIX;
			ctxt->rex_bits   = ctxt->b & 0xf;
			continue;
		case 0xf0:	/* LOCK */
			ctxt->lock_prefix = 1;
			break;
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			ctxt->rep_prefix = ctxt->b;
			break;
		default:
			goto done_prefixes;
		}

		/* Any legacy prefix after a REX prefix nullifies its effect. */
		ctxt->rex_prefix = REX_NONE;
		ctxt->rex_bits = 0;
	}

done_prefixes:

	/* REX prefix. */
	if (ctxt->rex_bits & REX_W)
		ctxt->op_bytes = 8;

	/* Opcode byte(s). */
	if (ctxt->b == 0xc4 || ctxt->b == 0xc5) {
		/* VEX or LDS/LES */
		u8 vex_2nd = insn_fetch(u8, ctxt);
		if (mode != X86EMUL_MODE_PROT64 && (vex_2nd & 0xc0) != 0xc0) {
			opcode = opcode_table[ctxt->b];
			ctxt->modrm = vex_2nd;
			/* the Mod/RM byte has been fetched already!  */
			goto done_modrm;
		}

		vex_prefix = true;
		rc = x86_decode_avx(ctxt, ctxt->b, vex_2nd, &opcode);
		if (rc != X86EMUL_CONTINUE)
			goto done;
	} else if (ctxt->b == 0x0f) {
		/* Two- or three-byte opcode */
		ctxt->opcode_len = 2;
		ctxt->b = insn_fetch(u8, ctxt);
		opcode = twobyte_table[ctxt->b];

		/* 0F_38 opcode map */
		if (ctxt->b == 0x38) {
			ctxt->opcode_len = 3;
			ctxt->b = insn_fetch(u8, ctxt);
			opcode = opcode_map_0f_38[ctxt->b];
		}
	} else {
		/* Opcode byte(s). */
		opcode = opcode_table[ctxt->b];
	}

	if (opcode.flags & ModRM)
		ctxt->modrm = insn_fetch(u8, ctxt);