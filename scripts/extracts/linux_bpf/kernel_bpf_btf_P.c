// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 48/55



			attr = kzalloc_obj(*attr);
			if (!attr)
				goto out;

			sysfs_bin_attr_init(attr);
			attr->attr.name = btf->name;
			attr->attr.mode = 0444;
			attr->size = btf->data_size;
			attr->private = btf->data;
			attr->read = sysfs_bin_attr_simple_read;

			err = sysfs_create_bin_file(btf_kobj, attr);
			if (err) {
				pr_warn("failed to register module [%s] BTF in sysfs: %d\n",
					mod->name, err);
				kfree(attr);
				err = 0;
				goto out;
			}

			btf_mod->sysfs_attr = attr;
		}

		break;
	case MODULE_STATE_LIVE:
		mutex_lock(&btf_module_mutex);
		list_for_each_entry_safe(btf_mod, tmp, &btf_modules, list) {
			if (btf_mod->module != module)
				continue;

			btf_mod->flags |= BTF_MODULE_F_LIVE;
			break;
		}
		mutex_unlock(&btf_module_mutex);
		break;
	case MODULE_STATE_GOING:
		mutex_lock(&btf_module_mutex);
		list_for_each_entry_safe(btf_mod, tmp, &btf_modules, list) {
			if (btf_mod->module != module)
				continue;

			/*
			 * For modules, we do the freeing of BTF IDR as soon as
			 * module goes away to disable BTF discovery, since the
			 * btf_try_get_module() on such BTFs will fail. This may
			 * be called again on btf_put(), but it's ok to do so.
			 */
			btf_free_id(btf_mod->btf);
			list_del(&btf_mod->list);
			if (btf_mod->sysfs_attr)
				sysfs_remove_bin_file(btf_kobj, btf_mod->sysfs_attr);
			purge_cand_cache(btf_mod->btf);
			btf_put(btf_mod->btf);
			kfree(btf_mod->sysfs_attr);
			kfree(btf_mod);
			break;
		}
		mutex_unlock(&btf_module_mutex);
		break;
	}
out:
	return notifier_from_errno(err);
}

static struct notifier_block btf_module_nb = {
	.notifier_call = btf_module_notify,
};

static int __init btf_module_init(void)
{
	register_module_notifier(&btf_module_nb);
	return 0;
}

fs_initcall(btf_module_init);
#endif /* CONFIG_DEBUG_INFO_BTF_MODULES */

struct module *btf_try_get_module(const struct btf *btf)
{
	struct module *res = NULL;
#ifdef CONFIG_DEBUG_INFO_BTF_MODULES
	struct btf_module *btf_mod, *tmp;

	mutex_lock(&btf_module_mutex);
	list_for_each_entry_safe(btf_mod, tmp, &btf_modules, list) {
		if (btf_mod->btf != btf)
			continue;

		/* We must only consider module whose __init routine has
		 * finished, hence we must check for BTF_MODULE_F_LIVE flag,
		 * which is set from the notifier callback for
		 * MODULE_STATE_LIVE.
		 */
		if ((btf_mod->flags & BTF_MODULE_F_LIVE) && try_module_get(btf_mod->module))
			res = btf_mod->module;

		break;
	}
	mutex_unlock(&btf_module_mutex);
#endif

	return res;
}

/* Returns struct btf corresponding to the struct module.
 * This function can return NULL or ERR_PTR.
 */
static struct btf *btf_get_module_btf(const struct module *module)
{
#ifdef CONFIG_DEBUG_INFO_BTF_MODULES
	struct btf_module *btf_mod, *tmp;
#endif
	struct btf *btf = NULL;

	if (!module) {
		btf = bpf_get_btf_vmlinux();
		if (!IS_ERR_OR_NULL(btf))
			btf_get(btf);
		return btf;
	}

#ifdef CONFIG_DEBUG_INFO_BTF_MODULES
	mutex_lock(&btf_module_mutex);
	list_for_each_entry_safe(btf_mod, tmp, &btf_modules, list) {
		if (btf_mod->module != module)
			continue;

		btf_get(btf_mod->btf);
		btf = btf_mod->btf;
		break;
	}
	mutex_unlock(&btf_module_mutex);
#endif

	return btf;
}

static int check_btf_kconfigs(const struct module *module, const char *feature)
{
	if (!module && IS_ENABLED(CONFIG_DEBUG_INFO_BTF)) {
		pr_err("missing vmlinux BTF, cannot register %s\n", feature);
		return -ENOENT;
	}
	if (module && IS_ENABLED(CONFIG_DEBUG_INFO_BTF_MODULES))
		pr_warn("missing module BTF, cannot register %s\n", feature);
	return 0;
}

BPF_CALL_4(bpf_btf_find_by_name_kind, char *, name, int, name_sz, u32, kind, int, flags)
{
	struct btf *btf = NULL;
	int btf_obj_fd = 0;
	long ret;

	if (flags)
		return -EINVAL;

	if (name_sz <= 1 || name[name_sz - 1])
		return -EINVAL;

	ret = bpf_find_btf_id(name, kind, &btf);
	if (ret > 0 && btf_is_module(btf)) {
		btf_obj_fd = __btf_new_fd(btf);
		if (btf_obj_fd < 0) {
			btf_put(btf);
			return btf_obj_fd;
		}
		return ret | (((u64)btf_obj_fd) << 32);
	}
	if (ret > 0)
		btf_put(btf);
	return ret;
}

const struct bpf_func_proto bpf_btf_find_by_name_kind_proto = {
	.func		= bpf_btf_find_by_name_kind,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_MEM | MEM_RDONLY,
	.arg2_type	= ARG_CONST_SIZE,
	.arg3_type	= ARG_ANYTHING,
	.arg4_type	= ARG_ANYTHING,
};

BTF_ID_LIST_GLOBAL(btf_tracing_ids, MAX_BTF_TRACING_TYPE)
#define BTF_TRACING_TYPE(name, type) BTF_ID(struct, type)
BTF_TRACING_TYPE_xxx
#undef BTF_TRACING_TYPE

/* Validate well-formedness of iter argument type.
 * On success, return positive BTF ID of iter state's STRUCT type.
 * On error, negative error is returned.
 */
int btf_check_iter_arg(struct btf *btf, const struct btf_type *func, int arg_idx)
{
	const struct btf_param *arg;
	const struct btf_type *t;
	const char *name;
	int btf_id;

	if (btf_type_vlen(func) <= arg_idx)
		return -EINVAL;