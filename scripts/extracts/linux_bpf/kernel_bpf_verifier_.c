// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 86/132



static int check_return_code(struct bpf_verifier_env *env, int regno, const char *reg_name);
static int process_bpf_exit_full(struct bpf_verifier_env *env,
				 bool *do_print_state, bool exception_exit);

static int check_kfunc_call(struct bpf_verifier_env *env, struct bpf_insn *insn,
			    int *insn_idx_p)
{
	bool sleepable, rcu_lock, rcu_unlock, preempt_disable, preempt_enable;
	u32 i, nargs, ptr_type_id, release_ref_obj_id;
	struct bpf_reg_state *regs = cur_regs(env);
	const char *func_name, *ptr_type_name;
	const struct btf_type *t, *ptr_type;
	struct bpf_kfunc_call_arg_meta meta;
	struct bpf_insn_aux_data *insn_aux;
	int err, insn_idx = *insn_idx_p;
	const struct btf_param *args;
	struct btf *desc_btf;

	/* skip for now, but return error when we find this in fixup_kfunc_call */
	if (!insn->imm)
		return 0;

	err = bpf_fetch_kfunc_arg_meta(env, insn->imm, insn->off, &meta);
	if (err == -EACCES && meta.func_name)
		verbose(env, "calling kernel function %s is not allowed\n", meta.func_name);
	if (err)
		return err;
	desc_btf = meta.btf;
	func_name = meta.func_name;
	insn_aux = &env->insn_aux_data[insn_idx];

	insn_aux->is_iter_next = bpf_is_iter_next_kfunc(&meta);

	if (!insn->off &&
	    (insn->imm == special_kfunc_list[KF_bpf_res_spin_lock] ||
	     insn->imm == special_kfunc_list[KF_bpf_res_spin_lock_irqsave])) {
		struct bpf_verifier_state *branch;
		struct bpf_reg_state *regs;

		branch = push_stack(env, env->insn_idx + 1, env->insn_idx, false);
		if (IS_ERR(branch)) {
			verbose(env, "failed to push state for failed lock acquisition\n");
			return PTR_ERR(branch);
		}

		regs = branch->frame[branch->curframe]->regs;

		/* Clear r0-r5 registers in forked state */
		for (i = 0; i < CALLER_SAVED_REGS; i++)
			bpf_mark_reg_not_init(env, &regs[caller_saved[i]]);

		mark_reg_unknown(env, regs, BPF_REG_0);
		err = __mark_reg_s32_range(env, regs, BPF_REG_0, -MAX_ERRNO, -1);
		if (err) {
			verbose(env, "failed to mark s32 range for retval in forked state for lock\n");
			return err;
		}
		__mark_btf_func_reg_size(env, regs, BPF_REG_0, sizeof(u32));
	} else if (!insn->off && insn->imm == special_kfunc_list[KF___bpf_trap]) {
		verbose(env, "unexpected __bpf_trap() due to uninitialized variable?\n");
		return -EFAULT;
	}

	if (is_kfunc_destructive(&meta) && !capable(CAP_SYS_BOOT)) {
		verbose(env, "destructive kfunc calls require CAP_SYS_BOOT capability\n");
		return -EACCES;
	}

	sleepable = bpf_is_kfunc_sleepable(&meta);
	if (sleepable && !in_sleepable(env)) {
		verbose(env, "program must be sleepable to call sleepable kfunc %s\n", func_name);
		return -EACCES;
	}

	/* Track non-sleepable context for kfuncs, same as for helpers. */
	if (!in_sleepable_context(env))
		insn_aux->non_sleepable = true;

	/* Check the arguments */
	err = check_kfunc_args(env, &meta, insn_idx);
	if (err < 0)
		return err;

	if (is_bpf_rbtree_add_kfunc(meta.func_id)) {
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_rbtree_add_callback_state);
		if (err) {
			verbose(env, "kfunc %s#%d failed callback verification\n",
				func_name, meta.func_id);
			return err;
		}
	}

	if (meta.func_id == special_kfunc_list[KF_bpf_session_cookie]) {
		meta.r0_size = sizeof(u64);
		meta.r0_rdonly = false;
	}

	if (is_bpf_wq_set_callback_kfunc(meta.func_id)) {
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_timer_callback_state);
		if (err) {
			verbose(env, "kfunc %s#%d failed callback verification\n",
				func_name, meta.func_id);
			return err;
		}
	}

	if (is_task_work_add_kfunc(meta.func_id)) {
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_task_work_schedule_callback_state);
		if (err) {
			verbose(env, "kfunc %s#%d failed callback verification\n",
				func_name, meta.func_id);
			return err;
		}
	}

	rcu_lock = is_kfunc_bpf_rcu_read_lock(&meta);
	rcu_unlock = is_kfunc_bpf_rcu_read_unlock(&meta);

	preempt_disable = is_kfunc_bpf_preempt_disable(&meta);
	preempt_enable = is_kfunc_bpf_preempt_enable(&meta);

	if (rcu_lock) {
		env->cur_state->active_rcu_locks++;
	} else if (rcu_unlock) {
		struct bpf_func_state *state;
		struct bpf_reg_state *reg;
		u32 clear_mask = (1 << STACK_SPILL) | (1 << STACK_ITER);

		if (env->cur_state->active_rcu_locks == 0) {
			verbose(env, "unmatched rcu read unlock (kernel function %s)\n", func_name);
			return -EINVAL;
		}
		if (--env->cur_state->active_rcu_locks == 0) {
			bpf_for_each_reg_in_vstate_mask(env->cur_state, state, reg, clear_mask, ({
				if (reg->type & MEM_RCU) {
					reg->type &= ~(MEM_RCU | PTR_MAYBE_NULL);
					reg->type |= PTR_UNTRUSTED;
				}
			}));
		}
	} else if (preempt_disable) {
		env->cur_state->active_preempt_locks++;
	} else if (preempt_enable) {
		if (env->cur_state->active_preempt_locks == 0) {
			verbose(env, "unmatched attempt to enable preemption (kernel function %s)\n", func_name);
			return -EINVAL;
		}
		env->cur_state->active_preempt_locks--;
	}