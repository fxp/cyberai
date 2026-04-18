// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/cgroup/cgroup.c
// Segment 41/43



#ifdef CONFIG_PREEMPT_RT
/*
 * cgroup_task_dead() is called from finish_task_switch() which doesn't allow
 * scheduling even in RT. As the task_dead path requires grabbing css_set_lock,
 * this lead to sleeping in the invalid context warning bug. css_set_lock is too
 * big to become a raw_spinlock. The task_dead path doesn't need to run
 * synchronously but can't be delayed indefinitely either as the dead task pins
 * the cgroup and task_struct can be pinned indefinitely. Bounce through lazy
 * irq_work to allow batching while ensuring timely completion.
 */
static DEFINE_PER_CPU(struct llist_head, cgrp_dead_tasks);
static DEFINE_PER_CPU(struct irq_work, cgrp_dead_tasks_iwork);

static void cgrp_dead_tasks_iwork_fn(struct irq_work *iwork)
{
	struct llist_node *lnode;
	struct task_struct *task, *next;

	lnode = llist_del_all(this_cpu_ptr(&cgrp_dead_tasks));
	llist_for_each_entry_safe(task, next, lnode, cg_dead_lnode) {
		do_cgroup_task_dead(task);
		put_task_struct(task);
	}
}

static void __init cgroup_rt_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		init_llist_head(per_cpu_ptr(&cgrp_dead_tasks, cpu));
		per_cpu(cgrp_dead_tasks_iwork, cpu) =
			IRQ_WORK_INIT_LAZY(cgrp_dead_tasks_iwork_fn);
	}
}

void cgroup_task_dead(struct task_struct *task)
{
	get_task_struct(task);
	llist_add(&task->cg_dead_lnode, this_cpu_ptr(&cgrp_dead_tasks));
	irq_work_queue(this_cpu_ptr(&cgrp_dead_tasks_iwork));
}
#else	/* CONFIG_PREEMPT_RT */
static void __init cgroup_rt_init(void) {}

void cgroup_task_dead(struct task_struct *task)
{
	do_cgroup_task_dead(task);
}
#endif	/* CONFIG_PREEMPT_RT */

void cgroup_task_release(struct task_struct *task)
{
	struct cgroup_subsys *ss;
	int ssid;

	do_each_subsys_mask(ss, ssid, have_release_callback) {
		ss->release(task);
	} while_each_subsys_mask();
}

void cgroup_task_free(struct task_struct *task)
{
	struct css_set *cset = task_css_set(task);

	if (!list_empty(&task->cg_list)) {
		spin_lock_irq(&css_set_lock);
		css_set_skip_task_iters(task_css_set(task), task);
		list_del_init(&task->cg_list);
		spin_unlock_irq(&css_set_lock);
	}

	put_css_set(cset);
}

static int __init cgroup_disable(char *str)
{
	struct cgroup_subsys *ss;
	char *token;
	int i;

	while ((token = strsep(&str, ",")) != NULL) {
		if (!*token)
			continue;

		for_each_subsys(ss, i) {
			if (strcmp(token, ss->name) &&
			    strcmp(token, ss->legacy_name))
				continue;

			static_branch_disable(cgroup_subsys_enabled_key[i]);
			pr_info("Disabling %s control group subsystem\n",
				ss->name);
		}

		for (i = 0; i < OPT_FEATURE_COUNT; i++) {
			if (strcmp(token, cgroup_opt_feature_names[i]))
				continue;
			cgroup_feature_disable_mask |= 1 << i;
			pr_info("Disabling %s control group feature\n",
				cgroup_opt_feature_names[i]);
			break;
		}
	}
	return 1;
}
__setup("cgroup_disable=", cgroup_disable);

void __init __weak enable_debug_cgroup(void) { }

static int __init enable_cgroup_debug(char *str)
{
	cgroup_debug = true;
	enable_debug_cgroup();
	return 1;
}
__setup("cgroup_debug", enable_cgroup_debug);

static int __init cgroup_favordynmods_setup(char *str)
{
	return (kstrtobool(str, &have_favordynmods) == 0);
}
__setup("cgroup_favordynmods=", cgroup_favordynmods_setup);

/**
 * css_tryget_online_from_dir - get corresponding css from a cgroup dentry
 * @dentry: directory dentry of interest
 * @ss: subsystem of interest
 *
 * If @dentry is a directory for a cgroup which has @ss enabled on it, try
 * to get the corresponding css and return it.  If such css doesn't exist
 * or can't be pinned, an ERR_PTR value is returned.
 */
struct cgroup_subsys_state *css_tryget_online_from_dir(struct dentry *dentry,
						       struct cgroup_subsys *ss)
{
	struct kernfs_node *kn = kernfs_node_from_dentry(dentry);
	struct file_system_type *s_type = dentry->d_sb->s_type;
	struct cgroup_subsys_state *css = NULL;
	struct cgroup *cgrp;

	/* is @dentry a cgroup dir? */
	if ((s_type != &cgroup_fs_type && s_type != &cgroup2_fs_type) ||
	    !kn || kernfs_type(kn) != KERNFS_DIR)
		return ERR_PTR(-EBADF);

	rcu_read_lock();

	/*
	 * This path doesn't originate from kernfs and @kn could already
	 * have been or be removed at any point.  @kn->priv is RCU
	 * protected for this access.  See css_release_work_fn() for details.
	 */
	cgrp = rcu_dereference(*(void __rcu __force **)&kn->priv);
	if (cgrp)
		css = cgroup_css(cgrp, ss);

	if (!css || !css_tryget_online(css))
		css = ERR_PTR(-ENOENT);

	rcu_read_unlock();
	return css;
}

/**
 * css_from_id - lookup css by id
 * @id: the cgroup id
 * @ss: cgroup subsys to be looked into
 *
 * Returns the css if there's valid one with @id, otherwise returns NULL.
 * Should be called under rcu_read_lock().
 */
struct cgroup_subsys_state *css_from_id(int id, struct cgroup_subsys *ss)
{
	WARN_ON_ONCE(!rcu_read_lock_held());
	return idr_find(&ss->css_idr, id);
}