// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 27/30



	return ctx;
}

static int bpf_task_work_schedule(struct task_struct *task, struct bpf_task_work *tw,
				  struct bpf_map *map, bpf_task_work_callback_t callback_fn,
				  struct bpf_prog_aux *aux, enum task_work_notify_mode mode)
{
	struct bpf_prog *prog;
	struct bpf_task_work_ctx *ctx;
	int err;

	BTF_TYPE_EMIT(struct bpf_task_work);

	prog = bpf_prog_inc_not_zero(aux->prog);
	if (IS_ERR(prog))
		return -EBADF;
	task = bpf_task_acquire(task);
	if (!task) {
		err = -EBADF;
		goto release_prog;
	}

	ctx = bpf_task_work_acquire_ctx(tw, map);
	if (IS_ERR(ctx)) {
		err = PTR_ERR(ctx);
		goto release_all;
	}

	ctx->task = task;
	ctx->callback_fn = callback_fn;
	ctx->prog = prog;
	ctx->mode = mode;
	ctx->map = map;
	ctx->map_val = (void *)tw - map->record->task_work_off;
	init_task_work(&ctx->work, bpf_task_work_callback);
	init_irq_work(&ctx->irq_work, bpf_task_work_irq);

	irq_work_queue(&ctx->irq_work);
	return 0;

release_all:
	bpf_task_release(task);
release_prog:
	bpf_prog_put(prog);
	return err;
}

/**
 * bpf_task_work_schedule_signal - Schedule BPF callback using task_work_add with TWA_SIGNAL
 * mode
 * @task: Task struct for which callback should be scheduled
 * @tw: Pointer to struct bpf_task_work in BPF map value for internal bookkeeping
 * @map__map: bpf_map that embeds struct bpf_task_work in the values
 * @callback: pointer to BPF subprogram to call
 * @aux: pointer to bpf_prog_aux of the caller BPF program, implicitly set by the verifier
 *
 * Return: 0 if task work has been scheduled successfully, negative error code otherwise
 */
__bpf_kfunc int bpf_task_work_schedule_signal(struct task_struct *task, struct bpf_task_work *tw,
					      void *map__map, bpf_task_work_callback_t callback,
					      struct bpf_prog_aux *aux)
{
	return bpf_task_work_schedule(task, tw, map__map, callback, aux, TWA_SIGNAL);
}

/**
 * bpf_task_work_schedule_resume - Schedule BPF callback using task_work_add with TWA_RESUME
 * mode
 * @task: Task struct for which callback should be scheduled
 * @tw: Pointer to struct bpf_task_work in BPF map value for internal bookkeeping
 * @map__map: bpf_map that embeds struct bpf_task_work in the values
 * @callback: pointer to BPF subprogram to call
 * @aux: pointer to bpf_prog_aux of the caller BPF program, implicitly set by the verifier
 *
 * Return: 0 if task work has been scheduled successfully, negative error code otherwise
 */
__bpf_kfunc int bpf_task_work_schedule_resume(struct task_struct *task, struct bpf_task_work *tw,
					      void *map__map, bpf_task_work_callback_t callback,
					      struct bpf_prog_aux *aux)
{
	return bpf_task_work_schedule(task, tw, map__map, callback, aux, TWA_RESUME);
}

static int make_file_dynptr(struct file *file, u32 flags, bool may_sleep,
			    struct bpf_dynptr_kern *ptr)
{
	struct bpf_dynptr_file_impl *state;

	/* flags is currently unsupported */
	if (flags) {
		bpf_dynptr_set_null(ptr);
		return -EINVAL;
	}

	state = kmalloc_nolock(sizeof(*state), 0, NUMA_NO_NODE);
	if (!state) {
		bpf_dynptr_set_null(ptr);
		return -ENOMEM;
	}
	state->offset = 0;
	state->size = U64_MAX; /* Don't restrict size, as file may change anyways */
	freader_init_from_file(&state->freader, NULL, 0, file, may_sleep);
	bpf_dynptr_init(ptr, state, BPF_DYNPTR_TYPE_FILE, 0, 0);
	bpf_dynptr_set_rdonly(ptr);
	return 0;
}

__bpf_kfunc int bpf_dynptr_from_file(struct file *file, u32 flags, struct bpf_dynptr *ptr__uninit)
{
	return make_file_dynptr(file, flags, false, (struct bpf_dynptr_kern *)ptr__uninit);
}

int bpf_dynptr_from_file_sleepable(struct file *file, u32 flags, struct bpf_dynptr *ptr__uninit)
{
	return make_file_dynptr(file, flags, true, (struct bpf_dynptr_kern *)ptr__uninit);
}

__bpf_kfunc int bpf_dynptr_file_discard(struct bpf_dynptr *dynptr)
{
	struct bpf_dynptr_kern *ptr = (struct bpf_dynptr_kern *)dynptr;
	struct bpf_dynptr_file_impl *df = ptr->data;

	if (!df)
		return 0;

	freader_cleanup(&df->freader);
	kfree_nolock(df);
	bpf_dynptr_set_null(ptr);
	return 0;
}

/**
 * bpf_timer_cancel_async - try to deactivate a timer
 * @timer:	bpf_timer to stop
 *
 * Returns:
 *
 *  *  0 when the timer was not active
 *  *  1 when the timer was active
 *  * -1 when the timer is currently executing the callback function and
 *       cannot be stopped
 *  * -ECANCELED when the timer will be cancelled asynchronously
 *  * -ENOMEM when out of memory
 *  * -EINVAL when the timer was not initialized
 *  * -ENOENT when this kfunc is racing with timer deletion
 */
__bpf_kfunc int bpf_timer_cancel_async(struct bpf_timer *timer)
{
	struct bpf_async_kern *async = (void *)timer;
	struct bpf_async_cb *cb;
	int ret;

	cb = READ_ONCE(async->cb);
	if (!cb)
		return -EINVAL;