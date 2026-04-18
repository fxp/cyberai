// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 9/30



	/* Only account in-flight cancellations when invoked from a timer
	 * callback, since we want to avoid waiting only if other _callbacks_
	 * are waiting on us, to avoid introducing lockups. Non-callback paths
	 * are ok, since nobody would synchronously wait for their completion.
	 */
	if (!cur_t)
		goto drop;
	atomic_inc(&t->cancelling);
	/* Need full barrier after relaxed atomic_inc */
	smp_mb__after_atomic();
	inc = true;
	if (atomic_read(&cur_t->cancelling)) {
		/* We're cancelling timer t, while some other timer callback is
		 * attempting to cancel us. In such a case, it might be possible
		 * that timer t belongs to the other callback, or some other
		 * callback waiting upon it (creating transitive dependencies
		 * upon us), and we will enter a deadlock if we continue
		 * cancelling and waiting for it synchronously, since it might
		 * do the same. Bail!
		 */
		atomic_dec(&t->cancelling);
		return -EDEADLK;
	}
drop:
	bpf_async_update_prog_callback(&t->cb, NULL, NULL);
	/* Cancel the timer and wait for associated callback to finish
	 * if it was running.
	 */
	ret = hrtimer_cancel(&t->timer);
	if (inc)
		atomic_dec(&t->cancelling);
	return ret;
}

static const struct bpf_func_proto bpf_timer_cancel_proto = {
	.func		= bpf_timer_cancel,
	.gpl_only	= true,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_TIMER,
};

static void bpf_async_process_op(struct bpf_async_cb *cb, u32 op,
				 u64 timer_nsec, u32 timer_mode)
{
	switch (cb->type) {
	case BPF_ASYNC_TYPE_TIMER: {
		struct bpf_hrtimer *t = container_of(cb, struct bpf_hrtimer, cb);

		switch (op) {
		case BPF_ASYNC_START:
			hrtimer_start(&t->timer, ns_to_ktime(timer_nsec), timer_mode);
			break;
		case BPF_ASYNC_CANCEL:
			hrtimer_try_to_cancel(&t->timer);
			break;
		}
		break;
	}
	case BPF_ASYNC_TYPE_WQ: {
		struct bpf_work *w = container_of(cb, struct bpf_work, cb);

		switch (op) {
		case BPF_ASYNC_START:
			schedule_work(&w->work);
			break;
		case BPF_ASYNC_CANCEL:
			cancel_work(&w->work);
			break;
		}
		break;
	}
	}
	bpf_async_refcount_put(cb);
}

static void bpf_async_irq_worker(struct irq_work *work)
{
	struct bpf_async_cb *cb = container_of(work, struct bpf_async_cb, worker);
	struct llist_node *pos, *n, *list;

	list = llist_del_all(&cb->async_cmds);
	if (!list)
		return;

	list = llist_reverse_order(list);
	this_cpu_write(async_cb_running, cb);
	llist_for_each_safe(pos, n, list) {
		struct bpf_async_cmd *cmd;

		cmd = container_of(pos, struct bpf_async_cmd, node);
		bpf_async_process_op(cb, cmd->op, cmd->nsec, cmd->mode);
		kfree_nolock(cmd);
	}
	this_cpu_write(async_cb_running, NULL);
}

static void bpf_async_cancel_and_free(struct bpf_async_kern *async)
{
	struct bpf_async_cb *cb;

	if (!READ_ONCE(async->cb))
		return;

	cb = xchg(&async->cb, NULL);
	if (!cb)
		return;

	bpf_async_update_prog_callback(cb, NULL, NULL);
	/*
	 * No refcount_inc_not_zero(&cb->refcnt) here. Dropping the last
	 * refcnt. Either synchronously or asynchronously in irq_work.
	 */

	if (!defer_timer_wq_op()) {
		bpf_async_process_op(cb, BPF_ASYNC_CANCEL, 0, 0);
	} else {
		(void)bpf_async_schedule_op(cb, BPF_ASYNC_CANCEL, 0, 0);
		/*
		 * bpf_async_schedule_op() either enqueues allocated cmd into llist
		 * or fails with ENOMEM and drop the last refcnt.
		 * This is unlikely, but safe, since bpf_async_cb_rcu_tasks_trace_free()
		 * callback will do additional timer/wq_cancel due to races anyway.
		 */
	}
}

/*
 * This function is called by map_delete/update_elem for individual element and
 * by ops->map_release_uref when the user space reference to a map reaches zero.
 */
void bpf_timer_cancel_and_free(void *val)
{
	bpf_async_cancel_and_free(val);
}

/*
 * This function is called by map_delete/update_elem for individual element and
 * by ops->map_release_uref when the user space reference to a map reaches zero.
 */
void bpf_wq_cancel_and_free(void *val)
{
	bpf_async_cancel_and_free(val);
}

BPF_CALL_2(bpf_kptr_xchg, void *, dst, void *, ptr)
{
	unsigned long *kptr = dst;

	/* This helper may be inlined by verifier. */
	return xchg(kptr, (unsigned long)ptr);
}

/* Unlike other PTR_TO_BTF_ID helpers the btf_id in bpf_kptr_xchg()
 * helper is determined dynamically by the verifier. Use BPF_PTR_POISON to
 * denote type that verifier will determine.
 */
static const struct bpf_func_proto bpf_kptr_xchg_proto = {
	.func         = bpf_kptr_xchg,
	.gpl_only     = false,
	.ret_type     = RET_PTR_TO_BTF_ID_OR_NULL,
	.ret_btf_id   = BPF_PTR_POISON,
	.arg1_type    = ARG_KPTR_XCHG_DEST,
	.arg2_type    = ARG_PTR_TO_BTF_ID_OR_NULL | OBJ_RELEASE,
	.arg2_btf_id  = BPF_PTR_POISON,
};

struct bpf_dynptr_file_impl {
	struct freader freader;
	/* 64 bit offset and size overriding 32 bit ones in bpf_dynptr_kern */
	u64 offset;
	u64 size;
};