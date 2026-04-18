// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 99/132



/* WARNING: This function does calculations on 64-bit values, but the actual
 * execution may occur on 32-bit values. Therefore, things like bitshifts
 * need extra checks in the 32-bit case.
 */
static int adjust_scalar_min_max_vals(struct bpf_verifier_env *env,
				      struct bpf_insn *insn,
				      struct bpf_reg_state *dst_reg,
				      struct bpf_reg_state src_reg)
{
	u8 opcode = BPF_OP(insn->code);
	s16 off = insn->off;
	bool alu32 = (BPF_CLASS(insn->code) != BPF_ALU64);
	int ret;

	if (!is_safe_to_compute_dst_reg_range(insn, &src_reg)) {
		__mark_reg_unknown(env, dst_reg);
		return 0;
	}

	if (sanitize_needed(opcode)) {
		ret = sanitize_val_alu(env, insn);
		if (ret < 0)
			return sanitize_err(env, insn, ret, NULL, NULL);
	}

	/* Calculate sign/unsigned bounds and tnum for alu32 and alu64 bit ops.
	 * There are two classes of instructions: The first class we track both
	 * alu32 and alu64 sign/unsigned bounds independently this provides the
	 * greatest amount of precision when alu operations are mixed with jmp32
	 * operations. These operations are BPF_ADD, BPF_SUB, BPF_MUL, BPF_ADD,
	 * and BPF_OR. This is possible because these ops have fairly easy to
	 * understand and calculate behavior in both 32-bit and 64-bit alu ops.
	 * See alu32 verifier tests for examples. The second class of
	 * operations, BPF_LSH, BPF_RSH, and BPF_ARSH, however are not so easy
	 * with regards to tracking sign/unsigned bounds because the bits may
	 * cross subreg boundaries in the alu64 case. When this happens we mark
	 * the reg unbounded in the subreg bound space and use the resulting
	 * tnum to calculate an approximation of the sign/unsigned bounds.
	 */
	switch (opcode) {
	case BPF_ADD:
		scalar32_min_max_add(dst_reg, &src_reg);
		scalar_min_max_add(dst_reg, &src_reg);
		dst_reg->var_off = tnum_add(dst_reg->var_off, src_reg.var_off);
		break;
	case BPF_SUB:
		scalar32_min_max_sub(dst_reg, &src_reg);
		scalar_min_max_sub(dst_reg, &src_reg);
		dst_reg->var_off = tnum_sub(dst_reg->var_off, src_reg.var_off);
		break;
	case BPF_NEG:
		env->fake_reg[0] = *dst_reg;
		__mark_reg_known(dst_reg, 0);
		scalar32_min_max_sub(dst_reg, &env->fake_reg[0]);
		scalar_min_max_sub(dst_reg, &env->fake_reg[0]);
		dst_reg->var_off = tnum_neg(env->fake_reg[0].var_off);
		break;
	case BPF_MUL:
		dst_reg->var_off = tnum_mul(dst_reg->var_off, src_reg.var_off);
		scalar32_min_max_mul(dst_reg, &src_reg);
		scalar_min_max_mul(dst_reg, &src_reg);
		break;
	case BPF_DIV:
		/* BPF div specification: x / 0 = 0 */
		if ((alu32 && src_reg.u32_min_value == 0) || (!alu32 && src_reg.umin_value == 0)) {
			___mark_reg_known(dst_reg, 0);
			break;
		}
		if (alu32)
			if (off == 1)
				scalar32_min_max_sdiv(dst_reg, &src_reg);
			else
				scalar32_min_max_udiv(dst_reg, &src_reg);
		else
			if (off == 1)
				scalar_min_max_sdiv(dst_reg, &src_reg);
			else
				scalar_min_max_udiv(dst_reg, &src_reg);
		break;
	case BPF_MOD:
		/* BPF mod specification: x % 0 = x */
		if ((alu32 && src_reg.u32_min_value == 0) || (!alu32 && src_reg.umin_value == 0))
			break;
		if (alu32)
			if (off == 1)
				scalar32_min_max_smod(dst_reg, &src_reg);
			else
				scalar32_min_max_umod(dst_reg, &src_reg);
		else
			if (off == 1)
				scalar_min_max_smod(dst_reg, &src_reg);
			else
				scalar_min_max_umod(dst_reg, &src_reg);
		break;
	case BPF_AND:
		if (tnum_is_const(src_reg.var_off)) {
			ret = maybe_fork_scalars(env, insn, dst_reg);
			if (ret)
				return ret;
		}
		dst_reg->var_off = tnum_and(dst_reg->var_off, src_reg.var_off);
		scalar32_min_max_and(dst_reg, &src_reg);
		scalar_min_max_and(dst_reg, &src_reg);
		break;
	case BPF_OR:
		if (tnum_is_const(src_reg.var_off)) {
			ret = maybe_fork_scalars(env, insn, dst_reg);
			if (ret)
				return ret;
		}
		dst_reg->var_off = tnum_or(dst_reg->var_off, src_reg.var_off);
		scalar32_min_max_or(dst_reg, &src_reg);
		scalar_min_max_or(dst_reg, &src_reg);
		break;
	case BPF_XOR:
		dst_reg->var_off = tnum_xor(dst_reg->var_off, src_reg.var_off);
		scalar32_min_max_xor(dst_reg, &src_reg);
		scalar_min_max_xor(dst_reg, &src_reg);
		break;
	case BPF_LSH:
		if (alu32)
			scalar32_min_max_lsh(dst_reg, &src_reg);
		else
			scalar_min_max_lsh(dst_reg, &src_reg);
		break;
	case BPF_RSH:
		if (alu32)
			scalar32_min_max_rsh(dst_reg, &src_reg);
		else
			scalar_min_max_rsh(dst_reg, &src_reg);
		break;
	case BPF_ARSH:
		if (alu32)
			scalar32_min_max_arsh(dst_reg, &src_reg);
		else
			scalar_min_max_arsh(dst_reg, &src_reg);
		break;
	case BPF_END:
		scalar_byte_swap(dst_reg, insn);
		break;
	default:
		break;
	}