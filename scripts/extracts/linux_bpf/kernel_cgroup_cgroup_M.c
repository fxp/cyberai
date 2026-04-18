// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 13/43



		if (root_flags & CGRP_ROOT_MEMORY_RECURSIVE_PROT)
			cgrp_dfl_root.flags |= CGRP_ROOT_MEMORY_RECURSIVE_PROT;
		else
			cgrp_dfl_root.flags &= ~CGRP_ROOT_MEMORY_RECURSIVE_PROT;

		if (root_flags & CGRP_ROOT_MEMORY_HUGETLB_ACCOUNTING)
			cgrp_dfl_root.flags |= CGRP_ROOT_MEMORY_HUGETLB_ACCOUNTING;
		else
			cgrp_dfl_root.flags &= ~CGRP_ROOT_MEMORY_HUGETLB_ACCOUNTING;

		if (root_flags & CGRP_ROOT_PIDS_LOCAL_EVENTS)
			cgrp_dfl_root.flags |= CGRP_ROOT_PIDS_LOCAL_EVENTS;
		else
			cgrp_dfl_root.flags &= ~CGRP_ROOT_PIDS_LOCAL_EVENTS;
	}
}

static int cgroup_show_options(struct seq_file *seq, struct kernfs_root *kf_root)
{
	if (cgrp_dfl_root.flags & CGRP_ROOT_NS_DELEGATE)
		seq_puts(seq, ",nsdelegate");
	if (cgrp_dfl_root.flags & CGRP_ROOT_FAVOR_DYNMODS)
		seq_puts(seq, ",favordynmods");
	if (cgrp_dfl_root.flags & CGRP_ROOT_MEMORY_LOCAL_EVENTS)
		seq_puts(seq, ",memory_localevents");
	if (cgrp_dfl_root.flags & CGRP_ROOT_MEMORY_RECURSIVE_PROT)
		seq_puts(seq, ",memory_recursiveprot");
	if (cgrp_dfl_root.flags & CGRP_ROOT_MEMORY_HUGETLB_ACCOUNTING)
		seq_puts(seq, ",memory_hugetlb_accounting");
	if (cgrp_dfl_root.flags & CGRP_ROOT_PIDS_LOCAL_EVENTS)
		seq_puts(seq, ",pids_localevents");
	return 0;
}

static int cgroup_reconfigure(struct fs_context *fc)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);

	apply_cgroup_root_flags(ctx->flags);
	return 0;
}

static void init_cgroup_housekeeping(struct cgroup *cgrp)
{
	struct cgroup_subsys *ss;
	int ssid;

	INIT_LIST_HEAD(&cgrp->self.sibling);
	INIT_LIST_HEAD(&cgrp->self.children);
	INIT_LIST_HEAD(&cgrp->cset_links);
	INIT_LIST_HEAD(&cgrp->pidlists);
	mutex_init(&cgrp->pidlist_mutex);
	cgrp->self.cgroup = cgrp;
	cgrp->self.flags |= CSS_ONLINE;
	cgrp->dom_cgrp = cgrp;
	cgrp->max_descendants = INT_MAX;
	cgrp->max_depth = INT_MAX;
	prev_cputime_init(&cgrp->prev_cputime);

	for_each_subsys(ss, ssid)
		INIT_LIST_HEAD(&cgrp->e_csets[ssid]);

#ifdef CONFIG_CGROUP_BPF
	for (int i = 0; i < ARRAY_SIZE(cgrp->bpf.revisions); i++)
		cgrp->bpf.revisions[i] = 1;
#endif

	init_waitqueue_head(&cgrp->offline_waitq);
	init_waitqueue_head(&cgrp->dying_populated_waitq);
	INIT_WORK(&cgrp->release_agent_work, cgroup1_release_agent);
}

void init_cgroup_root(struct cgroup_fs_context *ctx)
{
	struct cgroup_root *root = ctx->root;
	struct cgroup *cgrp = &root->cgrp;

	INIT_LIST_HEAD_RCU(&root->root_list);
	atomic_set(&root->nr_cgrps, 1);
	cgrp->root = root;
	init_cgroup_housekeeping(cgrp);

	/* DYNMODS must be modified through cgroup_favor_dynmods() */
	root->flags = ctx->flags & ~CGRP_ROOT_FAVOR_DYNMODS;
	if (ctx->release_agent)
		strscpy(root->release_agent_path, ctx->release_agent, PATH_MAX);
	if (ctx->name)
		strscpy(root->name, ctx->name, MAX_CGROUP_ROOT_NAMELEN);
	if (ctx->cpuset_clone_children)
		set_bit(CGRP_CPUSET_CLONE_CHILDREN, &root->cgrp.flags);
}

int cgroup_setup_root(struct cgroup_root *root, u32 ss_mask)
{
	LIST_HEAD(tmp_links);
	struct cgroup *root_cgrp = &root->cgrp;
	struct kernfs_syscall_ops *kf_sops;
	struct css_set *cset;
	int i, ret;

	lockdep_assert_held(&cgroup_mutex);

	ret = percpu_ref_init(&root_cgrp->self.refcnt, css_release,
			      0, GFP_KERNEL);
	if (ret)
		goto out;

	/*
	 * We're accessing css_set_count without locking css_set_lock here,
	 * but that's OK - it can only be increased by someone holding
	 * cgroup_lock, and that's us.  Later rebinding may disable
	 * controllers on the default hierarchy and thus create new csets,
	 * which can't be more than the existing ones.  Allocate 2x.
	 */
	ret = allocate_cgrp_cset_links(2 * css_set_count, &tmp_links);
	if (ret)
		goto cancel_ref;

	ret = cgroup_init_root_id(root);
	if (ret)
		goto cancel_ref;

	kf_sops = root == &cgrp_dfl_root ?
		&cgroup_kf_syscall_ops : &cgroup1_kf_syscall_ops;

	root->kf_root = kernfs_create_root(kf_sops,
					   KERNFS_ROOT_CREATE_DEACTIVATED |
					   KERNFS_ROOT_SUPPORT_EXPORTOP |
					   KERNFS_ROOT_SUPPORT_USER_XATTR |
					   KERNFS_ROOT_INVARIANT_PARENT,
					   root_cgrp);
	if (IS_ERR(root->kf_root)) {
		ret = PTR_ERR(root->kf_root);
		goto exit_root_id;
	}
	root_cgrp->kn = kernfs_root_to_node(root->kf_root);
	WARN_ON_ONCE(cgroup_ino(root_cgrp) != 1);
	root_cgrp->ancestors[0] = root_cgrp;

	ret = css_populate_dir(&root_cgrp->self);
	if (ret)
		goto destroy_root;

	ret = css_rstat_init(&root_cgrp->self);
	if (ret)
		goto destroy_root;

	ret = rebind_subsystems(root, ss_mask);
	if (ret)
		goto exit_stats;

	ret = blocking_notifier_call_chain(&cgroup_lifetime_notifier,
					   CGROUP_LIFETIME_ONLINE, root_cgrp);
	WARN_ON_ONCE(notifier_to_errno(ret));

	trace_cgroup_setup_root(root);

	/*
	 * There must be no failure case after here, since rebinding takes
	 * care of subsystems' refcounts, which are explicitly dropped in
	 * the failure exit path.
	 */
	list_add_rcu(&root->root_list, &cgroup_roots);
	cgroup_root_count++;