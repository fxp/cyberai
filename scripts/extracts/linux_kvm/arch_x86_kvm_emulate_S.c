// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 19/33



	return X86EMUL_CONTINUE;
}

static int em_rdpid(struct x86_emulate_ctxt *ctxt)
{
	u64 tsc_aux = 0;

	if (!ctxt->ops->guest_has_rdpid(ctxt))
		return emulate_ud(ctxt);

	ctxt->ops->get_msr(ctxt, MSR_TSC_AUX, &tsc_aux);
	ctxt->dst.val = tsc_aux;
	return X86EMUL_CONTINUE;
}

static int em_rdtsc(struct x86_emulate_ctxt *ctxt)
{
	u64 tsc = 0;

	ctxt->ops->get_msr(ctxt, MSR_IA32_TSC, &tsc);
	*reg_write(ctxt, VCPU_REGS_RAX) = (u32)tsc;
	*reg_write(ctxt, VCPU_REGS_RDX) = tsc >> 32;
	return X86EMUL_CONTINUE;
}

static int em_rdpmc(struct x86_emulate_ctxt *ctxt)
{
	u64 pmc;

	if (ctxt->ops->read_pmc(ctxt, reg_read(ctxt, VCPU_REGS_RCX), &pmc))
		return emulate_gp(ctxt, 0);
	*reg_write(ctxt, VCPU_REGS_RAX) = (u32)pmc;
	*reg_write(ctxt, VCPU_REGS_RDX) = pmc >> 32;
	return X86EMUL_CONTINUE;
}

static int em_mov(struct x86_emulate_ctxt *ctxt)
{
	memcpy(ctxt->dst.valptr, ctxt->src.valptr, sizeof(ctxt->src.valptr));
	return X86EMUL_CONTINUE;
}

static int em_movbe(struct x86_emulate_ctxt *ctxt)
{
	u16 tmp;

	if (!ctxt->ops->guest_has_movbe(ctxt))
		return emulate_ud(ctxt);

	switch (ctxt->op_bytes) {
	case 2:
		/*
		 * From MOVBE definition: "...When the operand size is 16 bits,
		 * the upper word of the destination register remains unchanged
		 * ..."
		 *
		 * Both casting ->valptr and ->val to u16 breaks strict aliasing
		 * rules so we have to do the operation almost per hand.
		 */
		tmp = (u16)ctxt->src.val;
		ctxt->dst.val &= ~0xffffUL;
		ctxt->dst.val |= (unsigned long)swab16(tmp);
		break;
	case 4:
		ctxt->dst.val = swab32((u32)ctxt->src.val);
		break;
	case 8:
		ctxt->dst.val = swab64(ctxt->src.val);
		break;
	default:
		BUG();
	}
	return X86EMUL_CONTINUE;
}

static int em_cr_write(struct x86_emulate_ctxt *ctxt)
{
	int cr_num = ctxt->modrm_reg;
	int r;

	if (ctxt->ops->set_cr(ctxt, cr_num, ctxt->src.val))
		return emulate_gp(ctxt, 0);

	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;

	if (cr_num == 0) {
		/*
		 * CR0 write might have updated CR0.PE and/or CR0.PG
		 * which can affect the cpu's execution mode.
		 */
		r = emulator_recalc_and_set_mode(ctxt);
		if (r != X86EMUL_CONTINUE)
			return r;
	}

	return X86EMUL_CONTINUE;
}

static int em_dr_write(struct x86_emulate_ctxt *ctxt)
{
	unsigned long val;

	if (ctxt->mode == X86EMUL_MODE_PROT64)
		val = ctxt->src.val & ~0ULL;
	else
		val = ctxt->src.val & ~0U;

	/* #UD condition is already handled. */
	if (ctxt->ops->set_dr(ctxt, ctxt->modrm_reg, val) < 0)
		return emulate_gp(ctxt, 0);

	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;
	return X86EMUL_CONTINUE;
}

static int em_wrmsr(struct x86_emulate_ctxt *ctxt)
{
	u64 msr_index = reg_read(ctxt, VCPU_REGS_RCX);
	u64 msr_data;
	int r;

	msr_data = (u32)reg_read(ctxt, VCPU_REGS_RAX)
		| ((u64)reg_read(ctxt, VCPU_REGS_RDX) << 32);
	r = ctxt->ops->set_msr_with_filter(ctxt, msr_index, msr_data);

	if (r == X86EMUL_PROPAGATE_FAULT)
		return emulate_gp(ctxt, 0);

	return r;
}

static int em_rdmsr(struct x86_emulate_ctxt *ctxt)
{
	u64 msr_index = reg_read(ctxt, VCPU_REGS_RCX);
	u64 msr_data;
	int r;

	r = ctxt->ops->get_msr_with_filter(ctxt, msr_index, &msr_data);

	if (r == X86EMUL_PROPAGATE_FAULT)
		return emulate_gp(ctxt, 0);

	if (r == X86EMUL_CONTINUE) {
		*reg_write(ctxt, VCPU_REGS_RAX) = (u32)msr_data;
		*reg_write(ctxt, VCPU_REGS_RDX) = msr_data >> 32;
	}
	return r;
}

static int em_store_sreg(struct x86_emulate_ctxt *ctxt, int segment)
{
	if (segment > VCPU_SREG_GS &&
	    (ctxt->ops->get_cr(ctxt, 4) & X86_CR4_UMIP) &&
	    ctxt->ops->cpl(ctxt) > 0)
		return emulate_gp(ctxt, 0);

	ctxt->dst.val = get_segment_selector(ctxt, segment);
	if (ctxt->dst.bytes == 4 && ctxt->dst.type == OP_MEM)
		ctxt->dst.bytes = 2;
	return X86EMUL_CONTINUE;
}

static int em_mov_rm_sreg(struct x86_emulate_ctxt *ctxt)
{
	if (ctxt->modrm_reg > VCPU_SREG_GS)
		return emulate_ud(ctxt);

	return em_store_sreg(ctxt, ctxt->modrm_reg);
}

static int em_mov_sreg_rm(struct x86_emulate_ctxt *ctxt)
{
	u16 sel = ctxt->src.val;

	if (ctxt->modrm_reg == VCPU_SREG_CS || ctxt->modrm_reg > VCPU_SREG_GS)
		return emulate_ud(ctxt);

	if (ctxt->modrm_reg == VCPU_SREG_SS)
		ctxt->interruptibility = KVM_X86_SHADOW_INT_MOV_SS;

	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;
	return load_segment_descriptor(ctxt, sel, ctxt->modrm_reg);
}

static int em_sldt(struct x86_emulate_ctxt *ctxt)
{
	return em_store_sreg(ctxt, VCPU_SREG_LDTR);
}

static int em_lldt(struct x86_emulate_ctxt *ctxt)
{
	u16 sel = ctxt->src.val;

	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;
	return load_segment_descriptor(ctxt, sel, VCPU_SREG_LDTR);
}

static int em_str(struct x86_emulate_ctxt *ctxt)
{
	return em_store_sreg(ctxt, VCPU_SREG_TR);
}

static int em_ltr(struct x86_emulate_ctxt *ctxt)
{
	u16 sel = ctxt->src.val;

	/* Disable writeback. */
	ctxt->dst.type = OP_NONE;
	return load_segment_descriptor(ctxt, sel, VCPU_SREG_TR);
}

static int em_invlpg(struct x86_emulate_ctxt *ctxt)
{
	int rc;
	ulong linear;
	unsigned int max_size;