// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 18/37



void bpf_link_init(struct bpf_link *link, enum bpf_link_type type,
		   const struct bpf_link_ops *ops, struct bpf_prog *prog,
		   enum bpf_attach_type attach_type)
{
	bpf_link_init_sleepable(link, type, ops, prog, attach_type, false);
}

static void bpf_link_free_id(int id)
{
	if (!id)
		return;

	spin_lock_bh(&link_idr_lock);
	idr_remove(&link_idr, id);
	spin_unlock_bh(&link_idr_lock);
}

/* Clean up bpf_link and corresponding anon_inode file and FD. After
 * anon_inode is created, bpf_link can't be just kfree()'d due to deferred
 * anon_inode's release() call. This helper marks bpf_link as
 * defunct, releases anon_inode file and puts reserved FD. bpf_prog's refcnt
 * is not decremented, it's the responsibility of a calling code that failed
 * to complete bpf_link initialization.
 * This helper eventually calls link's dealloc callback, but does not call
 * link's release callback.
 */
void bpf_link_cleanup(struct bpf_link_primer *primer)
{
	primer->link->prog = NULL;
	bpf_link_free_id(primer->id);
	fput(primer->file);
	put_unused_fd(primer->fd);
}

void bpf_link_inc(struct bpf_link *link)
{
	atomic64_inc(&link->refcnt);
}

static void bpf_link_dealloc(struct bpf_link *link)
{
	/* now that we know that bpf_link itself can't be reached, put underlying BPF program */
	if (link->prog)
		bpf_prog_put(link->prog);

	/* free bpf_link and its containing memory */
	if (link->ops->dealloc_deferred)
		link->ops->dealloc_deferred(link);
	else
		link->ops->dealloc(link);
}

static void bpf_link_defer_dealloc_rcu_gp(struct rcu_head *rcu)
{
	struct bpf_link *link = container_of(rcu, struct bpf_link, rcu);

	bpf_link_dealloc(link);
}

static bool bpf_link_is_tracepoint(struct bpf_link *link)
{
	/*
	 * Only these combinations support a tracepoint bpf_link.
	 * BPF_LINK_TYPE_TRACING raw_tp progs are hardcoded to use
	 * bpf_raw_tp_link_lops and thus dealloc_deferred(), see
	 * bpf_raw_tp_link_attach().
	 */
	return link->type == BPF_LINK_TYPE_RAW_TRACEPOINT ||
	       (link->type == BPF_LINK_TYPE_TRACING && link->attach_type == BPF_TRACE_RAW_TP);
}

/* bpf_link_free is guaranteed to be called from process context */
static void bpf_link_free(struct bpf_link *link)
{
	const struct bpf_link_ops *ops = link->ops;

	bpf_link_free_id(link->id);
	/* detach BPF program, clean up used resources */
	if (link->prog)
		ops->release(link);
	if (ops->dealloc_deferred) {
		/*
		 * Schedule BPF link deallocation, which will only then
		 * trigger putting BPF program refcount.
		 * If underlying BPF program is sleepable or BPF link's target
		 * attach hookpoint is sleepable or otherwise requires RCU GPs
		 * to ensure link and its underlying BPF program is not
		 * reachable anymore, we need to first wait for RCU tasks
		 * trace sync, and then go through "classic" RCU grace period.
		 *
		 * For tracepoint BPF links, we need to go through SRCU grace
		 * period wait instead when non-faultable tracepoint is used. We
		 * don't need to chain SRCU grace period waits, however, for the
		 * faultable case, since it exclusively uses RCU Tasks Trace.
		 */
		if (link->sleepable || (link->prog && link->prog->sleepable))
			/* RCU Tasks Trace grace period implies RCU grace period. */
			call_rcu_tasks_trace(&link->rcu, bpf_link_defer_dealloc_rcu_gp);
		/* We need to do a SRCU grace period wait for non-faultable tracepoint BPF links. */
		else if (bpf_link_is_tracepoint(link))
			call_tracepoint_unregister_atomic(&link->rcu, bpf_link_defer_dealloc_rcu_gp);
		else
			call_rcu(&link->rcu, bpf_link_defer_dealloc_rcu_gp);
	} else if (ops->dealloc) {
		bpf_link_dealloc(link);
	}
}

static void bpf_link_put_deferred(struct work_struct *work)
{
	struct bpf_link *link = container_of(work, struct bpf_link, work);

	bpf_link_free(link);
}

/* bpf_link_put might be called from atomic context. It needs to be called
 * from sleepable context in order to acquire sleeping locks during the process.
 */
void bpf_link_put(struct bpf_link *link)
{
	if (!atomic64_dec_and_test(&link->refcnt))
		return;

	INIT_WORK(&link->work, bpf_link_put_deferred);
	schedule_work(&link->work);
}
EXPORT_SYMBOL(bpf_link_put);

static void bpf_link_put_direct(struct bpf_link *link)
{
	if (!atomic64_dec_and_test(&link->refcnt))
		return;
	bpf_link_free(link);
}

static int bpf_link_release(struct inode *inode, struct file *filp)
{
	struct bpf_link *link = filp->private_data;

	bpf_link_put_direct(link);
	return 0;
}

#ifdef CONFIG_PROC_FS
#define BPF_PROG_TYPE(_id, _name, prog_ctx_type, kern_ctx_type)
#define BPF_MAP_TYPE(_id, _ops)
#define BPF_LINK_TYPE(_id, _name) [_id] = #_name,
static const char *bpf_link_type_strs[] = {
	[BPF_LINK_TYPE_UNSPEC] = "<invalid>",
#include <linux/bpf_types.h>
};
#undef BPF_PROG_TYPE
#undef BPF_MAP_TYPE
#undef BPF_LINK_TYPE