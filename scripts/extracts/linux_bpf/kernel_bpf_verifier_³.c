// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 115/132



	bpf_for_each_reg_in_vstate(st, func, reg, ({
		if (reg->type != SCALAR_VALUE)
			continue;
		if (!reg->id)
			continue;
		idset_cnt_inc(idset, reg->id & ~BPF_ADD_CONST);
	}));

	bpf_for_each_reg_in_vstate(st, func, reg, ({
		if (reg->type != SCALAR_VALUE)
			continue;
		if (!reg->id)
			continue;
		if (idset_cnt_get(idset, reg->id & ~BPF_ADD_CONST) == 1)
			clear_scalar_id(reg);
	}));
}

/* Return true if it's OK to have the same insn return a different type. */
static bool reg_type_mismatch_ok(enum bpf_reg_type type)
{
	switch (base_type(type)) {
	case PTR_TO_CTX:
	case PTR_TO_SOCKET:
	case PTR_TO_SOCK_COMMON:
	case PTR_TO_TCP_SOCK:
	case PTR_TO_XDP_SOCK:
	case PTR_TO_BTF_ID:
	case PTR_TO_ARENA:
		return false;
	default:
		return true;
	}
}

/* If an instruction was previously used with particular pointer types, then we
 * need to be careful to avoid cases such as the below, where it may be ok
 * for one branch accessing the pointer, but not ok for the other branch:
 *
 * R1 = sock_ptr
 * goto X;
 * ...
 * R1 = some_other_valid_ptr;
 * goto X;
 * ...
 * R2 = *(u32 *)(R1 + 0);
 */
static bool reg_type_mismatch(enum bpf_reg_type src, enum bpf_reg_type prev)
{
	return src != prev && (!reg_type_mismatch_ok(src) ||
			       !reg_type_mismatch_ok(prev));
}

static bool is_ptr_to_mem_or_btf_id(enum bpf_reg_type type)
{
	switch (base_type(type)) {
	case PTR_TO_MEM:
	case PTR_TO_BTF_ID:
		return true;
	default:
		return false;
	}
}

static bool is_ptr_to_mem(enum bpf_reg_type type)
{
	return base_type(type) == PTR_TO_MEM;
}

static int save_aux_ptr_type(struct bpf_verifier_env *env, enum bpf_reg_type type,
			     bool allow_trust_mismatch)
{
	enum bpf_reg_type *prev_type = &env->insn_aux_data[env->insn_idx].ptr_type;
	enum bpf_reg_type merged_type;

	if (*prev_type == NOT_INIT) {
		/* Saw a valid insn
		 * dst_reg = *(u32 *)(src_reg + off)
		 * save type to validate intersecting paths
		 */
		*prev_type = type;
	} else if (reg_type_mismatch(type, *prev_type)) {
		/* Abuser program is trying to use the same insn
		 * dst_reg = *(u32*) (src_reg + off)
		 * with different pointer types:
		 * src_reg == ctx in one branch and
		 * src_reg == stack|map in some other branch.
		 * Reject it.
		 */
		if (allow_trust_mismatch &&
		    is_ptr_to_mem_or_btf_id(type) &&
		    is_ptr_to_mem_or_btf_id(*prev_type)) {
			/*
			 * Have to support a use case when one path through
			 * the program yields TRUSTED pointer while another
			 * is UNTRUSTED. Fallback to UNTRUSTED to generate
			 * BPF_PROBE_MEM/BPF_PROBE_MEMSX.
			 * Same behavior of MEM_RDONLY flag.
			 */
			if (is_ptr_to_mem(type) || is_ptr_to_mem(*prev_type))
				merged_type = PTR_TO_MEM;
			else
				merged_type = PTR_TO_BTF_ID;
			if ((type & PTR_UNTRUSTED) || (*prev_type & PTR_UNTRUSTED))
				merged_type |= PTR_UNTRUSTED;
			if ((type & MEM_RDONLY) || (*prev_type & MEM_RDONLY))
				merged_type |= MEM_RDONLY;
			*prev_type = merged_type;
		} else {
			verbose(env, "same insn cannot be used with different pointers\n");
			return -EINVAL;
		}
	}

	return 0;
}

enum {
	PROCESS_BPF_EXIT = 1,
	INSN_IDX_UPDATED = 2,
};

static int process_bpf_exit_full(struct bpf_verifier_env *env,
				 bool *do_print_state,
				 bool exception_exit)
{
	struct bpf_func_state *cur_frame = cur_func(env);

	/* We must do check_reference_leak here before
	 * prepare_func_exit to handle the case when
	 * state->curframe > 0, it may be a callback function,
	 * for which reference_state must match caller reference
	 * state when it exits.
	 */
	int err = check_resource_leak(env, exception_exit,
				      exception_exit || !env->cur_state->curframe,
				      exception_exit ? "bpf_throw" :
				      "BPF_EXIT instruction in main prog");
	if (err)
		return err;

	/* The side effect of the prepare_func_exit which is
	 * being skipped is that it frees bpf_func_state.
	 * Typically, process_bpf_exit will only be hit with
	 * outermost exit. copy_verifier_state in pop_stack will
	 * handle freeing of any extra bpf_func_state left over
	 * from not processing all nested function exits. We
	 * also skip return code checks as they are not needed
	 * for exceptional exits.
	 */
	if (exception_exit)
		return PROCESS_BPF_EXIT;

	if (env->cur_state->curframe) {
		/* exit from nested function */
		err = prepare_func_exit(env, &env->insn_idx);
		if (err)
			return err;
		*do_print_state = true;
		return INSN_IDX_UPDATED;
	}