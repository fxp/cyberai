// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 28/30



	/*
	 * Unlike hrtimer_start() it's ok to synchronously call
	 * hrtimer_try_to_cancel() when refcnt reached zero, but deferring to
	 * irq_work is not, since irq callback may execute after RCU GP and
	 * cb could be freed at that time. Check for refcnt zero for
	 * consistency.
	 */
	if (!refcount_inc_not_zero(&cb->refcnt))
		return -ENOENT;

	if (!defer_timer_wq_op()) {
		struct bpf_hrtimer *t = container_of(cb, struct bpf_hrtimer, cb);

		ret = hrtimer_try_to_cancel(&t->timer);
		bpf_async_refcount_put(cb);
		return ret;
	} else {
		ret = bpf_async_schedule_op(cb, BPF_ASYNC_CANCEL, 0, 0);
		return ret ? ret : -ECANCELED;
	}
}

__bpf_kfunc_end_defs();

static void bpf_task_work_cancel_scheduled(struct irq_work *irq_work)
{
	struct bpf_task_work_ctx *ctx = container_of(irq_work, struct bpf_task_work_ctx, irq_work);

	bpf_task_work_cancel(ctx); /* this might put task_work callback's ref */
	bpf_task_work_ctx_put(ctx); /* and here we put map's own ref that was transferred to us */
}

void bpf_task_work_cancel_and_free(void *val)
{
	struct bpf_task_work_kern *twk = val;
	struct bpf_task_work_ctx *ctx;
	enum bpf_task_work_state state;

	ctx = xchg(&twk->ctx, NULL);
	if (!ctx)
		return;

	state = xchg(&ctx->state, BPF_TW_FREED);
	if (state == BPF_TW_SCHEDULED) {
		/* run in irq_work to avoid locks in NMI */
		init_irq_work(&ctx->irq_work, bpf_task_work_cancel_scheduled);
		irq_work_queue(&ctx->irq_work);
		return;
	}

	bpf_task_work_ctx_put(ctx); /* put bpf map's ref */
}

BTF_KFUNCS_START(generic_btf_ids)
#ifdef CONFIG_CRASH_DUMP
BTF_ID_FLAGS(func, crash_kexec, KF_DESTRUCTIVE)
#endif
BTF_ID_FLAGS(func, bpf_obj_new, KF_ACQUIRE | KF_RET_NULL | KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_obj_new_impl, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_percpu_obj_new, KF_ACQUIRE | KF_RET_NULL | KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_percpu_obj_new_impl, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_obj_drop, KF_RELEASE | KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_obj_drop_impl, KF_RELEASE)
BTF_ID_FLAGS(func, bpf_percpu_obj_drop, KF_RELEASE | KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_percpu_obj_drop_impl, KF_RELEASE)
BTF_ID_FLAGS(func, bpf_refcount_acquire, KF_ACQUIRE | KF_RET_NULL | KF_RCU | KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_refcount_acquire_impl, KF_ACQUIRE | KF_RET_NULL | KF_RCU)
BTF_ID_FLAGS(func, bpf_list_push_front, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_list_push_front_impl)
BTF_ID_FLAGS(func, bpf_list_push_back, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_list_push_back_impl)
BTF_ID_FLAGS(func, bpf_list_pop_front, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_list_pop_back, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_list_front, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_list_back, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_task_acquire, KF_ACQUIRE | KF_RCU | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_task_release, KF_RELEASE)
BTF_ID_FLAGS(func, bpf_rbtree_remove, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_rbtree_add, KF_IMPLICIT_ARGS)
BTF_ID_FLAGS(func, bpf_rbtree_add_impl)
BTF_ID_FLAGS(func, bpf_rbtree_first, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_rbtree_root, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_rbtree_left, KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_rbtree_right, KF_RET_NULL)

#ifdef CONFIG_CGROUPS
BTF_ID_FLAGS(func, bpf_cgroup_acquire, KF_ACQUIRE | KF_RCU | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_cgroup_release, KF_RELEASE)
BTF_ID_FLAGS(func, bpf_cgroup_ancestor, KF_ACQUIRE | KF_RCU | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_cgroup_from_id, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_task_under_cgroup, KF_RCU)
BTF_ID_FLAGS(func, bpf_task_get_cgroup1, KF_ACQUIRE | KF_RCU | KF_RET_NULL)
#endif
BTF_ID_FLAGS(func, bpf_task_from_pid, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_task_from_vpid, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_throw)
#ifdef CONFIG_BPF_EVENTS
BTF_ID_FLAGS(func, bpf_send_signal_task)
#endif
#ifdef CONFIG_KEYS
BTF_ID_FLAGS(func, bpf_lookup_user_key, KF_ACQUIRE | KF_RET_NULL | KF_SLEEPABLE)
BTF_ID_FLAGS(func, bpf_lookup_system_key, KF_ACQUIRE | KF_RET_NULL)
BTF_ID_FLAGS(func, bpf_key_put, KF_RELEASE)
#ifdef CONFIG_SYSTEM_DATA_VERIFICATION
BTF_ID_FLAGS(func, bpf_verify_pkcs7_signature, KF_SLEEPABLE)
#endif
#endif
#ifdef CONFIG_S390
BTF_ID_FLAGS(func, bpf_get_lowcore)
#endif
BTF_KFUNCS_END(generic_btf_ids)

static const struct btf_kfunc_id_set generic_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &generic_btf_ids,
};


BTF_ID_LIST(generic_dtor_ids)
BTF_ID(struct, task_struct)
BTF_ID(func, bpf_task_release_dtor)
#ifdef CONFIG_CGROUPS
BTF_ID(struct, cgroup)
BTF_ID(func, bpf_cgroup_release_dtor)
#endif