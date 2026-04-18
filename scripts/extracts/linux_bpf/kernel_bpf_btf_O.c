// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 47/55



	/* Otherwise return length we would have written */
	return ssnprintf.len;
}

#ifdef CONFIG_PROC_FS
static void bpf_btf_show_fdinfo(struct seq_file *m, struct file *filp)
{
	const struct btf *btf = filp->private_data;

	seq_printf(m, "btf_id:\t%u\n", READ_ONCE(btf->id));
}
#endif

static int btf_release(struct inode *inode, struct file *filp)
{
	btf_put(filp->private_data);
	return 0;
}

const struct file_operations btf_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= bpf_btf_show_fdinfo,
#endif
	.release	= btf_release,
};

static int __btf_new_fd(struct btf *btf)
{
	return anon_inode_getfd("btf", &btf_fops, btf, O_RDONLY | O_CLOEXEC);
}

int btf_new_fd(const union bpf_attr *attr, bpfptr_t uattr, u32 uattr_size)
{
	struct btf *btf;
	int ret;

	btf = btf_parse(attr, uattr, uattr_size);
	if (IS_ERR(btf))
		return PTR_ERR(btf);

	ret = btf_alloc_id(btf);
	if (ret) {
		btf_free(btf);
		return ret;
	}

	/*
	 * The BTF ID is published to the userspace.
	 * All BTF free must go through call_rcu() from
	 * now on (i.e. free by calling btf_put()).
	 */

	ret = __btf_new_fd(btf);
	if (ret < 0)
		btf_put(btf);

	return ret;
}

struct btf *btf_get_by_fd(int fd)
{
	struct btf *btf;
	CLASS(fd, f)(fd);

	btf = __btf_get_by_fd(f);
	if (!IS_ERR(btf))
		refcount_inc(&btf->refcnt);

	return btf;
}

int btf_get_info_by_fd(const struct btf *btf,
		       const union bpf_attr *attr,
		       union bpf_attr __user *uattr)
{
	struct bpf_btf_info __user *uinfo;
	struct bpf_btf_info info;
	u32 info_copy, btf_copy;
	void __user *ubtf;
	char __user *uname;
	u32 uinfo_len, uname_len, name_len;
	int ret = 0;

	uinfo = u64_to_user_ptr(attr->info.info);
	uinfo_len = attr->info.info_len;

	info_copy = min_t(u32, uinfo_len, sizeof(info));
	memset(&info, 0, sizeof(info));
	if (copy_from_user(&info, uinfo, info_copy))
		return -EFAULT;

	info.id = READ_ONCE(btf->id);
	ubtf = u64_to_user_ptr(info.btf);
	btf_copy = min_t(u32, btf->data_size, info.btf_size);
	if (copy_to_user(ubtf, btf->data, btf_copy))
		return -EFAULT;
	info.btf_size = btf->data_size;

	info.kernel_btf = btf->kernel_btf;

	uname = u64_to_user_ptr(info.name);
	uname_len = info.name_len;
	if (!uname ^ !uname_len)
		return -EINVAL;

	name_len = strlen(btf->name);
	info.name_len = name_len;

	if (uname) {
		if (uname_len >= name_len + 1) {
			if (copy_to_user(uname, btf->name, name_len + 1))
				return -EFAULT;
		} else {
			char zero = '\0';

			if (copy_to_user(uname, btf->name, uname_len - 1))
				return -EFAULT;
			if (put_user(zero, uname + uname_len - 1))
				return -EFAULT;
			/* let user-space know about too short buffer */
			ret = -ENOSPC;
		}
	}

	if (copy_to_user(uinfo, &info, info_copy) ||
	    put_user(info_copy, &uattr->info.info_len))
		return -EFAULT;

	return ret;
}

int btf_get_fd_by_id(u32 id)
{
	struct btf *btf;
	int fd;

	rcu_read_lock();
	btf = idr_find(&btf_idr, id);
	if (!btf || !refcount_inc_not_zero(&btf->refcnt))
		btf = ERR_PTR(-ENOENT);
	rcu_read_unlock();

	if (IS_ERR(btf))
		return PTR_ERR(btf);

	fd = __btf_new_fd(btf);
	if (fd < 0)
		btf_put(btf);

	return fd;
}

u32 btf_obj_id(const struct btf *btf)
{
	return READ_ONCE(btf->id);
}

bool btf_is_kernel(const struct btf *btf)
{
	return btf->kernel_btf;
}

bool btf_is_module(const struct btf *btf)
{
	return btf->kernel_btf && strcmp(btf->name, "vmlinux") != 0;
}

enum {
	BTF_MODULE_F_LIVE = (1 << 0),
};

#ifdef CONFIG_DEBUG_INFO_BTF_MODULES
struct btf_module {
	struct list_head list;
	struct module *module;
	struct btf *btf;
	struct bin_attribute *sysfs_attr;
	int flags;
};

static LIST_HEAD(btf_modules);
static DEFINE_MUTEX(btf_module_mutex);

static void purge_cand_cache(struct btf *btf);

static int btf_module_notify(struct notifier_block *nb, unsigned long op,
			     void *module)
{
	struct btf_module *btf_mod, *tmp;
	struct module *mod = module;
	struct btf *btf;
	int err = 0;

	if (mod->btf_data_size == 0 ||
	    (op != MODULE_STATE_COMING && op != MODULE_STATE_LIVE &&
	     op != MODULE_STATE_GOING))
		goto out;

	switch (op) {
	case MODULE_STATE_COMING:
		btf_mod = kzalloc_obj(*btf_mod);
		if (!btf_mod) {
			err = -ENOMEM;
			goto out;
		}
		btf = btf_parse_module(mod->name, mod->btf_data, mod->btf_data_size,
				       mod->btf_base_data, mod->btf_base_data_size);
		if (IS_ERR(btf)) {
			kfree(btf_mod);
			if (!IS_ENABLED(CONFIG_MODULE_ALLOW_BTF_MISMATCH)) {
				pr_warn("failed to validate module [%s] BTF: %ld\n",
					mod->name, PTR_ERR(btf));
				err = PTR_ERR(btf);
			} else {
				pr_warn_once("Kernel module BTF mismatch detected, BTF debug info may be unavailable for some modules\n");
			}
			goto out;
		}
		err = btf_alloc_id(btf);
		if (err) {
			btf_free(btf);
			kfree(btf_mod);
			goto out;
		}

		purge_cand_cache(NULL);
		mutex_lock(&btf_module_mutex);
		btf_mod->module = module;
		btf_mod->btf = btf;
		list_add(&btf_mod->list, &btf_modules);
		mutex_unlock(&btf_module_mutex);

		if (IS_ENABLED(CONFIG_SYSFS)) {
			struct bin_attribute *attr;