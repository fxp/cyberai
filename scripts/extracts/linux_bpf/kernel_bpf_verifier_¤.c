// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 100/132



	/*
	 * ALU32 ops are zero extended into 64bit register.
	 *
	 * BPF_END is already handled inside the helper (truncation),
	 * so skip zext here to avoid unexpected zero extension.
	 * e.g., le64: opcode=(BPF_END|BPF_ALU|BPF_TO_LE), imm=0x40
	 * This is a 64bit byte swap operation with alu32==true,
	 * but we should not zero extend the result.
	 */
	if (alu32 && opcode != BPF_END)
		zext_32_to_64(dst_reg);
	reg_bounds_sync(dst_reg);
	return 0;
}

/* Handles ALU ops other than BPF_END, BPF_NEG and BPF_MOV: computes new min/max
 * and var_off.
 */
static int adjust_reg_min_max_vals(struct bpf_verifier_env *env,
				   struct bpf_insn *insn)
{
	struct bpf_verifier_state *vstate = env->cur_state;
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	struct bpf_reg_state *regs = state->regs, *dst_reg, *src_reg;
	struct bpf_reg_state *ptr_reg = NULL, off_reg = {0};
	bool alu32 = (BPF_CLASS(insn->code) != BPF_ALU64);
	u8 opcode = BPF_OP(insn->code);
	int err;

	dst_reg = &regs[insn->dst_reg];
	if (BPF_SRC(insn->code) == BPF_X)
		src_reg = &regs[insn->src_reg];
	else
		src_reg = NULL;

	/* Case where at least one operand is an arena. */
	if (dst_reg->type == PTR_TO_ARENA || (src_reg && src_reg->type == PTR_TO_ARENA)) {
		struct bpf_insn_aux_data *aux = cur_aux(env);

		if (dst_reg->type != PTR_TO_ARENA)
			*dst_reg = *src_reg;

		dst_reg->subreg_def = env->insn_idx + 1;

		if (BPF_CLASS(insn->code) == BPF_ALU64)
			/*
			 * 32-bit operations zero upper bits automatically.
			 * 64-bit operations need to be converted to 32.
			 */
			aux->needs_zext = true;

		/* Any arithmetic operations are allowed on arena pointers */
		return 0;
	}

	if (dst_reg->type != SCALAR_VALUE)
		ptr_reg = dst_reg;

	if (BPF_SRC(insn->code) == BPF_X) {
		if (src_reg->type != SCALAR_VALUE) {
			if (dst_reg->type != SCALAR_VALUE) {
				/* Combining two pointers by any ALU op yields
				 * an arbitrary scalar. Disallow all math except
				 * pointer subtraction
				 */
				if (opcode == BPF_SUB && env->allow_ptr_leaks) {
					mark_reg_unknown(env, regs, insn->dst_reg);
					return 0;
				}
				verbose(env, "R%d pointer %s pointer prohibited\n",
					insn->dst_reg,
					bpf_alu_string[opcode >> 4]);
				return -EACCES;
			} else {
				/* scalar += pointer
				 * This is legal, but we have to reverse our
				 * src/dest handling in computing the range
				 */
				err = mark_chain_precision(env, insn->dst_reg);
				if (err)
					return err;
				return adjust_ptr_min_max_vals(env, insn,
							       src_reg, dst_reg);
			}
		} else if (ptr_reg) {
			/* pointer += scalar */
			err = mark_chain_precision(env, insn->src_reg);
			if (err)
				return err;
			return adjust_ptr_min_max_vals(env, insn,
						       dst_reg, src_reg);
		} else if (dst_reg->precise) {
			/* if dst_reg is precise, src_reg should be precise as well */
			err = mark_chain_precision(env, insn->src_reg);
			if (err)
				return err;
		}
	} else {
		/* Pretend the src is a reg with a known value, since we only
		 * need to be able to read from this state.
		 */
		off_reg.type = SCALAR_VALUE;
		__mark_reg_known(&off_reg, insn->imm);
		src_reg = &off_reg;
		if (ptr_reg) /* pointer += K */
			return adjust_ptr_min_max_vals(env, insn,
						       ptr_reg, src_reg);
	}

	/* Got here implies adding two SCALAR_VALUEs */
	if (WARN_ON_ONCE(ptr_reg)) {
		print_verifier_state(env, vstate, vstate->curframe, true);
		verbose(env, "verifier internal error: unexpected ptr_reg\n");
		return -EFAULT;
	}
	if (WARN_ON(!src_reg)) {
		print_verifier_state(env, vstate, vstate->curframe, true);
		verbose(env, "verifier internal error: no src_reg\n");
		return -EFAULT;
	}
	/*
	 * For alu32 linked register tracking, we need to check dst_reg's
	 * umax_value before the ALU operation. After adjust_scalar_min_max_vals(),
	 * alu32 ops will have zero-extended the result, making umax_value <= U32_MAX.
	 */
	u64 dst_umax = dst_reg->umax_value;

	err = adjust_scalar_min_max_vals(env, insn, dst_reg, *src_reg);
	if (err)
		return err;
	/*
	 * Compilers can generate the code
	 * r1 = r2
	 * r1 += 0x1
	 * if r2 < 1000 goto ...
	 * use r1 in memory access
	 * So remember constant delta between r2 and r1 and update r1 after
	 * 'if' condition.
	 */
	if (env->bpf_capable &&
	    (BPF_OP(insn->code) == BPF_ADD || BPF_OP(insn->code) == BPF_SUB) &&
	    dst_reg->id && is_reg_const(src_reg, alu32) &&
	    !(BPF_SRC(insn->code) == BPF_X && insn->src_reg == insn->dst_reg)) {
		u64 val = reg_const_value(src_reg, alu32);
		s32 off;

		if (!alu32 && ((s64)val < S32_MIN || (s64)val > S32_MAX))
			goto clear_id;

		if (alu32 && (dst_umax > U32_MAX))
			goto clear_id;

		off = (s32)val;

		if (BPF_OP(insn->code) == BPF_SUB) {
			/* Negating S32_MIN would overflow */
			if (off == S32_MIN)
				goto clear_id;
			off = -off;
		}