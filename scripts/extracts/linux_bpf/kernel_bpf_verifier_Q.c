// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 49/132



	if (!is_const) {
		verbose(env,
			"R%d doesn't have constant offset. %s has to be at the constant offset\n",
			regno, struct_name);
		return -EINVAL;
	}
	if (!map->btf) {
		verbose(env, "map '%s' has to have BTF in order to use %s\n", map->name,
			struct_name);
		return -EINVAL;
	}
	if (!btf_record_has_field(map->record, field_type)) {
		verbose(env, "map '%s' has no valid %s\n", map->name, struct_name);
		return -EINVAL;
	}
	switch (field_type) {
	case BPF_TIMER:
		field_off = map->record->timer_off;
		break;
	case BPF_TASK_WORK:
		field_off = map->record->task_work_off;
		break;
	case BPF_WORKQUEUE:
		field_off = map->record->wq_off;
		break;
	default:
		verifier_bug(env, "unsupported BTF field type: %s\n", struct_name);
		return -EINVAL;
	}
	if (field_off != val) {
		verbose(env, "off %lld doesn't point to 'struct %s' that is at %d\n",
			val, struct_name, field_off);
		return -EINVAL;
	}
	if (map_desc->ptr) {
		verifier_bug(env, "Two map pointers in a %s helper", struct_name);
		return -EFAULT;
	}
	map_desc->uid = reg->map_uid;
	map_desc->ptr = map;
	return 0;
}

static int process_timer_func(struct bpf_verifier_env *env, int regno,
			      struct bpf_map_desc *map)
{
	if (IS_ENABLED(CONFIG_PREEMPT_RT)) {
		verbose(env, "bpf_timer cannot be used for PREEMPT_RT.\n");
		return -EOPNOTSUPP;
	}
	return check_map_field_pointer(env, regno, BPF_TIMER, map);
}

static int process_timer_helper(struct bpf_verifier_env *env, int regno,
				struct bpf_call_arg_meta *meta)
{
	return process_timer_func(env, regno, &meta->map);
}

static int process_timer_kfunc(struct bpf_verifier_env *env, int regno,
			       struct bpf_kfunc_call_arg_meta *meta)
{
	return process_timer_func(env, regno, &meta->map);
}

static int process_kptr_func(struct bpf_verifier_env *env, int regno,
			     struct bpf_call_arg_meta *meta)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct btf_field *kptr_field;
	struct bpf_map *map_ptr;
	struct btf_record *rec;
	u32 kptr_off;

	if (type_is_ptr_alloc_obj(reg->type)) {
		rec = reg_btf_record(reg);
	} else { /* PTR_TO_MAP_VALUE */
		map_ptr = reg->map_ptr;
		if (!map_ptr->btf) {
			verbose(env, "map '%s' has to have BTF in order to use bpf_kptr_xchg\n",
				map_ptr->name);
			return -EINVAL;
		}
		rec = map_ptr->record;
		meta->map.ptr = map_ptr;
	}

	if (!tnum_is_const(reg->var_off)) {
		verbose(env,
			"R%d doesn't have constant offset. kptr has to be at the constant offset\n",
			regno);
		return -EINVAL;
	}

	if (!btf_record_has_field(rec, BPF_KPTR)) {
		verbose(env, "R%d has no valid kptr\n", regno);
		return -EINVAL;
	}

	kptr_off = reg->var_off.value;
	kptr_field = btf_record_find(rec, kptr_off, BPF_KPTR);
	if (!kptr_field) {
		verbose(env, "off=%d doesn't point to kptr\n", kptr_off);
		return -EACCES;
	}
	if (kptr_field->type != BPF_KPTR_REF && kptr_field->type != BPF_KPTR_PERCPU) {
		verbose(env, "off=%d kptr isn't referenced kptr\n", kptr_off);
		return -EACCES;
	}
	meta->kptr_field = kptr_field;
	return 0;
}

/* There are two register types representing a bpf_dynptr, one is PTR_TO_STACK
 * which points to a stack slot, and the other is CONST_PTR_TO_DYNPTR.
 *
 * In both cases we deal with the first 8 bytes, but need to mark the next 8
 * bytes as STACK_DYNPTR in case of PTR_TO_STACK. In case of
 * CONST_PTR_TO_DYNPTR, we are guaranteed to get the beginning of the object.
 *
 * Mutability of bpf_dynptr is at two levels, one is at the level of struct
 * bpf_dynptr itself, i.e. whether the helper is receiving a pointer to struct
 * bpf_dynptr or pointer to const struct bpf_dynptr. In the former case, it can
 * mutate the view of the dynptr and also possibly destroy it. In the latter
 * case, it cannot mutate the bpf_dynptr itself but it can still mutate the
 * memory that dynptr points to.
 *
 * The verifier will keep track both levels of mutation (bpf_dynptr's in
 * reg->type and the memory's in reg->dynptr.type), but there is no support for
 * readonly dynptr view yet, hence only the first case is tracked and checked.
 *
 * This is consistent with how C applies the const modifier to a struct object,
 * where the pointer itself inside bpf_dynptr becomes const but not what it
 * points to.
 *
 * Helpers which do not mutate the bpf_dynptr set MEM_RDONLY in their argument
 * type, and declare it as 'const struct bpf_dynptr *' in their prototype.
 */
static int process_dynptr_func(struct bpf_verifier_env *env, int regno, int insn_idx,
			       enum bpf_arg_type arg_type, int clone_ref_obj_id)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	int err;

	if (reg->type != PTR_TO_STACK && reg->type != CONST_PTR_TO_DYNPTR) {
		verbose(env,
			"arg#%d expected pointer to stack or const struct bpf_dynptr\n",
			regno - 1);
		return -EINVAL;
	}