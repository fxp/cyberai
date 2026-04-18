// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 39/43



/**
 * cgroup_css_set_fork - find or create a css_set for a child process
 * @kargs: the arguments passed to create the child process
 *
 * This functions finds or creates a new css_set which the child
 * process will be attached to in cgroup_post_fork(). By default,
 * the child process will be given the same css_set as its parent.
 *
 * If CLONE_INTO_CGROUP is specified this function will try to find an
 * existing css_set which includes the requested cgroup and if not create
 * a new css_set that the child will be attached to later. If this function
 * succeeds it will hold cgroup_threadgroup_rwsem on return. If
 * CLONE_INTO_CGROUP is requested this function will grab cgroup mutex
 * before grabbing cgroup_threadgroup_rwsem and will hold a reference
 * to the target cgroup.
 */
static int cgroup_css_set_fork(struct kernel_clone_args *kargs)
	__acquires(&cgroup_mutex) __acquires(&cgroup_threadgroup_rwsem)
{
	int ret;
	struct cgroup *dst_cgrp = NULL;
	struct css_set *cset;
	struct super_block *sb;

	if (kargs->flags & CLONE_INTO_CGROUP)
		cgroup_lock();

	cgroup_threadgroup_change_begin(current);

	spin_lock_irq(&css_set_lock);
	cset = task_css_set(current);
	get_css_set(cset);
	if (kargs->cgrp)
		kargs->kill_seq = kargs->cgrp->kill_seq;
	else
		kargs->kill_seq = cset->dfl_cgrp->kill_seq;
	spin_unlock_irq(&css_set_lock);

	if (!(kargs->flags & CLONE_INTO_CGROUP)) {
		kargs->cset = cset;
		return 0;
	}

	CLASS(fd_raw, f)(kargs->cgroup);
	if (fd_empty(f)) {
		ret = -EBADF;
		goto err;
	}
	sb = fd_file(f)->f_path.dentry->d_sb;

	dst_cgrp = cgroup_get_from_file(fd_file(f));
	if (IS_ERR(dst_cgrp)) {
		ret = PTR_ERR(dst_cgrp);
		dst_cgrp = NULL;
		goto err;
	}

	if (cgroup_is_dead(dst_cgrp)) {
		ret = -ENODEV;
		goto err;
	}

	/*
	 * Verify that we the target cgroup is writable for us. This is
	 * usually done by the vfs layer but since we're not going through
	 * the vfs layer here we need to do it "manually".
	 */
	ret = cgroup_may_write(dst_cgrp, sb);
	if (ret)
		goto err;

	/*
	 * Spawning a task directly into a cgroup works by passing a file
	 * descriptor to the target cgroup directory. This can even be an O_PATH
	 * file descriptor. But it can never be a cgroup.procs file descriptor.
	 * This was done on purpose so spawning into a cgroup could be
	 * conceptualized as an atomic
	 *
	 *   fd = openat(dfd_cgroup, "cgroup.procs", ...);
	 *   write(fd, <child-pid>, ...);
	 *
	 * sequence, i.e. it's a shorthand for the caller opening and writing
	 * cgroup.procs of the cgroup indicated by @dfd_cgroup. This allows us
	 * to always use the caller's credentials.
	 */
	ret = cgroup_attach_permissions(cset->dfl_cgrp, dst_cgrp, sb,
					!(kargs->flags & CLONE_THREAD),
					current->nsproxy->cgroup_ns);
	if (ret)
		goto err;

	kargs->cset = find_css_set(cset, dst_cgrp);
	if (!kargs->cset) {
		ret = -ENOMEM;
		goto err;
	}

	put_css_set(cset);
	kargs->cgrp = dst_cgrp;
	return ret;

err:
	cgroup_threadgroup_change_end(current);
	cgroup_unlock();
	if (dst_cgrp)
		cgroup_put(dst_cgrp);
	put_css_set(cset);
	if (kargs->cset)
		put_css_set(kargs->cset);
	return ret;
}

/**
 * cgroup_css_set_put_fork - drop references we took during fork
 * @kargs: the arguments passed to create the child process
 *
 * Drop references to the prepared css_set and target cgroup if
 * CLONE_INTO_CGROUP was requested.
 */
static void cgroup_css_set_put_fork(struct kernel_clone_args *kargs)
	__releases(&cgroup_threadgroup_rwsem) __releases(&cgroup_mutex)
{
	struct cgroup *cgrp = kargs->cgrp;
	struct css_set *cset = kargs->cset;

	cgroup_threadgroup_change_end(current);

	if (cset) {
		put_css_set(cset);
		kargs->cset = NULL;
	}

	if (kargs->flags & CLONE_INTO_CGROUP) {
		cgroup_unlock();
		if (cgrp) {
			cgroup_put(cgrp);
			kargs->cgrp = NULL;
		}
	}
}

/**
 * cgroup_can_fork - called on a new task before the process is exposed
 * @child: the child process
 * @kargs: the arguments passed to create the child process
 *
 * This prepares a new css_set for the child process which the child will
 * be attached to in cgroup_post_fork().
 * This calls the subsystem can_fork() callbacks. If the cgroup_can_fork()
 * callback returns an error, the fork aborts with that error code. This
 * allows for a cgroup subsystem to conditionally allow or deny new forks.
 */
int cgroup_can_fork(struct task_struct *child, struct kernel_clone_args *kargs)
{
	struct cgroup_subsys *ss;
	int i, j, ret;

	ret = cgroup_css_set_fork(kargs);
	if (ret)
		return ret;

	do_each_subsys_mask(ss, i, have_canfork_callback) {
		ret = ss->can_fork(child, kargs->cset);
		if (ret)
			goto out_revert;
	} while_each_subsys_mask();

	return 0;

out_revert:
	for_each_subsys(ss, j) {
		if (j >= i)
			break;
		if (ss->cancel_fork)
			ss->cancel_fork(child, kargs->cset);
	}

	cgroup_css_set_put_fork(kargs);

	return ret;
}