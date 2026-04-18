// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 108/132



/* For all R in linked_regs, copy known_reg range into R
 * if R->id == known_reg->id.
 */
static void sync_linked_regs(struct bpf_verifier_env *env, struct bpf_verifier_state *vstate,
			     struct bpf_reg_state *known_reg, struct linked_regs *linked_regs)
{
	struct bpf_reg_state fake_reg;
	struct bpf_reg_state *reg;
	struct linked_reg *e;
	int i;

	for (i = 0; i < linked_regs->cnt; ++i) {
		e = &linked_regs->entries[i];
		reg = e->is_reg ? &vstate->frame[e->frameno]->regs[e->regno]
				: &vstate->frame[e->frameno]->stack[e->spi].spilled_ptr;
		if (reg->type != SCALAR_VALUE || reg == known_reg)
			continue;
		if ((reg->id & ~BPF_ADD_CONST) != (known_reg->id & ~BPF_ADD_CONST))
			continue;
		/*
		 * Skip mixed 32/64-bit links: the delta relationship doesn't
		 * hold across different ALU widths.
		 */
		if (((reg->id ^ known_reg->id) & BPF_ADD_CONST) == BPF_ADD_CONST)
			continue;
		if ((!(reg->id & BPF_ADD_CONST) && !(known_reg->id & BPF_ADD_CONST)) ||
		    reg->delta == known_reg->delta) {
			s32 saved_subreg_def = reg->subreg_def;

			copy_register_state(reg, known_reg);
			reg->subreg_def = saved_subreg_def;
		} else {
			s32 saved_subreg_def = reg->subreg_def;
			s32 saved_off = reg->delta;
			u32 saved_id = reg->id;

			fake_reg.type = SCALAR_VALUE;
			__mark_reg_known(&fake_reg, (s64)reg->delta - (s64)known_reg->delta);

			/* reg = known_reg; reg += delta */
			copy_register_state(reg, known_reg);
			/*
			 * Must preserve off, id and subreg_def flag,
			 * otherwise another sync_linked_regs() will be incorrect.
			 */
			reg->delta = saved_off;
			reg->id = saved_id;
			reg->subreg_def = saved_subreg_def;

			scalar32_min_max_add(reg, &fake_reg);
			scalar_min_max_add(reg, &fake_reg);
			reg->var_off = tnum_add(reg->var_off, fake_reg.var_off);
			if ((reg->id | known_reg->id) & BPF_ADD_CONST32)
				zext_32_to_64(reg);
			reg_bounds_sync(reg);
		}
		if (e->is_reg)
			mark_reg_scratched(env, e->regno);
		else
			mark_stack_slot_scratched(env, e->spi);
	}
}

static int check_cond_jmp_op(struct bpf_verifier_env *env,
			     struct bpf_insn *insn, int *insn_idx)
{
	struct bpf_verifier_state *this_branch = env->cur_state;
	struct bpf_verifier_state *other_branch;
	struct bpf_reg_state *regs = this_branch->frame[this_branch->curframe]->regs;
	struct bpf_reg_state *dst_reg, *other_branch_regs, *src_reg = NULL;
	struct bpf_reg_state *eq_branch_regs;
	struct linked_regs linked_regs = {};
	u8 opcode = BPF_OP(insn->code);
	int insn_flags = 0;
	bool is_jmp32;
	int pred = -1;
	int err;

	/* Only conditional jumps are expected to reach here. */
	if (opcode == BPF_JA || opcode > BPF_JCOND) {
		verbose(env, "invalid BPF_JMP/JMP32 opcode %x\n", opcode);
		return -EINVAL;
	}

	if (opcode == BPF_JCOND) {
		struct bpf_verifier_state *cur_st = env->cur_state, *queued_st, *prev_st;
		int idx = *insn_idx;

		prev_st = find_prev_entry(env, cur_st->parent, idx);

		/* branch out 'fallthrough' insn as a new state to explore */
		queued_st = push_stack(env, idx + 1, idx, false);
		if (IS_ERR(queued_st))
			return PTR_ERR(queued_st);

		queued_st->may_goto_depth++;
		if (prev_st)
			widen_imprecise_scalars(env, prev_st, queued_st);
		*insn_idx += insn->off;
		return 0;
	}

	/* check src2 operand */
	err = check_reg_arg(env, insn->dst_reg, SRC_OP);
	if (err)
		return err;

	dst_reg = &regs[insn->dst_reg];
	if (BPF_SRC(insn->code) == BPF_X) {
		/* check src1 operand */
		err = check_reg_arg(env, insn->src_reg, SRC_OP);
		if (err)
			return err;

		src_reg = &regs[insn->src_reg];
		if (!(reg_is_pkt_pointer_any(dst_reg) && reg_is_pkt_pointer_any(src_reg)) &&
		    is_pointer_value(env, insn->src_reg)) {
			verbose(env, "R%d pointer comparison prohibited\n",
				insn->src_reg);
			return -EACCES;
		}

		if (src_reg->type == PTR_TO_STACK)
			insn_flags |= INSN_F_SRC_REG_STACK;
		if (dst_reg->type == PTR_TO_STACK)
			insn_flags |= INSN_F_DST_REG_STACK;
	} else {
		src_reg = &env->fake_reg[0];
		memset(src_reg, 0, sizeof(*src_reg));
		src_reg->type = SCALAR_VALUE;
		__mark_reg_known(src_reg, insn->imm);

		if (dst_reg->type == PTR_TO_STACK)
			insn_flags |= INSN_F_DST_REG_STACK;
	}

	if (insn_flags) {
		err = bpf_push_jmp_history(env, this_branch, insn_flags, 0);
		if (err)
			return err;
	}

	is_jmp32 = BPF_CLASS(insn->code) == BPF_JMP32;
	copy_register_state(&env->false_reg1, dst_reg);
	copy_register_state(&env->false_reg2, src_reg);
	copy_register_state(&env->true_reg1, dst_reg);
	copy_register_state(&env->true_reg2, src_reg);
	pred = is_branch_taken(env, dst_reg, src_reg, opcode, is_jmp32);
	if (pred >= 0) {
		/* If we get here with a dst_reg pointer type it is because
		 * above is_branch_taken() special cased the 0 comparison.
		 */
		if (!__is_pointer_value(false, dst_reg))
			err = mark_chain_precision(env, insn->dst_reg);
		if (BPF_SRC(insn->code) == BPF_X && !err &&
		    !__is_pointer_value(false, src_reg))
			err = mark_chain_precision(env, insn->src_reg);
		if (err)
			return err;
	}