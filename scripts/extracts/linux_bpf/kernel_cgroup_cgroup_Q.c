// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 17/43



	/*
	 * Re-initialize the cgroup_taskset structure in case it is reused
	 * again in another cgroup_migrate_add_task()/cgroup_migrate_execute()
	 * iteration.
	 */
	tset->nr_tasks = 0;
	tset->csets    = &tset->src_csets;
	return ret;
}

/**
 * cgroup_migrate_vet_dst - verify whether a cgroup can be migration destination
 * @dst_cgrp: destination cgroup to test
 *
 * On the default hierarchy, except for the mixable, (possible) thread root
 * and threaded cgroups, subtree_control must be zero for migration
 * destination cgroups with tasks so that child cgroups don't compete
 * against tasks.
 */
int cgroup_migrate_vet_dst(struct cgroup *dst_cgrp)
{
	/* v1 doesn't have any restriction */
	if (!cgroup_on_dfl(dst_cgrp))
		return 0;

	/* verify @dst_cgrp can host resources */
	if (!cgroup_is_valid_domain(dst_cgrp->dom_cgrp))
		return -EOPNOTSUPP;

	/*
	 * If @dst_cgrp is already or can become a thread root or is
	 * threaded, it doesn't matter.
	 */
	if (cgroup_can_be_thread_root(dst_cgrp) || cgroup_is_threaded(dst_cgrp))
		return 0;

	/* apply no-internal-process constraint */
	if (dst_cgrp->subtree_control)
		return -EBUSY;

	return 0;
}

/**
 * cgroup_migrate_finish - cleanup after attach
 * @mgctx: migration context
 *
 * Undo cgroup_migrate_add_src() and cgroup_migrate_prepare_dst().  See
 * those functions for details.
 */
void cgroup_migrate_finish(struct cgroup_mgctx *mgctx)
{
	struct css_set *cset, *tmp_cset;

	lockdep_assert_held(&cgroup_mutex);

	spin_lock_irq(&css_set_lock);

	list_for_each_entry_safe(cset, tmp_cset, &mgctx->preloaded_src_csets,
				 mg_src_preload_node) {
		cset->mg_src_cgrp = NULL;
		cset->mg_dst_cgrp = NULL;
		cset->mg_dst_cset = NULL;
		list_del_init(&cset->mg_src_preload_node);
		put_css_set_locked(cset);
	}

	list_for_each_entry_safe(cset, tmp_cset, &mgctx->preloaded_dst_csets,
				 mg_dst_preload_node) {
		cset->mg_src_cgrp = NULL;
		cset->mg_dst_cgrp = NULL;
		cset->mg_dst_cset = NULL;
		list_del_init(&cset->mg_dst_preload_node);
		put_css_set_locked(cset);
	}

	spin_unlock_irq(&css_set_lock);
}

/**
 * cgroup_migrate_add_src - add a migration source css_set
 * @src_cset: the source css_set to add
 * @dst_cgrp: the destination cgroup
 * @mgctx: migration context
 *
 * Tasks belonging to @src_cset are about to be migrated to @dst_cgrp.  Pin
 * @src_cset and add it to @mgctx->src_csets, which should later be cleaned
 * up by cgroup_migrate_finish().
 *
 * This function may be called without holding cgroup_threadgroup_rwsem
 * even if the target is a process.  Threads may be created and destroyed
 * but as long as cgroup_mutex is not dropped, no new css_set can be put
 * into play and the preloaded css_sets are guaranteed to cover all
 * migrations.
 */
void cgroup_migrate_add_src(struct css_set *src_cset,
			    struct cgroup *dst_cgrp,
			    struct cgroup_mgctx *mgctx)
{
	struct cgroup *src_cgrp;

	lockdep_assert_held(&cgroup_mutex);
	lockdep_assert_held(&css_set_lock);

	/*
	 * If ->dead, @src_set is associated with one or more dead cgroups
	 * and doesn't contain any migratable tasks.  Ignore it early so
	 * that the rest of migration path doesn't get confused by it.
	 */
	if (src_cset->dead)
		return;

	if (!list_empty(&src_cset->mg_src_preload_node))
		return;

	src_cgrp = cset_cgroup_from_root(src_cset, dst_cgrp->root);

	WARN_ON(src_cset->mg_src_cgrp);
	WARN_ON(src_cset->mg_dst_cgrp);
	WARN_ON(!list_empty(&src_cset->mg_tasks));
	WARN_ON(!list_empty(&src_cset->mg_node));

	src_cset->mg_src_cgrp = src_cgrp;
	src_cset->mg_dst_cgrp = dst_cgrp;
	get_css_set(src_cset);
	list_add_tail(&src_cset->mg_src_preload_node, &mgctx->preloaded_src_csets);
}

/**
 * cgroup_migrate_prepare_dst - prepare destination css_sets for migration
 * @mgctx: migration context
 *
 * Tasks are about to be moved and all the source css_sets have been
 * preloaded to @mgctx->preloaded_src_csets.  This function looks up and
 * pins all destination css_sets, links each to its source, and append them
 * to @mgctx->preloaded_dst_csets.
 *
 * This function must be called after cgroup_migrate_add_src() has been
 * called on each migration source css_set.  After migration is performed
 * using cgroup_migrate(), cgroup_migrate_finish() must be called on
 * @mgctx.
 */
int cgroup_migrate_prepare_dst(struct cgroup_mgctx *mgctx)
{
	struct css_set *src_cset, *tmp_cset;

	lockdep_assert_held(&cgroup_mutex);

	/* look up the dst cset for each src cset and link it to src */
	list_for_each_entry_safe(src_cset, tmp_cset, &mgctx->preloaded_src_csets,
				 mg_src_preload_node) {
		struct css_set *dst_cset;
		struct cgroup_subsys *ss;
		int ssid;

		dst_cset = find_css_set(src_cset, src_cset->mg_dst_cgrp);
		if (!dst_cset)
			return -ENOMEM;

		WARN_ON_ONCE(src_cset->mg_dst_cset || dst_cset->mg_dst_cset);