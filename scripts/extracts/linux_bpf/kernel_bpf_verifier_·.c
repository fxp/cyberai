// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 119/132



	datasec_id = find_btf_percpu_datasec(btf);
	if (datasec_id > 0) {
		datasec = btf_type_by_id(btf, datasec_id);
		for_each_vsi(i, datasec, vsi) {
			if (vsi->type == id) {
				percpu = true;
				break;
			}
		}
	}

	type = t->type;
	t = btf_type_skip_modifiers(btf, type, NULL);
	if (percpu) {
		aux->btf_var.reg_type = PTR_TO_BTF_ID | MEM_PERCPU;
		aux->btf_var.btf = btf;
		aux->btf_var.btf_id = type;
	} else if (!btf_type_is_struct(t)) {
		const struct btf_type *ret;
		const char *tname;
		u32 tsize;

		/* resolve the type size of ksym. */
		ret = btf_resolve_size(btf, t, &tsize);
		if (IS_ERR(ret)) {
			tname = btf_name_by_offset(btf, t->name_off);
			verbose(env, "ldimm64 unable to resolve the size of type '%s': %ld\n",
				tname, PTR_ERR(ret));
			return -EINVAL;
		}
		aux->btf_var.reg_type = PTR_TO_MEM | MEM_RDONLY;
		aux->btf_var.mem_size = tsize;
	} else {
		aux->btf_var.reg_type = PTR_TO_BTF_ID;
		aux->btf_var.btf = btf;
		aux->btf_var.btf_id = type;
	}

	return 0;
}

static int check_pseudo_btf_id(struct bpf_verifier_env *env,
			       struct bpf_insn *insn,
			       struct bpf_insn_aux_data *aux)
{
	struct btf *btf;
	int btf_fd;
	int err;

	btf_fd = insn[1].imm;
	if (btf_fd) {
		btf = btf_get_by_fd(btf_fd);
		if (IS_ERR(btf)) {
			verbose(env, "invalid module BTF object FD specified.\n");
			return -EINVAL;
		}
	} else {
		if (!btf_vmlinux) {
			verbose(env, "kernel is missing BTF, make sure CONFIG_DEBUG_INFO_BTF=y is specified in Kconfig.\n");
			return -EINVAL;
		}
		btf_get(btf_vmlinux);
		btf = btf_vmlinux;
	}

	err = __check_pseudo_btf_id(env, insn, aux, btf);
	if (err) {
		btf_put(btf);
		return err;
	}

	return __add_used_btf(env, btf);
}

static bool is_tracing_prog_type(enum bpf_prog_type type)
{
	switch (type) {
	case BPF_PROG_TYPE_KPROBE:
	case BPF_PROG_TYPE_TRACEPOINT:
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE:
		return true;
	default:
		return false;
	}
}

static bool bpf_map_is_cgroup_storage(struct bpf_map *map)
{
	return (map->map_type == BPF_MAP_TYPE_CGROUP_STORAGE ||
		map->map_type == BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE);
}

static int check_map_prog_compatibility(struct bpf_verifier_env *env,
					struct bpf_map *map,
					struct bpf_prog *prog)

{
	enum bpf_prog_type prog_type = resolve_prog_type(prog);

	if (map->excl_prog_sha &&
	    memcmp(map->excl_prog_sha, prog->digest, SHA256_DIGEST_SIZE)) {
		verbose(env, "program's hash doesn't match map's excl_prog_hash\n");
		return -EACCES;
	}

	if (btf_record_has_field(map->record, BPF_LIST_HEAD) ||
	    btf_record_has_field(map->record, BPF_RB_ROOT)) {
		if (is_tracing_prog_type(prog_type)) {
			verbose(env, "tracing progs cannot use bpf_{list_head,rb_root} yet\n");
			return -EINVAL;
		}
	}

	if (btf_record_has_field(map->record, BPF_SPIN_LOCK | BPF_RES_SPIN_LOCK)) {
		if (prog_type == BPF_PROG_TYPE_SOCKET_FILTER) {
			verbose(env, "socket filter progs cannot use bpf_spin_lock yet\n");
			return -EINVAL;
		}

		if (is_tracing_prog_type(prog_type)) {
			verbose(env, "tracing progs cannot use bpf_spin_lock yet\n");
			return -EINVAL;
		}
	}

	if ((bpf_prog_is_offloaded(prog->aux) || bpf_map_is_offloaded(map)) &&
	    !bpf_offload_prog_map_match(prog, map)) {
		verbose(env, "offload device mismatch between prog and map\n");
		return -EINVAL;
	}

	if (map->map_type == BPF_MAP_TYPE_STRUCT_OPS) {
		verbose(env, "bpf_struct_ops map cannot be used in prog\n");
		return -EINVAL;
	}

	if (prog->sleepable)
		switch (map->map_type) {
		case BPF_MAP_TYPE_HASH:
		case BPF_MAP_TYPE_LRU_HASH:
		case BPF_MAP_TYPE_ARRAY:
		case BPF_MAP_TYPE_PERCPU_HASH:
		case BPF_MAP_TYPE_PERCPU_ARRAY:
		case BPF_MAP_TYPE_LRU_PERCPU_HASH:
		case BPF_MAP_TYPE_ARRAY_OF_MAPS:
		case BPF_MAP_TYPE_HASH_OF_MAPS:
		case BPF_MAP_TYPE_RINGBUF:
		case BPF_MAP_TYPE_USER_RINGBUF:
		case BPF_MAP_TYPE_INODE_STORAGE:
		case BPF_MAP_TYPE_SK_STORAGE:
		case BPF_MAP_TYPE_TASK_STORAGE:
		case BPF_MAP_TYPE_CGRP_STORAGE:
		case BPF_MAP_TYPE_QUEUE:
		case BPF_MAP_TYPE_STACK:
		case BPF_MAP_TYPE_ARENA:
		case BPF_MAP_TYPE_INSN_ARRAY:
		case BPF_MAP_TYPE_PROG_ARRAY:
			break;
		default:
			verbose(env,
				"Sleepable programs can only use array, hash, ringbuf and local storage maps\n");
			return -EINVAL;
		}

	if (bpf_map_is_cgroup_storage(map) &&
	    bpf_cgroup_storage_assign(env->prog->aux, map)) {
		verbose(env, "only one cgroup storage of each type is allowed\n");
		return -EBUSY;
	}