// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 13/33



	if (ctxt->op_bytes == 4)
		ctxt->eflags = ((temp_eflags & mask) | (ctxt->eflags & vm86_mask));
	else if (ctxt->op_bytes == 2) {
		ctxt->eflags &= ~0xffff;
		ctxt->eflags |= temp_eflags;
	}

	ctxt->eflags &= ~EFLG_RESERVED_ZEROS_MASK; /* Clear reserved zeros */
	ctxt->eflags |= X86_EFLAGS_FIXED;
	ctxt->ops->set_nmi_mask(ctxt, false);

	return rc;
}

static int em_iret(struct x86_emulate_ctxt *ctxt)
{
	switch(ctxt->mode) {
	case X86EMUL_MODE_REAL:
		return emulate_iret_real(ctxt);
	case X86EMUL_MODE_VM86:
	case X86EMUL_MODE_PROT16:
	case X86EMUL_MODE_PROT32:
	case X86EMUL_MODE_PROT64:
	default:
		/* iret from protected mode unimplemented yet */
		return X86EMUL_UNHANDLEABLE;
	}
}

static int em_jmp_far(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	unsigned short sel;
	struct desc_struct new_desc;
	u8 cpl = ctxt->ops->cpl(ctxt);

	memcpy(&sel, ctxt->src.valptr + ctxt->op_bytes, 2);

	rc = __load_segment_descriptor(ctxt, sel, VCPU_SREG_CS, cpl,
				       X86_TRANSFER_CALL_JMP,
				       &new_desc);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	rc = assign_eip_far(ctxt, ctxt->src.val);
	/* Error handling is not implemented. */
	if (rc != X86EMUL_CONTINUE)
		return X86EMUL_UNHANDLEABLE;

	return rc;
}

static int em_jmp_abs(struct x86_emulate_ctxt *ctxt)
{
	return assign_eip_near(ctxt, ctxt->src.val);
}

static int em_call_near_abs(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	long int old_eip;

	old_eip = ctxt->_eip;
	rc = assign_eip_near(ctxt, ctxt->src.val);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	ctxt->src.val = old_eip;
	rc = em_push(ctxt);
	return rc;
}

static int em_cmpxchg8b(struct x86_emulate_ctxt *ctxt)
{
	u64 old = ctxt->dst.orig_val64;

	if (ctxt->dst.bytes == 16)
		return X86EMUL_UNHANDLEABLE;

	if (((u32) (old >> 0) != (u32) reg_read(ctxt, VCPU_REGS_RAX)) ||
	    ((u32) (old >> 32) != (u32) reg_read(ctxt, VCPU_REGS_RDX))) {
		*reg_write(ctxt, VCPU_REGS_RAX) = (u32) (old >> 0);
		*reg_write(ctxt, VCPU_REGS_RDX) = (u32) (old >> 32);
		ctxt->eflags &= ~X86_EFLAGS_ZF;
	} else {
		ctxt->dst.val64 = ((u64)reg_read(ctxt, VCPU_REGS_RCX) << 32) |
			(u32) reg_read(ctxt, VCPU_REGS_RBX);

		ctxt->eflags |= X86_EFLAGS_ZF;
	}
	return X86EMUL_CONTINUE;
}

static int em_ret(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	unsigned long eip = 0;

	rc = emulate_pop(ctxt, &eip, ctxt->op_bytes);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	return assign_eip_near(ctxt, eip);
}

static int em_ret_far(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	unsigned long eip = 0;
	unsigned long cs = 0;
	int cpl = ctxt->ops->cpl(ctxt);
	struct desc_struct new_desc;

	rc = emulate_pop(ctxt, &eip, ctxt->op_bytes);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	rc = emulate_pop(ctxt, &cs, ctxt->op_bytes);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	rc = __load_segment_descriptor(ctxt, (u16)cs, VCPU_SREG_CS, cpl,
				       X86_TRANSFER_RET,
				       &new_desc);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	rc = assign_eip_far(ctxt, eip);
	/* Error handling is not implemented. */
	if (rc != X86EMUL_CONTINUE)
		return X86EMUL_UNHANDLEABLE;

	return rc;
}

static int em_ret_far_imm(struct x86_emulate_ctxt *ctxt)
{
        int rc;

        rc = em_ret_far(ctxt);
        if (rc != X86EMUL_CONTINUE)
                return rc;
        rsp_increment(ctxt, ctxt->src.val);
        return X86EMUL_CONTINUE;
}

static int em_cmpxchg(struct x86_emulate_ctxt *ctxt)
{
	/* Save real source value, then compare EAX against destination. */
	ctxt->dst.orig_val = ctxt->dst.val;
	ctxt->dst.val = reg_read(ctxt, VCPU_REGS_RAX);
	ctxt->src.orig_val = ctxt->src.val;
	ctxt->src.val = ctxt->dst.orig_val;
	em_cmp(ctxt);

	if (ctxt->eflags & X86_EFLAGS_ZF) {
		/* Success: write back to memory; no update of EAX */
		ctxt->src.type = OP_NONE;
		ctxt->dst.val = ctxt->src.orig_val;
	} else {
		/* Failure: write the value we saw to EAX. */
		ctxt->src.type = OP_REG;
		ctxt->src.addr.reg = reg_rmw(ctxt, VCPU_REGS_RAX);
		ctxt->src.val = ctxt->dst.orig_val;
		/* Create write-cycle to dest by writing the same value */
		ctxt->dst.val = ctxt->dst.orig_val;
	}
	return X86EMUL_CONTINUE;
}

static int em_lseg(struct x86_emulate_ctxt *ctxt)
{
	int seg = ctxt->src2.val;
	unsigned short sel;
	int rc;

	memcpy(&sel, ctxt->src.valptr + ctxt->op_bytes, 2);

	rc = load_segment_descriptor(ctxt, sel, seg);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	ctxt->dst.val = ctxt->src.val;
	return rc;
}

static int em_rsm(struct x86_emulate_ctxt *ctxt)
{
	if (!ctxt->ops->is_smm(ctxt))
		return emulate_ud(ctxt);

	if (ctxt->ops->leave_smm(ctxt))
		ctxt->ops->triple_fault(ctxt);

	return emulator_recalc_and_set_mode(ctxt);
}

static void
setup_syscalls_segments(struct desc_struct *cs, struct desc_struct *ss)
{
	cs->l = 0;		/* will be adjusted later */
	set_desc_base(cs, 0);	/* flat segment */
	cs->g = 1;		/* 4kb granularity */
	set_desc_limit(cs, 0xfffff);	/* 4GB limit */
	cs->type = 0x0b;	/* Read, Execute, Accessed */
	cs->s = 1;
	cs->dpl = 0;		/* will be adjusted later */
	cs->p = 1;
	cs->d = 1;
	cs->avl = 0;