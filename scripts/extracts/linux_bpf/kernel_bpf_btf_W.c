// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 55/55



	/* Exactly one of the two type names may be suffixed with ___init, so
	 * if the strings are the same size, they can't possibly be no-cast
	 * aliases of one another. If you have two of the same type names, e.g.
	 * they're both nf_conn___init, it would be improper to return true
	 * because they are _not_ no-cast aliases, they are the same type.
	 */
	if (reg_len == arg_len)
		return false;

	/* Either of the two names must be the other name, suffixed with ___init. */
	if ((reg_len != arg_len + pattern_len) &&
	    (arg_len != reg_len + pattern_len))
		return false;

	if (reg_len < arg_len) {
		search_needle = strstr(arg_name, NOCAST_ALIAS_SUFFIX);
		cmp_len = reg_len;
	} else {
		search_needle = strstr(reg_name, NOCAST_ALIAS_SUFFIX);
		cmp_len = arg_len;
	}

	if (!search_needle)
		return false;

	/* ___init suffix must come at the end of the name */
	if (*(search_needle + pattern_len) != '\0')
		return false;

	return !strncmp(reg_name, arg_name, cmp_len);
}

#ifdef CONFIG_BPF_JIT
static int
btf_add_struct_ops(struct btf *btf, struct bpf_struct_ops *st_ops,
		   struct bpf_verifier_log *log)
{
	struct btf_struct_ops_tab *tab, *new_tab;
	int i, err;

	tab = btf->struct_ops_tab;
	if (!tab) {
		tab = kzalloc_flex(*tab, ops, 4);
		if (!tab)
			return -ENOMEM;
		tab->capacity = 4;
		btf->struct_ops_tab = tab;
	}

	for (i = 0; i < tab->cnt; i++)
		if (tab->ops[i].st_ops == st_ops)
			return -EEXIST;

	if (tab->cnt == tab->capacity) {
		new_tab = krealloc(tab,
				   struct_size(tab, ops, tab->capacity * 2),
				   GFP_KERNEL);
		if (!new_tab)
			return -ENOMEM;
		tab = new_tab;
		tab->capacity *= 2;
		btf->struct_ops_tab = tab;
	}

	tab->ops[btf->struct_ops_tab->cnt].st_ops = st_ops;

	err = bpf_struct_ops_desc_init(&tab->ops[btf->struct_ops_tab->cnt], btf, log);
	if (err)
		return err;

	btf->struct_ops_tab->cnt++;

	return 0;
}

const struct bpf_struct_ops_desc *
bpf_struct_ops_find_value(struct btf *btf, u32 value_id)
{
	const struct bpf_struct_ops_desc *st_ops_list;
	unsigned int i;
	u32 cnt;

	if (!value_id)
		return NULL;
	if (!btf->struct_ops_tab)
		return NULL;

	cnt = btf->struct_ops_tab->cnt;
	st_ops_list = btf->struct_ops_tab->ops;
	for (i = 0; i < cnt; i++) {
		if (st_ops_list[i].value_id == value_id)
			return &st_ops_list[i];
	}

	return NULL;
}

const struct bpf_struct_ops_desc *
bpf_struct_ops_find(struct btf *btf, u32 type_id)
{
	const struct bpf_struct_ops_desc *st_ops_list;
	unsigned int i;
	u32 cnt;

	if (!type_id)
		return NULL;
	if (!btf->struct_ops_tab)
		return NULL;

	cnt = btf->struct_ops_tab->cnt;
	st_ops_list = btf->struct_ops_tab->ops;
	for (i = 0; i < cnt; i++) {
		if (st_ops_list[i].type_id == type_id)
			return &st_ops_list[i];
	}

	return NULL;
}

int __register_bpf_struct_ops(struct bpf_struct_ops *st_ops)
{
	struct bpf_verifier_log *log;
	struct btf *btf;
	int err = 0;

	btf = btf_get_module_btf(st_ops->owner);
	if (!btf)
		return check_btf_kconfigs(st_ops->owner, "struct_ops");
	if (IS_ERR(btf))
		return PTR_ERR(btf);

	log = kzalloc_obj(*log, GFP_KERNEL | __GFP_NOWARN);
	if (!log) {
		err = -ENOMEM;
		goto errout;
	}

	log->level = BPF_LOG_KERNEL;

	err = btf_add_struct_ops(btf, st_ops, log);

errout:
	kfree(log);
	btf_put(btf);

	return err;
}
EXPORT_SYMBOL_GPL(__register_bpf_struct_ops);
#endif

bool btf_param_match_suffix(const struct btf *btf,
			    const struct btf_param *arg,
			    const char *suffix)
{
	int suffix_len = strlen(suffix), len;
	const char *param_name;

	/* In the future, this can be ported to use BTF tagging */
	param_name = btf_name_by_offset(btf, arg->name_off);
	if (str_is_empty(param_name))
		return false;
	len = strlen(param_name);
	if (len <= suffix_len)
		return false;
	param_name += len - suffix_len;
	return !strncmp(param_name, suffix, suffix_len);
}
