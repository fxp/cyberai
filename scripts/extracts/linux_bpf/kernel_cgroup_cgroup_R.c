// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 18/43



		/*
		 * If src cset equals dst, it's noop.  Drop the src.
		 * cgroup_migrate() will skip the cset too.  Note that we
		 * can't handle src == dst as some nodes are used by both.
		 */
		if (src_cset == dst_cset) {
			src_cset->mg_src_cgrp = NULL;
			src_cset->mg_dst_cgrp = NULL;
			list_del_init(&src_cset->mg_src_preload_node);
			put_css_set(src_cset);
			put_css_set(dst_cset);
			continue;
		}

		src_cset->mg_dst_cset = dst_cset;

		if (list_empty(&dst_cset->mg_dst_preload_node))
			list_add_tail(&dst_cset->mg_dst_preload_node,
				      &mgctx->preloaded_dst_csets);
		else
			put_css_set(dst_cset);

		for_each_subsys(ss, ssid)
			if (src_cset->subsys[ssid] != dst_cset->subsys[ssid])
				mgctx->ss_mask |= 1 << ssid;
	}

	return 0;
}

/**
 * cgroup_migrate - migrate a process or task to a cgroup
 * @leader: the leader of the process or the task to migrate
 * @threadgroup: whether @leader points to the whole process or a single task
 * @mgctx: migration context
 *
 * Migrate a process or task denoted by @leader.  If migrating a process,
 * the caller must be holding cgroup_threadgroup_rwsem.  The caller is also
 * responsible for invoking cgroup_migrate_add_src() and
 * cgroup_migrate_prepare_dst() on the targets before invoking this
 * function and following up with cgroup_migrate_finish().
 *
 * As long as a controller's ->can_attach() doesn't fail, this function is
 * guaranteed to succeed.  This means that, excluding ->can_attach()
 * failure, when migrating multiple targets, the success or failure can be
 * decided for all targets by invoking group_migrate_prepare_dst() before
 * actually starting migrating.
 */
int cgroup_migrate(struct task_struct *leader, bool threadgroup,
		   struct cgroup_mgctx *mgctx)
{
	struct task_struct *task;

	/*
	 * The following thread iteration should be inside an RCU critical
	 * section to prevent tasks from being freed while taking the snapshot.
	 * spin_lock_irq() implies RCU critical section here.
	 */
	spin_lock_irq(&css_set_lock);
	task = leader;
	do {
		cgroup_migrate_add_task(task, mgctx);
		if (!threadgroup)
			break;
	} while_each_thread(leader, task);
	spin_unlock_irq(&css_set_lock);

	return cgroup_migrate_execute(mgctx);
}

/**
 * cgroup_attach_task - attach a task or a whole threadgroup to a cgroup
 * @dst_cgrp: the cgroup to attach to
 * @leader: the task or the leader of the threadgroup to be attached
 * @threadgroup: attach the whole threadgroup?
 *
 * Call holding cgroup_mutex and cgroup_threadgroup_rwsem.
 */
int cgroup_attach_task(struct cgroup *dst_cgrp, struct task_struct *leader,
		       bool threadgroup)
{
	DEFINE_CGROUP_MGCTX(mgctx);
	struct task_struct *task;
	int ret = 0;

	/* look up all src csets */
	spin_lock_irq(&css_set_lock);
	task = leader;
	do {
		cgroup_migrate_add_src(task_css_set(task), dst_cgrp, &mgctx);
		if (!threadgroup)
			break;
	} while_each_thread(leader, task);
	spin_unlock_irq(&css_set_lock);

	/* prepare dst csets and commit */
	ret = cgroup_migrate_prepare_dst(&mgctx);
	if (!ret)
		ret = cgroup_migrate(leader, threadgroup, &mgctx);

	cgroup_migrate_finish(&mgctx);

	if (!ret)
		TRACE_CGROUP_PATH(attach_task, dst_cgrp, leader, threadgroup);

	return ret;
}

struct task_struct *cgroup_procs_write_start(char *buf, bool threadgroup,
					     enum cgroup_attach_lock_mode *lock_mode)
{
	struct task_struct *tsk;
	pid_t pid;

	if (kstrtoint(strstrip(buf), 0, &pid) || pid < 0)
		return ERR_PTR(-EINVAL);

retry_find_task:
	rcu_read_lock();
	if (pid) {
		tsk = find_task_by_vpid(pid);
		if (!tsk) {
			tsk = ERR_PTR(-ESRCH);
			goto out_unlock_rcu;
		}
	} else {
		tsk = current;
	}

	if (threadgroup)
		tsk = tsk->group_leader;

	/*
	 * kthreads may acquire PF_NO_SETAFFINITY during initialization.
	 * If userland migrates such a kthread to a non-root cgroup, it can
	 * become trapped in a cpuset, or RT kthread may be born in a
	 * cgroup with no rt_runtime allocated.  Just say no.
	 */
	if (tsk->no_cgroup_migration || (tsk->flags & PF_NO_SETAFFINITY)) {
		tsk = ERR_PTR(-EINVAL);
		goto out_unlock_rcu;
	}
	get_task_struct(tsk);
	rcu_read_unlock();

	/*
	 * If we migrate a single thread, we don't care about threadgroup
	 * stability. If the thread is `current`, it won't exit(2) under our
	 * hands or change PID through exec(2). We exclude
	 * cgroup_update_dfl_csses and other cgroup_{proc,thread}s_write callers
	 * by cgroup_mutex. Therefore, we can skip the global lock.
	 */
	lockdep_assert_held(&cgroup_mutex);

	if (pid || threadgroup) {
		if (cgroup_enable_per_threadgroup_rwsem)
			*lock_mode = CGRP_ATTACH_LOCK_PER_THREADGROUP;
		else
			*lock_mode = CGRP_ATTACH_LOCK_GLOBAL;
	} else {
		*lock_mode = CGRP_ATTACH_LOCK_NONE;
	}

	cgroup_attach_lock(*lock_mode, tsk);