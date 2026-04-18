// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 23/43



	/*
	 * Show the number of live and dying csses associated with each of
	 * non-inhibited cgroup subsystems that is bound to cgroup v2.
	 *
	 * Without proper lock protection, racing is possible. So the
	 * numbers may not be consistent when that happens.
	 */
	rcu_read_lock();
	for (ssid = 0; ssid < CGROUP_SUBSYS_COUNT; ssid++) {
		dying_cnt[ssid] = -1;
		if ((BIT(ssid) & cgrp_dfl_inhibit_ss_mask) ||
		    (cgroup_subsys[ssid]->root !=  &cgrp_dfl_root))
			continue;
		css = rcu_dereference_raw(cgroup->subsys[ssid]);
		dying_cnt[ssid] = cgroup->nr_dying_subsys[ssid];
		seq_printf(seq, "nr_subsys_%s %d\n", cgroup_subsys[ssid]->name,
			   css ? (css->nr_descendants + 1) : 0);
	}

	seq_printf(seq, "nr_dying_descendants %d\n",
		   cgroup->nr_dying_descendants);
	for (ssid = 0; ssid < CGROUP_SUBSYS_COUNT; ssid++) {
		if (dying_cnt[ssid] >= 0)
			seq_printf(seq, "nr_dying_subsys_%s %d\n",
				   cgroup_subsys[ssid]->name, dying_cnt[ssid]);
	}
	rcu_read_unlock();
	return 0;
}

static int cgroup_core_local_stat_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	unsigned int sequence;
	u64 freeze_time;

	do {
		sequence = read_seqcount_begin(&cgrp->freezer.freeze_seq);
		freeze_time = cgrp->freezer.frozen_nsec;
		/* Add in current freezer interval if the cgroup is freezing. */
		if (test_bit(CGRP_FREEZE, &cgrp->flags))
			freeze_time += (ktime_get_ns() -
					cgrp->freezer.freeze_start_nsec);
	} while (read_seqcount_retry(&cgrp->freezer.freeze_seq, sequence));

	do_div(freeze_time, NSEC_PER_USEC);
	seq_printf(seq, "frozen_usec %llu\n", freeze_time);

	return 0;
}

#ifdef CONFIG_CGROUP_SCHED
/**
 * cgroup_tryget_css - try to get a cgroup's css for the specified subsystem
 * @cgrp: the cgroup of interest
 * @ss: the subsystem of interest
 *
 * Find and get @cgrp's css associated with @ss.  If the css doesn't exist
 * or is offline, %NULL is returned.
 */
static struct cgroup_subsys_state *cgroup_tryget_css(struct cgroup *cgrp,
						     struct cgroup_subsys *ss)
{
	struct cgroup_subsys_state *css;

	rcu_read_lock();
	css = cgroup_css(cgrp, ss);
	if (css && !css_tryget_online(css))
		css = NULL;
	rcu_read_unlock();

	return css;
}

static int cgroup_extra_stat_show(struct seq_file *seq, int ssid)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct cgroup_subsys *ss = cgroup_subsys[ssid];
	struct cgroup_subsys_state *css;
	int ret;

	if (!ss->css_extra_stat_show)
		return 0;

	css = cgroup_tryget_css(cgrp, ss);
	if (!css)
		return 0;

	ret = ss->css_extra_stat_show(seq, css);
	css_put(css);
	return ret;
}

static int cgroup_local_stat_show(struct seq_file *seq,
				  struct cgroup *cgrp, int ssid)
{
	struct cgroup_subsys *ss = cgroup_subsys[ssid];
	struct cgroup_subsys_state *css;
	int ret;

	if (!ss->css_local_stat_show)
		return 0;

	css = cgroup_tryget_css(cgrp, ss);
	if (!css)
		return 0;

	ret = ss->css_local_stat_show(seq, css);
	css_put(css);
	return ret;
}
#endif

static int cpu_stat_show(struct seq_file *seq, void *v)
{
	int ret = 0;

	cgroup_base_stat_cputime_show(seq);
#ifdef CONFIG_CGROUP_SCHED
	ret = cgroup_extra_stat_show(seq, cpu_cgrp_id);
#endif
	return ret;
}

static int cpu_local_stat_show(struct seq_file *seq, void *v)
{
	struct cgroup __maybe_unused *cgrp = seq_css(seq)->cgroup;
	int ret = 0;

#ifdef CONFIG_CGROUP_SCHED
	ret = cgroup_local_stat_show(seq, cgrp, cpu_cgrp_id);
#endif
	return ret;
}

#ifdef CONFIG_PSI
static int cgroup_io_pressure_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct psi_group *psi = cgroup_psi(cgrp);

	return psi_show(seq, psi, PSI_IO);
}
static int cgroup_memory_pressure_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct psi_group *psi = cgroup_psi(cgrp);

	return psi_show(seq, psi, PSI_MEM);
}
static int cgroup_cpu_pressure_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	struct psi_group *psi = cgroup_psi(cgrp);

	return psi_show(seq, psi, PSI_CPU);
}

static ssize_t pressure_write(struct kernfs_open_file *of, char *buf,
			      size_t nbytes, enum psi_res res)
{
	struct cgroup_file_ctx *ctx = of->priv;
	struct psi_trigger *new;
	struct cgroup *cgrp;
	struct psi_group *psi;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENODEV;

	cgroup_get(cgrp);
	cgroup_kn_unlock(of->kn);

	/* Allow only one trigger per file descriptor */
	if (ctx->psi.trigger) {
		cgroup_put(cgrp);
		return -EBUSY;
	}

	psi = cgroup_psi(cgrp);
	new = psi_trigger_create(psi, buf, res, of->file, of);
	if (IS_ERR(new)) {
		cgroup_put(cgrp);
		return PTR_ERR(new);
	}

	smp_store_release(&ctx->psi.trigger, new);
	cgroup_put(cgrp);

	return nbytes;
}

static ssize_t cgroup_io_pressure_write(struct kernfs_open_file *of,
					  char *buf, size_t nbytes,
					  loff_t off)
{
	return pressure_write(of, buf, nbytes, PSI_IO);
}