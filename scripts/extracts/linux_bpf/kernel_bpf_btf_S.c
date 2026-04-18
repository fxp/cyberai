// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 51/55



static int __register_btf_kfunc_id_set(enum btf_kfunc_hook hook,
				       const struct btf_kfunc_id_set *kset)
{
	struct btf *btf;
	int ret, i;

	btf = btf_get_module_btf(kset->owner);
	if (!btf)
		return check_btf_kconfigs(kset->owner, "kfunc");
	if (IS_ERR(btf))
		return PTR_ERR(btf);

	for (i = 0; i < kset->set->cnt; i++) {
		ret = btf_check_kfunc_protos(btf, btf_relocate_id(btf, kset->set->pairs[i].id),
					     kset->set->pairs[i].flags);
		if (ret)
			goto err_out;
	}

	ret = btf_populate_kfunc_set(btf, hook, kset);

err_out:
	btf_put(btf);
	return ret;
}

/* This function must be invoked only from initcalls/module init functions */
int register_btf_kfunc_id_set(enum bpf_prog_type prog_type,
			      const struct btf_kfunc_id_set *kset)
{
	enum btf_kfunc_hook hook;

	/* All kfuncs need to be tagged as such in BTF.
	 * WARN() for initcall registrations that do not check errors.
	 */
	if (!(kset->set->flags & BTF_SET8_KFUNCS)) {
		WARN_ON(!kset->owner);
		return -EINVAL;
	}

	hook = bpf_prog_type_to_kfunc_hook(prog_type);
	return __register_btf_kfunc_id_set(hook, kset);
}
EXPORT_SYMBOL_GPL(register_btf_kfunc_id_set);

/* This function must be invoked only from initcalls/module init functions */
int register_btf_fmodret_id_set(const struct btf_kfunc_id_set *kset)
{
	return __register_btf_kfunc_id_set(BTF_KFUNC_HOOK_FMODRET, kset);
}
EXPORT_SYMBOL_GPL(register_btf_fmodret_id_set);

s32 btf_find_dtor_kfunc(struct btf *btf, u32 btf_id)
{
	struct btf_id_dtor_kfunc_tab *tab = btf->dtor_kfunc_tab;
	struct btf_id_dtor_kfunc *dtor;

	if (!tab)
		return -ENOENT;
	/* Even though the size of tab->dtors[0] is > sizeof(u32), we only need
	 * to compare the first u32 with btf_id, so we can reuse btf_id_cmp_func.
	 */
	BUILD_BUG_ON(offsetof(struct btf_id_dtor_kfunc, btf_id) != 0);
	dtor = bsearch(&btf_id, tab->dtors, tab->cnt, sizeof(tab->dtors[0]), btf_id_cmp_func);
	if (!dtor)
		return -ENOENT;
	return dtor->kfunc_btf_id;
}

static int btf_check_dtor_kfuncs(struct btf *btf, const struct btf_id_dtor_kfunc *dtors, u32 cnt)
{
	const struct btf_type *dtor_func, *dtor_func_proto, *t;
	const struct btf_param *args;
	s32 dtor_btf_id;
	u32 nr_args, i;

	for (i = 0; i < cnt; i++) {
		dtor_btf_id = btf_relocate_id(btf, dtors[i].kfunc_btf_id);

		dtor_func = btf_type_by_id(btf, dtor_btf_id);
		if (!dtor_func || !btf_type_is_func(dtor_func))
			return -EINVAL;

		dtor_func_proto = btf_type_by_id(btf, dtor_func->type);
		if (!dtor_func_proto || !btf_type_is_func_proto(dtor_func_proto))
			return -EINVAL;

		/* Make sure the prototype of the destructor kfunc is 'void func(type *)' */
		t = btf_type_by_id(btf, dtor_func_proto->type);
		if (!t || !btf_type_is_void(t))
			return -EINVAL;

		nr_args = btf_type_vlen(dtor_func_proto);
		if (nr_args != 1)
			return -EINVAL;
		args = btf_params(dtor_func_proto);
		t = btf_type_by_id(btf, args[0].type);
		/* Allow any pointer type, as width on targets Linux supports
		 * will be same for all pointer types (i.e. sizeof(void *))
		 */
		if (!t || !btf_type_is_ptr(t))
			return -EINVAL;

		if (IS_ENABLED(CONFIG_CFI)) {
			/* Ensure the destructor kfunc type matches btf_dtor_kfunc_t */
			t = btf_type_by_id(btf, t->type);
			if (!btf_type_is_void(t))
				return -EINVAL;
		}
	}
	return 0;
}

/* This function must be invoked only from initcalls/module init functions */
int register_btf_id_dtor_kfuncs(const struct btf_id_dtor_kfunc *dtors, u32 add_cnt,
				struct module *owner)
{
	struct btf_id_dtor_kfunc_tab *tab;
	struct btf *btf;
	u32 tab_cnt, i;
	int ret;

	btf = btf_get_module_btf(owner);
	if (!btf)
		return check_btf_kconfigs(owner, "dtor kfuncs");
	if (IS_ERR(btf))
		return PTR_ERR(btf);

	if (add_cnt >= BTF_DTOR_KFUNC_MAX_CNT) {
		pr_err("cannot register more than %d kfunc destructors\n", BTF_DTOR_KFUNC_MAX_CNT);
		ret = -E2BIG;
		goto end;
	}

	/* Ensure that the prototype of dtor kfuncs being registered is sane */
	ret = btf_check_dtor_kfuncs(btf, dtors, add_cnt);
	if (ret < 0)
		goto end;

	tab = btf->dtor_kfunc_tab;
	/* Only one call allowed for modules */
	if (WARN_ON_ONCE(tab && btf_is_module(btf))) {
		ret = -EINVAL;
		goto end;
	}

	tab_cnt = tab ? tab->cnt : 0;
	if (tab_cnt > U32_MAX - add_cnt) {
		ret = -EOVERFLOW;
		goto end;
	}
	if (tab_cnt + add_cnt >= BTF_DTOR_KFUNC_MAX_CNT) {
		pr_err("cannot register more than %d kfunc destructors\n", BTF_DTOR_KFUNC_MAX_CNT);
		ret = -E2BIG;
		goto end;
	}

	tab = krealloc(btf->dtor_kfunc_tab,
		       struct_size(tab, dtors, tab_cnt + add_cnt),
		       GFP_KERNEL | __GFP_NOWARN);
	if (!tab) {
		ret = -ENOMEM;
		goto end;
	}

	if (!btf->dtor_kfunc_tab)
		tab->cnt = 0;
	btf->dtor_kfunc_tab = tab;

	memcpy(tab->dtors + tab->cnt, dtors, add_cnt * sizeof(tab->dtors[0]));

	/* remap BTF ids based on BTF relocation (if any) */
	for (i = tab_cnt; i < tab_cnt + add_cnt; i++) {
		tab->dtors[i].btf_id = btf_relocate_id(btf, tab->dtors[i].btf_id);
		tab->dtors[i].kfunc_btf_id = btf_relocate_id(btf, tab->dtors[i].kfunc_btf_id);
	}