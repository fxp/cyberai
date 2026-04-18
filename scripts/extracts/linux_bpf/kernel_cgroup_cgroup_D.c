// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 36/43



/**
 * cgroup_drain_dying - wait for dying tasks to leave before rmdir
 * @cgrp: the cgroup being removed
 *
 * cgroup.procs and cgroup.threads use css_task_iter which filters out
 * PF_EXITING tasks so that userspace doesn't see tasks that have already been
 * reaped via waitpid(). However, cgroup_has_tasks() - which tests whether the
 * cgroup has non-empty css_sets - is only updated when dying tasks pass through
 * cgroup_task_dead() in finish_task_switch(). This creates a window where
 * cgroup.procs reads empty but cgroup_has_tasks() is still true, making rmdir
 * fail with -EBUSY from cgroup_destroy_locked() even though userspace sees no
 * tasks.
 *
 * This function aligns cgroup_has_tasks() with what userspace can observe. If
 * cgroup_has_tasks() but the task iterator sees nothing (all remaining tasks are
 * PF_EXITING), we wait for cgroup_task_dead() to finish processing them. As the
 * window between PF_EXITING and cgroup_task_dead() is short, the wait is brief.
 *
 * This function only concerns itself with this cgroup's own dying tasks.
 * Whether the cgroup has children is cgroup_destroy_locked()'s problem.
 *
 * Each cgroup_task_dead() kicks the waitqueue via cset->cgrp_links, and we
 * retry the full check from scratch.
 *
 * Must be called with cgroup_mutex held.
 */
static int cgroup_drain_dying(struct cgroup *cgrp)
	__releases(&cgroup_mutex) __acquires(&cgroup_mutex)
{
	struct css_task_iter it;
	struct task_struct *task;
	DEFINE_WAIT(wait);

	lockdep_assert_held(&cgroup_mutex);
retry:
	if (!cgroup_has_tasks(cgrp))
		return 0;

	/* Same iterator as cgroup.threads - if any task is visible, it's busy */
	css_task_iter_start(&cgrp->self, 0, &it);
	task = css_task_iter_next(&it);
	css_task_iter_end(&it);

	if (task)
		return -EBUSY;

	/*
	 * All remaining tasks are PF_EXITING and will pass through
	 * cgroup_task_dead() shortly. Wait for a kick and retry.
	 *
	 * cgroup_has_tasks() can't transition from false to true while we're
	 * holding cgroup_mutex, but the true to false transition happens
	 * under css_set_lock (via cgroup_task_dead()). We must retest and
	 * prepare_to_wait() under css_set_lock. Otherwise, the transition
	 * can happen between our first test and prepare_to_wait(), and we
	 * sleep with no one to wake us.
	 */
	spin_lock_irq(&css_set_lock);
	if (!cgroup_has_tasks(cgrp)) {
		spin_unlock_irq(&css_set_lock);
		return 0;
	}
	prepare_to_wait(&cgrp->dying_populated_waitq, &wait,
			TASK_UNINTERRUPTIBLE);
	spin_unlock_irq(&css_set_lock);
	mutex_unlock(&cgroup_mutex);
	schedule();
	finish_wait(&cgrp->dying_populated_waitq, &wait);
	mutex_lock(&cgroup_mutex);
	goto retry;
}

int cgroup_rmdir(struct kernfs_node *kn)
{
	struct cgroup *cgrp;
	int ret = 0;

	cgrp = cgroup_kn_lock_live(kn, false);
	if (!cgrp)
		return 0;

	ret = cgroup_drain_dying(cgrp);
	if (!ret) {
		ret = cgroup_destroy_locked(cgrp);
		if (!ret)
			TRACE_CGROUP_PATH(rmdir, cgrp);
	}

	cgroup_kn_unlock(kn);
	return ret;
}

static struct kernfs_syscall_ops cgroup_kf_syscall_ops = {
	.show_options		= cgroup_show_options,
	.mkdir			= cgroup_mkdir,
	.rmdir			= cgroup_rmdir,
	.show_path		= cgroup_show_path,
};

static void __init cgroup_init_subsys(struct cgroup_subsys *ss, bool early)
{
	struct cgroup_subsys_state *css;

	pr_debug("Initializing cgroup subsys %s\n", ss->name);

	cgroup_lock();

	idr_init(&ss->css_idr);
	INIT_LIST_HEAD(&ss->cfts);

	/* Create the root cgroup state for this subsystem */
	ss->root = &cgrp_dfl_root;
	css = ss->css_alloc(NULL);
	/* We don't handle early failures gracefully */
	BUG_ON(IS_ERR(css));
	init_and_link_css(css, ss, &cgrp_dfl_root.cgrp);

	/*
	 * Root csses are never destroyed and we can't initialize
	 * percpu_ref during early init.  Disable refcnting.
	 */
	css->flags |= CSS_NO_REF;

	if (early) {
		/* allocation can't be done safely during early init */
		css->id = 1;
	} else {
		css->id = cgroup_idr_alloc(&ss->css_idr, css, 1, 2, GFP_KERNEL);
		BUG_ON(css->id < 0);

		BUG_ON(ss_rstat_init(ss));
		BUG_ON(css_rstat_init(css));
	}

	/* Update the init_css_set to contain a subsys
	 * pointer to this state - since the subsystem is
	 * newly registered, all tasks and hence the
	 * init_css_set is in the subsystem's root cgroup. */
	init_css_set.subsys[ss->id] = css;

	have_fork_callback |= (bool)ss->fork << ss->id;
	have_exit_callback |= (bool)ss->exit << ss->id;
	have_release_callback |= (bool)ss->release << ss->id;
	have_canfork_callback |= (bool)ss->can_fork << ss->id;

	/* At system boot, before all subsystems have been
	 * registered, no tasks have been forked, so we don't
	 * need to invoke fork callbacks here. */
	BUG_ON(!list_empty(&init_task.tasks));

	BUG_ON(online_css(css));

	cgroup_unlock();
}