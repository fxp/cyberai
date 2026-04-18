// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 38/43



/*
 * __cgroup_get_from_id : get the cgroup associated with cgroup id
 * @id: cgroup id
 * On success return the cgrp or ERR_PTR on failure
 * There are no cgroup NS restrictions.
 */
struct cgroup *__cgroup_get_from_id(u64 id)
{
	struct kernfs_node *kn;
	struct cgroup *cgrp;

	kn = kernfs_find_and_get_node_by_id(cgrp_dfl_root.kf_root, id);
	if (!kn)
		return ERR_PTR(-ENOENT);

	if (kernfs_type(kn) != KERNFS_DIR) {
		kernfs_put(kn);
		return ERR_PTR(-ENOENT);
	}

	rcu_read_lock();

	cgrp = rcu_dereference(*(void __rcu __force **)&kn->priv);
	if (cgrp && !cgroup_tryget(cgrp))
		cgrp = NULL;

	rcu_read_unlock();
	kernfs_put(kn);

	if (!cgrp)
		return ERR_PTR(-ENOENT);
	return cgrp;
}

/*
 * cgroup_get_from_id : get the cgroup associated with cgroup id
 * @id: cgroup id
 * On success return the cgrp or ERR_PTR on failure
 * Only cgroups within current task's cgroup NS are valid.
 */
struct cgroup *cgroup_get_from_id(u64 id)
{
	struct cgroup *cgrp, *root_cgrp;

	cgrp = __cgroup_get_from_id(id);
	if (IS_ERR(cgrp))
		return cgrp;

	root_cgrp = current_cgns_cgroup_dfl();
	if (!cgroup_is_descendant(cgrp, root_cgrp)) {
		cgroup_put(cgrp);
		return ERR_PTR(-ENOENT);
	}

	return cgrp;
}
EXPORT_SYMBOL_GPL(cgroup_get_from_id);

/*
 * proc_cgroup_show()
 *  - Print task's cgroup paths into seq_file, one line for each hierarchy
 *  - Used for /proc/<pid>/cgroup.
 */
int proc_cgroup_show(struct seq_file *m, struct pid_namespace *ns,
		     struct pid *pid, struct task_struct *tsk)
{
	char *buf;
	int retval;
	struct cgroup_root *root;

	retval = -ENOMEM;
	buf = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buf)
		goto out;

	rcu_read_lock();
	spin_lock_irq(&css_set_lock);

	for_each_root(root) {
		struct cgroup_subsys *ss;
		struct cgroup *cgrp;
		int ssid, count = 0;

		if (root == &cgrp_dfl_root && !READ_ONCE(cgrp_dfl_visible))
			continue;

		cgrp = task_cgroup_from_root(tsk, root);
		/* The root has already been unmounted. */
		if (!cgrp)
			continue;

		seq_printf(m, "%d:", root->hierarchy_id);
		if (root != &cgrp_dfl_root)
			for_each_subsys(ss, ssid)
				if (root->subsys_mask & (1 << ssid))
					seq_printf(m, "%s%s", count++ ? "," : "",
						   ss->legacy_name);
		if (strlen(root->name))
			seq_printf(m, "%sname=%s", count ? "," : "",
				   root->name);
		seq_putc(m, ':');
		/*
		 * On traditional hierarchies, all zombie tasks show up as
		 * belonging to the root cgroup.  On the default hierarchy,
		 * while a zombie doesn't show up in "cgroup.procs" and
		 * thus can't be migrated, its /proc/PID/cgroup keeps
		 * reporting the cgroup it belonged to before exiting.  If
		 * the cgroup is removed before the zombie is reaped,
		 * " (deleted)" is appended to the cgroup path.
		 */
		if (cgroup_on_dfl(cgrp) || !(tsk->flags & PF_EXITING)) {
			retval = cgroup_path_ns_locked(cgrp, buf, PATH_MAX,
						current->nsproxy->cgroup_ns);
			if (retval == -E2BIG)
				retval = -ENAMETOOLONG;
			if (retval < 0)
				goto out_unlock;

			seq_puts(m, buf);
		} else {
			seq_puts(m, "/");
		}

		if (cgroup_on_dfl(cgrp) && cgroup_is_dead(cgrp))
			seq_puts(m, " (deleted)\n");
		else
			seq_putc(m, '\n');
	}

	retval = 0;
out_unlock:
	spin_unlock_irq(&css_set_lock);
	rcu_read_unlock();
	kfree(buf);
out:
	return retval;
}

/**
 * cgroup_fork - initialize cgroup related fields during copy_process()
 * @child: pointer to task_struct of forking parent process.
 *
 * A task is associated with the init_css_set until cgroup_post_fork()
 * attaches it to the target css_set.
 */
void cgroup_fork(struct task_struct *child)
{
	RCU_INIT_POINTER(child->cgroups, &init_css_set);
	INIT_LIST_HEAD(&child->cg_list);
}

/**
 * cgroup_v1v2_get_from_file - get a cgroup pointer from a file pointer
 * @f: file corresponding to cgroup_dir
 *
 * Find the cgroup from a file pointer associated with a cgroup directory.
 * Returns a pointer to the cgroup on success. ERR_PTR is returned if the
 * cgroup cannot be found.
 */
static struct cgroup *cgroup_v1v2_get_from_file(struct file *f)
{
	struct cgroup_subsys_state *css;

	css = css_tryget_online_from_dir(f->f_path.dentry, NULL);
	if (IS_ERR(css))
		return ERR_CAST(css);

	return css->cgroup;
}

/**
 * cgroup_get_from_file - same as cgroup_v1v2_get_from_file, but only supports
 * cgroup2.
 * @f: file corresponding to cgroup2_dir
 */
static struct cgroup *cgroup_get_from_file(struct file *f)
{
	struct cgroup *cgrp = cgroup_v1v2_get_from_file(f);

	if (IS_ERR(cgrp))
		return ERR_CAST(cgrp);

	if (!cgroup_on_dfl(cgrp)) {
		cgroup_put(cgrp);
		return ERR_PTR(-EBADF);
	}

	return cgrp;
}