// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 27/132



/* Write the stack: 'stack[ptr_regno + off] = value_regno'. 'ptr_regno' is
 * known to contain a variable offset.
 * This function checks whether the write is permitted and conservatively
 * tracks the effects of the write, considering that each stack slot in the
 * dynamic range is potentially written to.
 *
 * 'value_regno' can be -1, meaning that an unknown value is being written to
 * the stack.
 *
 * Spilled pointers in range are not marked as written because we don't know
 * what's going to be actually written. This means that read propagation for
 * future reads cannot be terminated by this write.
 *
 * For privileged programs, uninitialized stack slots are considered
 * initialized by this write (even though we don't know exactly what offsets
 * are going to be written to). The idea is that we don't want the verifier to
 * reject future reads that access slots written to through variable offsets.
 */
static int check_stack_write_var_off(struct bpf_verifier_env *env,
				     /* func where register points to */
				     struct bpf_func_state *state,
				     int ptr_regno, int off, int size,
				     int value_regno, int insn_idx)
{
	struct bpf_func_state *cur; /* state of the current function */
	int min_off, max_off;
	int i, err;
	struct bpf_reg_state *ptr_reg = NULL, *value_reg = NULL;
	struct bpf_insn *insn = &env->prog->insnsi[insn_idx];
	bool writing_zero = false;
	/* set if the fact that we're writing a zero is used to let any
	 * stack slots remain STACK_ZERO
	 */
	bool zero_used = false;

	cur = env->cur_state->frame[env->cur_state->curframe];
	ptr_reg = &cur->regs[ptr_regno];
	min_off = ptr_reg->smin_value + off;
	max_off = ptr_reg->smax_value + off + size;
	if (value_regno >= 0)
		value_reg = &cur->regs[value_regno];
	if ((value_reg && bpf_register_is_null(value_reg)) ||
	    (!value_reg && is_bpf_st_mem(insn) && insn->imm == 0))
		writing_zero = true;

	for (i = min_off; i < max_off; i++) {
		int spi;

		spi = bpf_get_spi(i);
		err = destroy_if_dynptr_stack_slot(env, state, spi);
		if (err)
			return err;
	}

	check_fastcall_stack_contract(env, state, insn_idx, min_off);
	/* Variable offset writes destroy any spilled pointers in range. */
	for (i = min_off; i < max_off; i++) {
		u8 new_type, *stype;
		int slot, spi;

		slot = -i - 1;
		spi = slot / BPF_REG_SIZE;
		stype = &state->stack[spi].slot_type[slot % BPF_REG_SIZE];
		mark_stack_slot_scratched(env, spi);

		if (!env->allow_ptr_leaks && *stype != STACK_MISC && *stype != STACK_ZERO) {
			/* Reject the write if range we may write to has not
			 * been initialized beforehand. If we didn't reject
			 * here, the ptr status would be erased below (even
			 * though not all slots are actually overwritten),
			 * possibly opening the door to leaks.
			 *
			 * We do however catch STACK_INVALID case below, and
			 * only allow reading possibly uninitialized memory
			 * later for CAP_PERFMON, as the write may not happen to
			 * that slot.
			 */
			verbose(env, "spilled ptr in range of var-offset stack write; insn %d, ptr off: %d",
				insn_idx, i);
			return -EINVAL;
		}

		/* If writing_zero and the spi slot contains a spill of value 0,
		 * maintain the spill type.
		 */
		if (writing_zero && *stype == STACK_SPILL &&
		    bpf_is_spilled_scalar_reg(&state->stack[spi])) {
			struct bpf_reg_state *spill_reg = &state->stack[spi].spilled_ptr;

			if (tnum_is_const(spill_reg->var_off) && spill_reg->var_off.value == 0) {
				zero_used = true;
				continue;
			}
		}

		/*
		 * Scrub slots if variable-offset stack write goes over spilled pointers.
		 * Otherwise bpf_is_spilled_reg() may == true && spilled_ptr.type == NOT_INIT
		 * and valid program is rejected by check_stack_read_fixed_off()
		 * with obscure "invalid size of register fill" message.
		 */
		scrub_special_slot(state, spi);

		/* Update the slot type. */
		new_type = STACK_MISC;
		if (writing_zero && *stype == STACK_ZERO) {
			new_type = STACK_ZERO;
			zero_used = true;
		}
		/* If the slot is STACK_INVALID, we check whether it's OK to
		 * pretend that it will be initialized by this write. The slot
		 * might not actually be written to, and so if we mark it as
		 * initialized future reads might leak uninitialized memory.
		 * For privileged programs, we will accept such reads to slots
		 * that may or may not be written because, if we're reject
		 * them, the error would be too confusing.
		 * Conservatively, treat STACK_POISON in a similar way.
		 */
		if ((*stype == STACK_INVALID || *stype == STACK_POISON) &&
		    !env->allow_uninit_stack) {
			verbose(env, "uninit stack in range of var-offset write prohibited for !root; insn %d, off: %d",
					insn_idx, i);
			return -EINVAL;
		}
		*stype = new_type;
	}
	if (zero_used) {
		/* backtracking doesn't work for STACK_ZERO yet. */
		err = mark_chain_precision(env, value_regno);
		if (err)
			return err;
	}
	return 0;
}