// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 68/132



	*ptr = env->ops->get_func_proto(func_id, env->prog);
	return *ptr && (*ptr)->func ? 0 : -EINVAL;
}

/* Check if we're in a sleepable context. */
static inline bool in_sleepable_context(struct bpf_verifier_env *env)
{
	return !env->cur_state->active_rcu_locks &&
	       !env->cur_state->active_preempt_locks &&
	       !env->cur_state->active_locks &&
	       !env->cur_state->active_irq_id &&
	       in_sleepable(env);
}

static const char *non_sleepable_context_description(struct bpf_verifier_env *env)
{
	if (env->cur_state->active_rcu_locks)
		return "rcu_read_lock region";
	if (env->cur_state->active_preempt_locks)
		return "non-preemptible region";
	if (env->cur_state->active_irq_id)
		return "IRQ-disabled region";
	if (env->cur_state->active_locks)
		return "lock region";
	return "non-sleepable prog";
}

static int check_helper_call(struct bpf_verifier_env *env, struct bpf_insn *insn,
			     int *insn_idx_p)
{
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);
	bool returns_cpu_specific_alloc_ptr = false;
	const struct bpf_func_proto *fn = NULL;
	enum bpf_return_type ret_type;
	enum bpf_type_flag ret_flag;
	struct bpf_reg_state *regs;
	struct bpf_call_arg_meta meta;
	int insn_idx = *insn_idx_p;
	bool changes_data;
	int i, err, func_id;

	/* find function prototype */
	func_id = insn->imm;
	err = bpf_get_helper_proto(env, insn->imm, &fn);
	if (err == -ERANGE) {
		verbose(env, "invalid func %s#%d\n", func_id_name(func_id), func_id);
		return -EINVAL;
	}

	if (err) {
		verbose(env, "program of this type cannot use helper %s#%d\n",
			func_id_name(func_id), func_id);
		return err;
	}

	/* eBPF programs must be GPL compatible to use GPL-ed functions */
	if (!env->prog->gpl_compatible && fn->gpl_only) {
		verbose(env, "cannot call GPL-restricted function from non-GPL compatible program\n");
		return -EINVAL;
	}

	if (fn->allowed && !fn->allowed(env->prog)) {
		verbose(env, "helper call is not allowed in probe\n");
		return -EINVAL;
	}

	/* With LD_ABS/IND some JITs save/restore skb from r1. */
	changes_data = bpf_helper_changes_pkt_data(func_id);
	if (changes_data && fn->arg1_type != ARG_PTR_TO_CTX) {
		verifier_bug(env, "func %s#%d: r1 != ctx", func_id_name(func_id), func_id);
		return -EFAULT;
	}

	memset(&meta, 0, sizeof(meta));
	meta.pkt_access = fn->pkt_access;

	err = check_func_proto(fn);
	if (err) {
		verifier_bug(env, "incorrect func proto %s#%d", func_id_name(func_id), func_id);
		return err;
	}

	if (fn->might_sleep && !in_sleepable_context(env)) {
		verbose(env, "sleepable helper %s#%d in %s\n", func_id_name(func_id), func_id,
			non_sleepable_context_description(env));
		return -EINVAL;
	}

	/* Track non-sleepable context for helpers. */
	if (!in_sleepable_context(env))
		env->insn_aux_data[insn_idx].non_sleepable = true;

	meta.func_id = func_id;
	/* check args */
	for (i = 0; i < MAX_BPF_FUNC_REG_ARGS; i++) {
		err = check_func_arg(env, i, &meta, fn, insn_idx);
		if (err)
			return err;
	}

	err = record_func_map(env, &meta, func_id, insn_idx);
	if (err)
		return err;

	err = record_func_key(env, &meta, func_id, insn_idx);
	if (err)
		return err;

	/* Mark slots with STACK_MISC in case of raw mode, stack offset
	 * is inferred from register state.
	 */
	for (i = 0; i < meta.access_size; i++) {
		err = check_mem_access(env, insn_idx, meta.regno, i, BPF_B,
				       BPF_WRITE, -1, false, false);
		if (err)
			return err;
	}

	regs = cur_regs(env);

	if (meta.release_regno) {
		err = -EINVAL;
		if (arg_type_is_dynptr(fn->arg_type[meta.release_regno - BPF_REG_1])) {
			err = unmark_stack_slots_dynptr(env, &regs[meta.release_regno]);
		} else if (func_id == BPF_FUNC_kptr_xchg && meta.ref_obj_id) {
			u32 ref_obj_id = meta.ref_obj_id;
			bool in_rcu = in_rcu_cs(env);
			struct bpf_func_state *state;
			struct bpf_reg_state *reg;

			err = release_reference_nomark(env->cur_state, ref_obj_id);
			if (!err) {
				bpf_for_each_reg_in_vstate(env->cur_state, state, reg, ({
					if (reg->ref_obj_id == ref_obj_id) {
						if (in_rcu && (reg->type & MEM_ALLOC) && (reg->type & MEM_PERCPU)) {
							reg->ref_obj_id = 0;
							reg->type &= ~MEM_ALLOC;
							reg->type |= MEM_RCU;
						} else {
							mark_reg_invalid(env, reg);
						}
					}
				}));
			}
		} else if (meta.ref_obj_id) {
			err = release_reference(env, meta.ref_obj_id);
		} else if (bpf_register_is_null(&regs[meta.release_regno])) {
			/* meta.ref_obj_id can only be 0 if register that is meant to be
			 * released is NULL, which must be > R0.
			 */
			err = 0;
		}
		if (err) {
			verbose(env, "func %s#%d reference has not been acquired before\n",
				func_id_name(func_id), func_id);
			return err;
		}
	}