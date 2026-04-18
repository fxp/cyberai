// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 98/132



	/* blow away the dst_reg umin_value/umax_value and rely on
	 * dst_reg var_off to refine the result.
	 */
	dst_reg->u32_min_value = 0;
	dst_reg->u32_max_value = U32_MAX;

	__mark_reg64_unbounded(dst_reg);
	__update_reg32_bounds(dst_reg);
}

static void scalar_min_max_arsh(struct bpf_reg_state *dst_reg,
				struct bpf_reg_state *src_reg)
{
	u64 umin_val = src_reg->umin_value;

	/* Upon reaching here, src_known is true and umax_val is equal
	 * to umin_val.
	 */
	dst_reg->smin_value >>= umin_val;
	dst_reg->smax_value >>= umin_val;

	dst_reg->var_off = tnum_arshift(dst_reg->var_off, umin_val, 64);

	/* blow away the dst_reg umin_value/umax_value and rely on
	 * dst_reg var_off to refine the result.
	 */
	dst_reg->umin_value = 0;
	dst_reg->umax_value = U64_MAX;

	/* Its not easy to operate on alu32 bounds here because it depends
	 * on bits being shifted in from upper 32-bits. Take easy way out
	 * and mark unbounded so we can recalculate later from tnum.
	 */
	__mark_reg32_unbounded(dst_reg);
	__update_reg_bounds(dst_reg);
}

static void scalar_byte_swap(struct bpf_reg_state *dst_reg, struct bpf_insn *insn)
{
	/*
	 * Byte swap operation - update var_off using tnum_bswap.
	 * Three cases:
	 * 1. bswap(16|32|64): opcode=0xd7 (BPF_END | BPF_ALU64 | BPF_TO_LE)
	 *    unconditional swap
	 * 2. to_le(16|32|64): opcode=0xd4 (BPF_END | BPF_ALU | BPF_TO_LE)
	 *    swap on big-endian, truncation or no-op on little-endian
	 * 3. to_be(16|32|64): opcode=0xdc (BPF_END | BPF_ALU | BPF_TO_BE)
	 *    swap on little-endian, truncation or no-op on big-endian
	 */

	bool alu64 = BPF_CLASS(insn->code) == BPF_ALU64;
	bool to_le = BPF_SRC(insn->code) == BPF_TO_LE;
	bool is_big_endian;
#ifdef CONFIG_CPU_BIG_ENDIAN
	is_big_endian = true;
#else
	is_big_endian = false;
#endif
	/* Apply bswap if alu64 or switch between big-endian and little-endian machines */
	bool need_bswap = alu64 || (to_le == is_big_endian);

	/*
	 * If the register is mutated, manually reset its scalar ID to break
	 * any existing ties and avoid incorrect bounds propagation.
	 */
	if (need_bswap || insn->imm == 16 || insn->imm == 32)
		clear_scalar_id(dst_reg);

	if (need_bswap) {
		if (insn->imm == 16)
			dst_reg->var_off = tnum_bswap16(dst_reg->var_off);
		else if (insn->imm == 32)
			dst_reg->var_off = tnum_bswap32(dst_reg->var_off);
		else if (insn->imm == 64)
			dst_reg->var_off = tnum_bswap64(dst_reg->var_off);
		/*
		 * Byteswap scrambles the range, so we must reset bounds.
		 * Bounds will be re-derived from the new tnum later.
		 */
		__mark_reg_unbounded(dst_reg);
	}
	/* For bswap16/32, truncate dst register to match the swapped size */
	if (insn->imm == 16 || insn->imm == 32)
		coerce_reg_to_size(dst_reg, insn->imm / 8);
}

static bool is_safe_to_compute_dst_reg_range(struct bpf_insn *insn,
					     const struct bpf_reg_state *src_reg)
{
	bool src_is_const = false;
	u64 insn_bitness = (BPF_CLASS(insn->code) == BPF_ALU64) ? 64 : 32;

	if (insn_bitness == 32) {
		if (tnum_subreg_is_const(src_reg->var_off)
		    && src_reg->s32_min_value == src_reg->s32_max_value
		    && src_reg->u32_min_value == src_reg->u32_max_value)
			src_is_const = true;
	} else {
		if (tnum_is_const(src_reg->var_off)
		    && src_reg->smin_value == src_reg->smax_value
		    && src_reg->umin_value == src_reg->umax_value)
			src_is_const = true;
	}

	switch (BPF_OP(insn->code)) {
	case BPF_ADD:
	case BPF_SUB:
	case BPF_NEG:
	case BPF_AND:
	case BPF_XOR:
	case BPF_OR:
	case BPF_MUL:
	case BPF_END:
		return true;

	/*
	 * Division and modulo operators range is only safe to compute when the
	 * divisor is a constant.
	 */
	case BPF_DIV:
	case BPF_MOD:
		return src_is_const;

	/* Shift operators range is only computable if shift dimension operand
	 * is a constant. Shifts greater than 31 or 63 are undefined. This
	 * includes shifts by a negative number.
	 */
	case BPF_LSH:
	case BPF_RSH:
	case BPF_ARSH:
		return (src_is_const && src_reg->umax_value < insn_bitness);
	default:
		return false;
	}
}

static int maybe_fork_scalars(struct bpf_verifier_env *env, struct bpf_insn *insn,
			      struct bpf_reg_state *dst_reg)
{
	struct bpf_verifier_state *branch;
	struct bpf_reg_state *regs;
	bool alu32;

	if (dst_reg->smin_value == -1 && dst_reg->smax_value == 0)
		alu32 = false;
	else if (dst_reg->s32_min_value == -1 && dst_reg->s32_max_value == 0)
		alu32 = true;
	else
		return 0;

	branch = push_stack(env, env->insn_idx, env->insn_idx, false);
	if (IS_ERR(branch))
		return PTR_ERR(branch);

	regs = branch->frame[branch->curframe]->regs;
	if (alu32) {
		__mark_reg32_known(&regs[insn->dst_reg], 0);
		__mark_reg32_known(dst_reg, -1ull);
	} else {
		__mark_reg_known(&regs[insn->dst_reg], 0);
		__mark_reg_known(dst_reg, -1ull);
	}
	return 0;
}