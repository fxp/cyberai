// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 33/55



	/* Check for gaps and overlap among sections */
	total = 0;
	expected_total = btf_data_size - hdr->hdr_len;
	for (i = 0; i < nr_secs; i++) {
		if (expected_total < secs[i].off) {
			btf_verifier_log(env, "Invalid section offset");
			return -EINVAL;
		}
		if (total < secs[i].off) {
			/* gap */
			btf_verifier_log(env, "Unsupported section found");
			return -EINVAL;
		}
		if (total > secs[i].off) {
			btf_verifier_log(env, "Section overlap found");
			return -EINVAL;
		}
		if (expected_total - total < secs[i].len) {
			btf_verifier_log(env,
					 "Total section length too long");
			return -EINVAL;
		}
		total += secs[i].len;
	}

	/* There is data other than hdr and known sections */
	if (expected_total != total) {
		btf_verifier_log(env, "Unsupported section found");
		return -EINVAL;
	}

	return 0;
}

static int btf_parse_hdr(struct btf_verifier_env *env)
{
	u32 hdr_len, hdr_copy, btf_data_size;
	const struct btf_header *hdr;
	struct btf *btf;

	btf = env->btf;
	btf_data_size = btf->data_size;

	if (btf_data_size < offsetofend(struct btf_header, hdr_len)) {
		btf_verifier_log(env, "hdr_len not found");
		return -EINVAL;
	}

	hdr = btf->data;
	hdr_len = hdr->hdr_len;
	if (btf_data_size < hdr_len) {
		btf_verifier_log(env, "btf_header not found");
		return -EINVAL;
	}

	/* Ensure the unsupported header fields are zero */
	if (hdr_len > sizeof(btf->hdr)) {
		u8 *expected_zero = btf->data + sizeof(btf->hdr);
		u8 *end = btf->data + hdr_len;

		for (; expected_zero < end; expected_zero++) {
			if (*expected_zero) {
				btf_verifier_log(env, "Unsupported btf_header");
				return -E2BIG;
			}
		}
	}

	hdr_copy = min_t(u32, hdr_len, sizeof(btf->hdr));
	memcpy(&btf->hdr, btf->data, hdr_copy);

	hdr = &btf->hdr;

	btf_verifier_log_hdr(env, btf_data_size);

	if (hdr->magic != BTF_MAGIC) {
		btf_verifier_log(env, "Invalid magic");
		return -EINVAL;
	}

	if (hdr->version != BTF_VERSION) {
		btf_verifier_log(env, "Unsupported version");
		return -ENOTSUPP;
	}

	if (hdr->flags) {
		btf_verifier_log(env, "Unsupported flags");
		return -ENOTSUPP;
	}

	if (!btf->base_btf && btf_data_size == hdr->hdr_len) {
		btf_verifier_log(env, "No data");
		return -EINVAL;
	}

	return btf_check_sec_info(env, btf_data_size);
}

static const char *alloc_obj_fields[] = {
	"bpf_spin_lock",
	"bpf_list_head",
	"bpf_list_node",
	"bpf_rb_root",
	"bpf_rb_node",
	"bpf_refcount",
};

static struct btf_struct_metas *
btf_parse_struct_metas(struct bpf_verifier_log *log, struct btf *btf)
{
	struct btf_struct_metas *tab = NULL;
	struct btf_id_set *aof;
	int i, n, id, ret;

	BUILD_BUG_ON(offsetof(struct btf_id_set, cnt) != 0);
	BUILD_BUG_ON(sizeof(struct btf_id_set) != sizeof(u32));

	aof = kmalloc_obj(*aof, GFP_KERNEL | __GFP_NOWARN);
	if (!aof)
		return ERR_PTR(-ENOMEM);
	aof->cnt = 0;

	for (i = 0; i < ARRAY_SIZE(alloc_obj_fields); i++) {
		/* Try to find whether this special type exists in user BTF, and
		 * if so remember its ID so we can easily find it among members
		 * of structs that we iterate in the next loop.
		 */
		struct btf_id_set *new_aof;

		id = btf_find_by_name_kind(btf, alloc_obj_fields[i], BTF_KIND_STRUCT);
		if (id < 0)
			continue;

		new_aof = krealloc(aof, struct_size(new_aof, ids, aof->cnt + 1),
				   GFP_KERNEL | __GFP_NOWARN);
		if (!new_aof) {
			ret = -ENOMEM;
			goto free_aof;
		}
		aof = new_aof;
		aof->ids[aof->cnt++] = id;
	}

	n = btf_nr_types(btf);
	for (i = 1; i < n; i++) {
		/* Try to find if there are kptrs in user BTF and remember their ID */
		struct btf_id_set *new_aof;
		struct btf_field_info tmp;
		const struct btf_type *t;

		t = btf_type_by_id(btf, i);
		if (!t) {
			ret = -EINVAL;
			goto free_aof;
		}

		ret = btf_find_kptr(btf, t, 0, 0, &tmp, BPF_KPTR);
		if (ret != BTF_FIELD_FOUND)
			continue;

		new_aof = krealloc(aof, struct_size(new_aof, ids, aof->cnt + 1),
				   GFP_KERNEL | __GFP_NOWARN);
		if (!new_aof) {
			ret = -ENOMEM;
			goto free_aof;
		}
		aof = new_aof;
		aof->ids[aof->cnt++] = i;
	}

	if (!aof->cnt) {
		kfree(aof);
		return NULL;
	}
	sort(&aof->ids, aof->cnt, sizeof(aof->ids[0]), btf_id_cmp_func, NULL);

	for (i = 1; i < n; i++) {
		struct btf_struct_metas *new_tab;
		const struct btf_member *member;
		struct btf_struct_meta *type;
		struct btf_record *record;
		const struct btf_type *t;
		int j, tab_cnt;

		t = btf_type_by_id(btf, i);
		if (!__btf_type_is_struct(t))
			continue;

		cond_resched();

		for_each_member(j, t, member) {
			if (btf_id_set_contains(aof, member->type))
				goto parse;
		}
		continue;
	parse:
		tab_cnt = tab ? tab->cnt : 0;
		new_tab = krealloc(tab, struct_size(new_tab, types, tab_cnt + 1),
				   GFP_KERNEL | __GFP_NOWARN);
		if (!new_tab) {
			ret = -ENOMEM;
			goto free;
		}
		if (!tab)
			new_tab->cnt = 0;
		tab = new_tab;