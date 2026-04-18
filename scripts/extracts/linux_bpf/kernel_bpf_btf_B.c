// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 34/55



		type = &tab->types[tab->cnt];
		type->btf_id = i;
		record = btf_parse_fields(btf, t, BPF_SPIN_LOCK | BPF_RES_SPIN_LOCK | BPF_LIST_HEAD | BPF_LIST_NODE |
						  BPF_RB_ROOT | BPF_RB_NODE | BPF_REFCOUNT |
						  BPF_KPTR, t->size);
		/* The record cannot be unset, treat it as an error if so */
		if (IS_ERR_OR_NULL(record)) {
			ret = PTR_ERR_OR_ZERO(record) ?: -EFAULT;
			goto free;
		}
		type->record = record;
		tab->cnt++;
	}
	kfree(aof);
	return tab;
free:
	btf_struct_metas_free(tab);
free_aof:
	kfree(aof);
	return ERR_PTR(ret);
}

struct btf_struct_meta *btf_find_struct_meta(const struct btf *btf, u32 btf_id)
{
	struct btf_struct_metas *tab;

	BUILD_BUG_ON(offsetof(struct btf_struct_meta, btf_id) != 0);
	tab = btf->struct_meta_tab;
	if (!tab)
		return NULL;
	return bsearch(&btf_id, tab->types, tab->cnt, sizeof(tab->types[0]), btf_id_cmp_func);
}

static int btf_check_type_tags(struct btf_verifier_env *env,
			       struct btf *btf, int start_id)
{
	int i, n, good_id = start_id - 1;
	bool in_tags;

	n = btf_nr_types(btf);
	for (i = start_id; i < n; i++) {
		const struct btf_type *t;
		int chain_limit = 32;
		u32 cur_id = i;

		t = btf_type_by_id(btf, i);
		if (!t)
			return -EINVAL;
		if (!btf_type_is_modifier(t))
			continue;

		cond_resched();

		in_tags = btf_type_is_type_tag(t);
		while (btf_type_is_modifier(t)) {
			if (!chain_limit--) {
				btf_verifier_log(env, "Max chain length or cycle detected");
				return -ELOOP;
			}
			if (btf_type_is_type_tag(t)) {
				if (!in_tags) {
					btf_verifier_log(env, "Type tags don't precede modifiers");
					return -EINVAL;
				}
			} else if (in_tags) {
				in_tags = false;
			}
			if (cur_id <= good_id)
				break;
			/* Move to next type */
			cur_id = t->type;
			t = btf_type_by_id(btf, cur_id);
			if (!t)
				return -EINVAL;
		}
		good_id = i;
	}
	return 0;
}

static int finalize_log(struct bpf_verifier_log *log, bpfptr_t uattr, u32 uattr_size)
{
	u32 log_true_size;
	int err;

	err = bpf_vlog_finalize(log, &log_true_size);

	if (uattr_size >= offsetofend(union bpf_attr, btf_log_true_size) &&
	    copy_to_bpfptr_offset(uattr, offsetof(union bpf_attr, btf_log_true_size),
				  &log_true_size, sizeof(log_true_size)))
		err = -EFAULT;

	return err;
}

static struct btf *btf_parse(const union bpf_attr *attr, bpfptr_t uattr, u32 uattr_size)
{
	bpfptr_t btf_data = make_bpfptr(attr->btf, uattr.is_kernel);
	char __user *log_ubuf = u64_to_user_ptr(attr->btf_log_buf);
	struct btf_struct_metas *struct_meta_tab;
	struct btf_verifier_env *env = NULL;
	struct btf *btf = NULL;
	u8 *data;
	int err, ret;

	if (attr->btf_size > BTF_MAX_SIZE)
		return ERR_PTR(-E2BIG);

	env = kzalloc_obj(*env, GFP_KERNEL | __GFP_NOWARN);
	if (!env)
		return ERR_PTR(-ENOMEM);

	/* user could have requested verbose verifier output
	 * and supplied buffer to store the verification trace
	 */
	err = bpf_vlog_init(&env->log, attr->btf_log_level,
			    log_ubuf, attr->btf_log_size);
	if (err)
		goto errout_free;

	btf = kzalloc_obj(*btf, GFP_KERNEL | __GFP_NOWARN);
	if (!btf) {
		err = -ENOMEM;
		goto errout;
	}
	env->btf = btf;
	btf->named_start_id = 0;

	data = kvmalloc(attr->btf_size, GFP_KERNEL | __GFP_NOWARN);
	if (!data) {
		err = -ENOMEM;
		goto errout;
	}

	btf->data = data;
	btf->data_size = attr->btf_size;

	if (copy_from_bpfptr(data, btf_data, attr->btf_size)) {
		err = -EFAULT;
		goto errout;
	}

	err = btf_parse_hdr(env);
	if (err)
		goto errout;

	btf->nohdr_data = btf->data + btf->hdr.hdr_len;

	err = btf_parse_str_sec(env);
	if (err)
		goto errout;

	err = btf_parse_layout_sec(env);
	if (err)
		goto errout;

	err = btf_parse_type_sec(env);
	if (err)
		goto errout;

	err = btf_check_type_tags(env, btf, 1);
	if (err)
		goto errout;

	struct_meta_tab = btf_parse_struct_metas(&env->log, btf);
	if (IS_ERR(struct_meta_tab)) {
		err = PTR_ERR(struct_meta_tab);
		goto errout;
	}
	btf->struct_meta_tab = struct_meta_tab;

	if (struct_meta_tab) {
		int i;

		for (i = 0; i < struct_meta_tab->cnt; i++) {
			err = btf_check_and_fixup_fields(btf, struct_meta_tab->types[i].record);
			if (err < 0)
				goto errout_meta;
		}
	}

	err = finalize_log(&env->log, uattr, uattr_size);
	if (err)
		goto errout_free;

	btf_verifier_env_free(env);
	refcount_set(&btf->refcnt, 1);
	return btf;

errout_meta:
	btf_free_struct_meta_tab(btf);
errout:
	/* overwrite err with -ENOSPC or -EFAULT */
	ret = finalize_log(&env->log, uattr, uattr_size);
	if (ret)
		err = ret;
errout_free:
	btf_verifier_env_free(env);
	if (btf)
		btf_free(btf);
	return ERR_PTR(err);
}

extern char __start_BTF[];
extern char __stop_BTF[];
extern struct btf *btf_vmlinux;