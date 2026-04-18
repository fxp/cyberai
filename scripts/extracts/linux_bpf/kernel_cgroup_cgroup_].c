// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 29/43



/**
 * css_has_online_children - does a css have online children
 * @css: the target css
 *
 * Returns %true if @css has any online children; otherwise, %false.  This
 * function can be called from any context but the caller is responsible
 * for synchronizing against on/offlining as necessary.
 */
bool css_has_online_children(struct cgroup_subsys_state *css)
{
	struct cgroup_subsys_state *child;
	bool ret = false;

	rcu_read_lock();
	css_for_each_child(child, css) {
		if (css_is_online(child)) {
			ret = true;
			break;
		}
	}
	rcu_read_unlock();
	return ret;
}

static struct css_set *css_task_iter_next_css_set(struct css_task_iter *it)
{
	struct list_head *l;
	struct cgrp_cset_link *link;
	struct css_set *cset;

	lockdep_assert_held(&css_set_lock);

	/* find the next threaded cset */
	if (it->tcset_pos) {
		l = it->tcset_pos->next;

		if (l != it->tcset_head) {
			it->tcset_pos = l;
			return container_of(l, struct css_set,
					    threaded_csets_node);
		}

		it->tcset_pos = NULL;
	}

	/* find the next cset */
	l = it->cset_pos;
	l = l->next;
	if (l == it->cset_head) {
		it->cset_pos = NULL;
		return NULL;
	}

	if (it->ss) {
		cset = container_of(l, struct css_set, e_cset_node[it->ss->id]);
	} else {
		link = list_entry(l, struct cgrp_cset_link, cset_link);
		cset = link->cset;
	}

	it->cset_pos = l;

	/* initialize threaded css_set walking */
	if (it->flags & CSS_TASK_ITER_THREADED) {
		if (it->cur_dcset)
			put_css_set_locked(it->cur_dcset);
		it->cur_dcset = cset;
		get_css_set(cset);

		it->tcset_head = &cset->threaded_csets;
		it->tcset_pos = &cset->threaded_csets;
	}

	return cset;
}

/**
 * css_task_iter_advance_css_set - advance a task iterator to the next css_set
 * @it: the iterator to advance
 *
 * Advance @it to the next css_set to walk.
 */
static void css_task_iter_advance_css_set(struct css_task_iter *it)
{
	struct css_set *cset;

	lockdep_assert_held(&css_set_lock);

	/* Advance to the next non-empty css_set and find first non-empty tasks list*/
	while ((cset = css_task_iter_next_css_set(it))) {
		if (!list_empty(&cset->tasks)) {
			it->cur_tasks_head = &cset->tasks;
			break;
		} else if (!list_empty(&cset->mg_tasks)) {
			it->cur_tasks_head = &cset->mg_tasks;
			break;
		} else if (!list_empty(&cset->dying_tasks)) {
			it->cur_tasks_head = &cset->dying_tasks;
			break;
		}
	}
	if (!cset) {
		it->task_pos = NULL;
		return;
	}
	it->task_pos = it->cur_tasks_head->next;

	/*
	 * We don't keep css_sets locked across iteration steps and thus
	 * need to take steps to ensure that iteration can be resumed after
	 * the lock is re-acquired.  Iteration is performed at two levels -
	 * css_sets and tasks in them.
	 *
	 * Once created, a css_set never leaves its cgroup lists, so a
	 * pinned css_set is guaranteed to stay put and we can resume
	 * iteration afterwards.
	 *
	 * Tasks may leave @cset across iteration steps.  This is resolved
	 * by registering each iterator with the css_set currently being
	 * walked and making css_set_move_task() advance iterators whose
	 * next task is leaving.
	 */
	if (it->cur_cset) {
		list_del(&it->iters_node);
		put_css_set_locked(it->cur_cset);
	}
	get_css_set(cset);
	it->cur_cset = cset;
	list_add(&it->iters_node, &cset->task_iters);
}

static void css_task_iter_skip(struct css_task_iter *it,
			       struct task_struct *task)
{
	lockdep_assert_held(&css_set_lock);

	if (it->task_pos == &task->cg_list) {
		it->task_pos = it->task_pos->next;
		it->flags |= CSS_TASK_ITER_SKIPPED;
	}
}

static void css_task_iter_advance(struct css_task_iter *it)
{
	struct task_struct *task;

	lockdep_assert_held(&css_set_lock);
repeat:
	if (it->task_pos) {
		/*
		 * Advance iterator to find next entry. We go through cset
		 * tasks, mg_tasks and dying_tasks, when consumed we move onto
		 * the next cset.
		 */
		if (it->flags & CSS_TASK_ITER_SKIPPED)
			it->flags &= ~CSS_TASK_ITER_SKIPPED;
		else
			it->task_pos = it->task_pos->next;

		if (it->task_pos == &it->cur_cset->tasks) {
			it->cur_tasks_head = &it->cur_cset->mg_tasks;
			it->task_pos = it->cur_tasks_head->next;
		}
		if (it->task_pos == &it->cur_cset->mg_tasks) {
			it->cur_tasks_head = &it->cur_cset->dying_tasks;
			it->task_pos = it->cur_tasks_head->next;
		}
		if (it->task_pos == &it->cur_cset->dying_tasks)
			css_task_iter_advance_css_set(it);
	} else {
		/* called from start, proceed to the first cset */
		css_task_iter_advance_css_set(it);
	}

	if (!it->task_pos)
		return;

	task = list_entry(it->task_pos, struct task_struct, cg_list);
	/*
	 * Hide tasks that are exiting but not yet removed. Keep zombie
	 * leaders with live threads visible.
	 */
	if ((task->flags & PF_EXITING) && !atomic_read(&task->signal->live))
		goto repeat;

	if (it->flags & CSS_TASK_ITER_PROCS) {
		/* if PROCS, skip over tasks which aren't group leaders */
		if (!thread_group_leader(task))
			goto repeat;