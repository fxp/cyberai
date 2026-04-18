// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 65/132



static int set_rbtree_add_callback_state(struct bpf_verifier_env *env,
					 struct bpf_func_state *caller,
					 struct bpf_func_state *callee,
					 int insn_idx)
{
	/* void bpf_rbtree_add_impl(struct bpf_rb_root *root, struct bpf_rb_node *node,
	 *                     bool (less)(struct bpf_rb_node *a, const struct bpf_rb_node *b));
	 *
	 * 'struct bpf_rb_node *node' arg to bpf_rbtree_add_impl is the same PTR_TO_BTF_ID w/ offset
	 * that 'less' callback args will be receiving. However, 'node' arg was release_reference'd
	 * by this point, so look at 'root'
	 */
	struct btf_field *field;

	field = reg_find_field_offset(&caller->regs[BPF_REG_1],
				      caller->regs[BPF_REG_1].var_off.value,
				      BPF_RB_ROOT);
	if (!field || !field->graph_root.value_btf_id)
		return -EFAULT;

	mark_reg_graph_node(callee->regs, BPF_REG_1, &field->graph_root);
	ref_set_non_owning(env, &callee->regs[BPF_REG_1]);
	mark_reg_graph_node(callee->regs, BPF_REG_2, &field->graph_root);
	ref_set_non_owning(env, &callee->regs[BPF_REG_2]);

	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_3]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_4]);
	bpf_mark_reg_not_init(env, &callee->regs[BPF_REG_5]);
	callee->in_callback_fn = true;
	callee->callback_ret_range = retval_range(0, 1);
	return 0;
}

static int set_task_work_schedule_callback_state(struct bpf_verifier_env *env,
						 struct bpf_func_state *caller,
						 struct bpf_func_state *callee,
						 int insn_idx)
{
	struct bpf_map *map_ptr = caller->regs[BPF_REG_3].map_ptr;

	/*
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
	callee->callback_ret_range = retval_range(S32_MIN, S32_MAX);
	return 0;
}

static bool is_rbtree_lock_required_kfunc(u32 btf_id);

/* Are we currently verifying the callback for a rbtree helper that must
 * be called with lock held? If so, no need to complain about unreleased
 * lock
 */
static bool in_rbtree_lock_required_cb(struct bpf_verifier_env *env)
{
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_insn *insn = env->prog->insnsi;
	struct bpf_func_state *callee;
	int kfunc_btf_id;

	if (!state->curframe)
		return false;

	callee = state->frame[state->curframe];

	if (!callee->in_callback_fn)
		return false;

	kfunc_btf_id = insn[callee->callsite].imm;
	return is_rbtree_lock_required_kfunc(kfunc_btf_id);
}

static bool retval_range_within(struct bpf_retval_range range, const struct bpf_reg_state *reg)
{
	if (range.return_32bit)
		return range.minval <= reg->s32_min_value && reg->s32_max_value <= range.maxval;
	else
		return range.minval <= reg->smin_value && reg->smax_value <= range.maxval;
}

static int prepare_func_exit(struct bpf_verifier_env *env, int *insn_idx)
{
	struct bpf_verifier_state *state = env->cur_state, *prev_st;
	struct bpf_func_state *caller, *callee;
	struct bpf_reg_state *r0;
	bool in_callback_fn;
	int err;

	callee = state->frame[state->curframe];
	r0 = &callee->regs[BPF_REG_0];
	if (r0->type == PTR_TO_STACK) {
		/* technically it's ok to return caller's stack pointer
		 * (or caller's caller's pointer) back to the caller,
		 * since these pointers are valid. Only current stack
		 * pointer will be invalid as soon as function exits,
		 * but let's be conservative
		 */
		verbose(env, "cannot return stack pointer to the caller\n");
		return -EINVAL;
	}

	caller = state->frame[state->curframe - 1];
	if (callee->in_callback_fn) {
		if (r0->type != SCALAR_VALUE) {
			verbose(env, "R0 not a scalar value\n");
			return -EACCES;
		}

		/* we are going to rely on register's precise value */
		err = mark_chain_precision(env, BPF_REG_0);
		if (err)
			return err;

		/* enforce R0 return value range, and bpf_callback_t returns 64bit */
		if (!retval_range_within(callee->callback_ret_range, r0)) {
			verbose_invalid_scalar(env, r0, callee->callback_ret_range,
					       "At callback return", "R0");
			return -EINVAL;
		}
		if (!bpf_calls_callback(env, callee->callsite)) {
			verifier_bug(env, "in callback at %d, callsite %d !calls_callback",
				     *insn_idx, callee->callsite);
			return -EFAULT;
		}
	} else {
		/* return to the caller whatever r0 had in the callee */
		caller->regs[BPF_REG_0] = *r0;
	}