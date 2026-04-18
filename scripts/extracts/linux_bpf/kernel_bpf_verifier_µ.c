// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 117/132



		env->jmps_processed++;
		if (opcode == BPF_CALL) {
			if (env->cur_state->active_locks) {
				if ((insn->src_reg == BPF_REG_0 &&
				     insn->imm != BPF_FUNC_spin_unlock &&
				     insn->imm != BPF_FUNC_kptr_xchg) ||
				    (insn->src_reg == BPF_PSEUDO_KFUNC_CALL &&
				     (insn->off != 0 || !kfunc_spin_allowed(insn->imm)))) {
					verbose(env,
						"function calls are not allowed while holding a lock\n");
					return -EINVAL;
				}
			}
			mark_reg_scratched(env, BPF_REG_0);
			if (insn->src_reg == BPF_PSEUDO_CALL)
				return check_func_call(env, insn, &env->insn_idx);
			if (insn->src_reg == BPF_PSEUDO_KFUNC_CALL)
				return check_kfunc_call(env, insn, &env->insn_idx);
			return check_helper_call(env, insn, &env->insn_idx);
		} else if (opcode == BPF_JA) {
			if (BPF_SRC(insn->code) == BPF_X)
				return check_indirect_jump(env, insn);

			if (class == BPF_JMP)
				env->insn_idx += insn->off + 1;
			else
				env->insn_idx += insn->imm + 1;
			return INSN_IDX_UPDATED;
		} else if (opcode == BPF_EXIT) {
			return process_bpf_exit_full(env, do_print_state, false);
		}
		return check_cond_jmp_op(env, insn, &env->insn_idx);
	}
	case BPF_LD: {
		u8 mode = BPF_MODE(insn->code);

		if (mode == BPF_ABS || mode == BPF_IND)
			return check_ld_abs(env, insn);

		if (mode == BPF_IMM) {
			err = check_ld_imm(env, insn);
			if (err)
				return err;

			env->insn_idx++;
			sanitize_mark_insn_seen(env);
		}
		return 0;
	}
	}
	/* all class values are handled above. silence compiler warning */
	return -EFAULT;
}

static int do_check(struct bpf_verifier_env *env)
{
	bool pop_log = !(env->log.level & BPF_LOG_LEVEL2);
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_insn *insns = env->prog->insnsi;
	int insn_cnt = env->prog->len;
	bool do_print_state = false;
	int prev_insn_idx = -1;

	for (;;) {
		struct bpf_insn *insn;
		struct bpf_insn_aux_data *insn_aux;
		int err;

		/* reset current history entry on each new instruction */
		env->cur_hist_ent = NULL;

		env->prev_insn_idx = prev_insn_idx;
		if (env->insn_idx >= insn_cnt) {
			verbose(env, "invalid insn idx %d insn_cnt %d\n",
				env->insn_idx, insn_cnt);
			return -EFAULT;
		}

		insn = &insns[env->insn_idx];
		insn_aux = &env->insn_aux_data[env->insn_idx];

		if (++env->insn_processed > BPF_COMPLEXITY_LIMIT_INSNS) {
			verbose(env,
				"BPF program is too large. Processed %d insn\n",
				env->insn_processed);
			return -E2BIG;
		}

		state->last_insn_idx = env->prev_insn_idx;
		state->insn_idx = env->insn_idx;

		if (bpf_is_prune_point(env, env->insn_idx)) {
			err = bpf_is_state_visited(env, env->insn_idx);
			if (err < 0)
				return err;
			if (err == 1) {
				/* found equivalent state, can prune the search */
				if (env->log.level & BPF_LOG_LEVEL) {
					if (do_print_state)
						verbose(env, "\nfrom %d to %d%s: safe\n",
							env->prev_insn_idx, env->insn_idx,
							env->cur_state->speculative ?
							" (speculative execution)" : "");
					else
						verbose(env, "%d: safe\n", env->insn_idx);
				}
				goto process_bpf_exit;
			}
		}

		if (bpf_is_jmp_point(env, env->insn_idx)) {
			err = bpf_push_jmp_history(env, state, 0, 0);
			if (err)
				return err;
		}

		if (signal_pending(current))
			return -EAGAIN;

		if (need_resched())
			cond_resched();

		if (env->log.level & BPF_LOG_LEVEL2 && do_print_state) {
			verbose(env, "\nfrom %d to %d%s:",
				env->prev_insn_idx, env->insn_idx,
				env->cur_state->speculative ?
				" (speculative execution)" : "");
			print_verifier_state(env, state, state->curframe, true);
			do_print_state = false;
		}

		if (env->log.level & BPF_LOG_LEVEL) {
			if (verifier_state_scratched(env))
				print_insn_state(env, state, state->curframe);

			verbose_linfo(env, env->insn_idx, "; ");
			env->prev_log_pos = env->log.end_pos;
			verbose(env, "%d: ", env->insn_idx);
			bpf_verbose_insn(env, insn);
			env->prev_insn_print_pos = env->log.end_pos - env->prev_log_pos;
			env->prev_log_pos = env->log.end_pos;
		}

		if (bpf_prog_is_offloaded(env->prog->aux)) {
			err = bpf_prog_offload_verify_insn(env, env->insn_idx,
							   env->prev_insn_idx);
			if (err)
				return err;
		}

		sanitize_mark_insn_seen(env);
		prev_insn_idx = env->insn_idx;

		/* Sanity check: precomputed constants must match verifier state */
		if (!state->speculative && insn_aux->const_reg_mask) {
			struct bpf_reg_state *regs = cur_regs(env);
			u16 mask = insn_aux->const_reg_mask;

			for (int r = 0; r < ARRAY_SIZE(insn_aux->const_reg_vals); r++) {
				u32 cval = insn_aux->const_reg_vals[r];

				if (!(mask & BIT(r)))
					continue;
				if (regs[r].type != SCALAR_VALUE)
					continue;
				if (!tnum_is_const(regs[r].var_off))
					continue;
				if (verifier_bug_if((u32)regs[r].var_off.value != cval,
						    env, "const R%d: %u != %llu",
						    r, cval, regs[r].var_off.value))
					return -EFAULT;
			}
		}