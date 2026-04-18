// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 11/37



	err = bpf_map_update_value(map, fd_file(f), key, value, attr->flags);
	if (!err)
		maybe_wait_bpf_programs(map);

	kvfree(value);
free_key:
	kvfree(key);
err_put:
	bpf_map_write_active_dec(map);
	return err;
}

#define BPF_MAP_DELETE_ELEM_LAST_FIELD key

static int map_delete_elem(union bpf_attr *attr, bpfptr_t uattr)
{
	bpfptr_t ukey = make_bpfptr(attr->key, uattr.is_kernel);
	struct bpf_map *map;
	void *key;
	int err;

	if (CHECK_ATTR(BPF_MAP_DELETE_ELEM))
		return -EINVAL;

	CLASS(fd, f)(attr->map_fd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	bpf_map_write_active_inc(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	key = ___bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	if (bpf_map_is_offloaded(map)) {
		err = bpf_map_offload_delete_elem(map, key);
		goto out;
	} else if (IS_FD_PROG_ARRAY(map) ||
		   map->map_type == BPF_MAP_TYPE_STRUCT_OPS) {
		/* These maps require sleepable context */
		err = map->ops->map_delete_elem(map, key);
		goto out;
	}

	bpf_disable_instrumentation();
	rcu_read_lock();
	err = map->ops->map_delete_elem(map, key);
	rcu_read_unlock();
	bpf_enable_instrumentation();
	if (!err)
		maybe_wait_bpf_programs(map);
out:
	kvfree(key);
err_put:
	bpf_map_write_active_dec(map);
	return err;
}

/* last field in 'union bpf_attr' used by this command */
#define BPF_MAP_GET_NEXT_KEY_LAST_FIELD next_key

static int map_get_next_key(union bpf_attr *attr)
{
	void __user *ukey = u64_to_user_ptr(attr->key);
	void __user *unext_key = u64_to_user_ptr(attr->next_key);
	struct bpf_map *map;
	void *key, *next_key;
	int err;

	if (CHECK_ATTR(BPF_MAP_GET_NEXT_KEY))
		return -EINVAL;

	CLASS(fd, f)(attr->map_fd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_READ))
		return -EPERM;

	if (ukey) {
		key = __bpf_copy_key(ukey, map->key_size);
		if (IS_ERR(key))
			return PTR_ERR(key);
	} else {
		key = NULL;
	}

	err = -ENOMEM;
	next_key = kvmalloc(map->key_size, GFP_USER);
	if (!next_key)
		goto free_key;

	if (bpf_map_is_offloaded(map)) {
		err = bpf_map_offload_get_next_key(map, key, next_key);
		goto out;
	}

	rcu_read_lock();
	err = map->ops->map_get_next_key(map, key, next_key);
	rcu_read_unlock();
out:
	if (err)
		goto free_next_key;

	err = -EFAULT;
	if (copy_to_user(unext_key, next_key, map->key_size) != 0)
		goto free_next_key;

	err = 0;

free_next_key:
	kvfree(next_key);
free_key:
	kvfree(key);
	return err;
}

int generic_map_delete_batch(struct bpf_map *map,
			     const union bpf_attr *attr,
			     union bpf_attr __user *uattr)
{
	void __user *keys = u64_to_user_ptr(attr->batch.keys);
	u32 cp, max_count;
	int err = 0;
	void *key;

	if (attr->batch.elem_flags & ~BPF_F_LOCK)
		return -EINVAL;

	if ((attr->batch.elem_flags & BPF_F_LOCK) &&
	    !btf_record_has_field(map->record, BPF_SPIN_LOCK)) {
		return -EINVAL;
	}

	max_count = attr->batch.count;
	if (!max_count)
		return 0;

	if (put_user(0, &uattr->batch.count))
		return -EFAULT;

	key = kvmalloc(map->key_size, GFP_USER | __GFP_NOWARN);
	if (!key)
		return -ENOMEM;

	for (cp = 0; cp < max_count; cp++) {
		err = -EFAULT;
		if (copy_from_user(key, keys + cp * map->key_size,
				   map->key_size))
			break;

		if (bpf_map_is_offloaded(map)) {
			err = bpf_map_offload_delete_elem(map, key);
			break;
		}

		bpf_disable_instrumentation();
		rcu_read_lock();
		err = map->ops->map_delete_elem(map, key);
		rcu_read_unlock();
		bpf_enable_instrumentation();
		if (err)
			break;
		cond_resched();
	}
	if (copy_to_user(&uattr->batch.count, &cp, sizeof(cp)))
		err = -EFAULT;

	kvfree(key);

	return err;
}

int generic_map_update_batch(struct bpf_map *map, struct file *map_file,
			     const union bpf_attr *attr,
			     union bpf_attr __user *uattr)
{
	void __user *values = u64_to_user_ptr(attr->batch.values);
	void __user *keys = u64_to_user_ptr(attr->batch.keys);
	u32 value_size, cp, max_count;
	void *key, *value;
	int err = 0;

	err = bpf_map_check_op_flags(map, attr->batch.elem_flags,
				     BPF_F_LOCK | BPF_F_CPU | BPF_F_ALL_CPUS);
	if (err)
		return err;

	value_size = bpf_map_value_size(map, attr->batch.elem_flags);

	max_count = attr->batch.count;
	if (!max_count)
		return 0;

	if (put_user(0, &uattr->batch.count))
		return -EFAULT;

	key = kvmalloc(map->key_size, GFP_USER | __GFP_NOWARN);
	if (!key)
		return -ENOMEM;

	value = kvmalloc(value_size, GFP_USER | __GFP_NOWARN);
	if (!value) {
		kvfree(key);
		return -ENOMEM;
	}

	for (cp = 0; cp < max_count; cp++) {
		err = -EFAULT;
		if (copy_from_user(key, keys + cp * map->key_size,
		    map->key_size) ||
		    copy_from_user(value, values + cp * value_size, value_size))
			break;

		err = bpf_map_update_value(map, map_file, key, value,
					   attr->batch.elem_flags);

		if (err)
			break;
		cond_resched();
	}

	if (copy_to_user(&uattr->batch.count, &cp, sizeof(cp)))
		err = -EFAULT;