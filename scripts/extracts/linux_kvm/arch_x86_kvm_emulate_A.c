// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 33/33



done:
	if (rc == X86EMUL_PROPAGATE_FAULT) {
		if (KVM_EMULATOR_BUG_ON(ctxt->exception.vector > 0x1f, ctxt))
			return EMULATION_FAILED;
		ctxt->have_exception = true;
	}
	if (rc == X86EMUL_INTERCEPTED)
		return EMULATION_INTERCEPTED;

	if (rc == X86EMUL_CONTINUE)
		writeback_registers(ctxt);

	return (rc == X86EMUL_UNHANDLEABLE) ? EMULATION_FAILED : EMULATION_OK;

twobyte_insn:
	switch (ctxt->b) {
	case 0x09:		/* wbinvd */
		(ctxt->ops->wbinvd)(ctxt);
		break;
	case 0x08:		/* invd */
	case 0x0d:		/* GrpP (prefetch) */
	case 0x18:		/* Grp16 (prefetch/nop) */
	case 0x1f:		/* nop */
		break;
	case 0x20: /* mov cr, reg */
		ctxt->dst.val = ops->get_cr(ctxt, ctxt->modrm_reg);
		break;
	case 0x21: /* mov from dr to reg */
		ctxt->dst.val = ops->get_dr(ctxt, ctxt->modrm_reg);
		break;
	case 0x40 ... 0x4f:	/* cmov */
		if (test_cc(ctxt->b, ctxt->eflags))
			ctxt->dst.val = ctxt->src.val;
		else if (ctxt->op_bytes != 4)
			ctxt->dst.type = OP_NONE; /* no writeback */
		break;
	case 0x80 ... 0x8f: /* jnz rel, etc*/
		if (test_cc(ctxt->b, ctxt->eflags))
			rc = jmp_rel(ctxt, ctxt->src.val);
		break;
	case 0x90 ... 0x9f:     /* setcc r/m8 */
		ctxt->dst.val = test_cc(ctxt->b, ctxt->eflags);
		break;
	case 0xb6 ... 0xb7:	/* movzx */
		ctxt->dst.bytes = ctxt->op_bytes;
		ctxt->dst.val = (ctxt->src.bytes == 1) ? (u8) ctxt->src.val
						       : (u16) ctxt->src.val;
		break;
	case 0xbe ... 0xbf:	/* movsx */
		ctxt->dst.bytes = ctxt->op_bytes;
		ctxt->dst.val = (ctxt->src.bytes == 1) ? (s8) ctxt->src.val :
							(s16) ctxt->src.val;
		break;
	default:
		goto cannot_emulate;
	}

threebyte_insn:

	if (rc != X86EMUL_CONTINUE)
		goto done;

	goto writeback;

cannot_emulate:
	return EMULATION_FAILED;
}

void emulator_invalidate_register_cache(struct x86_emulate_ctxt *ctxt)
{
	invalidate_registers(ctxt);
}

void emulator_writeback_register_cache(struct x86_emulate_ctxt *ctxt)
{
	writeback_registers(ctxt);
}

bool emulator_can_use_gpa(struct x86_emulate_ctxt *ctxt)
{
	if (ctxt->rep_prefix && (ctxt->d & String))
		return false;

	if (ctxt->d & TwoMemOp)
		return false;

	return true;
}
