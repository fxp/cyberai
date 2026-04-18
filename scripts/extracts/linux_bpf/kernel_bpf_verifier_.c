// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 64/132



	callee->regs[BPF_REG_3].type = PTR_TO_MAP_VALUE;
	__mark_reg_known_zero(&callee->regs[BPF_REG_3]);
	callee->regs[BPF_REG_3].map_ptr = caller->regs[BPF_REG_1].map_ptr;

	/* pointer to stack or null */
	callee->regs[BPF_REG_4] = caller->regs[BPF_REG_3];

	/* unused */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);
	return 0;
}

static int set_callee_state(struct bpf_verifier_env *env,
			    struct bpf_func_state *caller,
			    struct bpf_func_state *callee, int insn_idx)
{
	int i;

	/* copy r1 - r5 args that callee can access.  The copy includes parent
	 * pointers, which connects us up to the liveness chain
	 */
	for (i = BPF_REG_1; i <= BPF_REG_5; i++)
		callee->regs[i] = caller->regs[i];
	return 0;
}

static int set_map_elem_callback_state(struct bpf_verifier_env *env,
				       struct bpf_func_state *caller,
				       struct bpf_func_state *callee,
				       int insn_idx)
{
	struct bpf_insn_aux_data *insn_aux = &env->insn_aux_data[insn_idx];
	struct bpf_map *map;
	int err;

	/* valid map_ptr and poison value does not matter */
	map = insn_aux->map_ptr_state.map_ptr;
	if (!map->ops->map_set_for_each_callback_args ||
	    !map->ops->map_for_each_callback) {
		verbose(env, "callback function not allowed for map\n");
		return -ENOTSUPP;
	}

	err = map->ops->map_set_for_each_callback_args(env, caller, callee);
	if (err)
		return err;

	callee->in_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 1);
	return 0;
}

static int set_loop_callback_state(struct bpf_verifier_env *env,
				   struct bpf_func_state *caller,
				   struct bpf_func_state *callee,
				   int insn_idx)
{
	/* bpf_loop(u32 nr_loops, void *callback_fn, void *callback_ctx,
	 *	    u64 flags);
	 * callback_fn(u64 index, void *callback_ctx);
	 */
	callee->regs[BPF_REG_1].type = SCALAR_VALUE;
	callee->regs[BPF_REG_2] = caller->regs[BPF_REG_3];

	/* unused */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_3]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_4]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);

	callee->in_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 1);
	return 0;
}

static int set_timer_callback_state(struct bpf_verifier_env *env,
				    struct bpf_func_state *caller,
				    struct bpf_func_state *callee,
				    int insn_idx)
{
	struct bpf_map *map_ptr = caller->regs[BPF_REG_1].map_ptr;

	/* bpf_timer_set_callback(struct bpf_timer *timer, void *callback_fn);
	 * callback_fn(struct bpf_map *map, void *key, void *value);
	 */
	callee->regs[BPF_REG_1].type = CONST_PTR_TO_MAP;
	__mark_reg_known_zero(&callee->regs[BPF_REG_1]);
	callee->regs[BPF_REG_1].map_ptr = map_ptr;

	callee->regs[BPF_REG_2].type = PTR_TO_MAP_KEY;
	__mark_reg_known_zero(&callee->regs[BPF_REG_2]);
	callee->regs[BPF_REG_2].map_ptr = map_ptr;

	callee->regs[BPF_REG_3].type = PTR_TO_MAP_VALUE;
	__mark_reg_known_zero(&callee->regs[BPF_REG_3]);
	callee->regs[BPF_REG_3].map_ptr = map_ptr;

	/* unused */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_4]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);
	callee->in_async_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 0);
	return 0;
}

static int set_find_vma_callback_state(struct bpf_verifier_env *env,
				       struct bpf_func_state *caller,
				       struct bpf_func_state *callee,
				       int insn_idx)
{
	/* bpf_find_vma(struct task_struct *task, u64 addr,
	 *               void *callback_fn, void *callback_ctx, u64 flags)
	 * (callback_fn)(struct task_struct *task,
	 *               struct vm_area_struct *vma, void *callback_ctx);
	 */
	callee->regs[BPF_REG_1] = caller->regs[BPF_REG_1];

	callee->regs[BPF_REG_2].type = PTR_TO_BTF_ID;
	__mark_reg_known_zero(&callee->regs[BPF_REG_2]);
	callee->regs[BPF_REG_2].btf =  btf_vmlinux;
	callee->regs[BPF_REG_2].btf_id = btf_tracing_ids[BTF_TRACING_TYPE_VMA];

	/* pointer to stack or null */
	callee->regs[BPF_REG_3] = caller->regs[BPF_REG_4];

	/* unused */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_4]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);
	callee->in_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 1);
	return 0;
}

static int set_user_ringbuf_callback_state(struct bpf_verifier_env *env,
					   struct bpf_func_state *caller,
					   struct bpf_func_state *callee,
					   int insn_idx)
{
	/* bpf_user_ringbuf_drain(struct bpf_map *map, void *callback_fn, void
	 *			  callback_ctx, u64 flags);
	 * callback_fn(const struct bpf_dynptr_t* dynptr, void *callback_ctx);
	 */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_0]);
	mark_dynptr_cb_reg(env, &callee->regs[BPF_REG_1], BPF_DYNPTR_TYPE_LOCAL);
	callee->regs[BPF_REG_2] = caller->regs[BPF_REG_3];

	/* unused */
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_3]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_4]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);

	callee->in_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 1);
	return 0;
}