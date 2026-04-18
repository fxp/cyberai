// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 16/30



__bpf_kfunc int bpf_rbtree_add_impl(struct bpf_rb_root *root, struct bpf_rb_node *node,
				    bool (less)(struct bpf_rb_node *a, const struct bpf_rb_node *b),
				    void *meta__ign, u64 off)
{
	return bpf_rbtree_add(root, node, less, meta__ign, off);
}

__bpf_kfunc struct bpf_rb_node *bpf_rbtree_first(struct bpf_rb_root *root)
{
	struct rb_root_cached *r = (struct rb_root_cached *)root;

	return (struct bpf_rb_node *)rb_first_cached(r);
}

__bpf_kfunc struct bpf_rb_node *bpf_rbtree_root(struct bpf_rb_root *root)
{
	struct rb_root_cached *r = (struct rb_root_cached *)root;

	return (struct bpf_rb_node *)r->rb_root.rb_node;
}

__bpf_kfunc struct bpf_rb_node *bpf_rbtree_left(struct bpf_rb_root *root, struct bpf_rb_node *node)
{
	struct bpf_rb_node_kern *node_internal = (struct bpf_rb_node_kern *)node;

	if (READ_ONCE(node_internal->owner) != root)
		return NULL;

	return (struct bpf_rb_node *)node_internal->rb_node.rb_left;
}

__bpf_kfunc struct bpf_rb_node *bpf_rbtree_right(struct bpf_rb_root *root, struct bpf_rb_node *node)
{
	struct bpf_rb_node_kern *node_internal = (struct bpf_rb_node_kern *)node;

	if (READ_ONCE(node_internal->owner) != root)
		return NULL;

	return (struct bpf_rb_node *)node_internal->rb_node.rb_right;
}

/**
 * bpf_task_acquire - Acquire a reference to a task. A task acquired by this
 * kfunc which is not stored in a map as a kptr, must be released by calling
 * bpf_task_release().
 * @p: The task on which a reference is being acquired.
 */
__bpf_kfunc struct task_struct *bpf_task_acquire(struct task_struct *p)
{
	if (refcount_inc_not_zero(&p->rcu_users))
		return p;
	return NULL;
}

/**
 * bpf_task_release - Release the reference acquired on a task.
 * @p: The task on which a reference is being released.
 */
__bpf_kfunc void bpf_task_release(struct task_struct *p)
{
	put_task_struct_rcu_user(p);
}

__bpf_kfunc void bpf_task_release_dtor(void *p)
{
	put_task_struct_rcu_user(p);
}
CFI_NOSEAL(bpf_task_release_dtor);

#ifdef CONFIG_CGROUPS
/**
 * bpf_cgroup_acquire - Acquire a reference to a cgroup. A cgroup acquired by
 * this kfunc which is not stored in a map as a kptr, must be released by
 * calling bpf_cgroup_release().
 * @cgrp: The cgroup on which a reference is being acquired.
 */
__bpf_kfunc struct cgroup *bpf_cgroup_acquire(struct cgroup *cgrp)
{
	return cgroup_tryget(cgrp) ? cgrp : NULL;
}

/**
 * bpf_cgroup_release - Release the reference acquired on a cgroup.
 * If this kfunc is invoked in an RCU read region, the cgroup is guaranteed to
 * not be freed until the current grace period has ended, even if its refcount
 * drops to 0.
 * @cgrp: The cgroup on which a reference is being released.
 */
__bpf_kfunc void bpf_cgroup_release(struct cgroup *cgrp)
{
	cgroup_put(cgrp);
}

__bpf_kfunc void bpf_cgroup_release_dtor(void *cgrp)
{
	cgroup_put(cgrp);
}
CFI_NOSEAL(bpf_cgroup_release_dtor);

/**
 * bpf_cgroup_ancestor - Perform a lookup on an entry in a cgroup's ancestor
 * array. A cgroup returned by this kfunc which is not subsequently stored in a
 * map, must be released by calling bpf_cgroup_release().
 * @cgrp: The cgroup for which we're performing a lookup.
 * @level: The level of ancestor to look up.
 */
__bpf_kfunc struct cgroup *bpf_cgroup_ancestor(struct cgroup *cgrp, int level)
{
	struct cgroup *ancestor;

	if (level > cgrp->level || level < 0)
		return NULL;

	/* cgrp's refcnt could be 0 here, but ancestors can still be accessed */
	ancestor = cgrp->ancestors[level];
	if (!cgroup_tryget(ancestor))
		return NULL;
	return ancestor;
}

/**
 * bpf_cgroup_from_id - Find a cgroup from its ID. A cgroup returned by this
 * kfunc which is not subsequently stored in a map, must be released by calling
 * bpf_cgroup_release().
 * @cgid: cgroup id.
 */
__bpf_kfunc struct cgroup *bpf_cgroup_from_id(u64 cgid)
{
	struct cgroup *cgrp;

	cgrp = __cgroup_get_from_id(cgid);
	if (IS_ERR(cgrp))
		return NULL;
	return cgrp;
}

/**
 * bpf_task_under_cgroup - wrap task_under_cgroup_hierarchy() as a kfunc, test
 * task's membership of cgroup ancestry.
 * @task: the task to be tested
 * @ancestor: possible ancestor of @task's cgroup
 *
 * Tests whether @task's default cgroup hierarchy is a descendant of @ancestor.
 * It follows all the same rules as cgroup_is_descendant, and only applies
 * to the default hierarchy.
 */
__bpf_kfunc long bpf_task_under_cgroup(struct task_struct *task,
				       struct cgroup *ancestor)
{
	long ret;

	rcu_read_lock();
	ret = task_under_cgroup_hierarchy(task, ancestor);
	rcu_read_unlock();
	return ret;
}

BPF_CALL_2(bpf_current_task_under_cgroup, struct bpf_map *, map, u32, idx)
{
	struct bpf_array *array = container_of(map, struct bpf_array, map);
	struct cgroup *cgrp;

	if (unlikely(idx >= array->map.max_entries))
		return -E2BIG;

	cgrp = READ_ONCE(array->ptrs[idx]);
	if (unlikely(!cgrp))
		return -EAGAIN;

	return task_under_cgroup_hierarchy(current, cgrp);
}