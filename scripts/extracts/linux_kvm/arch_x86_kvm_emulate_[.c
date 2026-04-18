// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 27/33



/*
 * Insns below are selected by the prefix which indexed by the third opcode
 * byte.
 */
static const struct opcode opcode_map_0f_38[256] = {
	/* 0x00 - 0x1f */
	X16(N), X16(N),
	/* 0x20 - 0x2f */
	X8(N),
	X2(N), GP(SrcReg | DstMem | ModRM | Mov | Aligned, &pfx_0f_e7_0f_38_2a), N, N, N, N, N,
	/* 0x30 - 0x7f */
	X16(N), X16(N), X16(N), X16(N), X16(N),
	/* 0x80 - 0xef */
	X16(N), X16(N), X16(N), X16(N), X16(N), X16(N), X16(N),
	/* 0xf0 - 0xf1 */
	GP(EmulateOnUD | ModRM, &three_byte_0f_38_f0),
	GP(EmulateOnUD | ModRM, &three_byte_0f_38_f1),
	/* 0xf2 - 0xff */
	N, N, X4(N), X8(N)
};

#undef D
#undef N
#undef G
#undef GD
#undef I
#undef GP
#undef EXT
#undef MD
#undef ID

#undef D2bv
#undef D2bvIP
#undef I2bv
#undef I2bvIP
#undef I6ALU

static bool is_shstk_instruction(struct x86_emulate_ctxt *ctxt)
{
	return ctxt->d & ShadowStack;
}

static bool is_ibt_instruction(struct x86_emulate_ctxt *ctxt)
{
	u64 flags = ctxt->d;

	if (!(flags & IsBranch))
		return false;

	/*
	 * All far JMPs and CALLs (including SYSCALL, SYSENTER, and INTn) are
	 * indirect and thus affect IBT state.  All far RETs (including SYSEXIT
	 * and IRET) are protected via Shadow Stacks and thus don't affect IBT
	 * state.  IRET #GPs when returning to virtual-8086 and IBT or SHSTK is
	 * enabled, but that should be handled by IRET emulation (in the very
	 * unlikely scenario that KVM adds support for fully emulating IRET).
	 */
	if (!(flags & NearBranch))
		return ctxt->execute != em_iret &&
		       ctxt->execute != em_ret_far &&
		       ctxt->execute != em_ret_far_imm &&
		       ctxt->execute != em_sysexit;

	switch (flags & SrcMask) {
	case SrcReg:
	case SrcMem:
	case SrcMem16:
	case SrcMem32:
		return true;
	case SrcMemFAddr:
	case SrcImmFAddr:
		/* Far branches should be handled above. */
		WARN_ON_ONCE(1);
		return true;
	case SrcNone:
	case SrcImm:
	case SrcImmByte:
	/*
	 * Note, ImmU16 is used only for the stack adjustment operand on ENTER
	 * and RET instructions.  ENTER isn't a branch and RET FAR is handled
	 * by the NearBranch check above.  RET itself isn't an indirect branch.
	 */
	case SrcImmU16:
		return false;
	default:
		WARN_ONCE(1, "Unexpected Src operand '%llx' on branch",
			  flags & SrcMask);
		return false;
	}
}

static unsigned imm_size(struct x86_emulate_ctxt *ctxt)
{
	unsigned size;

	size = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
	if (size == 8)
		size = 4;
	return size;
}

static int decode_imm(struct x86_emulate_ctxt *ctxt, struct operand *op,
		      unsigned size, bool sign_extension)
{
	int rc = X86EMUL_CONTINUE;

	op->type = OP_IMM;
	op->bytes = size;
	op->addr.mem.ea = ctxt->_eip;
	/* NB. Immediates are sign-extended as necessary. */
	switch (op->bytes) {
	case 1:
		op->val = insn_fetch(s8, ctxt);
		break;
	case 2:
		op->val = insn_fetch(s16, ctxt);
		break;
	case 4:
		op->val = insn_fetch(s32, ctxt);
		break;
	case 8:
		op->val = insn_fetch(s64, ctxt);
		break;
	}
	if (!sign_extension) {
		switch (op->bytes) {
		case 1:
			op->val &= 0xff;
			break;
		case 2:
			op->val &= 0xffff;
			break;
		case 4:
			op->val &= 0xffffffff;
			break;
		}
	}
done:
	return rc;
}

static int decode_operand(struct x86_emulate_ctxt *ctxt, struct operand *op,
			  unsigned d)
{
	int rc = X86EMUL_CONTINUE;