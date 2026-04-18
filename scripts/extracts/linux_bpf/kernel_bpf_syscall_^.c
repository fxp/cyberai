// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 30/37



		user_prog_tags = u64_to_user_ptr(info.prog_tags);
		ulen = min_t(u32, info.nr_prog_tags, ulen);
		if (prog->aux->func_cnt) {
			for (i = 0; i < ulen; i++) {
				if (copy_to_user(user_prog_tags[i],
						 prog->aux->func[i]->tag,
						 BPF_TAG_SIZE))
					return -EFAULT;
			}
		} else {
			if (copy_to_user(user_prog_tags[0],
					 prog->tag, BPF_TAG_SIZE))
				return -EFAULT;
		}
	}

done:
	if (copy_to_user(uinfo, &info, info_len) ||
	    put_user(info_len, &uattr->info.info_len))
		return -EFAULT;

	return 0;
}

static int bpf_map_get_info_by_fd(struct file *file,
				  struct bpf_map *map,
				  const union bpf_attr *attr,
				  union bpf_attr __user *uattr)
{
	struct bpf_map_info __user *uinfo = u64_to_user_ptr(attr->info.info);
	struct bpf_map_info info;
	u32 info_len = attr->info.info_len;
	int err;

	err = bpf_check_uarg_tail_zero(USER_BPFPTR(uinfo), sizeof(info), info_len);
	if (err)
		return err;
	info_len = min_t(u32, sizeof(info), info_len);

	memset(&info, 0, sizeof(info));
	if (copy_from_user(&info, uinfo, info_len))
		return -EFAULT;

	info.type = map->map_type;
	info.id = map->id;
	info.key_size = map->key_size;
	info.value_size = map->value_size;
	info.max_entries = map->max_entries;
	info.map_flags = map->map_flags;
	info.map_extra = map->map_extra;
	memcpy(info.name, map->name, sizeof(map->name));

	if (map->btf) {
		info.btf_id = btf_obj_id(map->btf);
		info.btf_key_type_id = map->btf_key_type_id;
		info.btf_value_type_id = map->btf_value_type_id;
	}
	info.btf_vmlinux_value_type_id = map->btf_vmlinux_value_type_id;
	if (map->map_type == BPF_MAP_TYPE_STRUCT_OPS)
		bpf_map_struct_ops_info_fill(&info, map);

	if (bpf_map_is_offloaded(map)) {
		err = bpf_map_offload_info_fill(&info, map);
		if (err)
			return err;
	}

	if (info.hash) {
		char __user *uhash = u64_to_user_ptr(info.hash);

		if (!map->ops->map_get_hash)
			return -EINVAL;

		if (info.hash_size != SHA256_DIGEST_SIZE)
			return -EINVAL;

		if (!READ_ONCE(map->frozen))
			return -EPERM;

		err = map->ops->map_get_hash(map, SHA256_DIGEST_SIZE, map->sha);
		if (err != 0)
			return err;

		if (copy_to_user(uhash, map->sha, SHA256_DIGEST_SIZE) != 0)
			return -EFAULT;
	} else if (info.hash_size) {
		return -EINVAL;
	}

	if (copy_to_user(uinfo, &info, info_len) ||
	    put_user(info_len, &uattr->info.info_len))
		return -EFAULT;

	return 0;
}

static int bpf_btf_get_info_by_fd(struct file *file,
				  struct btf *btf,
				  const union bpf_attr *attr,
				  union bpf_attr __user *uattr)
{
	struct bpf_btf_info __user *uinfo = u64_to_user_ptr(attr->info.info);
	u32 info_len = attr->info.info_len;
	int err;

	err = bpf_check_uarg_tail_zero(USER_BPFPTR(uinfo), sizeof(*uinfo), info_len);
	if (err)
		return err;

	return btf_get_info_by_fd(btf, attr, uattr);
}

static int bpf_link_get_info_by_fd(struct file *file,
				  struct bpf_link *link,
				  const union bpf_attr *attr,
				  union bpf_attr __user *uattr)
{
	struct bpf_link_info __user *uinfo = u64_to_user_ptr(attr->info.info);
	struct bpf_link_info info;
	u32 info_len = attr->info.info_len;
	int err;

	err = bpf_check_uarg_tail_zero(USER_BPFPTR(uinfo), sizeof(info), info_len);
	if (err)
		return err;
	info_len = min_t(u32, sizeof(info), info_len);

	memset(&info, 0, sizeof(info));
	if (copy_from_user(&info, uinfo, info_len))
		return -EFAULT;

	info.type = link->type;
	info.id = link->id;
	if (link->prog)
		info.prog_id = link->prog->aux->id;

	if (link->ops->fill_link_info) {
		err = link->ops->fill_link_info(link, &info);
		if (err)
			return err;
	}

	if (copy_to_user(uinfo, &info, info_len) ||
	    put_user(info_len, &uattr->info.info_len))
		return -EFAULT;

	return 0;
}


static int token_get_info_by_fd(struct file *file,
				struct bpf_token *token,
				const union bpf_attr *attr,
				union bpf_attr __user *uattr)
{
	struct bpf_token_info __user *uinfo = u64_to_user_ptr(attr->info.info);
	u32 info_len = attr->info.info_len;
	int err;

	err = bpf_check_uarg_tail_zero(USER_BPFPTR(uinfo), sizeof(*uinfo), info_len);
	if (err)
		return err;
	return bpf_token_get_info_by_fd(token, attr, uattr);
}

#define BPF_OBJ_GET_INFO_BY_FD_LAST_FIELD info.info

static int bpf_obj_get_info_by_fd(const union bpf_attr *attr,
				  union bpf_attr __user *uattr)
{
	if (CHECK_ATTR(BPF_OBJ_GET_INFO_BY_FD))
		return -EINVAL;

	CLASS(fd, f)(attr->info.bpf_fd);
	if (fd_empty(f))
		return -EBADFD;