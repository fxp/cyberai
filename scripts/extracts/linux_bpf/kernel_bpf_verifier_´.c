// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 116/132



	/*
	 * Return from a regular global subprogram differs from return
	 * from the main program or async/exception callback.
	 * Main program exit implies return code restrictions
	 * that depend on program type.
	 * Exit from exception callback is equivalent to main program exit.
	 * Exit from async callback implies return code restrictions
	 * that depend on async scheduling mechanism.
	 */
	if (cur_frame->subprogno &&
	    !cur_frame->in_async_callback_fn &&
	    !cur_frame->in_exception_callback_fn)
		err = check_global_subprog_return_code(env);
	else
		err = check_return_code(env, BPF_REG_0, "R0");
	if (err)
		return err;
	return PROCESS_BPF_EXIT;
}

static int indirect_jump_min_max_index(struct bpf_verifier_env *env,
				       int regno,
				       struct bpf_map *map,
				       u32 *pmin_index, u32 *pmax_index)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	u64 min_index = reg->umin_value;
	u64 max_index = reg->umax_value;
	const u32 size = 8;

	if (min_index > (u64) U32_MAX * size) {
		verbose(env, "the sum of R%u umin_value %llu is too big\n", regno, reg->umin_value);
		return -ERANGE;
	}
	if (max_index > (u64) U32_MAX * size) {
		verbose(env, "the sum of R%u umax_value %llu is too big\n", regno, reg->umax_value);
		return -ERANGE;
	}

	min_index /= size;
	max_index /= size;

	if (max_index >= map->max_entries) {
		verbose(env, "R%u points to outside of jump table: [%llu,%llu] max_entries %u\n",
			     regno, min_index, max_index, map->max_entries);
		return -EINVAL;
	}

	*pmin_index = min_index;
	*pmax_index = max_index;
	return 0;
}

/* gotox *dst_reg */
static int check_indirect_jump(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	struct bpf_verifier_state *other_branch;
	struct bpf_reg_state *dst_reg;
	struct bpf_map *map;
	u32 min_index, max_index;
	int err = 0;
	int n;
	int i;

	dst_reg = reg_state(env, insn->dst_reg);
	if (dst_reg->type != PTR_TO_INSN) {
		verbose(env, "R%d has type %s, expected PTR_TO_INSN\n",
			     insn->dst_reg, reg_type_str(env, dst_reg->type));
		return -EINVAL;
	}

	map = dst_reg->map_ptr;
	if (verifier_bug_if(!map, env, "R%d has an empty map pointer", insn->dst_reg))
		return -EFAULT;

	if (verifier_bug_if(map->map_type != BPF_MAP_TYPE_INSN_ARRAY, env,
			    "R%d has incorrect map type %d", insn->dst_reg, map->map_type))
		return -EFAULT;

	err = indirect_jump_min_max_index(env, insn->dst_reg, map, &min_index, &max_index);
	if (err)
		return err;

	/* Ensure that the buffer is large enough */
	if (!env->gotox_tmp_buf || env->gotox_tmp_buf->cnt < max_index - min_index + 1) {
		env->gotox_tmp_buf = bpf_iarray_realloc(env->gotox_tmp_buf,
						        max_index - min_index + 1);
		if (!env->gotox_tmp_buf)
			return -ENOMEM;
	}

	n = bpf_copy_insn_array_uniq(map, min_index, max_index, env->gotox_tmp_buf->items);
	if (n < 0)
		return n;
	if (n == 0) {
		verbose(env, "register R%d doesn't point to any offset in map id=%d\n",
			     insn->dst_reg, map->id);
		return -EINVAL;
	}

	for (i = 0; i < n - 1; i++) {
		mark_indirect_target(env, env->gotox_tmp_buf->items[i]);
		other_branch = push_stack(env, env->gotox_tmp_buf->items[i],
					  env->insn_idx, env->cur_state->speculative);
		if (IS_ERR(other_branch))
			return PTR_ERR(other_branch);
	}
	env->insn_idx = env->gotox_tmp_buf->items[n-1];
	mark_indirect_target(env, env->insn_idx);
	return INSN_IDX_UPDATED;
}

static int do_check_insn(struct bpf_verifier_env *env, bool *do_print_state)
{
	int err;
	struct bpf_insn *insn = &env->prog->insnsi[env->insn_idx];
	u8 class = BPF_CLASS(insn->code);

	switch (class) {
	case BPF_ALU:
	case BPF_ALU64:
		return check_alu_op(env, insn);

	case BPF_LDX:
		return check_load_mem(env, insn, false,
				      BPF_MODE(insn->code) == BPF_MEMSX,
				      true, "ldx");

	case BPF_STX:
		if (BPF_MODE(insn->code) == BPF_ATOMIC)
			return check_atomic(env, insn);
		return check_store_reg(env, insn, false);

	case BPF_ST: {
		enum bpf_reg_type dst_reg_type;

		err = check_reg_arg(env, insn->dst_reg, SRC_OP);
		if (err)
			return err;

		dst_reg_type = cur_regs(env)[insn->dst_reg].type;

		err = check_mem_access(env, env->insn_idx, insn->dst_reg,
				       insn->off, BPF_SIZE(insn->code),
				       BPF_WRITE, -1, false, false);
		if (err)
			return err;

		return save_aux_ptr_type(env, dst_reg_type, false);
	}
	case BPF_JMP:
	case BPF_JMP32: {
		u8 opcode = BPF_OP(insn->code);