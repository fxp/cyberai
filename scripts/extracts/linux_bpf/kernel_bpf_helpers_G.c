// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 7/30



	callback_fn((u64)(long)map, (u64)(long)key, (u64)(long)value, 0, 0);
	/* The verifier checked that return value is zero. */

	this_cpu_write(hrtimer_running, NULL);
out:
	return HRTIMER_NORESTART;
}

static void bpf_wq_work(struct work_struct *work)
{
	struct bpf_work *w = container_of(work, struct bpf_work, work);
	struct bpf_async_cb *cb = &w->cb;
	struct bpf_map *map = cb->map;
	bpf_callback_t callback_fn;
	void *value = cb->value;
	void *key;
	u32 idx;

	BTF_TYPE_EMIT(struct bpf_wq);

	callback_fn = READ_ONCE(cb->callback_fn);
	if (!callback_fn)
		return;

	key = map_key_from_value(map, value, &idx);

        rcu_read_lock_trace();
        migrate_disable();

	callback_fn((u64)(long)map, (u64)(long)key, (u64)(long)value, 0, 0);

	migrate_enable();
	rcu_read_unlock_trace();
}

static void bpf_async_cb_rcu_free(struct rcu_head *rcu)
{
	struct bpf_async_cb *cb = container_of(rcu, struct bpf_async_cb, rcu);

	/*
	 * Drop the last reference to prog only after RCU GP, as set_callback()
	 * may race with cancel_and_free()
	 */
	if (cb->prog)
		bpf_prog_put(cb->prog);

	kfree_nolock(cb);
}

/* Callback from call_rcu_tasks_trace, chains to call_rcu for final free */
static void bpf_async_cb_rcu_tasks_trace_free(struct rcu_head *rcu)
{
	struct bpf_async_cb *cb = container_of(rcu, struct bpf_async_cb, rcu);
	struct bpf_hrtimer *t = container_of(cb, struct bpf_hrtimer, cb);
	struct bpf_work *w = container_of(cb, struct bpf_work, cb);
	bool retry = false;

	/*
	 * bpf_async_cancel_and_free() tried to cancel timer/wq, but it
	 * could have raced with timer/wq_start. Now refcnt is zero and
	 * srcu/rcu GP completed. Cancel timer/wq again.
	 */
	switch (cb->type) {
	case BPF_ASYNC_TYPE_TIMER:
		if (hrtimer_try_to_cancel(&t->timer) < 0)
			retry = true;
		break;
	case BPF_ASYNC_TYPE_WQ:
		if (!cancel_work(&w->work) && work_busy(&w->work))
			retry = true;
		break;
	}
	if (retry) {
		/*
		 * hrtimer or wq callback may still be running. It must be
		 * in rcu_tasks_trace or rcu CS, so wait for GP again.
		 * It won't retry forever, since refcnt zero prevents all
		 * operations on timer/wq.
		 */
		call_rcu_tasks_trace(&cb->rcu, bpf_async_cb_rcu_tasks_trace_free);
		return;
	}

	/* RCU Tasks Trace grace period implies RCU grace period. */
	bpf_async_cb_rcu_free(rcu);
}

static void worker_for_call_rcu(struct irq_work *work)
{
	struct bpf_async_cb *cb = container_of(work, struct bpf_async_cb, worker);

	call_rcu_tasks_trace(&cb->rcu, bpf_async_cb_rcu_tasks_trace_free);
}

static void bpf_async_refcount_put(struct bpf_async_cb *cb)
{
	if (!refcount_dec_and_test(&cb->refcnt))
		return;

	if (irqs_disabled()) {
		cb->worker = IRQ_WORK_INIT(worker_for_call_rcu);
		irq_work_queue(&cb->worker);
	} else {
		call_rcu_tasks_trace(&cb->rcu, bpf_async_cb_rcu_tasks_trace_free);
	}
}

static void bpf_async_cancel_and_free(struct bpf_async_kern *async);
static void bpf_async_irq_worker(struct irq_work *work);

static int __bpf_async_init(struct bpf_async_kern *async, struct bpf_map *map, u64 flags,
			    enum bpf_async_type type)
{
	struct bpf_async_cb *cb, *old_cb;
	struct bpf_hrtimer *t;
	struct bpf_work *w;
	clockid_t clockid;
	size_t size;

	switch (type) {
	case BPF_ASYNC_TYPE_TIMER:
		size = sizeof(struct bpf_hrtimer);
		break;
	case BPF_ASYNC_TYPE_WQ:
		size = sizeof(struct bpf_work);
		break;
	default:
		return -EINVAL;
	}

	old_cb = READ_ONCE(async->cb);
	if (old_cb)
		return -EBUSY;

	cb = bpf_map_kmalloc_nolock(map, size, 0, map->numa_node);
	if (!cb)
		return -ENOMEM;

	switch (type) {
	case BPF_ASYNC_TYPE_TIMER:
		clockid = flags & (MAX_CLOCKS - 1);
		t = (struct bpf_hrtimer *)cb;

		atomic_set(&t->cancelling, 0);
		hrtimer_setup(&t->timer, bpf_timer_cb, clockid, HRTIMER_MODE_REL_SOFT);
		cb->value = (void *)async - map->record->timer_off;
		break;
	case BPF_ASYNC_TYPE_WQ:
		w = (struct bpf_work *)cb;

		INIT_WORK(&w->work, bpf_wq_work);
		cb->value = (void *)async - map->record->wq_off;
		break;
	}
	cb->map = map;
	cb->prog = NULL;
	cb->flags = flags;
	cb->worker = IRQ_WORK_INIT(bpf_async_irq_worker);
	init_llist_head(&cb->async_cmds);
	refcount_set(&cb->refcnt, 1); /* map's reference */
	cb->type = type;
	rcu_assign_pointer(cb->callback_fn, NULL);

	old_cb = cmpxchg(&async->cb, NULL, cb);
	if (old_cb) {
		/* Lost the race to initialize this bpf_async_kern, drop the allocated object */
		kfree_nolock(cb);
		return -EBUSY;
	}
	/* Guarantee the order between async->cb and map->usercnt. So
	 * when there are concurrent uref release and bpf timer init, either
	 * bpf_timer_cancel_and_free() called by uref release reads a no-NULL
	 * timer or atomic64_read() below returns a zero usercnt.
	 */
	smp_mb();
	if (!atomic64_read(&map->usercnt)) {
		/* maps with timers must be either held by user space
		 * or pinned in bpffs.
		 */
		bpf_async_cancel_and_free(async);
		return -EPERM;
	}

	return 0;
}