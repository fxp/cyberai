// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 7/33



static int read_descriptor(struct x86_emulate_ctxt *ctxt,
			   struct segmented_address addr,
			   u16 *size, unsigned long *address, int op_bytes)
{
	int rc;

	if (op_bytes == 2)
		op_bytes = 3;
	*address = 0;
	rc = segmented_read_std(ctxt, addr, size, 2);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	addr.ea += 2;
	rc = segmented_read_std(ctxt, addr, address, op_bytes);
	return rc;
}

EM_ASM_2(add);
EM_ASM_2(or);
EM_ASM_2(adc);
EM_ASM_2(sbb);
EM_ASM_2(and);
EM_ASM_2(sub);
EM_ASM_2(xor);
EM_ASM_2(cmp);
EM_ASM_2(test);
EM_ASM_2(xadd);

EM_ASM_1SRC2(mul, mul_ex);
EM_ASM_1SRC2(imul, imul_ex);
EM_ASM_1SRC2EX(div, div_ex);
EM_ASM_1SRC2EX(idiv, idiv_ex);

EM_ASM_3WCL(shld);
EM_ASM_3WCL(shrd);

EM_ASM_2W(imul);

EM_ASM_1(not);
EM_ASM_1(neg);
EM_ASM_1(inc);
EM_ASM_1(dec);

EM_ASM_2CL(rol);
EM_ASM_2CL(ror);
EM_ASM_2CL(rcl);
EM_ASM_2CL(rcr);
EM_ASM_2CL(shl);
EM_ASM_2CL(shr);
EM_ASM_2CL(sar);

EM_ASM_2W(bsf);
EM_ASM_2W(bsr);
EM_ASM_2W(bt);
EM_ASM_2W(bts);
EM_ASM_2W(btr);
EM_ASM_2W(btc);

EM_ASM_2R(cmp, cmp_r);

static int em_bsf_c(struct x86_emulate_ctxt *ctxt)
{
	/* If src is zero, do not writeback, but update flags */
	if (ctxt->src.val == 0)
		ctxt->dst.type = OP_NONE;
	return em_bsf(ctxt);
}

static int em_bsr_c(struct x86_emulate_ctxt *ctxt)
{
	/* If src is zero, do not writeback, but update flags */
	if (ctxt->src.val == 0)
		ctxt->dst.type = OP_NONE;
	return em_bsr(ctxt);
}

static __always_inline u8 test_cc(unsigned int condition, unsigned long flags)
{
	return __emulate_cc(flags, condition & 0xf);
}

static void fetch_register_operand(struct operand *op)
{
	switch (op->bytes) {
	case 1:
		op->val = *(u8 *)op->addr.reg;
		break;
	case 2:
		op->val = *(u16 *)op->addr.reg;
		break;
	case 4:
		op->val = *(u32 *)op->addr.reg;
		break;
	case 8:
		op->val = *(u64 *)op->addr.reg;
		break;
	}
	op->orig_val = op->val;
}

static int em_fninit(struct x86_emulate_ctxt *ctxt)
{
	if (ctxt->ops->get_cr(ctxt, 0) & (X86_CR0_TS | X86_CR0_EM))
		return emulate_nm(ctxt);

	kvm_fpu_get();
	asm volatile("fninit");
	kvm_fpu_put();
	return X86EMUL_CONTINUE;
}

static int em_fnstcw(struct x86_emulate_ctxt *ctxt)
{
	u16 fcw;

	if (ctxt->ops->get_cr(ctxt, 0) & (X86_CR0_TS | X86_CR0_EM))
		return emulate_nm(ctxt);

	kvm_fpu_get();
	asm volatile("fnstcw %0": "+m"(fcw));
	kvm_fpu_put();

	ctxt->dst.val = fcw;

	return X86EMUL_CONTINUE;
}

static int em_fnstsw(struct x86_emulate_ctxt *ctxt)
{
	u16 fsw;

	if (ctxt->ops->get_cr(ctxt, 0) & (X86_CR0_TS | X86_CR0_EM))
		return emulate_nm(ctxt);

	kvm_fpu_get();
	asm volatile("fnstsw %0": "+m"(fsw));
	kvm_fpu_put();

	ctxt->dst.val = fsw;

	return X86EMUL_CONTINUE;
}

static void __decode_register_operand(struct x86_emulate_ctxt *ctxt,
				      struct operand *op, int reg)
{
	if ((ctxt->d & Avx) && ctxt->op_bytes == 32) {
		op->type = OP_YMM;
		op->bytes = 32;
		op->addr.xmm = reg;
		kvm_read_avx_reg(reg, &op->vec_val2);
		return;
	}
	if (ctxt->d & (Avx|Sse)) {
		op->type = OP_XMM;
		op->bytes = 16;
		op->addr.xmm = reg;
		kvm_read_sse_reg(reg, &op->vec_val);
		return;
	}
	if (ctxt->d & Mmx) {
		reg &= 7;
		op->type = OP_MM;
		op->bytes = 8;
		op->addr.mm = reg;
		return;
	}

	op->type = OP_REG;
	op->bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
	op->addr.reg = decode_register(ctxt, reg, ctxt->d & ByteOp);
	fetch_register_operand(op);
}

static void decode_register_operand(struct x86_emulate_ctxt *ctxt,
				    struct operand *op)
{
	unsigned int reg;

	if (ctxt->d & ModRM)
		reg = ctxt->modrm_reg;
	else
		reg = (ctxt->b & 7) | (ctxt->rex_bits & REX_B ? 8 : 0);

	__decode_register_operand(ctxt, op, reg);
}

static void adjust_modrm_seg(struct x86_emulate_ctxt *ctxt, int base_reg)
{
	if (base_reg == VCPU_REGS_RSP || base_reg == VCPU_REGS_RBP)
		ctxt->modrm_seg = VCPU_SREG_SS;
}

static int decode_modrm(struct x86_emulate_ctxt *ctxt,
			struct operand *op)
{
	u8 sib;
	int index_reg, base_reg, scale;
	int rc = X86EMUL_CONTINUE;
	ulong modrm_ea = 0;

	ctxt->modrm_reg = (ctxt->rex_bits & REX_R ? 8 : 0);
	index_reg = (ctxt->rex_bits & REX_X ? 8 : 0);
	base_reg = (ctxt->rex_bits & REX_B ? 8 : 0);

	ctxt->modrm_mod = (ctxt->modrm & 0xc0) >> 6;
	ctxt->modrm_reg |= (ctxt->modrm & 0x38) >> 3;
	ctxt->modrm_rm = base_reg | (ctxt->modrm & 0x07);
	ctxt->modrm_seg = VCPU_SREG_DS;

	if (ctxt->modrm_mod == 3 || (ctxt->d & NoMod)) {
		__decode_register_operand(ctxt, op, ctxt->modrm_rm);
		return rc;
	}

	op->type = OP_MEM;

	if (ctxt->ad_bytes == 2) {
		unsigned bx = reg_read(ctxt, VCPU_REGS_RBX);
		unsigned bp = reg_read(ctxt, VCPU_REGS_RBP);
		unsigned si = reg_read(ctxt, VCPU_REGS_RSI);
		unsigned di = reg_read(ctxt, VCPU_REGS_RDI);