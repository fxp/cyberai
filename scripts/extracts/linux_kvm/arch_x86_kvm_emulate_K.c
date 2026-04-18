// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 11/33



	if (seg == VCPU_SREG_TR) {
		old_desc = seg_desc;
		seg_desc.type |= 2; /* busy */
		ret = ctxt->ops->cmpxchg_emulated(ctxt, desc_addr, &old_desc, &seg_desc,
						  sizeof(seg_desc), &ctxt->exception);
		if (ret != X86EMUL_CONTINUE)
			return ret;
	}
load:
	ctxt->ops->set_segment(ctxt, selector, &seg_desc, base3, seg);
	if (desc)
		*desc = seg_desc;
	return X86EMUL_CONTINUE;
exception:
	return emulate_exception(ctxt, err_vec, err_code, true);
}

static int load_segment_descriptor(struct x86_emulate_ctxt *ctxt,
				   u16 selector, int seg)
{
	u8 cpl = ctxt->ops->cpl(ctxt);

	/*
	 * None of MOV, POP and LSS can load a NULL selector in CPL=3, but
	 * they can load it at CPL<3 (Intel's manual says only LSS can,
	 * but it's wrong).
	 *
	 * However, the Intel manual says that putting IST=1/DPL=3 in
	 * an interrupt gate will result in SS=3 (the AMD manual instead
	 * says it doesn't), so allow SS=3 in __load_segment_descriptor
	 * and only forbid it here.
	 */
	if (seg == VCPU_SREG_SS && selector == 3 &&
	    ctxt->mode == X86EMUL_MODE_PROT64)
		return emulate_exception(ctxt, GP_VECTOR, 0, true);

	return __load_segment_descriptor(ctxt, selector, seg, cpl,
					 X86_TRANSFER_NONE, NULL);
}

static void write_register_operand(struct operand *op)
{
	return assign_register(op->addr.reg, op->val, op->bytes);
}

static int writeback(struct x86_emulate_ctxt *ctxt, struct operand *op)
{
	switch (op->type) {
	case OP_REG:
		write_register_operand(op);
		break;
	case OP_MEM:
		if (ctxt->lock_prefix)
			return segmented_cmpxchg(ctxt,
						 op->addr.mem,
						 &op->orig_val,
						 &op->val,
						 op->bytes);
		else
			return segmented_write(ctxt,
					       op->addr.mem,
					       &op->val,
					       op->bytes);
	case OP_MEM_STR:
		return segmented_write(ctxt,
				       op->addr.mem,
				       op->data,
				       op->bytes * op->count);
	case OP_XMM:
		if (!(ctxt->d & Avx)) {
			kvm_write_sse_reg(op->addr.xmm, &op->vec_val);
			break;
		}
		/* full YMM write but with high bytes cleared */
		memset(op->valptr + 16, 0, 16);
		fallthrough;
	case OP_YMM:
		kvm_write_avx_reg(op->addr.xmm, &op->vec_val2);
		break;
	case OP_MM:
		kvm_write_mmx_reg(op->addr.mm, &op->mm_val);
		break;
	case OP_NONE:
		/* no writeback */
		break;
	default:
		break;
	}
	return X86EMUL_CONTINUE;
}

static int emulate_push(struct x86_emulate_ctxt *ctxt, const void *data, int len)
{
	struct segmented_address addr;

	rsp_increment(ctxt, -len);
	addr.ea = reg_read(ctxt, VCPU_REGS_RSP) & stack_mask(ctxt);
	addr.seg = VCPU_SREG_SS;

	return segmented_write(ctxt, addr, data, len);
}

static int em_push(struct x86_emulate_ctxt *ctxt)
{
	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;
	return emulate_push(ctxt, &ctxt->src.val, ctxt->op_bytes);
}

static int emulate_pop(struct x86_emulate_ctxt *ctxt,
		       void *dest, int len)
{
	int rc;
	struct segmented_address addr;

	addr.ea = reg_read(ctxt, VCPU_REGS_RSP) & stack_mask(ctxt);
	addr.seg = VCPU_SREG_SS;
	rc = segmented_read(ctxt, addr, dest, len);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	rsp_increment(ctxt, len);
	return rc;
}

static int em_pop(struct x86_emulate_ctxt *ctxt)
{
	return emulate_pop(ctxt, &ctxt->dst.val, ctxt->op_bytes);
}

static int emulate_popf(struct x86_emulate_ctxt *ctxt,
			void *dest, int len)
{
	int rc;
	unsigned long val = 0;
	unsigned long change_mask;
	int iopl = (ctxt->eflags & X86_EFLAGS_IOPL) >> X86_EFLAGS_IOPL_BIT;
	int cpl = ctxt->ops->cpl(ctxt);

	rc = emulate_pop(ctxt, &val, len);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	change_mask = X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF |
		      X86_EFLAGS_ZF | X86_EFLAGS_SF | X86_EFLAGS_OF |
		      X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_NT |
		      X86_EFLAGS_AC | X86_EFLAGS_ID;

	switch(ctxt->mode) {
	case X86EMUL_MODE_PROT64:
	case X86EMUL_MODE_PROT32:
	case X86EMUL_MODE_PROT16:
		if (cpl == 0)
			change_mask |= X86_EFLAGS_IOPL;
		if (cpl <= iopl)
			change_mask |= X86_EFLAGS_IF;
		break;
	case X86EMUL_MODE_VM86:
		if (iopl < 3)
			return emulate_gp(ctxt, 0);
		change_mask |= X86_EFLAGS_IF;
		break;
	default: /* real mode */
		change_mask |= (X86_EFLAGS_IOPL | X86_EFLAGS_IF);
		break;
	}

	*(unsigned long *)dest =
		(ctxt->eflags & ~change_mask) | (val & change_mask);

	return rc;
}

static int em_popf(struct x86_emulate_ctxt *ctxt)
{
	ctxt->dst.type = OP_REG;
	ctxt->dst.addr.reg = &ctxt->eflags;
	ctxt->dst.bytes = ctxt->op_bytes;
	return emulate_popf(ctxt, &ctxt->dst.val, ctxt->op_bytes);
}

static int em_enter(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	unsigned frame_size = ctxt->src.val;
	unsigned nesting_level = ctxt->src2.val & 31;
	ulong rbp;

	if (nesting_level)
		return X86EMUL_UNHANDLEABLE;