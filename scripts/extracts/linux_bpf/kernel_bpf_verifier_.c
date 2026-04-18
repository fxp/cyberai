// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 88/132



			if (meta.func_id == special_kfunc_list[KF_bpf_get_kmem_cache])
				type |= PTR_UNTRUSTED;
			else if (is_kfunc_rcu_protected(&meta) ||
				 (bpf_is_iter_next_kfunc(&meta) &&
				  (get_iter_from_state(env->cur_state, &meta)
					   ->type & MEM_RCU))) {
				/*
				 * If the iterator's constructor (the _new
				 * function e.g., bpf_iter_task_new) has been
				 * annotated with BPF kfunc flag
				 * KF_RCU_PROTECTED and was called within a RCU
				 * read-side critical section, also propagate
				 * the MEM_RCU flag to the pointer returned from
				 * the iterator's next function (e.g.,
				 * bpf_iter_task_next).
				 */
				type |= MEM_RCU;
			} else {
				/*
				 * Any PTR_TO_BTF_ID that is returned from a BPF
				 * kfunc should by default be treated as
				 * implicitly trusted.
				 */
				type |= PTR_TRUSTED;
			}

			mark_reg_known_zero(env, regs, BPF_REG_0);
			regs[BPF_REG_0].btf = desc_btf;
			regs[BPF_REG_0].type = type;
			regs[BPF_REG_0].btf_id = ptr_type_id;
		}

		if (is_kfunc_ret_null(&meta)) {
			regs[BPF_REG_0].type |= PTR_MAYBE_NULL;
			/* For mark_ptr_or_null_reg, see 93c230e3f5bd6 */
			regs[BPF_REG_0].id = ++env->id_gen;
		}
		mark_btf_func_reg_size(env, BPF_REG_0, sizeof(void *));
		if (is_kfunc_acquire(&meta)) {
			int id = acquire_reference(env, insn_idx);

			if (id < 0)
				return id;
			if (is_kfunc_ret_null(&meta))
				regs[BPF_REG_0].id = id;
			regs[BPF_REG_0].ref_obj_id = id;
		} else if (is_rbtree_node_type(ptr_type) || is_list_node_type(ptr_type)) {
			ref_set_non_owning(env, &regs[BPF_REG_0]);
		}

		if (reg_may_point_to_spin_lock(&regs[BPF_REG_0]) && !regs[BPF_REG_0].id)
			regs[BPF_REG_0].id = ++env->id_gen;
	} else if (btf_type_is_void(t)) {
		if (meta.btf == btf_vmlinux) {
			if (is_bpf_obj_drop_kfunc(meta.func_id) ||
			    is_bpf_percpu_obj_drop_kfunc(meta.func_id)) {
				insn_aux->kptr_struct_meta =
					btf_find_struct_meta(meta.arg_btf,
							     meta.arg_btf_id);
			}
		}
	}

	if (bpf_is_kfunc_pkt_changing(&meta))
		clear_all_pkt_pointers(env);

	nargs = btf_type_vlen(meta.func_proto);
	args = (const struct btf_param *)(meta.func_proto + 1);
	for (i = 0; i < nargs; i++) {
		u32 regno = i + 1;

		t = btf_type_skip_modifiers(desc_btf, args[i].type, NULL);
		if (btf_type_is_ptr(t))
			mark_btf_func_reg_size(env, regno, sizeof(void *));
		else
			/* scalar. ensured by check_kfunc_args() */
			mark_btf_func_reg_size(env, regno, t->size);
	}

	if (bpf_is_iter_next_kfunc(&meta)) {
		err = process_iter_next_call(env, insn_idx, &meta);
		if (err)
			return err;
	}

	if (meta.func_id == special_kfunc_list[KF_bpf_session_cookie])
		env->prog->call_session_cookie = true;

	if (is_bpf_throw_kfunc(insn))
		return process_bpf_exit_full(env, NULL, true);

	return 0;
}

static bool check_reg_sane_offset_scalar(struct bpf_verifier_env *env,
					 const struct bpf_reg_state *reg,
					 enum bpf_reg_type type)
{
	bool known = tnum_is_const(reg->var_off);
	s64 val = reg->var_off.value;
	s64 smin = reg->smin_value;

	if (known && (val >= BPF_MAX_VAR_OFF || val <= -BPF_MAX_VAR_OFF)) {
		verbose(env, "math between %s pointer and %lld is not allowed\n",
			reg_type_str(env, type), val);
		return false;
	}

	if (smin == S64_MIN) {
		verbose(env, "math between %s pointer and register with unbounded min value is not allowed\n",
			reg_type_str(env, type));
		return false;
	}

	if (smin >= BPF_MAX_VAR_OFF || smin <= -BPF_MAX_VAR_OFF) {
		verbose(env, "value %lld makes %s pointer be out of bounds\n",
			smin, reg_type_str(env, type));
		return false;
	}

	return true;
}

static bool check_reg_sane_offset_ptr(struct bpf_verifier_env *env,
				      const struct bpf_reg_state *reg,
				      enum bpf_reg_type type)
{
	bool known = tnum_is_const(reg->var_off);
	s64 val = reg->var_off.value;
	s64 smin = reg->smin_value;

	if (known && (val >= BPF_MAX_VAR_OFF || val <= -BPF_MAX_VAR_OFF)) {
		verbose(env, "%s pointer offset %lld is not allowed\n",
			reg_type_str(env, type), val);
		return false;
	}

	if (smin >= BPF_MAX_VAR_OFF || smin <= -BPF_MAX_VAR_OFF) {
		verbose(env, "%s pointer offset %lld is not allowed\n",
			reg_type_str(env, type), smin);
		return false;
	}

	return true;
}

enum {
	REASON_BOUNDS	= -1,
	REASON_TYPE	= -2,
	REASON_PATHS	= -3,
	REASON_LIMIT	= -4,
	REASON_STACK	= -5,
};

static int retrieve_ptr_limit(const struct bpf_reg_state *ptr_reg,
			      u32 *alu_limit, bool mask_to_left)
{
	u32 max = 0, ptr_limit = 0;

	switch (ptr_reg->type) {
	case PTR_TO_STACK:
		/* Offset 0 is out-of-bounds, but acceptable start for the
		 * left direction, see BPF_REG_FP. Also, unknown scalar
		 * offset where we would need to deal with min/max bounds is
		 * currently prohibited for unprivileged.
		 */
		max = MAX_BPF_STACK + mask_to_left;
		ptr_limit = -ptr_reg->var_off.value;
		break;
	case PTR_TO_MAP_VALUE:
		max = ptr_reg->map_ptr->value_size;
		ptr_limit = mask_to_left ? ptr_reg->smin_value : ptr_reg->umax_value;
		break;
	default:
		return REASON_TYPE;
	}