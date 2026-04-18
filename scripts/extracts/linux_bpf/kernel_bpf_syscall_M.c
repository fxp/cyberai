// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 13/37



static const struct bpf_prog_ops * const bpf_prog_types[] = {
#define BPF_PROG_TYPE(_id, _name, prog_ctx_type, kern_ctx_type) \
	[_id] = & _name ## _prog_ops,
#define BPF_MAP_TYPE(_id, _ops)
#define BPF_LINK_TYPE(_id, _name)
#include <linux/bpf_types.h>
#undef BPF_PROG_TYPE
#undef BPF_MAP_TYPE
#undef BPF_LINK_TYPE
};

static int find_prog_type(enum bpf_prog_type type, struct bpf_prog *prog)
{
	const struct bpf_prog_ops *ops;

	if (type >= ARRAY_SIZE(bpf_prog_types))
		return -EINVAL;
	type = array_index_nospec(type, ARRAY_SIZE(bpf_prog_types));
	ops = bpf_prog_types[type];
	if (!ops)
		return -EINVAL;

	if (!bpf_prog_is_offloaded(prog->aux))
		prog->aux->ops = ops;
	else
		prog->aux->ops = &bpf_offload_prog_ops;
	prog->type = type;
	return 0;
}

enum bpf_audit {
	BPF_AUDIT_LOAD,
	BPF_AUDIT_UNLOAD,
	BPF_AUDIT_MAX,
};

static const char * const bpf_audit_str[BPF_AUDIT_MAX] = {
	[BPF_AUDIT_LOAD]   = "LOAD",
	[BPF_AUDIT_UNLOAD] = "UNLOAD",
};

static void bpf_audit_prog(const struct bpf_prog *prog, unsigned int op)
{
	struct audit_context *ctx = NULL;
	struct audit_buffer *ab;

	if (WARN_ON_ONCE(op >= BPF_AUDIT_MAX))
		return;
	if (audit_enabled == AUDIT_OFF)
		return;
	if (!in_hardirq() && !irqs_disabled())
		ctx = audit_context();
	ab = audit_log_start(ctx, GFP_ATOMIC, AUDIT_BPF);
	if (unlikely(!ab))
		return;
	audit_log_format(ab, "prog-id=%u op=%s",
			 prog->aux->id, bpf_audit_str[op]);
	audit_log_end(ab);
}

static int bpf_prog_alloc_id(struct bpf_prog *prog)
{
	int id;

	idr_preload(GFP_KERNEL);
	spin_lock_bh(&prog_idr_lock);
	id = idr_alloc_cyclic(&prog_idr, prog, 1, INT_MAX, GFP_ATOMIC);
	if (id > 0)
		prog->aux->id = id;
	spin_unlock_bh(&prog_idr_lock);
	idr_preload_end();

	/* id is in [1, INT_MAX) */
	if (WARN_ON_ONCE(!id))
		return -ENOSPC;

	return id > 0 ? 0 : id;
}

void bpf_prog_free_id(struct bpf_prog *prog)
{
	unsigned long flags;

	/* cBPF to eBPF migrations are currently not in the idr store.
	 * Offloaded programs are removed from the store when their device
	 * disappears - even if someone grabs an fd to them they are unusable,
	 * simply waiting for refcnt to drop to be freed.
	 */
	if (!prog->aux->id)
		return;

	spin_lock_irqsave(&prog_idr_lock, flags);
	idr_remove(&prog_idr, prog->aux->id);
	prog->aux->id = 0;
	spin_unlock_irqrestore(&prog_idr_lock, flags);
}

static void __bpf_prog_put_rcu(struct rcu_head *rcu)
{
	struct bpf_prog_aux *aux = container_of(rcu, struct bpf_prog_aux, rcu);

	kvfree(aux->func_info);
	kfree(aux->func_info_aux);
	free_uid(aux->user);
	security_bpf_prog_free(aux->prog);
	bpf_prog_free(aux->prog);
}

static void __bpf_prog_put_noref(struct bpf_prog *prog, bool deferred)
{
	bpf_prog_kallsyms_del_all(prog);
	btf_put(prog->aux->btf);
	module_put(prog->aux->mod);
	kvfree(prog->aux->jited_linfo);
	kvfree(prog->aux->linfo);
	kfree(prog->aux->kfunc_tab);
	kfree(prog->aux->ctx_arg_info);
	if (prog->aux->attach_btf)
		btf_put(prog->aux->attach_btf);

	if (deferred) {
		if (prog->sleepable)
			call_rcu_tasks_trace(&prog->aux->rcu, __bpf_prog_put_rcu);
		else
			call_rcu(&prog->aux->rcu, __bpf_prog_put_rcu);
	} else {
		__bpf_prog_put_rcu(&prog->aux->rcu);
	}
}

static void bpf_prog_put_deferred(struct work_struct *work)
{
	struct bpf_prog_aux *aux;
	struct bpf_prog *prog;

	aux = container_of(work, struct bpf_prog_aux, work);
	prog = aux->prog;
	perf_event_bpf_event(prog, PERF_BPF_EVENT_PROG_UNLOAD, 0);
	bpf_audit_prog(prog, BPF_AUDIT_UNLOAD);
	bpf_prog_free_id(prog);
	__bpf_prog_put_noref(prog, true);
}

static void __bpf_prog_put(struct bpf_prog *prog)
{
	struct bpf_prog_aux *aux = prog->aux;

	if (atomic64_dec_and_test(&aux->refcnt)) {
		if (in_hardirq() || irqs_disabled()) {
			INIT_WORK(&aux->work, bpf_prog_put_deferred);
			schedule_work(&aux->work);
		} else {
			bpf_prog_put_deferred(&aux->work);
		}
	}
}

void bpf_prog_put(struct bpf_prog *prog)
{
	__bpf_prog_put(prog);
}
EXPORT_SYMBOL_GPL(bpf_prog_put);

static int bpf_prog_release(struct inode *inode, struct file *filp)
{
	struct bpf_prog *prog = filp->private_data;

	bpf_prog_put(prog);
	return 0;
}

struct bpf_prog_kstats {
	u64 nsecs;
	u64 cnt;
	u64 misses;
};

void notrace bpf_prog_inc_misses_counter(struct bpf_prog *prog)
{
	struct bpf_prog_stats *stats;
	unsigned int flags;

	if (unlikely(!prog->stats))
		return;

	stats = this_cpu_ptr(prog->stats);
	flags = u64_stats_update_begin_irqsave(&stats->syncp);
	u64_stats_inc(&stats->misses);
	u64_stats_update_end_irqrestore(&stats->syncp, flags);
}

static void bpf_prog_get_stats(const struct bpf_prog *prog,
			       struct bpf_prog_kstats *stats)
{
	u64 nsecs = 0, cnt = 0, misses = 0;
	int cpu;

	for_each_possible_cpu(cpu) {
		const struct bpf_prog_stats *st;
		unsigned int start;
		u64 tnsecs, tcnt, tmisses;