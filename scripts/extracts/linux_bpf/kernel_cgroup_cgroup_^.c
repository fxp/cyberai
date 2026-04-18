// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 30/43



		/* and dying leaders w/o live member threads */
		if (it->cur_tasks_head == &it->cur_cset->dying_tasks &&
		    !atomic_read(&task->signal->live))
			goto repeat;
	} else {
		/* skip all dying ones */
		if (it->cur_tasks_head == &it->cur_cset->dying_tasks)
			goto repeat;
	}
}

/**
 * css_task_iter_start - initiate task iteration
 * @css: the css to walk tasks of
 * @flags: CSS_TASK_ITER_* flags
 * @it: the task iterator to use
 *
 * Initiate iteration through the tasks of @css.  The caller can call
 * css_task_iter_next() to walk through the tasks until the function
 * returns NULL.  On completion of iteration, css_task_iter_end() must be
 * called.
 */
void css_task_iter_start(struct cgroup_subsys_state *css, unsigned int flags,
			 struct css_task_iter *it)
{
	unsigned long irqflags;

	memset(it, 0, sizeof(*it));

	spin_lock_irqsave(&css_set_lock, irqflags);

	it->ss = css->ss;
	it->flags = flags;

	if (CGROUP_HAS_SUBSYS_CONFIG && it->ss)
		it->cset_pos = &css->cgroup->e_csets[css->ss->id];
	else
		it->cset_pos = &css->cgroup->cset_links;

	it->cset_head = it->cset_pos;

	css_task_iter_advance(it);

	spin_unlock_irqrestore(&css_set_lock, irqflags);
}

/**
 * css_task_iter_next - return the next task for the iterator
 * @it: the task iterator being iterated
 *
 * The "next" function for task iteration.  @it should have been
 * initialized via css_task_iter_start().  Returns NULL when the iteration
 * reaches the end.
 */
struct task_struct *css_task_iter_next(struct css_task_iter *it)
{
	unsigned long irqflags;

	if (it->cur_task) {
		put_task_struct(it->cur_task);
		it->cur_task = NULL;
	}

	spin_lock_irqsave(&css_set_lock, irqflags);

	/* @it may be half-advanced by skips, finish advancing */
	if (it->flags & CSS_TASK_ITER_SKIPPED)
		css_task_iter_advance(it);

	if (it->task_pos) {
		it->cur_task = list_entry(it->task_pos, struct task_struct,
					  cg_list);
		get_task_struct(it->cur_task);
		css_task_iter_advance(it);
	}

	spin_unlock_irqrestore(&css_set_lock, irqflags);

	return it->cur_task;
}

/**
 * css_task_iter_end - finish task iteration
 * @it: the task iterator to finish
 *
 * Finish task iteration started by css_task_iter_start().
 */
void css_task_iter_end(struct css_task_iter *it)
{
	unsigned long irqflags;

	if (it->cur_cset) {
		spin_lock_irqsave(&css_set_lock, irqflags);
		list_del(&it->iters_node);
		put_css_set_locked(it->cur_cset);
		spin_unlock_irqrestore(&css_set_lock, irqflags);
	}

	if (it->cur_dcset)
		put_css_set(it->cur_dcset);

	if (it->cur_task)
		put_task_struct(it->cur_task);
}

static void cgroup_procs_release(struct kernfs_open_file *of)
{
	struct cgroup_file_ctx *ctx = of->priv;

	if (ctx->procs.started)
		css_task_iter_end(&ctx->procs.iter);
}

static void *cgroup_procs_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct kernfs_open_file *of = s->private;
	struct cgroup_file_ctx *ctx = of->priv;

	if (pos)
		(*pos)++;

	return css_task_iter_next(&ctx->procs.iter);
}

static void *__cgroup_procs_start(struct seq_file *s, loff_t *pos,
				  unsigned int iter_flags)
{
	struct kernfs_open_file *of = s->private;
	struct cgroup *cgrp = seq_css(s)->cgroup;
	struct cgroup_file_ctx *ctx = of->priv;
	struct css_task_iter *it = &ctx->procs.iter;

	/*
	 * When a seq_file is seeked, it's always traversed sequentially
	 * from position 0, so we can simply keep iterating on !0 *pos.
	 */
	if (!ctx->procs.started) {
		if (WARN_ON_ONCE((*pos)))
			return ERR_PTR(-EINVAL);
		css_task_iter_start(&cgrp->self, iter_flags, it);
		ctx->procs.started = true;
	} else if (!(*pos)) {
		css_task_iter_end(it);
		css_task_iter_start(&cgrp->self, iter_flags, it);
	} else
		return it->cur_task;

	return cgroup_procs_next(s, NULL, NULL);
}

static void *cgroup_procs_start(struct seq_file *s, loff_t *pos)
{
	struct cgroup *cgrp = seq_css(s)->cgroup;

	/*
	 * All processes of a threaded subtree belong to the domain cgroup
	 * of the subtree.  Only threads can be distributed across the
	 * subtree.  Reject reads on cgroup.procs in the subtree proper.
	 * They're always empty anyway.
	 */
	if (cgroup_is_threaded(cgrp))
		return ERR_PTR(-EOPNOTSUPP);

	return __cgroup_procs_start(s, pos, CSS_TASK_ITER_PROCS |
					    CSS_TASK_ITER_THREADED);
}

static int cgroup_procs_show(struct seq_file *s, void *v)
{
	seq_printf(s, "%d\n", task_pid_vnr(v));
	return 0;
}

static int cgroup_may_write(const struct cgroup *cgrp, struct super_block *sb)
{
	int ret;
	struct inode *inode;

	lockdep_assert_held(&cgroup_mutex);

	inode = kernfs_get_inode(sb, cgrp->procs_file.kn);
	if (!inode)
		return -ENOMEM;

	ret = inode_permission(&nop_mnt_idmap, inode, MAY_WRITE);
	iput(inode);
	return ret;
}

static int cgroup_procs_write_permission(struct cgroup *src_cgrp,
					 struct cgroup *dst_cgrp,
					 struct super_block *sb,
					 struct cgroup_namespace *ns)
{
	struct cgroup *com_cgrp = src_cgrp;
	int ret;

	lockdep_assert_held(&cgroup_mutex);