// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 50/132



	/* MEM_UNINIT and MEM_RDONLY are exclusive, when applied to an
	 * ARG_PTR_TO_DYNPTR (or ARG_PTR_TO_DYNPTR | DYNPTR_TYPE_*):
	 */
	if ((arg_type & (MEM_UNINIT | MEM_RDONLY)) == (MEM_UNINIT | MEM_RDONLY)) {
		verifier_bug(env, "misconfigured dynptr helper type flags");
		return -EFAULT;
	}

	/*  MEM_UNINIT - Points to memory that is an appropriate candidate for
	 *		 constructing a mutable bpf_dynptr object.
	 *
	 *		 Currently, this is only possible with PTR_TO_STACK
	 *		 pointing to a region of at least 16 bytes which doesn't
	 *		 contain an existing bpf_dynptr.
	 *
	 *  MEM_RDONLY - Points to a initialized bpf_dynptr that will not be
	 *		 mutated or destroyed. However, the memory it points to
	 *		 may be mutated.
	 *
	 *  None       - Points to a initialized dynptr that can be mutated and
	 *		 destroyed, including mutation of the memory it points
	 *		 to.
	 */
	if (arg_type & MEM_UNINIT) {
		int i;

		if (!is_dynptr_reg_valid_uninit(env, reg)) {
			verbose(env, "Dynptr has to be an uninitialized dynptr\n");
			return -EINVAL;
		}

		/* we write BPF_DW bits (8 bytes) at a time */
		for (i = 0; i < BPF_DYNPTR_SIZE; i += 8) {
			err = check_mem_access(env, insn_idx, regno,
					       i, BPF_DW, BPF_WRITE, -1, false, false);
			if (err)
				return err;
		}

		err = mark_stack_slots_dynptr(env, reg, arg_type, insn_idx, clone_ref_obj_id);
	} else /* MEM_RDONLY and None case from above */ {
		/* For the reg->type == PTR_TO_STACK case, bpf_dynptr is never const */
		if (reg->type == CONST_PTR_TO_DYNPTR && !(arg_type & MEM_RDONLY)) {
			verbose(env, "cannot pass pointer to const bpf_dynptr, the helper mutates it\n");
			return -EINVAL;
		}

		if (!is_dynptr_reg_valid_init(env, reg)) {
			verbose(env,
				"Expected an initialized dynptr as arg #%d\n",
				regno - 1);
			return -EINVAL;
		}

		/* Fold modifiers (in this case, MEM_RDONLY) when checking expected type */
		if (!is_dynptr_type_expected(env, reg, arg_type & ~MEM_RDONLY)) {
			verbose(env,
				"Expected a dynptr of type %s as arg #%d\n",
				dynptr_type_str(arg_to_dynptr_type(arg_type)), regno - 1);
			return -EINVAL;
		}

		err = mark_dynptr_read(env, reg);
	}
	return err;
}

static u32 iter_ref_obj_id(struct bpf_verifier_env *env, struct bpf_reg_state *reg, int spi)
{
	struct bpf_func_state *state = bpf_func(env, reg);

	return state->stack[spi].spilled_ptr.ref_obj_id;
}

static bool is_iter_kfunc(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & (KF_ITER_NEW | KF_ITER_NEXT | KF_ITER_DESTROY);
}

static bool is_iter_new_kfunc(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_ITER_NEW;
}


static bool is_iter_destroy_kfunc(struct bpf_kfunc_call_arg_meta *meta)
{
	return meta->kfunc_flags & KF_ITER_DESTROY;
}

static bool is_kfunc_arg_iter(struct bpf_kfunc_call_arg_meta *meta, int arg_idx,
			      const struct btf_param *arg)
{
	/* btf_check_iter_kfuncs() guarantees that first argument of any iter
	 * kfunc is iter state pointer
	 */
	if (is_iter_kfunc(meta))
		return arg_idx == 0;

	/* iter passed as an argument to a generic kfunc */
	return btf_param_match_suffix(meta->btf, arg, "__iter");
}

static int process_iter_arg(struct bpf_verifier_env *env, int regno, int insn_idx,
			    struct bpf_kfunc_call_arg_meta *meta)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	const struct btf_type *t;
	int spi, err, i, nr_slots, btf_id;

	if (reg->type != PTR_TO_STACK) {
		verbose(env, "arg#%d expected pointer to an iterator on stack\n", regno - 1);
		return -EINVAL;
	}

	/* For iter_{new,next,destroy} functions, btf_check_iter_kfuncs()
	 * ensures struct convention, so we wouldn't need to do any BTF
	 * validation here. But given iter state can be passed as a parameter
	 * to any kfunc, if arg has "__iter" suffix, we need to be a bit more
	 * conservative here.
	 */
	btf_id = btf_check_iter_arg(meta->btf, meta->func_proto, regno - 1);
	if (btf_id < 0) {
		verbose(env, "expected valid iter pointer as arg #%d\n", regno - 1);
		return -EINVAL;
	}
	t = btf_type_by_id(meta->btf, btf_id);
	nr_slots = t->size / BPF_REG_SIZE;

	if (is_iter_new_kfunc(meta)) {
		/* bpf_iter_<type>_new() expects pointer to uninit iter state */
		if (!is_iter_reg_valid_uninit(env, reg, nr_slots)) {
			verbose(env, "expected uninitialized iter_%s as arg #%d\n",
				iter_type_str(meta->btf, btf_id), regno - 1);
			return -EINVAL;
		}

		for (i = 0; i < nr_slots * 8; i += BPF_REG_SIZE) {
			err = check_mem_access(env, insn_idx, regno,
					       i, BPF_DW, BPF_WRITE, -1, false, false);
			if (err)
				return err;
		}