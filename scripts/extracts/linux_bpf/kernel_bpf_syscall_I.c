// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 9/37



	if (attr->btf_vmlinux_value_type_id) {
		if (attr->map_type != BPF_MAP_TYPE_STRUCT_OPS ||
		    attr->btf_key_type_id || attr->btf_value_type_id)
			return -EINVAL;
	} else if (attr->btf_key_type_id && !attr->btf_value_type_id) {
		return -EINVAL;
	}

	if (attr->map_type != BPF_MAP_TYPE_BLOOM_FILTER &&
	    attr->map_type != BPF_MAP_TYPE_ARENA &&
	    attr->map_extra != 0)
		return -EINVAL;

	f_flags = bpf_get_file_flag(attr->map_flags);
	if (f_flags < 0)
		return f_flags;

	if (numa_node != NUMA_NO_NODE &&
	    ((unsigned int)numa_node >= nr_node_ids ||
	     !node_online(numa_node)))
		return -EINVAL;

	/* find map type and init map: hashtable vs rbtree vs bloom vs ... */
	map_type = attr->map_type;
	if (map_type >= ARRAY_SIZE(bpf_map_types))
		return -EINVAL;
	map_type = array_index_nospec(map_type, ARRAY_SIZE(bpf_map_types));
	ops = bpf_map_types[map_type];
	if (!ops)
		return -EINVAL;

	if (ops->map_alloc_check) {
		err = ops->map_alloc_check(attr);
		if (err)
			return err;
	}
	if (attr->map_ifindex)
		ops = &bpf_map_offload_ops;
	if (!ops->map_mem_usage)
		return -EINVAL;

	if (token_flag) {
		token = bpf_token_get_from_fd(attr->map_token_fd);
		if (IS_ERR(token))
			return PTR_ERR(token);

		/* if current token doesn't grant map creation permissions,
		 * then we can't use this token, so ignore it and rely on
		 * system-wide capabilities checks
		 */
		if (!bpf_token_allow_cmd(token, BPF_MAP_CREATE) ||
		    !bpf_token_allow_map_type(token, attr->map_type)) {
			bpf_token_put(token);
			token = NULL;
		}
	}

	err = -EPERM;

	/* Intent here is for unprivileged_bpf_disabled to block BPF map
	 * creation for unprivileged users; other actions depend
	 * on fd availability and access to bpffs, so are dependent on
	 * object creation success. Even with unprivileged BPF disabled,
	 * capability checks are still carried out.
	 */
	if (sysctl_unprivileged_bpf_disabled && !bpf_token_capable(token, CAP_BPF))
		goto put_token;

	/* check privileged map type permissions */
	switch (map_type) {
	case BPF_MAP_TYPE_ARRAY:
	case BPF_MAP_TYPE_PERCPU_ARRAY:
	case BPF_MAP_TYPE_PROG_ARRAY:
	case BPF_MAP_TYPE_PERF_EVENT_ARRAY:
	case BPF_MAP_TYPE_CGROUP_ARRAY:
	case BPF_MAP_TYPE_ARRAY_OF_MAPS:
	case BPF_MAP_TYPE_HASH:
	case BPF_MAP_TYPE_PERCPU_HASH:
	case BPF_MAP_TYPE_HASH_OF_MAPS:
	case BPF_MAP_TYPE_RINGBUF:
	case BPF_MAP_TYPE_USER_RINGBUF:
	case BPF_MAP_TYPE_CGROUP_STORAGE:
	case BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE:
		/* unprivileged */
		break;
	case BPF_MAP_TYPE_SK_STORAGE:
	case BPF_MAP_TYPE_INODE_STORAGE:
	case BPF_MAP_TYPE_TASK_STORAGE:
	case BPF_MAP_TYPE_CGRP_STORAGE:
	case BPF_MAP_TYPE_BLOOM_FILTER:
	case BPF_MAP_TYPE_LPM_TRIE:
	case BPF_MAP_TYPE_REUSEPORT_SOCKARRAY:
	case BPF_MAP_TYPE_STACK_TRACE:
	case BPF_MAP_TYPE_QUEUE:
	case BPF_MAP_TYPE_STACK:
	case BPF_MAP_TYPE_LRU_HASH:
	case BPF_MAP_TYPE_LRU_PERCPU_HASH:
	case BPF_MAP_TYPE_STRUCT_OPS:
	case BPF_MAP_TYPE_CPUMAP:
	case BPF_MAP_TYPE_ARENA:
	case BPF_MAP_TYPE_INSN_ARRAY:
		if (!bpf_token_capable(token, CAP_BPF))
			goto put_token;
		break;
	case BPF_MAP_TYPE_SOCKMAP:
	case BPF_MAP_TYPE_SOCKHASH:
	case BPF_MAP_TYPE_DEVMAP:
	case BPF_MAP_TYPE_DEVMAP_HASH:
	case BPF_MAP_TYPE_XSKMAP:
		if (!bpf_token_capable(token, CAP_NET_ADMIN))
			goto put_token;
		break;
	default:
		WARN(1, "unsupported map type %d", map_type);
		goto put_token;
	}

	map = ops->map_alloc(attr);
	if (IS_ERR(map)) {
		err = PTR_ERR(map);
		goto put_token;
	}
	map->ops = ops;
	map->map_type = map_type;

	err = bpf_obj_name_cpy(map->name, attr->map_name,
			       sizeof(attr->map_name));
	if (err < 0)
		goto free_map;

	preempt_disable();
	map->cookie = gen_cookie_next(&bpf_map_cookie);
	preempt_enable();

	atomic64_set(&map->refcnt, 1);
	atomic64_set(&map->usercnt, 1);
	mutex_init(&map->freeze_mutex);
	spin_lock_init(&map->owner_lock);

	if (attr->btf_key_type_id || attr->btf_value_type_id ||
	    /* Even the map's value is a kernel's struct,
	     * the bpf_prog.o must have BTF to begin with
	     * to figure out the corresponding kernel's
	     * counter part.  Thus, attr->btf_fd has
	     * to be valid also.
	     */
	    attr->btf_vmlinux_value_type_id) {
		struct btf *btf;

		btf = btf_get_by_fd(attr->btf_fd);
		if (IS_ERR(btf)) {
			err = PTR_ERR(btf);
			goto free_map;
		}
		if (btf_is_kernel(btf)) {
			btf_put(btf);
			err = -EACCES;
			goto free_map;
		}
		map->btf = btf;

		if (attr->btf_value_type_id) {
			err = map_check_btf(map, token, btf, attr->btf_key_type_id,
					    attr->btf_value_type_id);
			if (err)
				goto free_map;
		}

		map->btf_key_type_id = attr->btf_key_type_id;
		map->btf_value_type_id = attr->btf_value_type_id;
		map->btf_vmlinux_value_type_id =
			attr->btf_vmlinux_value_type_id;
	}

	if (attr->excl_prog_hash) {
		bpfptr_t uprog_hash = make_bpfptr(attr->excl_prog_hash, uattr.is_kernel);

		if (attr->excl_prog_hash_size != SHA256_DIGEST_SIZE) {
			err = -EINVAL;
			goto free_map;
		}