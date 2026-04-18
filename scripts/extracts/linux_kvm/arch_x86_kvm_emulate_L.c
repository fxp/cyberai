// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 12/33



	rbp = reg_read(ctxt, VCPU_REGS_RBP);
	rc = emulate_push(ctxt, &rbp, stack_size(ctxt));
	if (rc != X86EMUL_CONTINUE)
		return rc;
	assign_masked(reg_rmw(ctxt, VCPU_REGS_RBP), reg_read(ctxt, VCPU_REGS_RSP),
		      stack_mask(ctxt));
	assign_masked(reg_rmw(ctxt, VCPU_REGS_RSP),
		      reg_read(ctxt, VCPU_REGS_RSP) - frame_size,
		      stack_mask(ctxt));
	return X86EMUL_CONTINUE;
}

static int em_leave(struct x86_emulate_ctxt *ctxt)
{
	assign_masked(reg_rmw(ctxt, VCPU_REGS_RSP), reg_read(ctxt, VCPU_REGS_RBP),
		      stack_mask(ctxt));
	return emulate_pop(ctxt, reg_rmw(ctxt, VCPU_REGS_RBP), ctxt->op_bytes);
}

static int em_push_sreg(struct x86_emulate_ctxt *ctxt)
{
	int seg = ctxt->src2.val;

	ctxt->src.val = get_segment_selector(ctxt, seg);
	if (ctxt->op_bytes == 4) {
		rsp_increment(ctxt, -2);
		ctxt->op_bytes = 2;
	}

	return em_push(ctxt);
}

static int em_pop_sreg(struct x86_emulate_ctxt *ctxt)
{
	int seg = ctxt->src2.val;
	unsigned long selector = 0;
	int rc;

	rc = emulate_pop(ctxt, &selector, 2);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	if (seg == VCPU_SREG_SS)
		ctxt->interruptibility = KVM_X86_SHADOW_INT_MOV_SS;
	if (ctxt->op_bytes > 2)
		rsp_increment(ctxt, ctxt->op_bytes - 2);

	rc = load_segment_descriptor(ctxt, (u16)selector, seg);
	return rc;
}

static int em_pusha(struct x86_emulate_ctxt *ctxt)
{
	unsigned long old_esp = reg_read(ctxt, VCPU_REGS_RSP);
	int rc = X86EMUL_CONTINUE;
	int reg = VCPU_REGS_RAX;

	while (reg <= VCPU_REGS_RDI) {
		(reg == VCPU_REGS_RSP) ?
		(ctxt->src.val = old_esp) : (ctxt->src.val = reg_read(ctxt, reg));

		rc = em_push(ctxt);
		if (rc != X86EMUL_CONTINUE)
			return rc;

		++reg;
	}

	return rc;
}

static int em_pushf(struct x86_emulate_ctxt *ctxt)
{
	ctxt->src.val = (unsigned long)ctxt->eflags & ~X86_EFLAGS_VM;
	return em_push(ctxt);
}

static int em_popa(struct x86_emulate_ctxt *ctxt)
{
	int rc = X86EMUL_CONTINUE;
	int reg = VCPU_REGS_RDI;
	u32 val = 0;

	while (reg >= VCPU_REGS_RAX) {
		if (reg == VCPU_REGS_RSP) {
			rsp_increment(ctxt, ctxt->op_bytes);
			--reg;
		}

		rc = emulate_pop(ctxt, &val, ctxt->op_bytes);
		if (rc != X86EMUL_CONTINUE)
			break;
		assign_register(reg_rmw(ctxt, reg), val, ctxt->op_bytes);
		--reg;
	}
	return rc;
}

static int __emulate_int_real(struct x86_emulate_ctxt *ctxt, int irq)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	int rc;
	struct desc_ptr dt;
	gva_t cs_addr;
	gva_t eip_addr;
	u16 cs, eip;

	/* TODO: Add limit checks */
	ctxt->src.val = ctxt->eflags;
	rc = em_push(ctxt);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	ctxt->eflags &= ~(X86_EFLAGS_IF | X86_EFLAGS_TF | X86_EFLAGS_AC);

	ctxt->src.val = get_segment_selector(ctxt, VCPU_SREG_CS);
	rc = em_push(ctxt);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	ctxt->src.val = ctxt->_eip;
	rc = em_push(ctxt);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	ops->get_idt(ctxt, &dt);

	eip_addr = dt.address + (irq << 2);
	cs_addr = dt.address + (irq << 2) + 2;

	rc = linear_read_system(ctxt, cs_addr, &cs, 2);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	rc = linear_read_system(ctxt, eip_addr, &eip, 2);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	rc = load_segment_descriptor(ctxt, cs, VCPU_SREG_CS);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	ctxt->_eip = eip;

	return rc;
}

int emulate_int_real(struct x86_emulate_ctxt *ctxt, int irq)
{
	int rc;

	invalidate_registers(ctxt);
	rc = __emulate_int_real(ctxt, irq);
	if (rc == X86EMUL_CONTINUE)
		writeback_registers(ctxt);
	return rc;
}

static int emulate_int(struct x86_emulate_ctxt *ctxt, int irq)
{
	switch(ctxt->mode) {
	case X86EMUL_MODE_REAL:
		return __emulate_int_real(ctxt, irq);
	case X86EMUL_MODE_VM86:
	case X86EMUL_MODE_PROT16:
	case X86EMUL_MODE_PROT32:
	case X86EMUL_MODE_PROT64:
	default:
		/* Protected mode interrupts unimplemented yet */
		return X86EMUL_UNHANDLEABLE;
	}
}

static int emulate_iret_real(struct x86_emulate_ctxt *ctxt)
{
	int rc = X86EMUL_CONTINUE;
	unsigned long temp_eip = 0;
	unsigned long temp_eflags = 0;
	unsigned long cs = 0;
	unsigned long mask = X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF |
			     X86_EFLAGS_ZF | X86_EFLAGS_SF | X86_EFLAGS_TF |
			     X86_EFLAGS_IF | X86_EFLAGS_DF | X86_EFLAGS_OF |
			     X86_EFLAGS_IOPL | X86_EFLAGS_NT | X86_EFLAGS_RF |
			     X86_EFLAGS_AC | X86_EFLAGS_ID |
			     X86_EFLAGS_FIXED;
	unsigned long vm86_mask = X86_EFLAGS_VM | X86_EFLAGS_VIF |
				  X86_EFLAGS_VIP;

	/* TODO: Add stack limit check */

	rc = emulate_pop(ctxt, &temp_eip, ctxt->op_bytes);

	if (rc != X86EMUL_CONTINUE)
		return rc;

	if (temp_eip & ~0xffff)
		return emulate_gp(ctxt, 0);

	rc = emulate_pop(ctxt, &cs, ctxt->op_bytes);

	if (rc != X86EMUL_CONTINUE)
		return rc;

	rc = emulate_pop(ctxt, &temp_eflags, ctxt->op_bytes);

	if (rc != X86EMUL_CONTINUE)
		return rc;

	rc = load_segment_descriptor(ctxt, (u16)cs, VCPU_SREG_CS);

	if (rc != X86EMUL_CONTINUE)
		return rc;

	ctxt->_eip = temp_eip;