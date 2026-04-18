// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 109/132



	if (pred == 1) {
		/* Only follow the goto, ignore fall-through. If needed, push
		 * the fall-through branch for simulation under speculative
		 * execution.
		 */
		if (!env->bypass_spec_v1) {
			err = sanitize_speculative_path(env, insn, *insn_idx + 1, *insn_idx);
			if (err < 0)
				return err;
		}
		if (env->log.level & BPF_LOG_LEVEL)
			print_insn_state(env, this_branch, this_branch->curframe);
		*insn_idx += insn->off;
		return 0;
	} else if (pred == 0) {
		/* Only follow the fall-through branch, since that's where the
		 * program will go. If needed, push the goto branch for
		 * simulation under speculative execution.
		 */
		if (!env->bypass_spec_v1) {
			err = sanitize_speculative_path(env, insn, *insn_idx + insn->off + 1,
							*insn_idx);
			if (err < 0)
				return err;
		}
		if (env->log.level & BPF_LOG_LEVEL)
			print_insn_state(env, this_branch, this_branch->curframe);
		return 0;
	}

	/* Push scalar registers sharing same ID to jump history,
	 * do this before creating 'other_branch', so that both
	 * 'this_branch' and 'other_branch' share this history
	 * if parent state is created.
	 */
	if (BPF_SRC(insn->code) == BPF_X && src_reg->type == SCALAR_VALUE && src_reg->id)
		collect_linked_regs(env, this_branch, src_reg->id, &linked_regs);
	if (dst_reg->type == SCALAR_VALUE && dst_reg->id)
		collect_linked_regs(env, this_branch, dst_reg->id, &linked_regs);
	if (linked_regs.cnt > 1) {
		err = bpf_push_jmp_history(env, this_branch, 0, linked_regs_pack(&linked_regs));
		if (err)
			return err;
	}

	other_branch = push_stack(env, *insn_idx + insn->off + 1, *insn_idx, false);
	if (IS_ERR(other_branch))
		return PTR_ERR(other_branch);
	other_branch_regs = other_branch->frame[other_branch->curframe]->regs;

	err = regs_bounds_sanity_check_branches(env);
	if (err)
		return err;

	copy_register_state(dst_reg, &env->false_reg1);
	copy_register_state(src_reg, &env->false_reg2);
	copy_register_state(&other_branch_regs[insn->dst_reg], &env->true_reg1);
	if (BPF_SRC(insn->code) == BPF_X)
		copy_register_state(&other_branch_regs[insn->src_reg], &env->true_reg2);

	if (BPF_SRC(insn->code) == BPF_X &&
	    src_reg->type == SCALAR_VALUE && src_reg->id &&
	    !WARN_ON_ONCE(src_reg->id != other_branch_regs[insn->src_reg].id)) {
		sync_linked_regs(env, this_branch, src_reg, &linked_regs);
		sync_linked_regs(env, other_branch, &other_branch_regs[insn->src_reg],
				 &linked_regs);
	}
	if (dst_reg->type == SCALAR_VALUE && dst_reg->id &&
	    !WARN_ON_ONCE(dst_reg->id != other_branch_regs[insn->dst_reg].id)) {
		sync_linked_regs(env, this_branch, dst_reg, &linked_regs);
		sync_linked_regs(env, other_branch, &other_branch_regs[insn->dst_reg],
				 &linked_regs);
	}

	/* if one pointer register is compared to another pointer
	 * register check if PTR_MAYBE_NULL could be lifted.
	 * E.g. register A - maybe null
	 *      register B - not null
	 * for JNE A, B, ... - A is not null in the false branch;
	 * for JEQ A, B, ... - A is not null in the true branch.
	 *
	 * Since PTR_TO_BTF_ID points to a kernel struct that does
	 * not need to be null checked by the BPF program, i.e.,
	 * could be null even without PTR_MAYBE_NULL marking, so
	 * only propagate nullness when neither reg is that type.
	 */
	if (!is_jmp32 && BPF_SRC(insn->code) == BPF_X &&
	    __is_pointer_value(false, src_reg) && __is_pointer_value(false, dst_reg) &&
	    type_may_be_null(src_reg->type) != type_may_be_null(dst_reg->type) &&
	    base_type(src_reg->type) != PTR_TO_BTF_ID &&
	    base_type(dst_reg->type) != PTR_TO_BTF_ID) {
		eq_branch_regs = NULL;
		switch (opcode) {
		case BPF_JEQ:
			eq_branch_regs = other_branch_regs;
			break;
		case BPF_JNE:
			eq_branch_regs = regs;
			break;
		default:
			/* do nothing */
			break;
		}
		if (eq_branch_regs) {
			if (type_may_be_null(src_reg->type))
				mark_ptr_not_null_reg(&eq_branch_regs[insn->src_reg]);
			else
				mark_ptr_not_null_reg(&eq_branch_regs[insn->dst_reg]);
		}
	}