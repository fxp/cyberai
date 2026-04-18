// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 83/132



			/* Skip next '__sz' or '__szk' argument */
			i++;
			break;
		}
		case KF_ARG_PTR_TO_CALLBACK:
			if (reg->type != PTR_TO_FUNC) {
				verbose(env, "arg%d expected pointer to func\n", i);
				return -EINVAL;
			}
			meta->subprogno = reg->subprogno;
			break;
		case KF_ARG_PTR_TO_REFCOUNTED_KPTR:
			if (!type_is_ptr_alloc_obj(reg->type)) {
				verbose(env, "arg#%d is neither owning or non-owning ref\n", i);
				return -EINVAL;
			}
			if (!type_is_non_owning_ref(reg->type))
				meta->arg_owning_ref = true;

			rec = reg_btf_record(reg);
			if (!rec) {
				verifier_bug(env, "Couldn't find btf_record");
				return -EFAULT;
			}

			if (rec->refcount_off < 0) {
				verbose(env, "arg#%d doesn't point to a type with bpf_refcount field\n", i);
				return -EINVAL;
			}

			meta->arg_btf = reg->btf;
			meta->arg_btf_id = reg->btf_id;
			break;
		case KF_ARG_PTR_TO_CONST_STR:
			if (reg->type != PTR_TO_MAP_VALUE) {
				verbose(env, "arg#%d doesn't point to a const string\n", i);
				return -EINVAL;
			}
			ret = check_reg_const_str(env, reg, regno);
			if (ret)
				return ret;
			break;
		case KF_ARG_PTR_TO_WORKQUEUE:
			if (reg->type != PTR_TO_MAP_VALUE) {
				verbose(env, "arg#%d doesn't point to a map value\n", i);
				return -EINVAL;
			}
			ret = check_map_field_pointer(env, regno, BPF_WORKQUEUE, &meta->map);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_TIMER:
			if (reg->type != PTR_TO_MAP_VALUE) {
				verbose(env, "arg#%d doesn't point to a map value\n", i);
				return -EINVAL;
			}
			ret = process_timer_kfunc(env, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_TASK_WORK:
			if (reg->type != PTR_TO_MAP_VALUE) {
				verbose(env, "arg#%d doesn't point to a map value\n", i);
				return -EINVAL;
			}
			ret = check_map_field_pointer(env, regno, BPF_TASK_WORK, &meta->map);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_IRQ_FLAG:
			if (reg->type != PTR_TO_STACK) {
				verbose(env, "arg#%d doesn't point to an irq flag on stack\n", i);
				return -EINVAL;
			}
			ret = process_irq_flag(env, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_RES_SPIN_LOCK:
		{
			int flags = PROCESS_RES_LOCK;

			if (reg->type != PTR_TO_MAP_VALUE && reg->type != (PTR_TO_BTF_ID | MEM_ALLOC)) {
				verbose(env, "arg#%d doesn't point to map value or allocated object\n", i);
				return -EINVAL;
			}

			if (!is_bpf_res_spin_lock_kfunc(meta->func_id))
				return -EFAULT;
			if (meta->func_id == special_kfunc_list[KF_bpf_res_spin_lock] ||
			    meta->func_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave])
				flags |= PROCESS_SPIN_LOCK;
			if (meta->func_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave] ||
			    meta->func_id == special_kfunc_list[KF_bpf_res_spin_unlock_irqrestore])
				flags |= PROCESS_LOCK_IRQ;
			ret = process_spin_lock(env, regno, flags);
			if (ret < 0)
				return ret;
			break;
		}
		}
	}

	if (is_kfunc_release(meta) && !meta->release_regno) {
		verbose(env, "release kernel function %s expects refcounted PTR_TO_BTF_ID\n",
			func_name);
		return -EINVAL;
	}

	return 0;
}

int bpf_fetch_kfunc_arg_meta(struct bpf_verifier_env *env,
			     s32 func_id,
			     s16 offset,
			     struct bpf_kfunc_call_arg_meta *meta)
{
	struct bpf_kfunc_meta kfunc;
	int err;

	err = fetch_kfunc_meta(env, func_id, offset, &kfunc);
	if (err)
		return err;

	memset(meta, 0, sizeof(*meta));
	meta->btf = kfunc.btf;
	meta->func_id = kfunc.id;
	meta->func_proto = kfunc.proto;
	meta->func_name = kfunc.name;

	if (!kfunc.flags || !btf_kfunc_is_allowed(kfunc.btf, kfunc.id, env->prog))
		return -EACCES;

	meta->kfunc_flags = *kfunc.flags;

	return 0;
}

/*
 * Determine how many bytes a helper accesses through a stack pointer at
 * argument position @arg (0-based, corresponding to R1-R5).
 *
 * Returns:
 *   > 0   known read access size in bytes
 *     0   doesn't read anything directly
 * S64_MIN unknown
 *   < 0   known write access of (-return) bytes
 */
s64 bpf_helper_stack_access_bytes(struct bpf_verifier_env *env, struct bpf_insn *insn,
				  int arg, int insn_idx)
{
	struct bpf_insn_aux_data *aux = &env->insn_aux_data[insn_idx];
	const struct bpf_func_proto *fn;
	enum bpf_arg_type at;
	s64 size;

	if (bpf_get_helper_proto(env, insn->imm, &fn) < 0)
		return S64_MIN;

	at = fn->arg_type[arg];

	switch (base_type(at)) {
	case ARG_PTR_TO_MAP_KEY:
	case ARG_PTR_TO_MAP_VALUE: {
		bool is_key = base_type(at) == ARG_PTR_TO_MAP_KEY;
		u64 val;
		int i, map_reg;

		for (i = 0; i < arg; i++) {
			if (base_type(fn->arg_type[i]) == ARG_CONST_MAP_PTR)
				break;
		}
		if (i >= arg)
			goto scan_all_maps;

		map_reg = BPF_REG_1 + i;

		if (!(aux->const_reg_map_mask & BIT(map_reg)))
			goto scan_all_maps;