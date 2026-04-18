// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 34/37



	prog = bpf_prog_get(attr->prog_bind_map.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	map = bpf_map_get(attr->prog_bind_map.map_fd);
	if (IS_ERR(map)) {
		ret = PTR_ERR(map);
		goto out_prog_put;
	}

	mutex_lock(&prog->aux->used_maps_mutex);

	used_maps_old = prog->aux->used_maps;

	for (i = 0; i < prog->aux->used_map_cnt; i++)
		if (used_maps_old[i] == map) {
			bpf_map_put(map);
			goto out_unlock;
		}

	used_maps_new = kmalloc_objs(used_maps_new[0],
				     prog->aux->used_map_cnt + 1);
	if (!used_maps_new) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	/* The bpf program will not access the bpf map, but for the sake of
	 * simplicity, increase sleepable_refcnt for sleepable program as well.
	 */
	if (prog->sleepable)
		atomic64_inc(&map->sleepable_refcnt);
	memcpy(used_maps_new, used_maps_old,
	       sizeof(used_maps_old[0]) * prog->aux->used_map_cnt);
	used_maps_new[prog->aux->used_map_cnt] = map;

	prog->aux->used_map_cnt++;
	prog->aux->used_maps = used_maps_new;

	kfree(used_maps_old);

out_unlock:
	mutex_unlock(&prog->aux->used_maps_mutex);

	if (ret)
		bpf_map_put(map);
out_prog_put:
	bpf_prog_put(prog);
	return ret;
}

#define BPF_TOKEN_CREATE_LAST_FIELD token_create.bpffs_fd

static int token_create(union bpf_attr *attr)
{
	if (CHECK_ATTR(BPF_TOKEN_CREATE))
		return -EINVAL;

	/* no flags are supported yet */
	if (attr->token_create.flags)
		return -EINVAL;

	return bpf_token_create(attr);
}

#define BPF_PROG_STREAM_READ_BY_FD_LAST_FIELD prog_stream_read.prog_fd

static int prog_stream_read(union bpf_attr *attr)
{
	char __user *buf = u64_to_user_ptr(attr->prog_stream_read.stream_buf);
	u32 len = attr->prog_stream_read.stream_buf_len;
	struct bpf_prog *prog;
	int ret;

	if (CHECK_ATTR(BPF_PROG_STREAM_READ_BY_FD))
		return -EINVAL;

	prog = bpf_prog_get(attr->prog_stream_read.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	ret = bpf_prog_stream_read(prog, attr->prog_stream_read.stream_id, buf, len);
	bpf_prog_put(prog);

	return ret;
}

#define BPF_PROG_ASSOC_STRUCT_OPS_LAST_FIELD prog_assoc_struct_ops.prog_fd

static int prog_assoc_struct_ops(union bpf_attr *attr)
{
	struct bpf_prog *prog;
	struct bpf_map *map;
	int ret;

	if (CHECK_ATTR(BPF_PROG_ASSOC_STRUCT_OPS))
		return -EINVAL;

	if (attr->prog_assoc_struct_ops.flags)
		return -EINVAL;

	prog = bpf_prog_get(attr->prog_assoc_struct_ops.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	if (prog->type == BPF_PROG_TYPE_STRUCT_OPS) {
		ret = -EINVAL;
		goto put_prog;
	}

	map = bpf_map_get(attr->prog_assoc_struct_ops.map_fd);
	if (IS_ERR(map)) {
		ret = PTR_ERR(map);
		goto put_prog;
	}

	if (map->map_type != BPF_MAP_TYPE_STRUCT_OPS) {
		ret = -EINVAL;
		goto put_map;
	}

	ret = bpf_prog_assoc_struct_ops(prog, map);

put_map:
	bpf_map_put(map);
put_prog:
	bpf_prog_put(prog);
	return ret;
}

static int __sys_bpf(enum bpf_cmd cmd, bpfptr_t uattr, unsigned int size)
{
	union bpf_attr attr;
	int err;

	err = bpf_check_uarg_tail_zero(uattr, sizeof(attr), size);
	if (err)
		return err;
	size = min_t(u32, size, sizeof(attr));

	/* copy attributes from user space, may be less than sizeof(bpf_attr) */
	memset(&attr, 0, sizeof(attr));
	if (copy_from_bpfptr(&attr, uattr, size) != 0)
		return -EFAULT;

	err = security_bpf(cmd, &attr, size, uattr.is_kernel);
	if (err < 0)
		return err;