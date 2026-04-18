// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 12/43



		/*
		 * Controllers from default hierarchy that need to be rebound
		 * are all disabled together in one go.
		 */
		cgrp_dfl_root.subsys_mask &= ~dfl_disable_ss_mask;
		WARN_ON(cgroup_apply_control(scgrp));
		cgroup_finalize_control(scgrp, 0);
	}

	do_each_subsys_mask(ss, ssid, ss_mask) {
		struct cgroup_root *src_root = ss->root;
		struct cgroup *scgrp = &src_root->cgrp;
		struct cgroup_subsys_state *css = cgroup_css(scgrp, ss);
		struct css_set *cset, *cset_pos;
		struct css_task_iter *it;

		WARN_ON(!css || cgroup_css(dcgrp, ss));

		if (src_root != &cgrp_dfl_root) {
			/* disable from the source */
			src_root->subsys_mask &= ~(1 << ssid);
			WARN_ON(cgroup_apply_control(scgrp));
			cgroup_finalize_control(scgrp, 0);
		}

		/* rebind */
		RCU_INIT_POINTER(scgrp->subsys[ssid], NULL);
		rcu_assign_pointer(dcgrp->subsys[ssid], css);
		ss->root = dst_root;

		spin_lock_irq(&css_set_lock);
		css->cgroup = dcgrp;
		WARN_ON(!list_empty(&dcgrp->e_csets[ss->id]));
		list_for_each_entry_safe(cset, cset_pos, &scgrp->e_csets[ss->id],
					 e_cset_node[ss->id]) {
			list_move_tail(&cset->e_cset_node[ss->id],
				       &dcgrp->e_csets[ss->id]);
			/*
			 * all css_sets of scgrp together in same order to dcgrp,
			 * patch in-flight iterators to preserve correct iteration.
			 * since the iterator is always advanced right away and
			 * finished when it->cset_pos meets it->cset_head, so only
			 * update it->cset_head is enough here.
			 */
			list_for_each_entry(it, &cset->task_iters, iters_node)
				if (it->cset_head == &scgrp->e_csets[ss->id])
					it->cset_head = &dcgrp->e_csets[ss->id];
		}
		spin_unlock_irq(&css_set_lock);

		/* default hierarchy doesn't enable controllers by default */
		dst_root->subsys_mask |= 1 << ssid;
		if (dst_root == &cgrp_dfl_root) {
			static_branch_enable(cgroup_subsys_on_dfl_key[ssid]);
		} else {
			dcgrp->subtree_control |= 1 << ssid;
			static_branch_disable(cgroup_subsys_on_dfl_key[ssid]);
		}

		ret = cgroup_apply_control(dcgrp);
		if (ret)
			pr_warn("partial failure to rebind %s controller (err=%d)\n",
				ss->name, ret);

		if (ss->bind)
			ss->bind(css);
	} while_each_subsys_mask();

	kernfs_activate(dcgrp->kn);
	return 0;
}

int cgroup_show_path(struct seq_file *sf, struct kernfs_node *kf_node,
		     struct kernfs_root *kf_root)
{
	int len = 0;
	char *buf = NULL;
	struct cgroup_root *kf_cgroot = cgroup_root_from_kf(kf_root);
	struct cgroup *ns_cgroup;

	buf = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	spin_lock_irq(&css_set_lock);
	ns_cgroup = current_cgns_cgroup_from_root(kf_cgroot);
	len = kernfs_path_from_node(kf_node, ns_cgroup->kn, buf, PATH_MAX);
	spin_unlock_irq(&css_set_lock);

	if (len == -E2BIG)
		len = -ERANGE;
	else if (len > 0) {
		seq_escape(sf, buf, " \t\n\\");
		len = 0;
	}
	kfree(buf);
	return len;
}

enum cgroup2_param {
	Opt_nsdelegate,
	Opt_favordynmods,
	Opt_memory_localevents,
	Opt_memory_recursiveprot,
	Opt_memory_hugetlb_accounting,
	Opt_pids_localevents,
	nr__cgroup2_params
};

static const struct fs_parameter_spec cgroup2_fs_parameters[] = {
	fsparam_flag("nsdelegate",		Opt_nsdelegate),
	fsparam_flag("favordynmods",		Opt_favordynmods),
	fsparam_flag("memory_localevents",	Opt_memory_localevents),
	fsparam_flag("memory_recursiveprot",	Opt_memory_recursiveprot),
	fsparam_flag("memory_hugetlb_accounting", Opt_memory_hugetlb_accounting),
	fsparam_flag("pids_localevents",	Opt_pids_localevents),
	{}
};

static int cgroup2_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);
	struct fs_parse_result result;
	int opt;

	opt = fs_parse(fc, cgroup2_fs_parameters, param, &result);
	if (opt < 0)
		return opt;

	switch (opt) {
	case Opt_nsdelegate:
		ctx->flags |= CGRP_ROOT_NS_DELEGATE;
		return 0;
	case Opt_favordynmods:
		ctx->flags |= CGRP_ROOT_FAVOR_DYNMODS;
		return 0;
	case Opt_memory_localevents:
		ctx->flags |= CGRP_ROOT_MEMORY_LOCAL_EVENTS;
		return 0;
	case Opt_memory_recursiveprot:
		ctx->flags |= CGRP_ROOT_MEMORY_RECURSIVE_PROT;
		return 0;
	case Opt_memory_hugetlb_accounting:
		ctx->flags |= CGRP_ROOT_MEMORY_HUGETLB_ACCOUNTING;
		return 0;
	case Opt_pids_localevents:
		ctx->flags |= CGRP_ROOT_PIDS_LOCAL_EVENTS;
		return 0;
	}
	return -EINVAL;
}

struct cgroup_of_peak *of_peak(struct kernfs_open_file *of)
{
	struct cgroup_file_ctx *ctx = of->priv;

	return &ctx->peak;
}

static void apply_cgroup_root_flags(unsigned int root_flags)
{
	if (current->nsproxy->cgroup_ns == &init_cgroup_ns) {
		if (root_flags & CGRP_ROOT_NS_DELEGATE)
			cgrp_dfl_root.flags |= CGRP_ROOT_NS_DELEGATE;
		else
			cgrp_dfl_root.flags &= ~CGRP_ROOT_NS_DELEGATE;

		cgroup_favor_dynmods(&cgrp_dfl_root,
				     root_flags & CGRP_ROOT_FAVOR_DYNMODS);

		if (root_flags & CGRP_ROOT_MEMORY_LOCAL_EVENTS)
			cgrp_dfl_root.flags |= CGRP_ROOT_MEMORY_LOCAL_EVENTS;
		else
			cgrp_dfl_root.flags &= ~CGRP_ROOT_MEMORY_LOCAL_EVENTS;