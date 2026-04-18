// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 32/43



static struct cftype cgroup_psi_files[] = {
#ifdef CONFIG_PSI
	{
		.name = "io.pressure",
		.file_offset = offsetof(struct cgroup, psi_files[PSI_IO]),
		.seq_show = cgroup_io_pressure_show,
		.write = cgroup_io_pressure_write,
		.poll = cgroup_pressure_poll,
		.release = cgroup_pressure_release,
	},
	{
		.name = "memory.pressure",
		.file_offset = offsetof(struct cgroup, psi_files[PSI_MEM]),
		.seq_show = cgroup_memory_pressure_show,
		.write = cgroup_memory_pressure_write,
		.poll = cgroup_pressure_poll,
		.release = cgroup_pressure_release,
	},
	{
		.name = "cpu.pressure",
		.file_offset = offsetof(struct cgroup, psi_files[PSI_CPU]),
		.seq_show = cgroup_cpu_pressure_show,
		.write = cgroup_cpu_pressure_write,
		.poll = cgroup_pressure_poll,
		.release = cgroup_pressure_release,
	},
#ifdef CONFIG_IRQ_TIME_ACCOUNTING
	{
		.name = "irq.pressure",
		.file_offset = offsetof(struct cgroup, psi_files[PSI_IRQ]),
		.seq_show = cgroup_irq_pressure_show,
		.write = cgroup_irq_pressure_write,
		.poll = cgroup_pressure_poll,
		.release = cgroup_pressure_release,
	},
#endif
	{
		.name = "cgroup.pressure",
		.seq_show = cgroup_pressure_show,
		.write = cgroup_pressure_write,
	},
#endif /* CONFIG_PSI */
	{ }	/* terminate */
};

/*
 * css destruction is four-stage process.
 *
 * 1. Destruction starts.  Killing of the percpu_ref is initiated.
 *    Implemented in kill_css().
 *
 * 2. When the percpu_ref is confirmed to be visible as killed on all CPUs
 *    and thus css_tryget_online() is guaranteed to fail, the css can be
 *    offlined by invoking offline_css().  After offlining, the base ref is
 *    put.  Implemented in css_killed_work_fn().
 *
 * 3. When the percpu_ref reaches zero, the only possible remaining
 *    accessors are inside RCU read sections.  css_release() schedules the
 *    RCU callback.
 *
 * 4. After the grace period, the css can be freed.  Implemented in
 *    css_free_rwork_fn().
 *
 * It is actually hairier because both step 2 and 4 require process context
 * and thus involve punting to css->destroy_work adding two additional
 * steps to the already complex sequence.
 */
static void css_free_rwork_fn(struct work_struct *work)
{
	struct cgroup_subsys_state *css = container_of(to_rcu_work(work),
				struct cgroup_subsys_state, destroy_rwork);
	struct cgroup_subsys *ss = css->ss;
	struct cgroup *cgrp = css->cgroup;

	percpu_ref_exit(&css->refcnt);
	css_rstat_exit(css);

	if (!css_is_self(css)) {
		/* css free path */
		struct cgroup_subsys_state *parent = css->parent;
		int id = css->id;

		ss->css_free(css);
		cgroup_idr_remove(&ss->css_idr, id);
		cgroup_put(cgrp);

		if (parent)
			css_put(parent);
	} else {
		/* cgroup free path */
		atomic_dec(&cgrp->root->nr_cgrps);
		if (!cgroup_on_dfl(cgrp))
			cgroup1_pidlist_destroy_all(cgrp);
		cancel_work_sync(&cgrp->release_agent_work);
		bpf_cgrp_storage_free(cgrp);

		if (cgroup_parent(cgrp)) {
			/*
			 * We get a ref to the parent, and put the ref when
			 * this cgroup is being freed, so it's guaranteed
			 * that the parent won't be destroyed before its
			 * children.
			 */
			cgroup_put(cgroup_parent(cgrp));
			kernfs_put(cgrp->kn);
			psi_cgroup_free(cgrp);
			kfree(cgrp);
		} else {
			/*
			 * This is root cgroup's refcnt reaching zero,
			 * which indicates that the root should be
			 * released.
			 */
			cgroup_destroy_root(cgrp->root);
		}
	}
}

static void css_release_work_fn(struct work_struct *work)
{
	struct cgroup_subsys_state *css =
		container_of(work, struct cgroup_subsys_state, destroy_work);
	struct cgroup_subsys *ss = css->ss;
	struct cgroup *cgrp = css->cgroup;

	cgroup_lock();

	css->flags |= CSS_RELEASED;
	list_del_rcu(&css->sibling);

	if (!css_is_self(css)) {
		struct cgroup *parent_cgrp;

		css_rstat_flush(css);

		cgroup_idr_replace(&ss->css_idr, NULL, css->id);
		if (ss->css_released)
			ss->css_released(css);

		cgrp->nr_dying_subsys[ss->id]--;
		/*
		 * When a css is released and ready to be freed, its
		 * nr_descendants must be zero. However, the corresponding
		 * cgrp->nr_dying_subsys[ss->id] may not be 0 if a subsystem
		 * is activated and deactivated multiple times with one or
		 * more of its previous activation leaving behind dying csses.
		 */
		WARN_ON_ONCE(css->nr_descendants);
		parent_cgrp = cgroup_parent(cgrp);
		while (parent_cgrp) {
			parent_cgrp->nr_dying_subsys[ss->id]--;
			parent_cgrp = cgroup_parent(parent_cgrp);
		}
	} else {
		struct cgroup *tcgrp;

		/* cgroup release path */
		TRACE_CGROUP_PATH(release, cgrp);

		css_rstat_flush(&cgrp->self);

		spin_lock_irq(&css_set_lock);
		for (tcgrp = cgroup_parent(cgrp); tcgrp;
		     tcgrp = cgroup_parent(tcgrp))
			tcgrp->nr_dying_descendants--;
		spin_unlock_irq(&css_set_lock);