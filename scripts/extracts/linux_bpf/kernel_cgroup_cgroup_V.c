// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 22/43



/**
 * cgroup_enable_threaded - make @cgrp threaded
 * @cgrp: the target cgroup
 *
 * Called when "threaded" is written to the cgroup.type interface file and
 * tries to make @cgrp threaded and join the parent's resource domain.
 * This function is never called on the root cgroup as cgroup.type doesn't
 * exist on it.
 */
static int cgroup_enable_threaded(struct cgroup *cgrp)
{
	struct cgroup *parent = cgroup_parent(cgrp);
	struct cgroup *dom_cgrp = parent->dom_cgrp;
	struct cgroup *dsct;
	struct cgroup_subsys_state *d_css;
	int ret;

	lockdep_assert_held(&cgroup_mutex);

	/* noop if already threaded */
	if (cgroup_is_threaded(cgrp))
		return 0;

	/*
	 * If @cgroup is populated or has domain controllers enabled, it
	 * can't be switched.  While the below cgroup_can_be_thread_root()
	 * test can catch the same conditions, that's only when @parent is
	 * not mixable, so let's check it explicitly.
	 */
	if (cgroup_is_populated(cgrp) ||
	    cgrp->subtree_control & ~cgrp_dfl_threaded_ss_mask)
		return -EOPNOTSUPP;

	/* we're joining the parent's domain, ensure its validity */
	if (!cgroup_is_valid_domain(dom_cgrp) ||
	    !cgroup_can_be_thread_root(dom_cgrp))
		return -EOPNOTSUPP;

	/*
	 * The following shouldn't cause actual migrations and should
	 * always succeed.
	 */
	cgroup_save_control(cgrp);

	cgroup_for_each_live_descendant_pre(dsct, d_css, cgrp)
		if (dsct == cgrp || cgroup_is_threaded(dsct))
			dsct->dom_cgrp = dom_cgrp;

	ret = cgroup_apply_control(cgrp);
	if (!ret)
		parent->nr_threaded_children++;

	cgroup_finalize_control(cgrp, ret);
	return ret;
}

static int cgroup_type_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;

	if (cgroup_is_threaded(cgrp))
		seq_puts(seq, "threaded\n");
	else if (!cgroup_is_valid_domain(cgrp))
		seq_puts(seq, "domain invalid\n");
	else if (cgroup_is_thread_root(cgrp))
		seq_puts(seq, "domain threaded\n");
	else
		seq_puts(seq, "domain\n");

	return 0;
}

static ssize_t cgroup_type_write(struct kernfs_open_file *of, char *buf,
				 size_t nbytes, loff_t off)
{
	struct cgroup *cgrp;
	int ret;

	/* only switching to threaded mode is supported */
	if (strcmp(strstrip(buf), "threaded"))
		return -EINVAL;

	/* drain dying csses before we re-apply (threaded) subtree control */
	cgrp = cgroup_kn_lock_live(of->kn, true);
	if (!cgrp)
		return -ENOENT;

	/* threaded can only be enabled */
	ret = cgroup_enable_threaded(cgrp);

	cgroup_kn_unlock(of->kn);
	return ret ?: nbytes;
}

static int cgroup_max_descendants_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	int descendants = READ_ONCE(cgrp->max_descendants);

	if (descendants == INT_MAX)
		seq_puts(seq, "max\n");
	else
		seq_printf(seq, "%d\n", descendants);

	return 0;
}

static ssize_t cgroup_max_descendants_write(struct kernfs_open_file *of,
					   char *buf, size_t nbytes, loff_t off)
{
	struct cgroup *cgrp;
	int descendants;
	ssize_t ret;

	buf = strstrip(buf);
	if (!strcmp(buf, "max")) {
		descendants = INT_MAX;
	} else {
		ret = kstrtoint(buf, 0, &descendants);
		if (ret)
			return ret;
	}

	if (descendants < 0)
		return -ERANGE;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENOENT;

	cgrp->max_descendants = descendants;

	cgroup_kn_unlock(of->kn);

	return nbytes;
}

static int cgroup_max_depth_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;
	int depth = READ_ONCE(cgrp->max_depth);

	if (depth == INT_MAX)
		seq_puts(seq, "max\n");
	else
		seq_printf(seq, "%d\n", depth);

	return 0;
}

static ssize_t cgroup_max_depth_write(struct kernfs_open_file *of,
				      char *buf, size_t nbytes, loff_t off)
{
	struct cgroup *cgrp;
	ssize_t ret;
	int depth;

	buf = strstrip(buf);
	if (!strcmp(buf, "max")) {
		depth = INT_MAX;
	} else {
		ret = kstrtoint(buf, 0, &depth);
		if (ret)
			return ret;
	}

	if (depth < 0)
		return -ERANGE;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENOENT;

	cgrp->max_depth = depth;

	cgroup_kn_unlock(of->kn);

	return nbytes;
}

static int cgroup_events_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgrp = seq_css(seq)->cgroup;

	seq_printf(seq, "populated %d\n", cgroup_is_populated(cgrp));
	seq_printf(seq, "frozen %d\n", test_bit(CGRP_FROZEN, &cgrp->flags));

	return 0;
}

static int cgroup_stat_show(struct seq_file *seq, void *v)
{
	struct cgroup *cgroup = seq_css(seq)->cgroup;
	struct cgroup_subsys_state *css;
	int dying_cnt[CGROUP_SUBSYS_COUNT];
	int ssid;

	seq_printf(seq, "nr_descendants %d\n",
		   cgroup->nr_descendants);