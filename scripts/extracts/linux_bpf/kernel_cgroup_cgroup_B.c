// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 34/43



	/*
	 * Now that init_cgroup_housekeeping() has been called and cgrp->self
	 * is setup, it is safe to perform rstat initialization on it.
	 */
	ret = css_rstat_init(&cgrp->self);
	if (ret)
		goto out_kernfs_remove;

	ret = psi_cgroup_alloc(cgrp);
	if (ret)
		goto out_stat_exit;

	for (tcgrp = cgrp; tcgrp; tcgrp = cgroup_parent(tcgrp))
		cgrp->ancestors[tcgrp->level] = tcgrp;

	/*
	 * New cgroup inherits effective freeze counter, and
	 * if the parent has to be frozen, the child has too.
	 */
	cgrp->freezer.e_freeze = parent->freezer.e_freeze;
	seqcount_spinlock_init(&cgrp->freezer.freeze_seq, &css_set_lock);
	if (cgrp->freezer.e_freeze) {
		/*
		 * Set the CGRP_FREEZE flag, so when a process will be
		 * attached to the child cgroup, it will become frozen.
		 * At this point the new cgroup is unpopulated, so we can
		 * consider it frozen immediately.
		 */
		set_bit(CGRP_FREEZE, &cgrp->flags);
		cgrp->freezer.freeze_start_nsec = ktime_get_ns();
		set_bit(CGRP_FROZEN, &cgrp->flags);
	}

	if (notify_on_release(parent))
		set_bit(CGRP_NOTIFY_ON_RELEASE, &cgrp->flags);

	if (test_bit(CGRP_CPUSET_CLONE_CHILDREN, &parent->flags))
		set_bit(CGRP_CPUSET_CLONE_CHILDREN, &cgrp->flags);

	cgrp->self.serial_nr = css_serial_nr_next++;

	ret = blocking_notifier_call_chain_robust(&cgroup_lifetime_notifier,
						  CGROUP_LIFETIME_ONLINE,
						  CGROUP_LIFETIME_OFFLINE, cgrp);
	ret = notifier_to_errno(ret);
	if (ret)
		goto out_psi_free;

	/* allocation complete, commit to creation */
	spin_lock_irq(&css_set_lock);
	for (i = 0; i < level; i++) {
		tcgrp = cgrp->ancestors[i];
		tcgrp->nr_descendants++;

		/*
		 * If the new cgroup is frozen, all ancestor cgroups get a new
		 * frozen descendant, but their state can't change because of
		 * this.
		 */
		if (cgrp->freezer.e_freeze)
			tcgrp->freezer.nr_frozen_descendants++;
	}
	spin_unlock_irq(&css_set_lock);

	list_add_tail_rcu(&cgrp->self.sibling, &cgroup_parent(cgrp)->self.children);
	atomic_inc(&root->nr_cgrps);
	cgroup_get_live(parent);

	/*
	 * On the default hierarchy, a child doesn't automatically inherit
	 * subtree_control from the parent.  Each is configured manually.
	 */
	if (!cgroup_on_dfl(cgrp))
		cgrp->subtree_control = cgroup_control(cgrp);

	cgroup_propagate_control(cgrp);

	return cgrp;

out_psi_free:
	psi_cgroup_free(cgrp);
out_stat_exit:
	css_rstat_exit(&cgrp->self);
out_kernfs_remove:
	kernfs_remove(cgrp->kn);
out_cancel_ref:
	percpu_ref_exit(&cgrp->self.refcnt);
out_free_cgrp:
	kfree(cgrp);
	return ERR_PTR(ret);
}

static bool cgroup_check_hierarchy_limits(struct cgroup *parent)
{
	struct cgroup *cgroup;
	int ret = false;
	int level = 0;

	lockdep_assert_held(&cgroup_mutex);

	for (cgroup = parent; cgroup; cgroup = cgroup_parent(cgroup)) {
		if (cgroup->nr_descendants >= cgroup->max_descendants)
			goto fail;

		if (level >= cgroup->max_depth)
			goto fail;

		level++;
	}

	ret = true;
fail:
	return ret;
}

int cgroup_mkdir(struct kernfs_node *parent_kn, const char *name, umode_t mode)
{
	struct cgroup *parent, *cgrp;
	int ret;

	/* do not accept '\n' to prevent making /proc/<pid>/cgroup unparsable */
	if (strchr(name, '\n'))
		return -EINVAL;

	parent = cgroup_kn_lock_live(parent_kn, false);
	if (!parent)
		return -ENODEV;

	if (!cgroup_check_hierarchy_limits(parent)) {
		ret = -EAGAIN;
		goto out_unlock;
	}

	cgrp = cgroup_create(parent, name, mode);
	if (IS_ERR(cgrp)) {
		ret = PTR_ERR(cgrp);
		goto out_unlock;
	}

	/*
	 * This extra ref will be put in css_free_rwork_fn() and guarantees
	 * that @cgrp->kn is always accessible.
	 */
	kernfs_get(cgrp->kn);

	ret = css_populate_dir(&cgrp->self);
	if (ret)
		goto out_destroy;

	ret = cgroup_apply_control_enable(cgrp);
	if (ret)
		goto out_destroy;

	TRACE_CGROUP_PATH(mkdir, cgrp);

	/* let's create and online css's */
	kernfs_activate(cgrp->kn);

	ret = 0;
	goto out_unlock;

out_destroy:
	cgroup_destroy_locked(cgrp);
out_unlock:
	cgroup_kn_unlock(parent_kn);
	return ret;
}

/*
 * This is called when the refcnt of a css is confirmed to be killed.
 * css_tryget_online() is now guaranteed to fail.  Tell the subsystem to
 * initiate destruction and put the css ref from kill_css().
 */
static void css_killed_work_fn(struct work_struct *work)
{
	struct cgroup_subsys_state *css =
		container_of(work, struct cgroup_subsys_state, destroy_work);

	cgroup_lock();

	do {
		offline_css(css);
		css_put(css);
		/* @css can't go away while we're holding cgroup_mutex */
		css = css->parent;
	} while (css && atomic_dec_and_test(&css->online_cnt));

	cgroup_unlock();
}

/* css kill confirmation processing requires process context, bounce */
static void css_killed_ref_fn(struct percpu_ref *ref)
{
	struct cgroup_subsys_state *css =
		container_of(ref, struct cgroup_subsys_state, refcnt);

	if (atomic_dec_and_test(&css->online_cnt)) {
		INIT_WORK(&css->destroy_work, css_killed_work_fn);
		queue_work(cgroup_offline_wq, &css->destroy_work);
	}
}