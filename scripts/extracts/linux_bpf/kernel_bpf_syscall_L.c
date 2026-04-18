// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 12/37



	kvfree(value);
	kvfree(key);

	return err;
}

int generic_map_lookup_batch(struct bpf_map *map,
				    const union bpf_attr *attr,
				    union bpf_attr __user *uattr)
{
	void __user *uobatch = u64_to_user_ptr(attr->batch.out_batch);
	void __user *ubatch = u64_to_user_ptr(attr->batch.in_batch);
	void __user *values = u64_to_user_ptr(attr->batch.values);
	void __user *keys = u64_to_user_ptr(attr->batch.keys);
	void *buf, *buf_prevkey, *prev_key, *key, *value;
	u32 value_size, cp, max_count;
	int err;

	err = bpf_map_check_op_flags(map, attr->batch.elem_flags, BPF_F_LOCK | BPF_F_CPU);
	if (err)
		return err;

	value_size = bpf_map_value_size(map, attr->batch.elem_flags);

	max_count = attr->batch.count;
	if (!max_count)
		return 0;

	if (put_user(0, &uattr->batch.count))
		return -EFAULT;

	buf_prevkey = kvmalloc(map->key_size, GFP_USER | __GFP_NOWARN);
	if (!buf_prevkey)
		return -ENOMEM;

	buf = kvmalloc(map->key_size + value_size, GFP_USER | __GFP_NOWARN);
	if (!buf) {
		kvfree(buf_prevkey);
		return -ENOMEM;
	}

	err = -EFAULT;
	prev_key = NULL;
	if (ubatch && copy_from_user(buf_prevkey, ubatch, map->key_size))
		goto free_buf;
	key = buf;
	value = key + map->key_size;
	if (ubatch)
		prev_key = buf_prevkey;

	for (cp = 0; cp < max_count;) {
		rcu_read_lock();
		err = map->ops->map_get_next_key(map, prev_key, key);
		rcu_read_unlock();
		if (err)
			break;
		err = bpf_map_copy_value(map, key, value,
					 attr->batch.elem_flags);

		if (err == -ENOENT)
			goto next_key;

		if (err)
			goto free_buf;

		if (copy_to_user(keys + cp * map->key_size, key,
				 map->key_size)) {
			err = -EFAULT;
			goto free_buf;
		}
		if (copy_to_user(values + cp * value_size, value, value_size)) {
			err = -EFAULT;
			goto free_buf;
		}

		cp++;
next_key:
		if (!prev_key)
			prev_key = buf_prevkey;

		swap(prev_key, key);
		cond_resched();
	}

	if (err == -EFAULT)
		goto free_buf;

	if ((copy_to_user(&uattr->batch.count, &cp, sizeof(cp)) ||
		    (cp && copy_to_user(uobatch, prev_key, map->key_size))))
		err = -EFAULT;

free_buf:
	kvfree(buf_prevkey);
	kvfree(buf);
	return err;
}

#define BPF_MAP_LOOKUP_AND_DELETE_ELEM_LAST_FIELD flags

static int map_lookup_and_delete_elem(union bpf_attr *attr)
{
	void __user *ukey = u64_to_user_ptr(attr->key);
	void __user *uvalue = u64_to_user_ptr(attr->value);
	struct bpf_map *map;
	void *key, *value;
	u32 value_size;
	int err;

	if (CHECK_ATTR(BPF_MAP_LOOKUP_AND_DELETE_ELEM))
		return -EINVAL;

	if (attr->flags & ~BPF_F_LOCK)
		return -EINVAL;

	CLASS(fd, f)(attr->map_fd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	bpf_map_write_active_inc(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_READ) ||
	    !(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	if (attr->flags &&
	    (map->map_type == BPF_MAP_TYPE_QUEUE ||
	     map->map_type == BPF_MAP_TYPE_STACK)) {
		err = -EINVAL;
		goto err_put;
	}

	if ((attr->flags & BPF_F_LOCK) &&
	    !btf_record_has_field(map->record, BPF_SPIN_LOCK)) {
		err = -EINVAL;
		goto err_put;
	}

	key = __bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	value_size = bpf_map_value_size(map, 0);

	err = -ENOMEM;
	value = kvmalloc(value_size, GFP_USER | __GFP_NOWARN);
	if (!value)
		goto free_key;

	err = -ENOTSUPP;
	if (map->map_type == BPF_MAP_TYPE_QUEUE ||
	    map->map_type == BPF_MAP_TYPE_STACK) {
		err = map->ops->map_pop_elem(map, value);
	} else if (map->map_type == BPF_MAP_TYPE_HASH ||
		   map->map_type == BPF_MAP_TYPE_PERCPU_HASH ||
		   map->map_type == BPF_MAP_TYPE_LRU_HASH ||
		   map->map_type == BPF_MAP_TYPE_LRU_PERCPU_HASH ||
		   map->map_type == BPF_MAP_TYPE_STACK_TRACE) {
		if (!bpf_map_is_offloaded(map)) {
			bpf_disable_instrumentation();
			rcu_read_lock();
			err = map->ops->map_lookup_and_delete_elem(map, key, value, attr->flags);
			rcu_read_unlock();
			bpf_enable_instrumentation();
		}
	}

	if (err)
		goto free_value;

	if (copy_to_user(uvalue, value, value_size) != 0) {
		err = -EFAULT;
		goto free_value;
	}

	err = 0;

free_value:
	kvfree(value);
free_key:
	kvfree(key);
err_put:
	bpf_map_write_active_dec(map);
	return err;
}

#define BPF_MAP_FREEZE_LAST_FIELD map_fd

static int map_freeze(const union bpf_attr *attr)
{
	int err = 0;
	struct bpf_map *map;

	if (CHECK_ATTR(BPF_MAP_FREEZE))
		return -EINVAL;

	CLASS(fd, f)(attr->map_fd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);

	if (map->map_type == BPF_MAP_TYPE_STRUCT_OPS || !IS_ERR_OR_NULL(map->record))
		return -ENOTSUPP;

	if (!(map_get_sys_perms(map, f) & FMODE_CAN_WRITE))
		return -EPERM;

	mutex_lock(&map->freeze_mutex);
	if (bpf_map_write_active(map)) {
		err = -EBUSY;
		goto err_put;
	}
	if (READ_ONCE(map->frozen)) {
		err = -EBUSY;
		goto err_put;
	}

	WRITE_ONCE(map->frozen, true);
err_put:
	mutex_unlock(&map->freeze_mutex);
	return err;
}