// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 26/30



	if (irqs_disabled()) {
		ctx->irq_work = IRQ_WORK_INIT(bpf_task_work_destroy);
		irq_work_queue(&ctx->irq_work);
	} else {
		bpf_task_work_destroy(&ctx->irq_work);
	}
}

static void bpf_task_work_cancel(struct bpf_task_work_ctx *ctx)
{
	/*
	 * Scheduled task_work callback holds ctx ref, so if we successfully
	 * cancelled, we put that ref on callback's behalf. If we couldn't
	 * cancel, callback will inevitably run or has already completed
	 * running, and it would have taken care of its ctx ref itself.
	 */
	if (task_work_cancel(ctx->task, &ctx->work))
		bpf_task_work_ctx_put(ctx);
}

static void bpf_task_work_callback(struct callback_head *cb)
{
	struct bpf_task_work_ctx *ctx = container_of(cb, struct bpf_task_work_ctx, work);
	enum bpf_task_work_state state;
	u32 idx;
	void *key;

	/* Read lock is needed to protect ctx and map key/value access */
	guard(rcu_tasks_trace)();
	/*
	 * This callback may start running before bpf_task_work_irq() switched to
	 * SCHEDULED state, so handle both transition variants SCHEDULING|SCHEDULED -> RUNNING.
	 */
	state = cmpxchg(&ctx->state, BPF_TW_SCHEDULING, BPF_TW_RUNNING);
	if (state == BPF_TW_SCHEDULED)
		state = cmpxchg(&ctx->state, BPF_TW_SCHEDULED, BPF_TW_RUNNING);
	if (state == BPF_TW_FREED) {
		bpf_task_work_ctx_put(ctx);
		return;
	}

	key = (void *)map_key_from_value(ctx->map, ctx->map_val, &idx);

	migrate_disable();
	ctx->callback_fn(ctx->map, key, ctx->map_val);
	migrate_enable();

	bpf_task_work_ctx_reset(ctx);
	(void)cmpxchg(&ctx->state, BPF_TW_RUNNING, BPF_TW_STANDBY);

	bpf_task_work_ctx_put(ctx);
}

static void bpf_task_work_irq(struct irq_work *irq_work)
{
	struct bpf_task_work_ctx *ctx = container_of(irq_work, struct bpf_task_work_ctx, irq_work);
	enum bpf_task_work_state state;
	int err;

	guard(rcu)();

	if (cmpxchg(&ctx->state, BPF_TW_PENDING, BPF_TW_SCHEDULING) != BPF_TW_PENDING) {
		bpf_task_work_ctx_put(ctx);
		return;
	}

	err = task_work_add(ctx->task, &ctx->work, ctx->mode);
	if (err) {
		bpf_task_work_ctx_reset(ctx);
		/*
		 * try to switch back to STANDBY for another task_work reuse, but we might have
		 * gone to FREED already, which is fine as we already cleaned up after ourselves
		 */
		(void)cmpxchg(&ctx->state, BPF_TW_SCHEDULING, BPF_TW_STANDBY);
		bpf_task_work_ctx_put(ctx);
		return;
	}

	/*
	 * It's technically possible for just scheduled task_work callback to
	 * complete running by now, going SCHEDULING -> RUNNING and then
	 * dropping its ctx refcount. Instead of capturing an extra ref just
	 * to protect below ctx->state access, we rely on rcu_read_lock
	 * above to prevent kfree_rcu from freeing ctx before we return.
	 */
	state = cmpxchg(&ctx->state, BPF_TW_SCHEDULING, BPF_TW_SCHEDULED);
	if (state == BPF_TW_FREED)
		bpf_task_work_cancel(ctx); /* clean up if we switched into FREED state */
}

static struct bpf_task_work_ctx *bpf_task_work_fetch_ctx(struct bpf_task_work *tw,
							 struct bpf_map *map)
{
	struct bpf_task_work_kern *twk = (void *)tw;
	struct bpf_task_work_ctx *ctx, *old_ctx;

	ctx = READ_ONCE(twk->ctx);
	if (ctx)
		return ctx;

	ctx = bpf_map_kmalloc_nolock(map, sizeof(*ctx), 0, NUMA_NO_NODE);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	memset(ctx, 0, sizeof(*ctx));
	refcount_set(&ctx->refcnt, 1); /* map's own ref */
	ctx->state = BPF_TW_STANDBY;

	old_ctx = cmpxchg(&twk->ctx, NULL, ctx);
	if (old_ctx) {
		/*
		 * tw->ctx is set by concurrent BPF program, release allocated
		 * memory and try to reuse already set context.
		 */
		kfree_nolock(ctx);
		return old_ctx;
	}

	return ctx; /* Success */
}

static struct bpf_task_work_ctx *bpf_task_work_acquire_ctx(struct bpf_task_work *tw,
							   struct bpf_map *map)
{
	struct bpf_task_work_ctx *ctx;

	/*
	 * Sleepable BPF programs hold rcu_read_lock_trace but not
	 * regular rcu_read_lock. Since kfree_rcu waits for regular
	 * RCU GP, the ctx can be freed while we're between reading
	 * the pointer and incrementing the refcount. Take regular
	 * rcu_read_lock to prevent kfree_rcu from freeing the ctx
	 * before we can tryget it.
	 */
	scoped_guard(rcu) {
		ctx = bpf_task_work_fetch_ctx(tw, map);
		if (IS_ERR(ctx))
			return ctx;

		/* try to get ref for task_work callback to hold */
		if (!bpf_task_work_ctx_tryget(ctx))
			return ERR_PTR(-EBUSY);
	}

	if (cmpxchg(&ctx->state, BPF_TW_STANDBY, BPF_TW_PENDING) != BPF_TW_STANDBY) {
		/* lost acquiring race or map_release_uref() stole it from us, put ref and bail */
		bpf_task_work_ctx_put(ctx);
		return ERR_PTR(-EBUSY);
	}

	/*
	 * If no process or bpffs is holding a reference to the map, no new callbacks should be
	 * scheduled. This does not address any race or correctness issue, but rather is a policy
	 * choice: dropping user references should stop everything.
	 */
	if (!atomic64_read(&map->usercnt)) {
		/* drop ref we just got for task_work callback itself */
		bpf_task_work_ctx_put(ctx);
		/* transfer map's ref into cancel_and_free() */
		bpf_task_work_cancel_and_free(tw);
		return ERR_PTR(-EBUSY);
	}