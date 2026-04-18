// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 66/132



	/* for callbacks like bpf_loop or bpf_for_each_map_elem go back to callsite,
	 * there function call logic would reschedule callback visit. If iteration
	 * converges is_state_visited() would prune that visit eventually.
	 */
	in_callback_fn = callee->in_callback_fn;
	if (in_callback_fn)
		*insn_idx = callee->callsite;
	else
		*insn_idx = callee->callsite + 1;

	if (env->log.level & BPF_LOG_LEVEL) {
		verbose(env, "returning from callee:\n");
		print_verifier_state(env, state, callee->frameno, true);
		verbose(env, "to caller at %d:\n", *insn_idx);
		print_verifier_state(env, state, caller->frameno, true);
	}
	/* clear everything in the callee. In case of exceptional exits using
	 * bpf_throw, this will be done by copy_verifier_state for extra frames. */
	free_func_state(callee);
	state->frame[state->curframe--] = NULL;

	/* for callbacks widen imprecise scalars to make programs like below verify:
	 *
	 *   struct ctx { int i; }
	 *   void cb(int idx, struct ctx *ctx) { ctx->i++; ... }
	 *   ...
	 *   struct ctx = { .i = 0; }
	 *   bpf_loop(100, cb, &ctx, 0);
	 *
	 * This is similar to what is done in process_iter_next_call() for open
	 * coded iterators.
	 */
	prev_st = in_callback_fn ? find_prev_entry(env, state, *insn_idx) : NULL;
	if (prev_st) {
		err = widen_imprecise_scalars(env, prev_st, state);
		if (err)
			return err;
	}
	return 0;
}

static int do_refine_retval_range(struct bpf_verifier_env *env,
				  struct bpf_reg_state *regs, int ret_type,
				  int func_id,
				  struct bpf_call_arg_meta *meta)
{
	struct bpf_reg_state *ret_reg = &regs[BPF_REG_0];

	if (ret_type != RET_INTEGER)
		return 0;

	switch (func_id) {
	case BPF_FUNC_get_stack:
	case BPF_FUNC_get_task_stack:
	case BPF_FUNC_probe_read_str:
	case BPF_FUNC_probe_read_kernel_str:
	case BPF_FUNC_probe_read_user_str:
		ret_reg->smax_value = meta->msize_max_value;
		ret_reg->s32_max_value = meta->msize_max_value;
		ret_reg->smin_value = -MAX_ERRNO;
		ret_reg->s32_min_value = -MAX_ERRNO;
		reg_bounds_sync(ret_reg);
		break;
	case BPF_FUNC_get_smp_processor_id:
		ret_reg->umax_value = nr_cpu_ids - 1;
		ret_reg->u32_max_value = nr_cpu_ids - 1;
		ret_reg->smax_value = nr_cpu_ids - 1;
		ret_reg->s32_max_value = nr_cpu_ids - 1;
		ret_reg->umin_value = 0;
		ret_reg->u32_min_value = 0;
		ret_reg->smin_value = 0;
		ret_reg->s32_min_value = 0;
		reg_bounds_sync(ret_reg);
		break;
	}

	return reg_bounds_sanity_check(env, ret_reg, "retval");
}

static int
record_func_map(struct bpf_verifier_env *env, struct bpf_call_arg_meta *meta,
		int func_id, int insn_idx)
{
	struct bpf_insn_aux_data *aux = &env->insn_aux_data[insn_idx];
	struct bpf_map *map = meta->map.ptr;

	if (func_id != BPF_FUNC_tail_call &&
	    func_id != BPF_FUNC_map_lookup_elem &&
	    func_id != BPF_FUNC_map_update_elem &&
	    func_id != BPF_FUNC_map_delete_elem &&
	    func_id != BPF_FUNC_map_push_elem &&
	    func_id != BPF_FUNC_map_pop_elem &&
	    func_id != BPF_FUNC_map_peek_elem &&
	    func_id != BPF_FUNC_for_each_map_elem &&
	    func_id != BPF_FUNC_redirect_map &&
	    func_id != BPF_FUNC_map_lookup_percpu_elem)
		return 0;

	if (map == NULL) {
		verifier_bug(env, "expected map for helper call");
		return -EFAULT;
	}

	/* In case of read-only, some additional restrictions
	 * need to be applied in order to prevent altering the
	 * state of the map from program side.
	 */
	if ((map->map_flags & BPF_F_RDONLY_PROG) &&
	    (func_id == BPF_FUNC_map_delete_elem ||
	     func_id == BPF_FUNC_map_update_elem ||
	     func_id == BPF_FUNC_map_push_elem ||
	     func_id == BPF_FUNC_map_pop_elem)) {
		verbose(env, "write into map forbidden\n");
		return -EACCES;
	}

	if (!aux->map_ptr_state.map_ptr)
		bpf_map_ptr_store(aux, meta->map.ptr,
				  !meta->map.ptr->bypass_spec_v1, false);
	else if (aux->map_ptr_state.map_ptr != meta->map.ptr)
		bpf_map_ptr_store(aux, meta->map.ptr,
				  !meta->map.ptr->bypass_spec_v1, true);
	return 0;
}

static int
record_func_key(struct bpf_verifier_env *env, struct bpf_call_arg_meta *meta,
		int func_id, int insn_idx)
{
	struct bpf_insn_aux_data *aux = &env->insn_aux_data[insn_idx];
	struct bpf_reg_state *reg;
	struct bpf_map *map = meta->map.ptr;
	u64 val, max;
	int err;

	if (func_id != BPF_FUNC_tail_call)
		return 0;
	if (!map || map->map_type != BPF_MAP_TYPE_PROG_ARRAY) {
		verbose(env, "expected prog array map for tail call");
		return -EINVAL;
	}

	reg = reg_state(env, BPF_REG_3);
	val = reg->var_off.value;
	max = map->max_entries;

	if (!(is_reg_const(reg, false) && val < max)) {
		bpf_map_key_store(aux, BPF_MAP_KEY_POISON);
		return 0;
	}

	err = mark_chain_precision(env, BPF_REG_3);
	if (err)
		return err;
	if (bpf_map_key_unseen(aux))
		bpf_map_key_store(aux, val);
	else if (!bpf_map_key_poisoned(aux) &&
		  bpf_map_key_immediate(aux) != val)
		bpf_map_key_store(aux, BPF_MAP_KEY_POISON);
	return 0;
}