// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 24/55



		/* Type exists only in program BTF. Assume that it's a MEM_ALLOC
		 * kptr allocated via bpf_obj_new
		 */
		field->kptr.dtor = NULL;
		id = info->kptr.type_id;
		kptr_btf = (struct btf *)btf;
		goto found_dtor;
	}
	if (id < 0)
		return id;

	/* Find and stash the function pointer for the destruction function that
	 * needs to be eventually invoked from the map free path.
	 */
	if (info->type == BPF_KPTR_REF) {
		const struct btf_type *dtor_func;
		const char *dtor_func_name;
		unsigned long addr;
		s32 dtor_btf_id;

		/* This call also serves as a whitelist of allowed objects that
		 * can be used as a referenced pointer and be stored in a map at
		 * the same time.
		 */
		dtor_btf_id = btf_find_dtor_kfunc(kptr_btf, id);
		if (dtor_btf_id < 0) {
			ret = dtor_btf_id;
			goto end_btf;
		}

		dtor_func = btf_type_by_id(kptr_btf, dtor_btf_id);
		if (!dtor_func) {
			ret = -ENOENT;
			goto end_btf;
		}

		if (btf_is_module(kptr_btf)) {
			mod = btf_try_get_module(kptr_btf);
			if (!mod) {
				ret = -ENXIO;
				goto end_btf;
			}
		}

		/* We already verified dtor_func to be btf_type_is_func
		 * in register_btf_id_dtor_kfuncs.
		 */
		dtor_func_name = __btf_name_by_offset(kptr_btf, dtor_func->name_off);
		addr = kallsyms_lookup_name(dtor_func_name);
		if (!addr) {
			ret = -EINVAL;
			goto end_mod;
		}
		field->kptr.dtor = (void *)addr;
	}

found_dtor:
	field->kptr.btf_id = id;
	field->kptr.btf = kptr_btf;
	field->kptr.module = mod;
	return 0;
end_mod:
	module_put(mod);
end_btf:
	btf_put(kptr_btf);
	return ret;
}

static int btf_parse_graph_root(const struct btf *btf,
				struct btf_field *field,
				struct btf_field_info *info,
				const char *node_type_name,
				size_t node_type_align)
{
	const struct btf_type *t, *n = NULL;
	const struct btf_member *member;
	u32 offset;
	int i;

	t = btf_type_by_id(btf, info->graph_root.value_btf_id);
	/* We've already checked that value_btf_id is a struct type. We
	 * just need to figure out the offset of the list_node, and
	 * verify its type.
	 */
	for_each_member(i, t, member) {
		if (strcmp(info->graph_root.node_name,
			   __btf_name_by_offset(btf, member->name_off)))
			continue;
		/* Invalid BTF, two members with same name */
		if (n)
			return -EINVAL;
		n = btf_type_by_id(btf, member->type);
		if (!__btf_type_is_struct(n))
			return -EINVAL;
		if (strcmp(node_type_name, __btf_name_by_offset(btf, n->name_off)))
			return -EINVAL;
		offset = __btf_member_bit_offset(n, member);
		if (offset % 8)
			return -EINVAL;
		offset /= 8;
		if (offset % node_type_align)
			return -EINVAL;

		field->graph_root.btf = (struct btf *)btf;
		field->graph_root.value_btf_id = info->graph_root.value_btf_id;
		field->graph_root.node_offset = offset;
	}
	if (!n)
		return -ENOENT;
	return 0;
}

static int btf_parse_list_head(const struct btf *btf, struct btf_field *field,
			       struct btf_field_info *info)
{
	return btf_parse_graph_root(btf, field, info, "bpf_list_node",
					    __alignof__(struct bpf_list_node));
}

static int btf_parse_rb_root(const struct btf *btf, struct btf_field *field,
			     struct btf_field_info *info)
{
	return btf_parse_graph_root(btf, field, info, "bpf_rb_node",
					    __alignof__(struct bpf_rb_node));
}

static int btf_field_cmp(const void *_a, const void *_b, const void *priv)
{
	const struct btf_field *a = (const struct btf_field *)_a;
	const struct btf_field *b = (const struct btf_field *)_b;

	if (a->offset < b->offset)
		return -1;
	else if (a->offset > b->offset)
		return 1;
	return 0;
}

struct btf_record *btf_parse_fields(const struct btf *btf, const struct btf_type *t,
				    u32 field_mask, u32 value_size)
{
	struct btf_field_info info_arr[BTF_FIELDS_MAX];
	u32 next_off = 0, field_type_size;
	struct btf_record *rec;
	int ret, i, cnt;

	ret = btf_find_field(btf, t, field_mask, info_arr, ARRAY_SIZE(info_arr));
	if (ret < 0)
		return ERR_PTR(ret);
	if (!ret)
		return NULL;

	cnt = ret;
	/* This needs to be kzalloc to zero out padding and unused fields, see
	 * comment in btf_record_equal.
	 */
	rec = kzalloc_flex(*rec, fields, cnt, GFP_KERNEL_ACCOUNT | __GFP_NOWARN);
	if (!rec)
		return ERR_PTR(-ENOMEM);

	rec->spin_lock_off = -EINVAL;
	rec->res_spin_lock_off = -EINVAL;
	rec->timer_off = -EINVAL;
	rec->wq_off = -EINVAL;
	rec->refcount_off = -EINVAL;
	rec->task_work_off = -EINVAL;
	for (i = 0; i < cnt; i++) {
		field_type_size = btf_field_type_size(info_arr[i].type);
		if (info_arr[i].off + field_type_size > value_size) {
			WARN_ONCE(1, "verifier bug off %d size %d", info_arr[i].off, value_size);
			ret = -EFAULT;
			goto end;
		}
		if (info_arr[i].off < next_off) {
			ret = -EEXIST;
			goto end;
		}
		next_off = info_arr[i].off + field_type_size;

		rec->field_mask |= info_arr[i].type;
		rec->fields[i].offset = info_arr[i].off;
		rec->fields[i].type = info_arr[i].type;
		rec->fields[i].size = field_type_size;