// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 42/43



/**
 * cgroup_get_from_path - lookup and get a cgroup from its default hierarchy path
 * @path: path on the default hierarchy
 *
 * Find the cgroup at @path on the default hierarchy, increment its
 * reference count and return it.  Returns pointer to the found cgroup on
 * success, ERR_PTR(-ENOENT) if @path doesn't exist or if the cgroup has already
 * been released and ERR_PTR(-ENOTDIR) if @path points to a non-directory.
 */
struct cgroup *cgroup_get_from_path(const char *path)
{
	struct kernfs_node *kn;
	struct cgroup *cgrp = ERR_PTR(-ENOENT);
	struct cgroup *root_cgrp;

	root_cgrp = current_cgns_cgroup_dfl();
	kn = kernfs_walk_and_get(root_cgrp->kn, path);
	if (!kn)
		goto out;

	if (kernfs_type(kn) != KERNFS_DIR) {
		cgrp = ERR_PTR(-ENOTDIR);
		goto out_kernfs;
	}

	rcu_read_lock();

	cgrp = rcu_dereference(*(void __rcu __force **)&kn->priv);
	if (!cgrp || !cgroup_tryget(cgrp))
		cgrp = ERR_PTR(-ENOENT);

	rcu_read_unlock();

out_kernfs:
	kernfs_put(kn);
out:
	return cgrp;
}
EXPORT_SYMBOL_GPL(cgroup_get_from_path);

/**
 * cgroup_v1v2_get_from_fd - get a cgroup pointer from a fd
 * @fd: fd obtained by open(cgroup_dir)
 *
 * Find the cgroup from a fd which should be obtained
 * by opening a cgroup directory.  Returns a pointer to the
 * cgroup on success. ERR_PTR is returned if the cgroup
 * cannot be found.
 */
struct cgroup *cgroup_v1v2_get_from_fd(int fd)
{
	CLASS(fd_raw, f)(fd);
	if (fd_empty(f))
		return ERR_PTR(-EBADF);

	return cgroup_v1v2_get_from_file(fd_file(f));
}

/**
 * cgroup_get_from_fd - same as cgroup_v1v2_get_from_fd, but only supports
 * cgroup2.
 * @fd: fd obtained by open(cgroup2_dir)
 */
struct cgroup *cgroup_get_from_fd(int fd)
{
	struct cgroup *cgrp = cgroup_v1v2_get_from_fd(fd);

	if (IS_ERR(cgrp))
		return ERR_CAST(cgrp);

	if (!cgroup_on_dfl(cgrp)) {
		cgroup_put(cgrp);
		return ERR_PTR(-EBADF);
	}
	return cgrp;
}
EXPORT_SYMBOL_GPL(cgroup_get_from_fd);

static u64 power_of_ten(int power)
{
	u64 v = 1;
	while (power--)
		v *= 10;
	return v;
}

/**
 * cgroup_parse_float - parse a floating number
 * @input: input string
 * @dec_shift: number of decimal digits to shift
 * @v: output
 *
 * Parse a decimal floating point number in @input and store the result in
 * @v with decimal point right shifted @dec_shift times.  For example, if
 * @input is "12.3456" and @dec_shift is 3, *@v will be set to 12345.
 * Returns 0 on success, -errno otherwise.
 *
 * There's nothing cgroup specific about this function except that it's
 * currently the only user.
 */
int cgroup_parse_float(const char *input, unsigned dec_shift, s64 *v)
{
	s64 whole, frac = 0;
	int fstart = 0, fend = 0, flen;

	if (!sscanf(input, "%lld.%n%lld%n", &whole, &fstart, &frac, &fend))
		return -EINVAL;
	if (frac < 0)
		return -EINVAL;

	flen = fend > fstart ? fend - fstart : 0;
	if (flen < dec_shift)
		frac *= power_of_ten(dec_shift - flen);
	else
		frac = DIV_ROUND_CLOSEST_ULL(frac, power_of_ten(flen - dec_shift));

	*v = whole * power_of_ten(dec_shift) + frac;
	return 0;
}

/*
 * sock->sk_cgrp_data handling.  For more info, see sock_cgroup_data
 * definition in cgroup-defs.h.
 */
#ifdef CONFIG_SOCK_CGROUP_DATA

void cgroup_sk_alloc(struct sock_cgroup_data *skcd)
{
	struct cgroup *cgroup;

	rcu_read_lock();
	/* Don't associate the sock with unrelated interrupted task's cgroup. */
	if (in_interrupt()) {
		cgroup = &cgrp_dfl_root.cgrp;
		cgroup_get(cgroup);
		goto out;
	}

	while (true) {
		struct css_set *cset;

		cset = task_css_set(current);
		if (likely(cgroup_tryget(cset->dfl_cgrp))) {
			cgroup = cset->dfl_cgrp;
			break;
		}
		cpu_relax();
	}
out:
	skcd->cgroup = cgroup;
	cgroup_bpf_get(cgroup);
	rcu_read_unlock();
}

void cgroup_sk_clone(struct sock_cgroup_data *skcd)
{
	struct cgroup *cgrp = sock_cgroup_ptr(skcd);

	/*
	 * We might be cloning a socket which is left in an empty
	 * cgroup and the cgroup might have already been rmdir'd.
	 * Don't use cgroup_get_live().
	 */
	cgroup_get(cgrp);
	cgroup_bpf_get(cgrp);
}

void cgroup_sk_free(struct sock_cgroup_data *skcd)
{
	struct cgroup *cgrp = sock_cgroup_ptr(skcd);

	cgroup_bpf_put(cgrp);
	cgroup_put(cgrp);
}

#endif	/* CONFIG_SOCK_CGROUP_DATA */

#ifdef CONFIG_SYSFS
static ssize_t show_delegatable_files(struct cftype *files, char *buf,
				      ssize_t size, const char *prefix)
{
	struct cftype *cft;
	ssize_t ret = 0;

	for (cft = files; cft && cft->name[0] != '\0'; cft++) {
		if (!(cft->flags & CFTYPE_NS_DELEGATABLE))
			continue;

		if (prefix)
			ret += snprintf(buf + ret, size - ret, "%s.", prefix);

		ret += snprintf(buf + ret, size - ret, "%s\n", cft->name);

		if (WARN_ON(ret >= size))
			break;
	}

	return ret;
}

static ssize_t delegate_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	struct cgroup_subsys *ss;
	int ssid;
	ssize_t ret = 0;