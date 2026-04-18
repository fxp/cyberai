// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 101/132



		if (dst_reg->id & BPF_ADD_CONST) {
			/*
			 * If the register already went through rX += val
			 * we cannot accumulate another val into rx->off.
			 */
clear_id:
			clear_scalar_id(dst_reg);
		} else {
			if (alu32)
				dst_reg->id |= BPF_ADD_CONST32;
			else
				dst_reg->id |= BPF_ADD_CONST64;
			dst_reg->delta = off;
		}
	} else {
		/*
		 * Make sure ID is cleared otherwise dst_reg min/max could be
		 * incorrectly propagated into other registers by sync_linked_regs()
		 */
		clear_scalar_id(dst_reg);
	}
	return 0;
}

/* check validity of 32-bit and 64-bit arithmetic operations */
static int check_alu_op(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	struct bpf_reg_state *regs = cur_regs(env);
	u8 opcode = BPF_OP(insn->code);
	int err;

	if (opcode == BPF_END || opcode == BPF_NEG) {
		/* check src operand */
		err = check_reg_arg(env, insn->dst_reg, SRC_OP);
		if (err)
			return err;

		if (is_pointer_value(env, insn->dst_reg)) {
			verbose(env, "R%d pointer arithmetic prohibited\n",
				insn->dst_reg);
			return -EACCES;
		}

		/* check dest operand */
		if (regs[insn->dst_reg].type == SCALAR_VALUE) {
			err = check_reg_arg(env, insn->dst_reg, DST_OP_NO_MARK);
			err = err ?: adjust_scalar_min_max_vals(env, insn,
							 &regs[insn->dst_reg],
							 regs[insn->dst_reg]);
		} else {
			err = check_reg_arg(env, insn->dst_reg, DST_OP);
		}
		if (err)
			return err;

	} else if (opcode == BPF_MOV) {

		if (BPF_SRC(insn->code) == BPF_X) {
			if (insn->off == BPF_ADDR_SPACE_CAST) {
				if (!env->prog->aux->arena) {
					verbose(env, "addr_space_cast insn can only be used in a program that has an associated arena\n");
					return -EINVAL;
				}
			}

			/* check src operand */
			err = check_reg_arg(env, insn->src_reg, SRC_OP);
			if (err)
				return err;
		}

		/* check dest operand, mark as required later */
		err = check_reg_arg(env, insn->dst_reg, DST_OP_NO_MARK);
		if (err)
			return err;

		if (BPF_SRC(insn->code) == BPF_X) {
			struct bpf_reg_state *src_reg = regs + insn->src_reg;
			struct bpf_reg_state *dst_reg = regs + insn->dst_reg;

			if (BPF_CLASS(insn->code) == BPF_ALU64) {
				if (insn->imm) {
					/* off == BPF_ADDR_SPACE_CAST */
					mark_reg_unknown(env, regs, insn->dst_reg);
					if (insn->imm == 1) { /* cast from as(1) to as(0) */
						dst_reg->type = PTR_TO_ARENA;
						/* PTR_TO_ARENA is 32-bit */
						dst_reg->subreg_def = env->insn_idx + 1;
					}
				} else if (insn->off == 0) {
					/* case: R1 = R2
					 * copy register state to dest reg
					 */
					assign_scalar_id_before_mov(env, src_reg);
					copy_register_state(dst_reg, src_reg);
					dst_reg->subreg_def = DEF_NOT_SUBREG;
				} else {
					/* case: R1 = (s8, s16 s32)R2 */
					if (is_pointer_value(env, insn->src_reg)) {
						verbose(env,
							"R%d sign-extension part of pointer\n",
							insn->src_reg);
						return -EACCES;
					} else if (src_reg->type == SCALAR_VALUE) {
						bool no_sext;

						no_sext = src_reg->umax_value < (1ULL << (insn->off - 1));
						if (no_sext)
							assign_scalar_id_before_mov(env, src_reg);
						copy_register_state(dst_reg, src_reg);
						if (!no_sext)
							clear_scalar_id(dst_reg);
						coerce_reg_to_size_sx(dst_reg, insn->off >> 3);
						dst_reg->subreg_def = DEF_NOT_SUBREG;
					} else {
						mark_reg_unknown(env, regs, insn->dst_reg);
					}
				}
			} else {
				/* R1 = (u32) R2 */
				if (is_pointer_value(env, insn->src_reg)) {
					verbose(env,
						"R%d partial copy of pointer\n",
						insn->src_reg);
					return -EACCES;
				} else if (src_reg->type == SCALAR_VALUE) {
					if (insn->off == 0) {
						bool is_src_reg_u32 = get_reg_width(src_reg) <= 32;

						if (is_src_reg_u32)
							assign_scalar_id_before_mov(env, src_reg);
						copy_register_state(dst_reg, src_reg);
						/* Make sure ID is cleared if src_reg is not in u32
						 * range otherwise dst_reg min/max could be incorrectly
						 * propagated into src_reg by sync_linked_regs()
						 */
						if (!is_src_reg_u32)
							clear_scalar_id(dst_reg);
						dst_reg->subreg_def = env->insn_idx + 1;
					} else {
						/* case: W1 = (s8, s16)W2 */
						bool no_sext = src_reg->umax_value < (1ULL << (insn->off - 1));