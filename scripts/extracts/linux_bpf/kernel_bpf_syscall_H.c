// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 8/37



/* dst and src must have at least "size" number of bytes.
 * Return strlen on success and < 0 on error.
 */
int bpf_obj_name_cpy(char *dst, const char *src, unsigned int size)
{
	const char *end = src + size;
	const char *orig_src = src;

	memset(dst, 0, size);
	/* Copy all isalnum(), '_' and '.' chars. */
	while (src < end && *src) {
		if (!isalnum(*src) &&
		    *src != '_' && *src != '.')
			return -EINVAL;
		*dst++ = *src++;
	}

	/* No '\0' found in "size" number of bytes */
	if (src == end)
		return -EINVAL;

	return src - orig_src;
}
EXPORT_SYMBOL_GPL(bpf_obj_name_cpy);

int map_check_no_btf(struct bpf_map *map,
		     const struct btf *btf,
		     const struct btf_type *key_type,
		     const struct btf_type *value_type)
{
	return -ENOTSUPP;
}

static int map_check_btf(struct bpf_map *map, struct bpf_token *token,
			 const struct btf *btf, u32 btf_key_id, u32 btf_value_id)
{
	const struct btf_type *key_type, *value_type;
	u32 key_size, value_size;
	int ret = 0;

	/* Some maps allow key to be unspecified. */
	if (btf_key_id) {
		key_type = btf_type_id_size(btf, &btf_key_id, &key_size);
		if (!key_type || key_size != map->key_size)
			return -EINVAL;
	} else {
		key_type = btf_type_by_id(btf, 0);
		if (!map->ops->map_check_btf)
			return -EINVAL;
	}

	value_type = btf_type_id_size(btf, &btf_value_id, &value_size);
	if (!value_type || value_size != map->value_size)
		return -EINVAL;

	map->record = btf_parse_fields(btf, value_type,
				       BPF_SPIN_LOCK | BPF_RES_SPIN_LOCK | BPF_TIMER | BPF_KPTR | BPF_LIST_HEAD |
				       BPF_RB_ROOT | BPF_REFCOUNT | BPF_WORKQUEUE | BPF_UPTR |
				       BPF_TASK_WORK,
				       map->value_size);
	if (!IS_ERR_OR_NULL(map->record)) {
		int i;

		if (!bpf_token_capable(token, CAP_BPF)) {
			ret = -EPERM;
			goto free_map_tab;
		}
		if (map->map_flags & (BPF_F_RDONLY_PROG | BPF_F_WRONLY_PROG)) {
			ret = -EACCES;
			goto free_map_tab;
		}
		for (i = 0; i < sizeof(map->record->field_mask) * 8; i++) {
			switch (map->record->field_mask & (1 << i)) {
			case 0:
				continue;
			case BPF_SPIN_LOCK:
			case BPF_RES_SPIN_LOCK:
				if (map->map_type != BPF_MAP_TYPE_HASH &&
				    map->map_type != BPF_MAP_TYPE_ARRAY &&
				    map->map_type != BPF_MAP_TYPE_CGROUP_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_SK_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_INODE_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_TASK_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_CGRP_STORAGE) {
					ret = -EOPNOTSUPP;
					goto free_map_tab;
				}
				break;
			case BPF_TIMER:
			case BPF_WORKQUEUE:
			case BPF_TASK_WORK:
				if (map->map_type != BPF_MAP_TYPE_HASH &&
				    map->map_type != BPF_MAP_TYPE_LRU_HASH &&
				    map->map_type != BPF_MAP_TYPE_ARRAY) {
					ret = -EOPNOTSUPP;
					goto free_map_tab;
				}
				break;
			case BPF_KPTR_UNREF:
			case BPF_KPTR_REF:
			case BPF_KPTR_PERCPU:
			case BPF_REFCOUNT:
				if (map->map_type != BPF_MAP_TYPE_HASH &&
				    map->map_type != BPF_MAP_TYPE_PERCPU_HASH &&
				    map->map_type != BPF_MAP_TYPE_LRU_HASH &&
				    map->map_type != BPF_MAP_TYPE_LRU_PERCPU_HASH &&
				    map->map_type != BPF_MAP_TYPE_ARRAY &&
				    map->map_type != BPF_MAP_TYPE_PERCPU_ARRAY &&
				    map->map_type != BPF_MAP_TYPE_SK_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_INODE_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_TASK_STORAGE &&
				    map->map_type != BPF_MAP_TYPE_CGRP_STORAGE) {
					ret = -EOPNOTSUPP;
					goto free_map_tab;
				}
				break;
			case BPF_UPTR:
				if (map->map_type != BPF_MAP_TYPE_TASK_STORAGE) {
					ret = -EOPNOTSUPP;
					goto free_map_tab;
				}
				break;
			case BPF_LIST_HEAD:
			case BPF_RB_ROOT:
				if (map->map_type != BPF_MAP_TYPE_HASH &&
				    map->map_type != BPF_MAP_TYPE_LRU_HASH &&
				    map->map_type != BPF_MAP_TYPE_ARRAY) {
					ret = -EOPNOTSUPP;
					goto free_map_tab;
				}
				break;
			default:
				/* Fail if map_type checks are missing for a field type */
				ret = -EOPNOTSUPP;
				goto free_map_tab;
			}
		}
	}

	ret = btf_check_and_fixup_fields(btf, map->record);
	if (ret < 0)
		goto free_map_tab;

	if (map->ops->map_check_btf) {
		ret = map->ops->map_check_btf(map, btf, key_type, value_type);
		if (ret < 0)
			goto free_map_tab;
	}

	return ret;
free_map_tab:
	bpf_map_free_record(map);
	return ret;
}

#define BPF_MAP_CREATE_LAST_FIELD excl_prog_hash_size
/* called via syscall */
static int map_create(union bpf_attr *attr, bpfptr_t uattr)
{
	const struct bpf_map_ops *ops;
	struct bpf_token *token = NULL;
	int numa_node = bpf_map_attr_numa_node(attr);
	u32 map_type = attr->map_type;
	struct bpf_map *map;
	bool token_flag;
	int f_flags;
	int err;

	err = CHECK_ATTR(BPF_MAP_CREATE);
	if (err)
		return -EINVAL;

	/* check BPF_F_TOKEN_FD flag, remember if it's set, and then clear it
	 * to avoid per-map type checks tripping on unknown flag
	 */
	token_flag = attr->map_flags & BPF_F_TOKEN_FD;
	attr->map_flags &= ~BPF_F_TOKEN_FD;