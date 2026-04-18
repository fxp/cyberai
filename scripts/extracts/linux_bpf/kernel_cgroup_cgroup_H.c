// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 40/43



/**
 * cgroup_cancel_fork - called if a fork failed after cgroup_can_fork()
 * @child: the child process
 * @kargs: the arguments passed to create the child process
 *
 * This calls the cancel_fork() callbacks if a fork failed *after*
 * cgroup_can_fork() succeeded and cleans up references we took to
 * prepare a new css_set for the child process in cgroup_can_fork().
 */
void cgroup_cancel_fork(struct task_struct *child,
			struct kernel_clone_args *kargs)
{
	struct cgroup_subsys *ss;
	int i;

	for_each_subsys(ss, i)
		if (ss->cancel_fork)
			ss->cancel_fork(child, kargs->cset);

	cgroup_css_set_put_fork(kargs);
}

/**
 * cgroup_post_fork - finalize cgroup setup for the child process
 * @child: the child process
 * @kargs: the arguments passed to create the child process
 *
 * Attach the child process to its css_set calling the subsystem fork()
 * callbacks.
 */
void cgroup_post_fork(struct task_struct *child,
		      struct kernel_clone_args *kargs)
	__releases(&cgroup_threadgroup_rwsem) __releases(&cgroup_mutex)
{
	unsigned int cgrp_kill_seq = 0;
	unsigned long cgrp_flags = 0;
	bool kill = false;
	struct cgroup_subsys *ss;
	struct css_set *cset;
	int i;

	cset = kargs->cset;
	kargs->cset = NULL;

	spin_lock_irq(&css_set_lock);

	/* init tasks are special, only link regular threads */
	if (likely(child->pid)) {
		if (kargs->cgrp) {
			cgrp_flags = kargs->cgrp->flags;
			cgrp_kill_seq = kargs->cgrp->kill_seq;
		} else {
			cgrp_flags = cset->dfl_cgrp->flags;
			cgrp_kill_seq = cset->dfl_cgrp->kill_seq;
		}

		WARN_ON_ONCE(!list_empty(&child->cg_list));
		cset->nr_tasks++;
		css_set_move_task(child, NULL, cset, false);
	} else {
		put_css_set(cset);
		cset = NULL;
	}

	if (!(child->flags & PF_KTHREAD)) {
		if (unlikely(test_bit(CGRP_FREEZE, &cgrp_flags))) {
			/*
			 * If the cgroup has to be frozen, the new task has
			 * too. Let's set the JOBCTL_TRAP_FREEZE jobctl bit to
			 * get the task into the frozen state.
			 */
			spin_lock(&child->sighand->siglock);
			WARN_ON_ONCE(child->frozen);
			child->jobctl |= JOBCTL_TRAP_FREEZE;
			spin_unlock(&child->sighand->siglock);

			/*
			 * Calling cgroup_update_frozen() isn't required here,
			 * because it will be called anyway a bit later from
			 * do_freezer_trap(). So we avoid cgroup's transient
			 * switch from the frozen state and back.
			 */
		}

		/*
		 * If the cgroup is to be killed notice it now and take the
		 * child down right after we finished preparing it for
		 * userspace.
		 */
		kill = kargs->kill_seq != cgrp_kill_seq;
	}

	spin_unlock_irq(&css_set_lock);

	/*
	 * Call ss->fork().  This must happen after @child is linked on
	 * css_set; otherwise, @child might change state between ->fork()
	 * and addition to css_set.
	 */
	do_each_subsys_mask(ss, i, have_fork_callback) {
		ss->fork(child);
	} while_each_subsys_mask();

	/* Make the new cset the root_cset of the new cgroup namespace. */
	if (kargs->flags & CLONE_NEWCGROUP) {
		struct css_set *rcset = child->nsproxy->cgroup_ns->root_cset;

		get_css_set(cset);
		child->nsproxy->cgroup_ns->root_cset = cset;
		put_css_set(rcset);
	}

	/* Cgroup has to be killed so take down child immediately. */
	if (unlikely(kill))
		do_send_sig_info(SIGKILL, SEND_SIG_NOINFO, child, PIDTYPE_TGID);

	cgroup_css_set_put_fork(kargs);
}

/**
 * cgroup_task_exit - detach cgroup from exiting task
 * @tsk: pointer to task_struct of exiting process
 *
 * Description: Detach cgroup from @tsk.
 *
 */
void cgroup_task_exit(struct task_struct *tsk)
{
	struct cgroup_subsys *ss;
	int i;

	/* see cgroup_post_fork() for details */
	do_each_subsys_mask(ss, i, have_exit_callback) {
		ss->exit(tsk);
	} while_each_subsys_mask();
}

static void do_cgroup_task_dead(struct task_struct *tsk)
{
	struct cgrp_cset_link *link;
	struct css_set *cset;
	unsigned long flags;

	spin_lock_irqsave(&css_set_lock, flags);

	WARN_ON_ONCE(list_empty(&tsk->cg_list));
	cset = task_css_set(tsk);
	css_set_move_task(tsk, cset, NULL, false);
	cset->nr_tasks--;
	/* matches the signal->live check in css_task_iter_advance() */
	if (thread_group_leader(tsk) && atomic_read(&tsk->signal->live))
		list_add_tail(&tsk->cg_list, &cset->dying_tasks);

	/* kick cgroup_drain_dying() waiters, see cgroup_rmdir() */
	list_for_each_entry(link, &cset->cgrp_links, cgrp_link)
		if (waitqueue_active(&link->cgrp->dying_populated_waitq))
			wake_up(&link->cgrp->dying_populated_waitq);

	if (dl_task(tsk))
		dec_dl_tasks_cs(tsk);

	WARN_ON_ONCE(cgroup_task_frozen(tsk));
	if (unlikely(!(tsk->flags & PF_KTHREAD) &&
		     test_bit(CGRP_FREEZE, &task_dfl_cgroup(tsk)->flags)))
		cgroup_update_frozen(task_dfl_cgroup(tsk));

	spin_unlock_irqrestore(&css_set_lock, flags);
}