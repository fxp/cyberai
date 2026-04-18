// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 8/30



BPF_CALL_3(bpf_timer_init, struct bpf_async_kern *, timer, struct bpf_map *, map,
	   u64, flags)
{
	clock_t clockid = flags & (MAX_CLOCKS - 1);

	BUILD_BUG_ON(MAX_CLOCKS != 16);
	BUILD_BUG_ON(sizeof(struct bpf_async_kern) > sizeof(struct bpf_timer));
	BUILD_BUG_ON(__alignof__(struct bpf_async_kern) != __alignof__(struct bpf_timer));

	if (flags >= MAX_CLOCKS ||
	    /* similar to timerfd except _ALARM variants are not supported */
	    (clockid != CLOCK_MONOTONIC &&
	     clockid != CLOCK_REALTIME &&
	     clockid != CLOCK_BOOTTIME))
		return -EINVAL;

	return __bpf_async_init(timer, map, flags, BPF_ASYNC_TYPE_TIMER);
}

static const struct bpf_func_proto bpf_timer_init_proto = {
	.func		= bpf_timer_init,
	.gpl_only	= true,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_TIMER,
	.arg2_type	= ARG_CONST_MAP_PTR,
	.arg3_type	= ARG_ANYTHING,
};

static int bpf_async_update_prog_callback(struct bpf_async_cb *cb,
					  struct bpf_prog *prog,
					  void *callback_fn)
{
	struct bpf_prog *prev;

	/* Acquire a guard reference on prog to prevent it from being freed during the loop */
	if (prog) {
		prog = bpf_prog_inc_not_zero(prog);
		if (IS_ERR(prog))
			return PTR_ERR(prog);
	}

	do {
		if (prog)
			prog = bpf_prog_inc_not_zero(prog);
		prev = xchg(&cb->prog, prog);
		rcu_assign_pointer(cb->callback_fn, callback_fn);

		/*
		 * Release previous prog, make sure that if other CPU is contending,
		 * to set bpf_prog, references are not leaked as each iteration acquires and
		 * releases one reference.
		 */
		if (prev)
			bpf_prog_put(prev);

	} while (READ_ONCE(cb->prog) != prog ||
		 (void __force *)READ_ONCE(cb->callback_fn) != callback_fn);

	if (prog)
		bpf_prog_put(prog);

	return 0;
}

static DEFINE_PER_CPU(struct bpf_async_cb *, async_cb_running);

static int bpf_async_schedule_op(struct bpf_async_cb *cb, enum bpf_async_op op,
				 u64 nsec, u32 timer_mode)
{
	/*
	 * Do not schedule another operation on this cpu if it's in irq_work
	 * callback that is processing async_cmds queue. Otherwise the following
	 * loop is possible:
	 * bpf_timer_start() -> bpf_async_schedule_op() -> irq_work_queue().
	 * irqrestore -> bpf_async_irq_worker() -> tracepoint -> bpf_timer_start().
	 */
	if (this_cpu_read(async_cb_running) == cb) {
		bpf_async_refcount_put(cb);
		return -EDEADLK;
	}

	struct bpf_async_cmd *cmd = kmalloc_nolock(sizeof(*cmd), 0, NUMA_NO_NODE);

	if (!cmd) {
		bpf_async_refcount_put(cb);
		return -ENOMEM;
	}
	init_llist_node(&cmd->node);
	cmd->nsec = nsec;
	cmd->mode = timer_mode;
	cmd->op = op;
	if (llist_add(&cmd->node, &cb->async_cmds))
		irq_work_queue(&cb->worker);
	return 0;
}

static int __bpf_async_set_callback(struct bpf_async_kern *async, void *callback_fn,
				    struct bpf_prog *prog)
{
	struct bpf_async_cb *cb;

	cb = READ_ONCE(async->cb);
	if (!cb)
		return -EINVAL;

	return bpf_async_update_prog_callback(cb, prog, callback_fn);
}

BPF_CALL_3(bpf_timer_set_callback, struct bpf_async_kern *, timer, void *, callback_fn,
	   struct bpf_prog_aux *, aux)
{
	return __bpf_async_set_callback(timer, callback_fn, aux->prog);
}

static const struct bpf_func_proto bpf_timer_set_callback_proto = {
	.func		= bpf_timer_set_callback,
	.gpl_only	= true,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_TIMER,
	.arg2_type	= ARG_PTR_TO_FUNC,
};

static bool defer_timer_wq_op(void)
{
	return in_hardirq() || irqs_disabled();
}

BPF_CALL_3(bpf_timer_start, struct bpf_async_kern *, async, u64, nsecs, u64, flags)
{
	struct bpf_hrtimer *t;
	u32 mode;

	if (flags & ~(BPF_F_TIMER_ABS | BPF_F_TIMER_CPU_PIN))
		return -EINVAL;

	t = READ_ONCE(async->timer);
	if (!t || !READ_ONCE(t->cb.prog))
		return -EINVAL;

	if (flags & BPF_F_TIMER_ABS)
		mode = HRTIMER_MODE_ABS_SOFT;
	else
		mode = HRTIMER_MODE_REL_SOFT;

	if (flags & BPF_F_TIMER_CPU_PIN)
		mode |= HRTIMER_MODE_PINNED;

	/*
	 * bpf_async_cancel_and_free() could have dropped refcnt to zero. In
	 * such case BPF progs are not allowed to arm the timer to prevent UAF.
	 */
	if (!refcount_inc_not_zero(&t->cb.refcnt))
		return -ENOENT;

	if (!defer_timer_wq_op()) {
		hrtimer_start(&t->timer, ns_to_ktime(nsecs), mode);
		bpf_async_refcount_put(&t->cb);
		return 0;
	} else {
		return bpf_async_schedule_op(&t->cb, BPF_ASYNC_START, nsecs, mode);
	}
}

static const struct bpf_func_proto bpf_timer_start_proto = {
	.func		= bpf_timer_start,
	.gpl_only	= true,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_TIMER,
	.arg2_type	= ARG_ANYTHING,
	.arg3_type	= ARG_ANYTHING,
};

BPF_CALL_1(bpf_timer_cancel, struct bpf_async_kern *, async)
{
	struct bpf_hrtimer *t, *cur_t;
	bool inc = false;
	int ret = 0;

	if (defer_timer_wq_op())
		return -EOPNOTSUPP;

	t = READ_ONCE(async->timer);
	if (!t)
		return -EINVAL;

	cur_t = this_cpu_read(hrtimer_running);
	if (cur_t == t) {
		/* If bpf callback_fn is trying to bpf_timer_cancel()
		 * its own timer the hrtimer_cancel() will deadlock
		 * since it waits for callback_fn to finish.
		 */
		return -EDEADLK;
	}