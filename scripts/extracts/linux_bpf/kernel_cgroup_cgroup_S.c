// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 19/43



	if (threadgroup) {
		if (!thread_group_leader(tsk)) {
			/*
			 * A race with de_thread from another thread's exec()
			 * may strip us of our leadership. If this happens,
			 * throw this task away and try again.
			 */
			cgroup_attach_unlock(*lock_mode, tsk);
			put_task_struct(tsk);
			goto retry_find_task;
		}
	}

	return tsk;

out_unlock_rcu:
	rcu_read_unlock();
	return tsk;
}

void cgroup_procs_write_finish(struct task_struct *task,
			       enum cgroup_attach_lock_mode lock_mode)
{
	cgroup_attach_unlock(lock_mode, task);

	/* release reference from cgroup_procs_write_start() */
	put_task_struct(task);
}

static void cgroup_print_ss_mask(struct seq_file *seq, u32 ss_mask)
{
	struct cgroup_subsys *ss;
	bool printed = false;
	int ssid;

	do_each_subsys_mask(ss, ssid, ss_mask) {
		if (printed)
			seq_putc(seq, ' ');
		seq_puts(seq, ss->name);
		printed = true;
	} while_each_subsys_mask();
	if (printed)
		seq_putc(seq, '\n');
}

/* show controllers which are enabled from the parent */
static int cgroup_controllers_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;

	cgroup_print_ss_mask(seq, cgroup_control(cgrp));
	return 0;
}

/* show controllers which are enabled for a given cgroup's children */
static int cgroup_subtree_control_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;

	cgroup_print_ss_mask(seq, cgrp->subtree_control);
	return 0;
}

/**
 * cgroup_update_dfl_csses - update css assoc of a subtree in default hierarchy
 * @cgrp: root of the subtree to update csses for
 *
 * @cgrp's control masks have changed and its subtree's css associations
 * need to be updated accordingly.  This function looks up all css_sets
 * which are attached to the subtree, creates the matching updated css_sets
 * and migrates the tasks to the new ones.
 */
static int cgroup_update_dfl_csses(struct cgroup *cgrp)
{
	DEFINE_CGROUP_MGCTX(mgctx);
	struct cgroup_subsys_state *d_css;
	struct cgroup *dsct;
	struct css_set *src_cset;
	enum cgroup_attach_lock_mode lock_mode;
	bool has_tasks;
	int ret;

	lockdep_assert_held(&cgroup_mutex);

	/* look up all csses currently attached to @cgrp's subtree */
	spin_lock_irq(&css_set_lock);
	cgroup_for_each_live_descendant_pre(dsct, d_css, cgrp) {
		struct cgrp_cset_link *link;

		/*
		 * As cgroup_update_dfl_csses() is only called by
		 * cgroup_apply_control(). The csses associated with the
		 * given cgrp will not be affected by changes made to
		 * its subtree_control file. We can skip them.
		 */
		if (dsct == cgrp)
			continue;

		list_for_each_entry(link, &dsct->cset_links, cset_link)
			cgroup_migrate_add_src(link->cset, dsct, &mgctx);
	}
	spin_unlock_irq(&css_set_lock);

	/*
	 * We need to write-lock threadgroup_rwsem while migrating tasks.
	 * However, if there are no source csets for @cgrp, changing its
	 * controllers isn't gonna produce any task migrations and the
	 * write-locking can be skipped safely.
	 */
	has_tasks = !list_empty(&mgctx.preloaded_src_csets);

	if (has_tasks)
		lock_mode = CGRP_ATTACH_LOCK_GLOBAL;
	else
		lock_mode = CGRP_ATTACH_LOCK_NONE;

	cgroup_attach_lock(lock_mode, NULL);

	/* NULL dst indicates self on default hierarchy */
	ret = cgroup_migrate_prepare_dst(&mgctx);
	if (ret)
		goto out_finish;

	spin_lock_irq(&css_set_lock);
	list_for_each_entry(src_cset, &mgctx.preloaded_src_csets,
			    mg_src_preload_node) {
		struct task_struct *task, *ntask;

		/* all tasks in src_csets need to be migrated */
		list_for_each_entry_safe(task, ntask, &src_cset->tasks, cg_list)
			cgroup_migrate_add_task(task, &mgctx);
	}
	spin_unlock_irq(&css_set_lock);

	ret = cgroup_migrate_execute(&mgctx);
out_finish:
	cgroup_migrate_finish(&mgctx);
	cgroup_attach_unlock(lock_mode, NULL);
	return ret;
}

/**
 * cgroup_lock_and_drain_offline - lock cgroup_mutex and drain offlined csses
 * @cgrp: root of the target subtree
 *
 * Because css offlining is asynchronous, userland may try to re-enable a
 * controller while the previous css is still around.  This function grabs
 * cgroup_mutex and drains the previous css instances of @cgrp's subtree.
 */
void cgroup_lock_and_drain_offline(struct cgroup *cgrp)
	__acquires(&cgroup_mutex)
{
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;
	struct cgroup_subsys *ss;
	int ssid;

restart:
	cgroup_lock();

	cgroup_for_each_live_descendant_post(dsct, d_css, cgrp) {
		for_each_subsys(ss, ssid) {
			struct cgroup_subsys_state *css = cgroup_css(dsct, ss);
			DEFINE_WAIT(wait);

			if (!css || !percpu_ref_is_dying(&css->refcnt))
				continue;

			cgroup_get_live(dsct);
			prepare_to_wait(&dsct->offline_waitq, &wait,
					TASK_UNINTERRUPTIBLE);

			cgroup_unlock();
			schedule();
			finish_wait(&dsct->offline_waitq, &wait);

			cgroup_put(dsct);
			goto restart;
		}
	}
}