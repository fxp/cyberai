// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 89/132



	if (ptr_limit >= max)
		return REASON_LIMIT;
	*alu_limit = ptr_limit;
	return 0;
}

static bool can_skip_alu_sanitation(const struct bpf_verifier_env *env,
				    const struct bpf_insn *insn)
{
	return env->bypass_spec_v1 ||
		BPF_SRC(insn->code) == BPF_K ||
		cur_aux(env)->nospec;
}

static int update_alu_sanitation_state(struct bpf_insn_aux_data *aux,
				       u32 alu_state, u32 alu_limit)
{
	/* If we arrived here from different branches with different
	 * state or limits to sanitize, then this won't work.
	 */
	if (aux->alu_state &&
	    (aux->alu_state != alu_state ||
	     aux->alu_limit != alu_limit))
		return REASON_PATHS;

	/* Corresponding fixup done in do_misc_fixups(). */
	aux->alu_state = alu_state;
	aux->alu_limit = alu_limit;
	return 0;
}

static int sanitize_val_alu(struct bpf_verifier_env *env,
			    struct bpf_insn *insn)
{
	struct bpf_insn_aux_data *aux = cur_aux(env);

	if (can_skip_alu_sanitation(env, insn))
		return 0;

	return update_alu_sanitation_state(aux, BPF_ALU_NON_POINTER, 0);
}

static bool sanitize_needed(u8 opcode)
{
	return opcode == BPF_ADD || opcode == BPF_SUB;
}

struct bpf_sanitize_info {
	struct bpf_insn_aux_data aux;
	bool mask_to_left;
};

static int sanitize_speculative_path(struct bpf_verifier_env *env,
				     const struct bpf_insn *insn,
				     u32 next_idx, u32 curr_idx)
{
	struct bpf_verifier_state *branch;
	struct bpf_reg_state *regs;

	branch = push_stack(env, next_idx, curr_idx, true);
	if (!IS_ERR(branch) && insn) {
		regs = branch->frame[branch->curframe]->regs;
		if (BPF_SRC(insn->code) == BPF_K) {
			mark_reg_unknown(env, regs, insn->dst_reg);
		} else if (BPF_SRC(insn->code) == BPF_X) {
			mark_reg_unknown(env, regs, insn->dst_reg);
			mark_reg_unknown(env, regs, insn->src_reg);
		}
	}
	return PTR_ERR_OR_ZERO(branch);
}

static int sanitize_ptr_alu(struct bpf_verifier_env *env,
			    struct bpf_insn *insn,
			    const struct bpf_reg_state *ptr_reg,
			    const struct bpf_reg_state *off_reg,
			    struct bpf_reg_state *dst_reg,
			    struct bpf_sanitize_info *info,
			    const bool commit_window)
{
	struct bpf_insn_aux_data *aux = commit_window ? cur_aux(env) : &info->aux;
	struct bpf_verifier_state *vstate = env->cur_state;
	bool off_is_imm = tnum_is_const(off_reg->var_off);
	bool off_is_neg = off_reg->smin_value < 0;
	bool ptr_is_dst_reg = ptr_reg == dst_reg;
	u8 opcode = BPF_OP(insn->code);
	u32 alu_state, alu_limit;
	struct bpf_reg_state tmp;
	int err;

	if (can_skip_alu_sanitation(env, insn))
		return 0;

	/* We already marked aux for masking from non-speculative
	 * paths, thus we got here in the first place. We only care
	 * to explore bad access from here.
	 */
	if (vstate->speculative)
		goto do_sim;

	if (!commit_window) {
		if (!tnum_is_const(off_reg->var_off) &&
		    (off_reg->smin_value < 0) != (off_reg->smax_value < 0))
			return REASON_BOUNDS;

		info->mask_to_left = (opcode == BPF_ADD &&  off_is_neg) ||
				     (opcode == BPF_SUB && !off_is_neg);
	}

	err = retrieve_ptr_limit(ptr_reg, &alu_limit, info->mask_to_left);
	if (err < 0)
		return err;

	if (commit_window) {
		/* In commit phase we narrow the masking window based on
		 * the observed pointer move after the simulated operation.
		 */
		alu_state = info->aux.alu_state;
		alu_limit = abs(info->aux.alu_limit - alu_limit);
	} else {
		alu_state  = off_is_neg ? BPF_ALU_NEG_VALUE : 0;
		alu_state |= off_is_imm ? BPF_ALU_IMMEDIATE : 0;
		alu_state |= ptr_is_dst_reg ?
			     BPF_ALU_SANITIZE_SRC : BPF_ALU_SANITIZE_DST;

		/* Limit pruning on unknown scalars to enable deep search for
		 * potential masking differences from other program paths.
		 */
		if (!off_is_imm)
			env->explore_alu_limits = true;
	}

	err = update_alu_sanitation_state(aux, alu_state, alu_limit);
	if (err < 0)
		return err;
do_sim:
	/* If we're in commit phase, we're done here given we already
	 * pushed the truncated dst_reg into the speculative verification
	 * stack.
	 *
	 * Also, when register is a known constant, we rewrite register-based
	 * operation to immediate-based, and thus do not need masking (and as
	 * a consequence, do not need to simulate the zero-truncation either).
	 */
	if (commit_window || off_is_imm)
		return 0;

	/* Simulate and find potential out-of-bounds access under
	 * speculative execution from truncation as a result of
	 * masking when off was not within expected range. If off
	 * sits in dst, then we temporarily need to move ptr there
	 * to simulate dst (== 0) +/-= ptr. Needed, for example,
	 * for cases where we use K-based arithmetic in one direction
	 * and truncated reg-based in the other in order to explore
	 * bad access.
	 */
	if (!ptr_is_dst_reg) {
		tmp = *dst_reg;
		copy_register_state(dst_reg, ptr_reg);
	}
	err = sanitize_speculative_path(env, NULL, env->insn_idx + 1, env->insn_idx);
	if (err < 0)
		return REASON_STACK;
	if (!ptr_is_dst_reg)
		*dst_reg = tmp;
	return 0;
}