// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 104/132



static int flip_opcode(u32 opcode)
{
	/* How can we transform "a <op> b" into "b <op> a"? */
	static const u8 opcode_flip[16] = {
		/* these stay the same */
		[BPF_JEQ  >> 4] = BPF_JEQ,
		[BPF_JNE  >> 4] = BPF_JNE,
		[BPF_JSET >> 4] = BPF_JSET,
		/* these swap "lesser" and "greater" (L and G in the opcodes) */
		[BPF_JGE  >> 4] = BPF_JLE,
		[BPF_JGT  >> 4] = BPF_JLT,
		[BPF_JLE  >> 4] = BPF_JGE,
		[BPF_JLT  >> 4] = BPF_JGT,
		[BPF_JSGE >> 4] = BPF_JSLE,
		[BPF_JSGT >> 4] = BPF_JSLT,
		[BPF_JSLE >> 4] = BPF_JSGE,
		[BPF_JSLT >> 4] = BPF_JSGT
	};
	return opcode_flip[opcode >> 4];
}

static int is_pkt_ptr_branch_taken(struct bpf_reg_state *dst_reg,
				   struct bpf_reg_state *src_reg,
				   u8 opcode)
{
	struct bpf_reg_state *pkt;

	if (src_reg->type == PTR_TO_PACKET_END) {
		pkt = dst_reg;
	} else if (dst_reg->type == PTR_TO_PACKET_END) {
		pkt = src_reg;
		opcode = flip_opcode(opcode);
	} else {
		return -1;
	}

	if (pkt->range >= 0)
		return -1;

	switch (opcode) {
	case BPF_JLE:
		/* pkt <= pkt_end */
		fallthrough;
	case BPF_JGT:
		/* pkt > pkt_end */
		if (pkt->range == BEYOND_PKT_END)
			/* pkt has at last one extra byte beyond pkt_end */
			return opcode == BPF_JGT;
		break;
	case BPF_JLT:
		/* pkt < pkt_end */
		fallthrough;
	case BPF_JGE:
		/* pkt >= pkt_end */
		if (pkt->range == BEYOND_PKT_END || pkt->range == AT_PKT_END)
			return opcode == BPF_JGE;
		break;
	}
	return -1;
}

/* compute branch direction of the expression "if (<reg1> opcode <reg2>) goto target;"
 * and return:
 *  1 - branch will be taken and "goto target" will be executed
 *  0 - branch will not be taken and fall-through to next insn
 * -1 - unknown. Example: "if (reg1 < 5)" is unknown when register value
 *      range [0,10]
 */
static int is_branch_taken(struct bpf_verifier_env *env, struct bpf_reg_state *reg1,
			   struct bpf_reg_state *reg2, u8 opcode, bool is_jmp32)
{
	if (reg_is_pkt_pointer_any(reg1) && reg_is_pkt_pointer_any(reg2) && !is_jmp32)
		return is_pkt_ptr_branch_taken(reg1, reg2, opcode);

	if (__is_pointer_value(false, reg1) || __is_pointer_value(false, reg2)) {
		u64 val;

		/* arrange that reg2 is a scalar, and reg1 is a pointer */
		if (!is_reg_const(reg2, is_jmp32)) {
			opcode = flip_opcode(opcode);
			swap(reg1, reg2);
		}
		/* and ensure that reg2 is a constant */
		if (!is_reg_const(reg2, is_jmp32))
			return -1;

		if (!reg_not_null(reg1))
			return -1;

		/* If pointer is valid tests against zero will fail so we can
		 * use this to direct branch taken.
		 */
		val = reg_const_value(reg2, is_jmp32);
		if (val != 0)
			return -1;

		switch (opcode) {
		case BPF_JEQ:
			return 0;
		case BPF_JNE:
			return 1;
		default:
			return -1;
		}
	}

	/* now deal with two scalars, but not necessarily constants */
	return is_scalar_branch_taken(env, reg1, reg2, opcode, is_jmp32);
}

/* Opcode that corresponds to a *false* branch condition.
 * E.g., if r1 < r2, then reverse (false) condition is r1 >= r2
 */
static u8 rev_opcode(u8 opcode)
{
	switch (opcode) {
	case BPF_JEQ:		return BPF_JNE;
	case BPF_JNE:		return BPF_JEQ;
	/* JSET doesn't have it's reverse opcode in BPF, so add
	 * BPF_X flag to denote the reverse of that operation
	 */
	case BPF_JSET:		return BPF_JSET | BPF_X;
	case BPF_JSET | BPF_X:	return BPF_JSET;
	case BPF_JGE:		return BPF_JLT;
	case BPF_JGT:		return BPF_JLE;
	case BPF_JLE:		return BPF_JGT;
	case BPF_JLT:		return BPF_JGE;
	case BPF_JSGE:		return BPF_JSLT;
	case BPF_JSGT:		return BPF_JSLE;
	case BPF_JSLE:		return BPF_JSGT;
	case BPF_JSLT:		return BPF_JSGE;
	default:		return 0;
	}
}

/* Refine range knowledge for <reg1> <op> <reg>2 conditional operation. */
static void regs_refine_cond_op(struct bpf_reg_state *reg1, struct bpf_reg_state *reg2,
				u8 opcode, bool is_jmp32)
{
	struct tnum t;
	u64 val;

	/* In case of GE/GT/SGE/JST, reuse LE/LT/SLE/SLT logic from below */
	switch (opcode) {
	case BPF_JGE:
	case BPF_JGT:
	case BPF_JSGE:
	case BPF_JSGT:
		opcode = flip_opcode(opcode);
		swap(reg1, reg2);
		break;
	default:
		break;
	}

	switch (opcode) {
	case BPF_JEQ:
		if (is_jmp32) {
			reg1->u32_min_value = max(reg1->u32_min_value, reg2->u32_min_value);
			reg1->u32_max_value = min(reg1->u32_max_value, reg2->u32_max_value);
			reg1->s32_min_value = max(reg1->s32_min_value, reg2->s32_min_value);
			reg1->s32_max_value = min(reg1->s32_max_value, reg2->s32_max_value);
			reg2->u32_min_value = reg1->u32_min_value;
			reg2->u32_max_value = reg1->u32_max_value;
			reg2->s32_min_value = reg1->s32_min_value;
			reg2->s32_max_value = reg1->s32_max_value;