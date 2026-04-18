// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 24/43



static ssize_t cgroup_memory_pressure_write(struct kernfs_open_file *of,
					  char *buf, size_t nbytes,
					  loff_t off)
{
	return pressure_write(of, buf, nbytes, PSI_MEM);
}

static ssize_t cgroup_cpu_pressure_write(struct kernfs_open_file *of,
					  char *buf, size_t nbytes,
					  loff_t off)
{
	return pressure_write(of, buf, nbytes, PSI_CPU);
}

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
static int cgroup_irq_pressure_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct psi_group *psi = cgroup_psi(cgrp);

	return psi_show(seq, psi, PSI_IRQ);
}

static ssize_t cgroup_irq_pressure_write(struct kernfs_open_file *of,
					 char *buf, size_t nbytes,
					 loff_t off)
{
	return pressure_write(of, buf, nbytes, PSI_IRQ);
}
#endif

static int cgroup_pressure_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct psi_group *psi = cgroup_psi(cgrp);

	seq_printf(seq, "%d\n", psi->enabled);

	return 0;
}

static ssize_t cgroup_pressure_write(struct kernfs_open_file *of,
				     char *buf, size_t nbytes,
				     loff_t off)
{
	ssize_t ret;
	int enable;
	struct cgroup *cgrp;
	struct psi_group *psi;

	ret = kstrtoint(strstrip(buf), 0, &enable);
	if (ret)
		return ret;

	if (enable < 0 || enable > 1)
		return -ERANGE;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENOENT;

	psi = cgroup_psi(cgrp);
	if (psi->enabled != enable) {
		int i;

		/* show or hide {cpu,memory,io,irq}.pressure files */
		for (i = 0; i < NR_PSI_RESOURCES; i++)
			cgroup_file_show(&cgrp->psi_files[i], enable);

		psi->enabled = enable;
		if (enable)
			psi_cgroup_restart(psi);
	}

	cgroup_kn_unlock(of->kn);

	return nbytes;
}

static __poll_t cgroup_pressure_poll(struct kernfs_open_file *of,
					  poll_table *pt)
{
	struct cgroup_file_ctx *ctx = of->priv;

	return psi_trigger_poll(&ctx->psi.trigger, of->file, pt);
}

static void cgroup_pressure_release(struct kernfs_open_file *of)
{
	struct cgroup_file_ctx *ctx = of->priv;

	psi_trigger_destroy(ctx->psi.trigger);
}

bool cgroup_psi_enabled(void)
{
	if (static_branch_likely(&psi_disabled))
		return false;

	return (cgroup_feature_disable_mask & (1 << OPT_FEATURE_PRESSURE)) == 0;
}

#else /* CONFIG_PSI */
bool cgroup_psi_enabled(void)
{
	return false;
}

#endif /* CONFIG_PSI */

static int cgroup_freeze_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;

	seq_printf(seq, "%d\n", cgrp->freezer.freeze);

	return 0;
}

static ssize_t cgroup_freeze_write(struct kernfs_open_file *of,
				   char *buf, size_t nbytes, loff_t off)
{
	struct cgroup *cgrp;
	ssize_t ret;
	int freeze;

	ret = kstrtoint(strstrip(buf), 0, &freeze);
	if (ret)
		return ret;

	if (freeze < 0 || freeze > 1)
		return -ERANGE;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENOENT;

	cgroup_freeze(cgrp, freeze);

	cgroup_kn_unlock(of->kn);

	return nbytes;
}

static void __cgroup_kill(struct cgroup *cgrp)
{
	struct css_task_iter it;
	struct task_struct *task;

	lockdep_assert_held(&cgroup_mutex);

	spin_lock_irq(&css_set_lock);
	cgrp->kill_seq++;
	spin_unlock_irq(&css_set_lock);

	css_task_iter_start(&cgrp->self, CSS_TASK_ITER_PROCS | CSS_TASK_ITER_THREADED, &it);
	while ((task = css_task_iter_next(&it))) {
		/* Ignore kernel threads here. */
		if (task->flags & PF_KTHREAD)
			continue;

		/* Skip tasks that are already dying. */
		if (__fatal_signal_pending(task))
			continue;

		send_sig(SIGKILL, task, 0);
	}
	css_task_iter_end(&it);
}

static void cgroup_kill(struct cgroup *cgrp)
{
	struct cgroup_subsys_state *css;
	struct cgroup *dsct;

	lockdep_assert_held(&cgroup_mutex);

	cgroup_for_each_live_descendant_pre(dsct, css, cgrp)
		__cgroup_kill(dsct);
}

static ssize_t cgroup_kill_write(struct kernfs_open_file *of, char *buf,
				 size_t nbytes, loff_t off)
{
	ssize_t ret = 0;
	int kill;
	struct cgroup *cgrp;

	ret = kstrtoint(strstrip(buf), 0, &kill);
	if (ret)
		return ret;

	if (kill != 1)
		return -ERANGE;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENOENT;

	/*
	 * Killing is a process directed operation, i.e. the whole thread-group
	 * is taken down so act like we do for cgroup.procs and only make this
	 * writable in non-threaded cgroups.
	 */
	if (cgroup_is_threaded(cgrp))
		ret = -EOPNOTSUPP;
	else
		cgroup_kill(cgrp);

	cgroup_kn_unlock(of->kn);

	return ret ?: nbytes;
}

static int cgroup_file_open(struct kernfs_open_file *of)
{
	struct cftype *cft = of_cft(of);
	struct cgroup_file_ctx *ctx;
	int ret;

	ctx = kzalloc_obj(*ctx);
	if (!ctx)
		return -ENOMEM;

	ctx->ns = current->nsproxy->cgroup_ns;
	get_cgroup_ns(ctx->ns);
	of->priv = ctx;

	if (!cft->open)
		return 0;

	ret = cft->open(of);
	if (ret) {
		put_cgroup_ns(ctx->ns);
		kfree(ctx);
	}
	return ret;
}

static void cgroup_file_release(struct kernfs_open_file *of)
{
	struct cftype *cft = of_cft(of);
	struct cgroup_file_ctx *ctx = of->priv;