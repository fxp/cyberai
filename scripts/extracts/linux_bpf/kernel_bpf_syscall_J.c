// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 10/37



		map->excl_prog_sha = kzalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
		if (!map->excl_prog_sha) {
			err = -ENOMEM;
			goto free_map;
		}

		if (copy_from_bpfptr(map->excl_prog_sha, uprog_hash, SHA256_DIGEST_SIZE)) {
			err = -EFAULT;
			goto free_map;
		}
	} else if (attr->excl_prog_hash_size) {
		err = -EINVAL;
		goto free_map;
	}

	err = security_bpf_map_create(map, attr, token, uattr.is_kernel);
	if (err)
		goto free_map_sec;

	err = bpf_map_alloc_id(map);
	if (err)
		goto free_map_sec;

	bpf_map_save_memcg(map);
	bpf_token_put(token);

	err = bpf_map_new_fd(map, f_flags);
	if (err < 0) {
		/* failed to allocate fd.
		 * bpf_map_put_with_uref() is needed because the above
		 * bpf_map_alloc_id() has published the map
		 * to the userspace and the userspace may
		 * have refcnt-ed it through BPF_MAP_GET_FD_BY_ID.
		 */
		bpf_map_put_with_uref(map);
		return err;
	}

	return err;

free_map_sec:
	security_bpf_map_free(map);
free_map:
	bpf_map_free(map);
put_token:
	bpf_token_put(token);
	return err;
}

void bpf_map_inc(struct bpf_map *map)
{
	atomic64_inc(&map->refcnt);
}
EXPORT_SYMBOL_GPL(bpf_map_inc);

void bpf_map_inc_with_uref(struct bpf_map *map)
{
	atomic64_inc(&map->refcnt);
	atomic64_inc(&map->usercnt);
}
EXPORT_SYMBOL_GPL(bpf_map_inc_with_uref);

struct bpf_map *bpf_map_get(u32 ufd)
{
	CLASS(fd, f)(ufd);
	struct bpf_map *map = __bpf_map_get(f);

	if (!IS_ERR(map))
		bpf_map_inc(map);

	return map;
}
EXPORT_SYMBOL_NS(bpf_map_get, "BPF_INTERNAL");

struct bpf_map *bpf_map_get_with_uref(u32 ufd)
{
	CLASS(fd, f)(ufd);
	struct bpf_map *map = __bpf_map_get(f);

	if (!IS_ERR(map))
		bpf_map_inc_with_uref(map);

	return map;
}

/* map_idr_lock should have been held or the map should have been
 * protected by rcu read lock.
 */
struct bpf_map *__bpf_map_inc_not_zero(struct bpf_map *map, bool uref)
{
	int refold;

	refold = atomic64_fetch_add_unless(&map->refcnt, 1, 0);
	if (!refold)
		return ERR_PTR(-ENOENT);
	if (uref)
		atomic64_inc(&map->usercnt);

	return map;
}

struct bpf_map *bpf_map_inc_not_zero(struct bpf_map *map)
{
	lockdep_assert(rcu_read_lock_held());
	return __bpf_map_inc_not_zero(map, false);
}
EXPORT_SYMBOL_GPL(bpf_map_inc_not_zero);

int __weak bpf_stackmap_extract(struct bpf_map *map, void *key, void *value,
				bool delete)
{
	return -ENOTSUPP;
}

static void *__bpf_copy_key(void __user *ukey, u64 key_size)
{
	if (key_size)
		return vmemdup_user(ukey, key_size);

	if (ukey)
		return ERR_PTR(-EINVAL);

	return NULL;
}

static void *___bpf_copy_key(bpfptr_t ukey, u64 key_size)
{
	if (key_size)
		return kvmemdup_bpfptr(ukey, key_size);

	if (!bpfptr_is_null(ukey))
		return ERR_PTR(-EINVAL);

	return NULL;
}

/* last field in 'union bpf_attr' used by this command */
#define BPF_MAP_LOOKUP_ELEM_LAST_FIELD flags

static int map_lookup_elem(union bpf_attr *attr)
{
	void __user *ukey = u64_to_user_ptr(attr->key);
	void __user *uvalue = u64_to_user_ptr(attr->value);
	struct bpf_map *map;
	void *key, *value;
	u32 value_size;
	int err;

	if (CHECK_ATTR(BPF_MAP_LOOKUP_ELEM))
		return -EINVAL;

	CLASS(fd, f)(attr->map_fd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_READ))
		return -EPERM;

	err = bpf_map_check_op_flags(map, attr->flags, BPF_F_LOCK | BPF_F_CPU);
	if (err)
		return err;

	key = __bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key))
		return PTR_ERR(key);

	value_size = bpf_map_value_size(map, attr->flags);

	err = -ENOMEM;
	value = kvmalloc(value_size, GFP_USER | __GFP_NOWARN);
	if (!value)
		goto free_key;

	if (map->map_type == BPF_MAP_TYPE_BLOOM_FILTER) {
		if (copy_from_user(value, uvalue, value_size))
			err = -EFAULT;
		else
			err = bpf_map_copy_value(map, key, value, attr->flags);
		goto free_value;
	}

	err = bpf_map_copy_value(map, key, value, attr->flags);
	if (err)
		goto free_value;

	err = -EFAULT;
	if (copy_to_user(uvalue, value, value_size) != 0)
		goto free_value;

	err = 0;

free_value:
	kvfree(value);
free_key:
	kvfree(key);
	return err;
}


#define BPF_MAP_UPDATE_ELEM_LAST_FIELD flags

static int map_update_elem(union bpf_attr *attr, bpfptr_t uattr)
{
	bpfptr_t ukey = make_bpfptr(attr->key, uattr.is_kernel);
	bpfptr_t uvalue = make_bpfptr(attr->value, uattr.is_kernel);
	struct bpf_map *map;
	void *key, *value;
	u32 value_size;
	int err;

	if (CHECK_ATTR(BPF_MAP_UPDATE_ELEM))
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

	err = bpf_map_check_op_flags(map, attr->flags, ~0);
	if (err)
		goto err_put;

	key = ___bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	value_size = bpf_map_value_size(map, attr->flags);
	value = kvmemdup_bpfptr(uvalue, value_size);
	if (IS_ERR(value)) {
		err = PTR_ERR(value);
		goto free_key;
	}