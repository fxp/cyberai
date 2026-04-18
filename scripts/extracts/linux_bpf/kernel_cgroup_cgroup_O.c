// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 15/43



	switch (opt) {
	case Opt_cpuset_v2_mode:
		ctx->flags |= CGRP_ROOT_CPUSET_V2_MODE;
		return 0;
	}
	return -EINVAL;
}

static const struct fs_context_operations cpuset_fs_context_ops = {
	.get_tree	= cgroup1_get_tree,
	.free		= cgroup_fs_context_free,
	.parse_param	= cpuset_parse_param,
};

/*
 * This is ugly, but preserves the userspace API for existing cpuset
 * users. If someone tries to mount the "cpuset" filesystem, we
 * silently switch it to mount "cgroup" instead
 */
static int cpuset_init_fs_context(struct fs_context *fc)
{
	char *agent = kstrdup("/sbin/cpuset_release_agent", GFP_USER);
	struct cgroup_fs_context *ctx;
	int err;

	err = cgroup_init_fs_context(fc);
	if (err) {
		kfree(agent);
		return err;
	}

	fc->ops = &cpuset_fs_context_ops;

	ctx = cgroup_fc2context(fc);
	ctx->subsys_mask = 1 << cpuset_cgrp_id;
	ctx->flags |= CGRP_ROOT_NOPREFIX;
	ctx->release_agent = agent;

	get_filesystem(&cgroup_fs_type);
	put_filesystem(fc->fs_type);
	fc->fs_type = &cgroup_fs_type;

	return 0;
}

static struct file_system_type cpuset_fs_type = {
	.name			= "cpuset",
	.init_fs_context	= cpuset_init_fs_context,
	.parameters		= cpuset_fs_parameters,
	.fs_flags		= FS_USERNS_MOUNT,
};
#endif

int cgroup_path_ns_locked(struct cgroup *cgrp, char *buf, size_t buflen,
			  struct cgroup_namespace *ns)
{
	struct cgroup *root = cset_cgroup_from_root(ns->root_cset, cgrp->root);

	return kernfs_path_from_node(cgrp->kn, root->kn, buf, buflen);
}

int cgroup_path_ns(struct cgroup *cgrp, char *buf, size_t buflen,
		   struct cgroup_namespace *ns)
{
	int ret;

	cgroup_lock();
	spin_lock_irq(&css_set_lock);

	ret = cgroup_path_ns_locked(cgrp, buf, buflen, ns);

	spin_unlock_irq(&css_set_lock);
	cgroup_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(cgroup_path_ns);

/**
 * cgroup_attach_lock - Lock for ->attach()
 * @lock_mode: whether acquire and acquire which rwsem
 * @tsk: thread group to lock
 *
 * cgroup migration sometimes needs to stabilize threadgroups against forks and
 * exits by write-locking cgroup_threadgroup_rwsem. However, some ->attach()
 * implementations (e.g. cpuset), also need to disable CPU hotplug.
 * Unfortunately, letting ->attach() operations acquire cpus_read_lock() can
 * lead to deadlocks.
 *
 * Bringing up a CPU may involve creating and destroying tasks which requires
 * read-locking threadgroup_rwsem, so threadgroup_rwsem nests inside
 * cpus_read_lock(). If we call an ->attach() which acquires the cpus lock while
 * write-locking threadgroup_rwsem, the locking order is reversed and we end up
 * waiting for an on-going CPU hotplug operation which in turn is waiting for
 * the threadgroup_rwsem to be released to create new tasks. For more details:
 *
 *   http://lkml.kernel.org/r/20220711174629.uehfmqegcwn2lqzu@wubuntu
 *
 * Resolve the situation by always acquiring cpus_read_lock() before optionally
 * write-locking cgroup_threadgroup_rwsem. This allows ->attach() to assume that
 * CPU hotplug is disabled on entry.
 *
 * When favordynmods is enabled, take per threadgroup rwsem to reduce overhead
 * on dynamic cgroup modifications. see the comment above
 * CGRP_ROOT_FAVOR_DYNMODS definition.
 *
 * tsk is not NULL only when writing to cgroup.procs.
 */
void cgroup_attach_lock(enum cgroup_attach_lock_mode lock_mode,
			struct task_struct *tsk)
{
	cpus_read_lock();

	switch (lock_mode) {
	case CGRP_ATTACH_LOCK_NONE:
		break;
	case CGRP_ATTACH_LOCK_GLOBAL:
		percpu_down_write(&cgroup_threadgroup_rwsem);
		break;
	case CGRP_ATTACH_LOCK_PER_THREADGROUP:
		down_write(&tsk->signal->cgroup_threadgroup_rwsem);
		break;
	default:
		pr_warn("cgroup: Unexpected attach lock mode.");
		break;
	}
}

/**
 * cgroup_attach_unlock - Undo cgroup_attach_lock()
 * @lock_mode: whether release and release which rwsem
 * @tsk: thread group to lock
 */
void cgroup_attach_unlock(enum cgroup_attach_lock_mode lock_mode,
			  struct task_struct *tsk)
{
	switch (lock_mode) {
	case CGRP_ATTACH_LOCK_NONE:
		break;
	case CGRP_ATTACH_LOCK_GLOBAL:
		percpu_up_write(&cgroup_threadgroup_rwsem);
		break;
	case CGRP_ATTACH_LOCK_PER_THREADGROUP:
		up_write(&tsk->signal->cgroup_threadgroup_rwsem);
		break;
	default:
		pr_warn("cgroup: Unexpected attach lock mode.");
		break;
	}

	cpus_read_unlock();
}

/**
 * cgroup_migrate_add_task - add a migration target task to a migration context
 * @task: target task
 * @mgctx: target migration context
 *
 * Add @task, which is a migration target, to @mgctx->tset.  This function
 * becomes noop if @task doesn't need to be migrated.  @task's css_set
 * should have been added as a migration source and @task->cg_list will be
 * moved from the css_set's tasks list to mg_tasks one.
 */
static void cgroup_migrate_add_task(struct task_struct *task,
				    struct cgroup_mgctx *mgctx)
{
	struct css_set *cset;

	lockdep_assert_held(&css_set_lock);

	/* @task either already exited or can't exit until the end */
	if (task->flags & PF_EXITING)
		return;