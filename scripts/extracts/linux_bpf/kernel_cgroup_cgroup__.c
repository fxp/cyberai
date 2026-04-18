// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 31/43



	/* find the common ancestor */
	while (!cgroup_is_descendant(dst_cgrp, com_cgrp))
		com_cgrp = cgroup_parent(com_cgrp);

	/* %current should be authorized to migrate to the common ancestor */
	ret = cgroup_may_write(com_cgrp, sb);
	if (ret)
		return ret;

	/*
	 * If namespaces are delegation boundaries, %current must be able
	 * to see both source and destination cgroups from its namespace.
	 */
	if ((cgrp_dfl_root.flags & CGRP_ROOT_NS_DELEGATE) &&
	    (!cgroup_is_descendant(src_cgrp, ns->root_cset->dfl_cgrp) ||
	     !cgroup_is_descendant(dst_cgrp, ns->root_cset->dfl_cgrp)))
		return -ENOENT;

	return 0;
}

static int cgroup_attach_permissions(struct cgroup *src_cgrp,
				     struct cgroup *dst_cgrp,
				     struct super_block *sb, bool threadgroup,
				     struct cgroup_namespace *ns)
{
	int ret = 0;

	ret = cgroup_procs_write_permission(src_cgrp, dst_cgrp, sb, ns);
	if (ret)
		return ret;

	ret = cgroup_migrate_vet_dst(dst_cgrp);
	if (ret)
		return ret;

	if (!threadgroup && (src_cgrp->dom_cgrp != dst_cgrp->dom_cgrp))
		ret = -EOPNOTSUPP;

	return ret;
}

static ssize_t __cgroup_procs_write(struct kernfs_open_file *of, char *buf,
				    bool threadgroup)
{
	struct cgroup_file_ctx *ctx = of->priv;
	struct cgroup *src_cgrp, *dst_cgrp;
	struct task_struct *task;
	ssize_t ret;
	enum cgroup_attach_lock_mode lock_mode;

	dst_cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!dst_cgrp)
		return -ENODEV;

	task = cgroup_procs_write_start(buf, threadgroup, &lock_mode);
	ret = PTR_ERR_OR_ZERO(task);
	if (ret)
		goto out_unlock;

	/* find the source cgroup */
	spin_lock_irq(&css_set_lock);
	src_cgrp = task_cgroup_from_root(task, &cgrp_dfl_root);
	spin_unlock_irq(&css_set_lock);

	/*
	 * Process and thread migrations follow same delegation rule. Check
	 * permissions using the credentials from file open to protect against
	 * inherited fd attacks.
	 */
	scoped_with_creds(of->file->f_cred)
		ret = cgroup_attach_permissions(src_cgrp, dst_cgrp,
						of->file->f_path.dentry->d_sb,
						threadgroup, ctx->ns);
	if (ret)
		goto out_finish;

	ret = cgroup_attach_task(dst_cgrp, task, threadgroup);

out_finish:
	cgroup_procs_write_finish(task, lock_mode);
out_unlock:
	cgroup_kn_unlock(of->kn);

	return ret;
}

static ssize_t cgroup_procs_write(struct kernfs_open_file *of,
				  char *buf, size_t nbytes, loff_t off)
{
	return __cgroup_procs_write(of, buf, true) ?: nbytes;
}

static void *cgroup_threads_start(struct seq_file *s, loff_t *pos)
{
	return __cgroup_procs_start(s, pos, 0);
}

static ssize_t cgroup_threads_write(struct kernfs_open_file *of,
				    char *buf, size_t nbytes, loff_t off)
{
	return __cgroup_procs_write(of, buf, false) ?: nbytes;
}

/* cgroup core interface files for the default hierarchy */
static struct cftype cgroup_base_files[] = {
	{
		.name = "cgroup.type",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = cgroup_type_show,
		.write = cgroup_type_write,
	},
	{
		.name = "cgroup.procs",
		.flags = CFTYPE_NS_DELEGATABLE,
		.file_offset = offsetof(struct cgroup, procs_file),
		.release = cgroup_procs_release,
		.seq_start = cgroup_procs_start,
		.seq_next = cgroup_procs_next,
		.seq_show = cgroup_procs_show,
		.write = cgroup_procs_write,
	},
	{
		.name = "cgroup.threads",
		.flags = CFTYPE_NS_DELEGATABLE,
		.release = cgroup_procs_release,
		.seq_start = cgroup_threads_start,
		.seq_next = cgroup_procs_next,
		.seq_show = cgroup_procs_show,
		.write = cgroup_threads_write,
	},
	{
		.name = "cgroup.controllers",
		.seq_show = cgroup_controllers_show,
	},
	{
		.name = "cgroup.subtree_control",
		.flags = CFTYPE_NS_DELEGATABLE,
		.seq_show = cgroup_subtree_control_show,
		.write = cgroup_subtree_control_write,
	},
	{
		.name = "cgroup.events",
		.flags = CFTYPE_NOT_ON_ROOT,
		.file_offset = offsetof(struct cgroup, events_file),
		.seq_show = cgroup_events_show,
	},
	{
		.name = "cgroup.max.descendants",
		.seq_show = cgroup_max_descendants_show,
		.write = cgroup_max_descendants_write,
	},
	{
		.name = "cgroup.max.depth",
		.seq_show = cgroup_max_depth_show,
		.write = cgroup_max_depth_write,
	},
	{
		.name = "cgroup.stat",
		.seq_show = cgroup_stat_show,
	},
	{
		.name = "cgroup.stat.local",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = cgroup_core_local_stat_show,
	},
	{
		.name = "cgroup.freeze",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = cgroup_freeze_show,
		.write = cgroup_freeze_write,
	},
	{
		.name = "cgroup.kill",
		.flags = CFTYPE_NOT_ON_ROOT,
		.write = cgroup_kill_write,
	},
	{
		.name = "cpu.stat",
		.seq_show = cpu_stat_show,
	},
	{
		.name = "cpu.stat.local",
		.seq_show = cpu_local_stat_show,
	},
	{ }	/* terminate */
};