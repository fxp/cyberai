// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 14/43



	/*
	 * Link the root cgroup in this hierarchy into all the css_set
	 * objects.
	 */
	spin_lock_irq(&css_set_lock);
	hash_for_each(css_set_table, i, cset, hlist) {
		link_css_set(&tmp_links, cset, root_cgrp);
		if (css_set_populated(cset))
			cgroup_update_populated(root_cgrp, true);
	}
	spin_unlock_irq(&css_set_lock);

	BUG_ON(!list_empty(&root_cgrp->self.children));
	BUG_ON(atomic_read(&root->nr_cgrps) != 1);

	ret = 0;
	goto out;

exit_stats:
	css_rstat_exit(&root_cgrp->self);
destroy_root:
	kernfs_destroy_root(root->kf_root);
	root->kf_root = NULL;
exit_root_id:
	cgroup_exit_root_id(root);
cancel_ref:
	percpu_ref_exit(&root_cgrp->self.refcnt);
out:
	free_cgrp_cset_links(&tmp_links);
	return ret;
}

int cgroup_do_get_tree(struct fs_context *fc)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);
	int ret;

	ctx->kfc.root = ctx->root->kf_root;
	if (fc->fs_type == &cgroup2_fs_type)
		ctx->kfc.magic = CGROUP2_SUPER_MAGIC;
	else
		ctx->kfc.magic = CGROUP_SUPER_MAGIC;
	ret = kernfs_get_tree(fc);

	/*
	 * In non-init cgroup namespace, instead of root cgroup's dentry,
	 * we return the dentry corresponding to the cgroupns->root_cgrp.
	 */
	if (!ret && ctx->ns != &init_cgroup_ns) {
		struct dentry *nsdentry;
		struct super_block *sb = fc->root->d_sb;
		struct cgroup *cgrp;

		cgroup_lock();
		spin_lock_irq(&css_set_lock);

		cgrp = cset_cgroup_from_root(ctx->ns->root_cset, ctx->root);

		spin_unlock_irq(&css_set_lock);
		cgroup_unlock();

		nsdentry = kernfs_node_dentry(cgrp->kn, sb);
		dput(fc->root);
		if (IS_ERR(nsdentry)) {
			deactivate_locked_super(sb);
			ret = PTR_ERR(nsdentry);
			nsdentry = NULL;
		}
		fc->root = nsdentry;
	}

	if (!ctx->kfc.new_sb_created)
		cgroup_put(&ctx->root->cgrp);

	return ret;
}

/*
 * Destroy a cgroup filesystem context.
 */
static void cgroup_fs_context_free(struct fs_context *fc)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);

	kfree(ctx->name);
	kfree(ctx->release_agent);
	put_cgroup_ns(ctx->ns);
	kernfs_free_fs_context(fc);
	kfree(ctx);
}

static int cgroup_get_tree(struct fs_context *fc)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);
	int ret;

	WRITE_ONCE(cgrp_dfl_visible, true);
	cgroup_get_live(&cgrp_dfl_root.cgrp);
	ctx->root = &cgrp_dfl_root;

	ret = cgroup_do_get_tree(fc);
	if (!ret)
		apply_cgroup_root_flags(ctx->flags);
	return ret;
}

static const struct fs_context_operations cgroup_fs_context_ops = {
	.free		= cgroup_fs_context_free,
	.parse_param	= cgroup2_parse_param,
	.get_tree	= cgroup_get_tree,
	.reconfigure	= cgroup_reconfigure,
};

static const struct fs_context_operations cgroup1_fs_context_ops = {
	.free		= cgroup_fs_context_free,
	.parse_param	= cgroup1_parse_param,
	.get_tree	= cgroup1_get_tree,
	.reconfigure	= cgroup1_reconfigure,
};

/*
 * Initialise the cgroup filesystem creation/reconfiguration context.  Notably,
 * we select the namespace we're going to use.
 */
static int cgroup_init_fs_context(struct fs_context *fc)
{
	struct cgroup_fs_context *ctx;

	ctx = kzalloc_obj(struct cgroup_fs_context);
	if (!ctx)
		return -ENOMEM;

	ctx->ns = current->nsproxy->cgroup_ns;
	get_cgroup_ns(ctx->ns);
	fc->fs_private = &ctx->kfc;
	if (fc->fs_type == &cgroup2_fs_type)
		fc->ops = &cgroup_fs_context_ops;
	else
		fc->ops = &cgroup1_fs_context_ops;
	put_user_ns(fc->user_ns);
	fc->user_ns = get_user_ns(ctx->ns->user_ns);
	fc->global = true;

	if (have_favordynmods)
		ctx->flags |= CGRP_ROOT_FAVOR_DYNMODS;

	return 0;
}

static void cgroup_kill_sb(struct super_block *sb)
{
	struct kernfs_root *kf_root = kernfs_root_from_sb(sb);
	struct cgroup_root *root = cgroup_root_from_kf(kf_root);

	/*
	 * If @root doesn't have any children, start killing it.
	 * This prevents new mounts by disabling percpu_ref_tryget_live().
	 *
	 * And don't kill the default root.
	 */
	if (list_empty(&root->cgrp.self.children) && root != &cgrp_dfl_root &&
	    !percpu_ref_is_dying(&root->cgrp.self.refcnt))
		percpu_ref_kill(&root->cgrp.self.refcnt);
	cgroup_put(&root->cgrp);
	kernfs_kill_sb(sb);
}

struct file_system_type cgroup_fs_type = {
	.name			= "cgroup",
	.init_fs_context	= cgroup_init_fs_context,
	.parameters		= cgroup1_fs_parameters,
	.kill_sb		= cgroup_kill_sb,
	.fs_flags		= FS_USERNS_MOUNT,
};

static struct file_system_type cgroup2_fs_type = {
	.name			= "cgroup2",
	.init_fs_context	= cgroup_init_fs_context,
	.parameters		= cgroup2_fs_parameters,
	.kill_sb		= cgroup_kill_sb,
	.fs_flags		= FS_USERNS_MOUNT,
};

#ifdef CONFIG_CPUSETS_V1
enum cpuset_param {
	Opt_cpuset_v2_mode,
};

static const struct fs_parameter_spec cpuset_fs_parameters[] = {
	fsparam_flag  ("cpuset_v2_mode", Opt_cpuset_v2_mode),
	{}
};

static int cpuset_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct cgroup_fs_context *ctx = cgroup_fc2context(fc);
	struct fs_parse_result result;
	int opt;

	opt = fs_parse(fc, cpuset_fs_parameters, param, &result);
	if (opt < 0)
		return opt;