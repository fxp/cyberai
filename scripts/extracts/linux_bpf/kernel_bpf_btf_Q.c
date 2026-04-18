// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 49/55



	arg = &btf_params(func)[arg_idx];
	t = btf_type_skip_modifiers(btf, arg->type, NULL);
	if (!t || !btf_type_is_ptr(t))
		return -EINVAL;
	t = btf_type_skip_modifiers(btf, t->type, &btf_id);
	if (!t || !__btf_type_is_struct(t))
		return -EINVAL;

	name = btf_name_by_offset(btf, t->name_off);
	if (!name || strncmp(name, ITER_PREFIX, sizeof(ITER_PREFIX) - 1))
		return -EINVAL;

	return btf_id;
}

static int btf_check_iter_kfuncs(struct btf *btf, const char *func_name,
				 const struct btf_type *func, u32 func_flags)
{
	u32 flags = func_flags & (KF_ITER_NEW | KF_ITER_NEXT | KF_ITER_DESTROY);
	const char *sfx, *iter_name;
	const struct btf_type *t;
	char exp_name[128];
	u32 nr_args;
	int btf_id;

	/* exactly one of KF_ITER_{NEW,NEXT,DESTROY} can be set */
	if (!flags || (flags & (flags - 1)))
		return -EINVAL;

	/* any BPF iter kfunc should have `struct bpf_iter_<type> *` first arg */
	nr_args = btf_type_vlen(func);
	if (nr_args < 1)
		return -EINVAL;

	btf_id = btf_check_iter_arg(btf, func, 0);
	if (btf_id < 0)
		return btf_id;

	/* sizeof(struct bpf_iter_<type>) should be a multiple of 8 to
	 * fit nicely in stack slots
	 */
	t = btf_type_by_id(btf, btf_id);
	if (t->size == 0 || (t->size % 8))
		return -EINVAL;

	/* validate bpf_iter_<type>_{new,next,destroy}(struct bpf_iter_<type> *)
	 * naming pattern
	 */
	iter_name = btf_name_by_offset(btf, t->name_off) + sizeof(ITER_PREFIX) - 1;
	if (flags & KF_ITER_NEW)
		sfx = "new";
	else if (flags & KF_ITER_NEXT)
		sfx = "next";
	else /* (flags & KF_ITER_DESTROY) */
		sfx = "destroy";

	snprintf(exp_name, sizeof(exp_name), "bpf_iter_%s_%s", iter_name, sfx);
	if (strcmp(func_name, exp_name))
		return -EINVAL;

	/* only iter constructor should have extra arguments */
	if (!(flags & KF_ITER_NEW) && nr_args != 1)
		return -EINVAL;

	if (flags & KF_ITER_NEXT) {
		/* bpf_iter_<type>_next() should return pointer */
		t = btf_type_skip_modifiers(btf, func->type, NULL);
		if (!t || !btf_type_is_ptr(t))
			return -EINVAL;
	}

	if (flags & KF_ITER_DESTROY) {
		/* bpf_iter_<type>_destroy() should return void */
		t = btf_type_by_id(btf, func->type);
		if (!t || !btf_type_is_void(t))
			return -EINVAL;
	}

	return 0;
}

static int btf_check_kfunc_protos(struct btf *btf, u32 func_id, u32 func_flags)
{
	const struct btf_type *func;
	const char *func_name;
	int err;

	/* any kfunc should be FUNC -> FUNC_PROTO */
	func = btf_type_by_id(btf, func_id);
	if (!func || !btf_type_is_func(func))
		return -EINVAL;

	/* sanity check kfunc name */
	func_name = btf_name_by_offset(btf, func->name_off);
	if (!func_name || !func_name[0])
		return -EINVAL;

	func = btf_type_by_id(btf, func->type);
	if (!func || !btf_type_is_func_proto(func))
		return -EINVAL;

	if (func_flags & (KF_ITER_NEW | KF_ITER_NEXT | KF_ITER_DESTROY)) {
		err = btf_check_iter_kfuncs(btf, func_name, func, func_flags);
		if (err)
			return err;
	}

	return 0;
}

/* Kernel Function (kfunc) BTF ID set registration API */

static int btf_populate_kfunc_set(struct btf *btf, enum btf_kfunc_hook hook,
				  const struct btf_kfunc_id_set *kset)
{
	struct btf_kfunc_hook_filter *hook_filter;
	struct btf_id_set8 *add_set = kset->set;
	bool vmlinux_set = !btf_is_module(btf);
	bool add_filter = !!kset->filter;
	struct btf_kfunc_set_tab *tab;
	struct btf_id_set8 *set;
	u32 set_cnt, i;
	int ret;

	if (hook >= BTF_KFUNC_HOOK_MAX) {
		ret = -EINVAL;
		goto end;
	}

	if (!add_set->cnt)
		return 0;

	tab = btf->kfunc_set_tab;

	if (tab && add_filter) {
		u32 i;

		hook_filter = &tab->hook_filters[hook];
		for (i = 0; i < hook_filter->nr_filters; i++) {
			if (hook_filter->filters[i] == kset->filter) {
				add_filter = false;
				break;
			}
		}

		if (add_filter && hook_filter->nr_filters == BTF_KFUNC_FILTER_MAX_CNT) {
			ret = -E2BIG;
			goto end;
		}
	}

	if (!tab) {
		tab = kzalloc_obj(*tab, GFP_KERNEL | __GFP_NOWARN);
		if (!tab)
			return -ENOMEM;
		btf->kfunc_set_tab = tab;
	}

	set = tab->sets[hook];
	/* Warn when register_btf_kfunc_id_set is called twice for the same hook
	 * for module sets.
	 */
	if (WARN_ON_ONCE(set && !vmlinux_set)) {
		ret = -EINVAL;
		goto end;
	}

	/* In case of vmlinux sets, there may be more than one set being
	 * registered per hook. To create a unified set, we allocate a new set
	 * and concatenate all individual sets being registered. While each set
	 * is individually sorted, they may become unsorted when concatenated,
	 * hence re-sorting the final set again is required to make binary
	 * searching the set using btf_id_set8_contains function work.
	 *
	 * For module sets, we need to allocate as we may need to relocate
	 * BTF ids.
	 */
	set_cnt = set ? set->cnt : 0;

	if (set_cnt > U32_MAX - add_set->cnt) {
		ret = -EOVERFLOW;
		goto end;
	}

	if (set_cnt + add_set->cnt > BTF_KFUNC_SET_MAX_CNT) {
		ret = -E2BIG;
		goto end;
	}