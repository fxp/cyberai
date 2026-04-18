// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 37/43



/**
 * cgroup_init_early - cgroup initialization at system boot
 *
 * Initialize cgroups at system boot, and initialize any
 * subsystems that request early init.
 */
int __init cgroup_init_early(void)
{
	static struct cgroup_fs_context __initdata ctx;
	struct cgroup_subsys *ss;
	int i;

	ctx.root = &cgrp_dfl_root;
	init_cgroup_root(&ctx);
	cgrp_dfl_root.cgrp.self.flags |= CSS_NO_REF;

	RCU_INIT_POINTER(init_task.cgroups, &init_css_set);

	for_each_subsys(ss, i) {
		WARN(!ss->css_alloc || !ss->css_free || ss->name || ss->id,
		     "invalid cgroup_subsys %d:%s css_alloc=%p css_free=%p id:name=%d:%s\n",
		     i, cgroup_subsys_name[i], ss->css_alloc, ss->css_free,
		     ss->id, ss->name);
		WARN(strlen(cgroup_subsys_name[i]) > MAX_CGROUP_TYPE_NAMELEN,
		     "cgroup_subsys_name %s too long\n", cgroup_subsys_name[i]);
		WARN(ss->early_init && ss->css_rstat_flush,
		     "cgroup rstat cannot be used with early init subsystem\n");

		ss->id = i;
		ss->name = cgroup_subsys_name[i];
		if (!ss->legacy_name)
			ss->legacy_name = cgroup_subsys_name[i];

		if (ss->early_init)
			cgroup_init_subsys(ss, true);
	}
	return 0;
}

/**
 * cgroup_init - cgroup initialization
 *
 * Register cgroup filesystem and /proc file, and initialize
 * any subsystems that didn't request early init.
 */
int __init cgroup_init(void)
{
	struct cgroup_subsys *ss;
	int ssid;

	BUILD_BUG_ON(CGROUP_SUBSYS_COUNT > 32);
	BUG_ON(cgroup_init_cftypes(NULL, cgroup_base_files));
	BUG_ON(cgroup_init_cftypes(NULL, cgroup_psi_files));
	BUG_ON(cgroup_init_cftypes(NULL, cgroup1_base_files));

	BUG_ON(ss_rstat_init(NULL));

	get_user_ns(init_cgroup_ns.user_ns);
	cgroup_rt_init();

	cgroup_lock();

	/*
	 * Add init_css_set to the hash table so that dfl_root can link to
	 * it during init.
	 */
	hash_add(css_set_table, &init_css_set.hlist,
		 css_set_hash(init_css_set.subsys));

	cgroup_bpf_lifetime_notifier_init();

	BUG_ON(cgroup_setup_root(&cgrp_dfl_root, 0));

	cgroup_unlock();

	for_each_subsys(ss, ssid) {
		if (ss->early_init) {
			struct cgroup_subsys_state *css =
				init_css_set.subsys[ss->id];

			css->id = cgroup_idr_alloc(&ss->css_idr, css, 1, 2,
						   GFP_KERNEL);
			BUG_ON(css->id < 0);
		} else {
			cgroup_init_subsys(ss, false);
		}

		list_add_tail(&init_css_set.e_cset_node[ssid],
			      &cgrp_dfl_root.cgrp.e_csets[ssid]);

		/*
		 * Setting dfl_root subsys_mask needs to consider the
		 * disabled flag and cftype registration needs kmalloc,
		 * both of which aren't available during early_init.
		 */
		if (!cgroup_ssid_enabled(ssid))
			continue;

		if (cgroup1_ssid_disabled(ssid))
			pr_info("Disabling %s control group subsystem in v1 mounts\n",
				ss->legacy_name);

		cgrp_dfl_root.subsys_mask |= 1 << ss->id;

		/* implicit controllers must be threaded too */
		WARN_ON(ss->implicit_on_dfl && !ss->threaded);

		if (ss->implicit_on_dfl)
			cgrp_dfl_implicit_ss_mask |= 1 << ss->id;
		else if (!ss->dfl_cftypes)
			cgrp_dfl_inhibit_ss_mask |= 1 << ss->id;

		if (ss->threaded)
			cgrp_dfl_threaded_ss_mask |= 1 << ss->id;

		if (ss->dfl_cftypes == ss->legacy_cftypes) {
			WARN_ON(cgroup_add_cftypes(ss, ss->dfl_cftypes));
		} else {
			WARN_ON(cgroup_add_dfl_cftypes(ss, ss->dfl_cftypes));
			WARN_ON(cgroup_add_legacy_cftypes(ss, ss->legacy_cftypes));
		}

		if (ss->bind)
			ss->bind(init_css_set.subsys[ssid]);

		cgroup_lock();
		css_populate_dir(init_css_set.subsys[ssid]);
		cgroup_unlock();
	}

	/* init_css_set.subsys[] has been updated, re-hash */
	hash_del(&init_css_set.hlist);
	hash_add(css_set_table, &init_css_set.hlist,
		 css_set_hash(init_css_set.subsys));

	WARN_ON(sysfs_create_mount_point(fs_kobj, "cgroup"));
	WARN_ON(register_filesystem(&cgroup_fs_type));
	WARN_ON(register_filesystem(&cgroup2_fs_type));
	WARN_ON(!proc_create_single("cgroups", 0, NULL, proc_cgroupstats_show));
#ifdef CONFIG_CPUSETS_V1
	WARN_ON(register_filesystem(&cpuset_fs_type));
#endif

	ns_tree_add(&init_cgroup_ns);
	return 0;
}

static int __init cgroup_wq_init(void)
{
	/*
	 * There isn't much point in executing destruction path in
	 * parallel.  Good chunk is serialized with cgroup_mutex anyway.
	 * Use 1 for @max_active.
	 *
	 * We would prefer to do this in cgroup_init() above, but that
	 * is called before init_workqueues(): so leave this until after.
	 */
	cgroup_offline_wq = alloc_workqueue("cgroup_offline", WQ_PERCPU, 1);
	BUG_ON(!cgroup_offline_wq);

	cgroup_release_wq = alloc_workqueue("cgroup_release", WQ_PERCPU, 1);
	BUG_ON(!cgroup_release_wq);

	cgroup_free_wq = alloc_workqueue("cgroup_free", WQ_PERCPU, 1);
	BUG_ON(!cgroup_free_wq);
	return 0;
}
core_initcall(cgroup_wq_init);

void cgroup_path_from_kernfs_id(u64 id, char *buf, size_t buflen)
{
	struct kernfs_node *kn;

	kn = kernfs_find_and_get_node_by_id(cgrp_dfl_root.kf_root, id);
	if (!kn)
		return;
	kernfs_path(kn, buf, buflen);
	kernfs_put(kn);
}