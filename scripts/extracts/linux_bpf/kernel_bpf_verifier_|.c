// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 60/132



	/* ... and second from the function itself. */
	switch (func_id) {
	case BPF_FUNC_tail_call:
		if (map->map_type != BPF_MAP_TYPE_PROG_ARRAY)
			goto error;
		if (env->subprog_cnt > 1 && !bpf_allow_tail_call_in_subprogs(env)) {
			verbose(env, "mixing of tail_calls and bpf-to-bpf calls is not supported\n");
			return -EINVAL;
		}
		break;
	case BPF_FUNC_perf_event_read:
	case BPF_FUNC_perf_event_output:
	case BPF_FUNC_perf_event_read_value:
	case BPF_FUNC_skb_output:
	case BPF_FUNC_xdp_output:
		if (map->map_type != BPF_MAP_TYPE_PERF_EVENT_ARRAY)
			goto error;
		break;
	case BPF_FUNC_ringbuf_output:
	case BPF_FUNC_ringbuf_reserve:
	case BPF_FUNC_ringbuf_query:
	case BPF_FUNC_ringbuf_reserve_dynptr:
	case BPF_FUNC_ringbuf_submit_dynptr:
	case BPF_FUNC_ringbuf_discard_dynptr:
		if (map->map_type != BPF_MAP_TYPE_RINGBUF)
			goto error;
		break;
	case BPF_FUNC_user_ringbuf_drain:
		if (map->map_type != BPF_MAP_TYPE_USER_RINGBUF)
			goto error;
		break;
	case BPF_FUNC_get_stackid:
		if (map->map_type != BPF_MAP_TYPE_STACK_TRACE)
			goto error;
		break;
	case BPF_FUNC_current_task_under_cgroup:
	case BPF_FUNC_skb_under_cgroup:
		if (map->map_type != BPF_MAP_TYPE_CGROUP_ARRAY)
			goto error;
		break;
	case BPF_FUNC_redirect_map:
		if (map->map_type != BPF_MAP_TYPE_DEVMAP &&
		    map->map_type != BPF_MAP_TYPE_DEVMAP_HASH &&
		    map->map_type != BPF_MAP_TYPE_CPUMAP &&
		    map->map_type != BPF_MAP_TYPE_XSKMAP)
			goto error;
		break;
	case BPF_FUNC_sk_redirect_map:
	case BPF_FUNC_msg_redirect_map:
	case BPF_FUNC_sock_map_update:
		if (map->map_type != BPF_MAP_TYPE_SOCKMAP)
			goto error;
		break;
	case BPF_FUNC_sk_redirect_hash:
	case BPF_FUNC_msg_redirect_hash:
	case BPF_FUNC_sock_hash_update:
		if (map->map_type != BPF_MAP_TYPE_SOCKHASH)
			goto error;
		break;
	case BPF_FUNC_get_local_storage:
		if (map->map_type != BPF_MAP_TYPE_CGROUP_STORAGE &&
		    map->map_type != BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE)
			goto error;
		break;
	case BPF_FUNC_sk_select_reuseport:
		if (map->map_type != BPF_MAP_TYPE_REUSEPORT_SOCKARRAY &&
		    map->map_type != BPF_MAP_TYPE_SOCKMAP &&
		    map->map_type != BPF_MAP_TYPE_SOCKHASH)
			goto error;
		break;
	case BPF_FUNC_map_pop_elem:
		if (map->map_type != BPF_MAP_TYPE_QUEUE &&
		    map->map_type != BPF_MAP_TYPE_STACK)
			goto error;
		break;
	case BPF_FUNC_map_peek_elem:
	case BPF_FUNC_map_push_elem:
		if (map->map_type != BPF_MAP_TYPE_QUEUE &&
		    map->map_type != BPF_MAP_TYPE_STACK &&
		    map->map_type != BPF_MAP_TYPE_BLOOM_FILTER)
			goto error;
		break;
	case BPF_FUNC_map_lookup_percpu_elem:
		if (map->map_type != BPF_MAP_TYPE_PERCPU_ARRAY &&
		    map->map_type != BPF_MAP_TYPE_PERCPU_HASH &&
		    map->map_type != BPF_MAP_TYPE_LRU_PERCPU_HASH)
			goto error;
		break;
	case BPF_FUNC_sk_storage_get:
	case BPF_FUNC_sk_storage_delete:
		if (map->map_type != BPF_MAP_TYPE_SK_STORAGE)
			goto error;
		break;
	case BPF_FUNC_inode_storage_get:
	case BPF_FUNC_inode_storage_delete:
		if (map->map_type != BPF_MAP_TYPE_INODE_STORAGE)
			goto error;
		break;
	case BPF_FUNC_task_storage_get:
	case BPF_FUNC_task_storage_delete:
		if (map->map_type != BPF_MAP_TYPE_TASK_STORAGE)
			goto error;
		break;
	case BPF_FUNC_cgrp_storage_get:
	case BPF_FUNC_cgrp_storage_delete:
		if (map->map_type != BPF_MAP_TYPE_CGRP_STORAGE)
			goto error;
		break;
	default:
		break;
	}

	return 0;
error:
	verbose(env, "cannot pass map_type %d into func %s#%d\n",
		map->map_type, func_id_name(func_id), func_id);
	return -EINVAL;
}

static bool check_raw_mode_ok(const struct bpf_func_proto *fn)
{
	int count = 0;

	if (arg_type_is_raw_mem(fn->arg1_type))
		count++;
	if (arg_type_is_raw_mem(fn->arg2_type))
		count++;
	if (arg_type_is_raw_mem(fn->arg3_type))
		count++;
	if (arg_type_is_raw_mem(fn->arg4_type))
		count++;
	if (arg_type_is_raw_mem(fn->arg5_type))
		count++;

	/* We only support one arg being in raw mode at the moment,
	 * which is sufficient for the helper functions we have
	 * right now.
	 */
	return count <= 1;
}

static bool check_args_pair_invalid(const struct bpf_func_proto *fn, int arg)
{
	bool is_fixed = fn->arg_type[arg] & MEM_FIXED_SIZE;
	bool has_size = fn->arg_size[arg] != 0;
	bool is_next_size = false;

	if (arg + 1 < ARRAY_SIZE(fn->arg_type))
		is_next_size = arg_type_is_mem_size(fn->arg_type[arg + 1]);

	if (base_type(fn->arg_type[arg]) != ARG_PTR_TO_MEM)
		return is_next_size;

	return has_size == is_next_size || is_next_size == is_fixed;
}

static bool check_arg_pair_ok(const struct bpf_func_proto *fn)
{
	/* bpf_xxx(..., buf, len) call will access 'len'
	 * bytes from memory 'buf'. Both arg types need
	 * to be paired, so make sure there's no buggy
	 * helper function specification.
	 */
	if (arg_type_is_mem_size(fn->arg1_type) ||
	    check_args_pair_invalid(fn, 0) ||
	    check_args_pair_invalid(fn, 1) ||
	    check_args_pair_invalid(fn, 2) ||
	    check_args_pair_invalid(fn, 3) ||
	    check_args_pair_invalid(fn, 4))
		return false;

	return true;
}