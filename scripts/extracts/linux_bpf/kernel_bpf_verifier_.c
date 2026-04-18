// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 84/132



		i = aux->const_reg_vals[map_reg];
		if (i < env->used_map_cnt) {
			size = is_key ? env->used_maps[i]->key_size
				      : env->used_maps[i]->value_size;
			goto out;
		}
scan_all_maps:
		/*
		 * Map pointer is not known at this call site (e.g. different
		 * maps on merged paths).  Conservatively return the largest
		 * key_size or value_size across all maps used by the program.
		 */
		val = 0;
		for (i = 0; i < env->used_map_cnt; i++) {
			struct bpf_map *map = env->used_maps[i];
			u32 sz = is_key ? map->key_size : map->value_size;

			if (sz > val)
				val = sz;
			if (map->inner_map_meta) {
				sz = is_key ? map->inner_map_meta->key_size
					    : map->inner_map_meta->value_size;
				if (sz > val)
					val = sz;
			}
		}
		if (!val)
			return S64_MIN;
		size = val;
		goto out;
	}
	case ARG_PTR_TO_MEM:
		if (at & MEM_FIXED_SIZE) {
			size = fn->arg_size[arg];
			goto out;
		}
		if (arg + 1 < ARRAY_SIZE(fn->arg_type) &&
		    arg_type_is_mem_size(fn->arg_type[arg + 1])) {
			int size_reg = BPF_REG_1 + arg + 1;

			if (aux->const_reg_mask & BIT(size_reg)) {
				size = (s64)aux->const_reg_vals[size_reg];
				goto out;
			}
			/*
			 * Size arg is const on each path but differs across merged
			 * paths. MAX_BPF_STACK is a safe upper bound for reads.
			 */
			if (at & MEM_UNINIT)
				return 0;
			return MAX_BPF_STACK;
		}
		return S64_MIN;
	case ARG_PTR_TO_DYNPTR:
		size = BPF_DYNPTR_SIZE;
		break;
	case ARG_PTR_TO_STACK:
		/*
		 * Only used by bpf_calls_callback() helpers. The helper itself
		 * doesn't access stack. The callback subprog does and it's
		 * analyzed separately.
		 */
		return 0;
	default:
		return S64_MIN;
	}
out:
	/*
	 * MEM_UNINIT args are write-only: the helper initializes the
	 * buffer without reading it.
	 */
	if (at & MEM_UNINIT)
		return -size;
	return size;
}

/*
 * Determine how many bytes a kfunc accesses through a stack pointer at
 * argument position @arg (0-based, corresponding to R1-R5).
 *
 * Returns:
 *   > 0      known read access size in bytes
 *     0      doesn't access memory through that argument (ex: not a pointer)
 *   S64_MIN  unknown
 *   < 0      known write access of (-return) bytes
 */
s64 bpf_kfunc_stack_access_bytes(struct bpf_verifier_env *env, struct bpf_insn *insn,
				 int arg, int insn_idx)
{
	struct bpf_insn_aux_data *aux = &env->insn_aux_data[insn_idx];
	struct bpf_kfunc_call_arg_meta meta;
	const struct btf_param *args;
	const struct btf_type *t, *ref_t;
	const struct btf *btf;
	u32 nargs, type_size;
	s64 size;

	if (bpf_fetch_kfunc_arg_meta(env, insn->imm, insn->off, &meta) < 0)
		return S64_MIN;

	btf = meta.btf;
	args = btf_params(meta.func_proto);
	nargs = btf_type_vlen(meta.func_proto);
	if (arg >= nargs)
		return 0;

	t = btf_type_skip_modifiers(btf, args[arg].type, NULL);
	if (!btf_type_is_ptr(t))
		return 0;

	/* dynptr: fixed 16-byte on-stack representation */
	if (is_kfunc_arg_dynptr(btf, &args[arg])) {
		size = BPF_DYNPTR_SIZE;
		goto out;
	}

	/* ptr + __sz/__szk pair: size is in the next register */
	if (arg + 1 < nargs &&
	    (btf_param_match_suffix(btf, &args[arg + 1], "__sz") ||
	     btf_param_match_suffix(btf, &args[arg + 1], "__szk"))) {
		int size_reg = BPF_REG_1 + arg + 1;

		if (aux->const_reg_mask & BIT(size_reg)) {
			size = (s64)aux->const_reg_vals[size_reg];
			goto out;
		}
		return MAX_BPF_STACK;
	}

	/* fixed-size pointed-to type: resolve via BTF */
	ref_t = btf_type_skip_modifiers(btf, t->type, NULL);
	if (!IS_ERR(btf_resolve_size(btf, ref_t, &type_size))) {
		size = type_size;
		goto out;
	}

	return S64_MIN;
out:
	/* KF_ITER_NEW kfuncs initialize the iterator state at arg 0 */
	if (arg == 0 && meta.kfunc_flags & KF_ITER_NEW)
		return -size;
	if (is_kfunc_arg_uninit(btf, &args[arg]))
		return -size;
	return size;
}

/* check special kfuncs and return:
 *  1  - not fall-through to 'else' branch, continue verification
 *  0  - fall-through to 'else' branch
 * < 0 - not fall-through to 'else' branch, return error
 */
static int check_special_kfunc(struct bpf_verifier_env *env, struct bpf_kfunc_call_arg_meta *meta,
			       struct bpf_reg_state *regs, struct bpf_insn_aux_data *insn_aux,
			       const struct btf_type *ptr_type, struct btf *desc_btf)
{
	const struct btf_type *ret_t;
	int err = 0;

	if (meta->btf != btf_vmlinux)
		return 0;

	if (is_bpf_obj_new_kfunc(meta->func_id) || is_bpf_percpu_obj_new_kfunc(meta->func_id)) {
		struct btf_struct_meta *struct_meta;
		struct btf *ret_btf;
		u32 ret_btf_id;

		if (is_bpf_obj_new_kfunc(meta->func_id) && !bpf_global_ma_set)
			return -ENOMEM;

		if (((u64)(u32)meta->arg_constant.value) != meta->arg_constant.value) {
			verbose(env, "local type ID argument must be in range [0, U32_MAX]\n");
			return -EINVAL;
		}

		ret_btf = env->prog->aux->btf;
		ret_btf_id = meta->arg_constant.value;

		/* This may be NULL due to user not supplying a BTF */
		if (!ret_btf) {
			verbose(env, "bpf_obj_new/bpf_percpu_obj_new requires prog BTF\n");
			return -EINVAL;
		}