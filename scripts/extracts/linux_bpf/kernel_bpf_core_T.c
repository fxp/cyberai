// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 20/20



	ret = bpf_mem_alloc_init(&bpf_global_ma, 0, false);
	bpf_global_ma_set = !ret;
	return ret;
}
late_initcall(bpf_global_ma_init);
#endif

DEFINE_STATIC_KEY_FALSE(bpf_stats_enabled_key);
EXPORT_SYMBOL(bpf_stats_enabled_key);

/* All definitions of tracepoints related to BPF. */
#define CREATE_TRACE_POINTS
#include <linux/bpf_trace.h>

EXPORT_TRACEPOINT_SYMBOL_GPL(xdp_exception);
EXPORT_TRACEPOINT_SYMBOL_GPL(xdp_bulk_tx);

#ifdef CONFIG_BPF_SYSCALL

void bpf_get_linfo_file_line(struct btf *btf, const struct bpf_line_info *linfo,
			     const char **filep, const char **linep, int *nump)
{
	/* Get base component of the file path. */
	if (filep) {
		*filep = btf_name_by_offset(btf, linfo->file_name_off);
		*filep = kbasename(*filep);
	}

	/* Obtain the source line, and strip whitespace in prefix. */
	if (linep) {
		*linep = btf_name_by_offset(btf, linfo->line_off);
		while (isspace(**linep))
			*linep += 1;
	}

	if (nump)
		*nump = BPF_LINE_INFO_LINE_NUM(linfo->line_col);
}

const struct bpf_line_info *bpf_find_linfo(const struct bpf_prog *prog, u32 insn_off)
{
	const struct bpf_line_info *linfo;
	u32 nr_linfo;
	int l, r, m;

	nr_linfo = prog->aux->nr_linfo;
	if (!nr_linfo || insn_off >= prog->len)
		return NULL;

	linfo = prog->aux->linfo;
	/* Loop invariant: linfo[l].insn_off <= insns_off.
	 * linfo[0].insn_off == 0 which always satisfies above condition.
	 * Binary search is searching for rightmost linfo entry that satisfies
	 * the above invariant, giving us the desired record that covers given
	 * instruction offset.
	 */
	l = 0;
	r = nr_linfo - 1;
	while (l < r) {
		/* (r - l + 1) / 2 means we break a tie to the right, so if:
		 * l=1, r=2, linfo[l].insn_off <= insn_off, linfo[r].insn_off > insn_off,
		 * then m=2, we see that linfo[m].insn_off > insn_off, and so
		 * r becomes 1 and we exit the loop with correct l==1.
		 * If the tie was broken to the left, m=1 would end us up in
		 * an endless loop where l and m stay at 1 and r stays at 2.
		 */
		m = l + (r - l + 1) / 2;
		if (linfo[m].insn_off <= insn_off)
			l = m;
		else
			r = m - 1;
	}

	return &linfo[l];
}

int bpf_prog_get_file_line(struct bpf_prog *prog, unsigned long ip, const char **filep,
			   const char **linep, int *nump)
{
	int idx = -1, insn_start, insn_end, len;
	struct bpf_line_info *linfo;
	void **jited_linfo;
	struct btf *btf;
	int nr_linfo;

	btf = prog->aux->btf;
	linfo = prog->aux->linfo;
	jited_linfo = prog->aux->jited_linfo;

	if (!btf || !linfo || !jited_linfo)
		return -EINVAL;
	len = prog->aux->func ? prog->aux->func[prog->aux->func_idx]->len : prog->len;

	linfo = &prog->aux->linfo[prog->aux->linfo_idx];
	jited_linfo = &prog->aux->jited_linfo[prog->aux->linfo_idx];

	insn_start = linfo[0].insn_off;
	insn_end = insn_start + len;
	nr_linfo = prog->aux->nr_linfo - prog->aux->linfo_idx;

	for (int i = 0; i < nr_linfo &&
	     linfo[i].insn_off >= insn_start && linfo[i].insn_off < insn_end; i++) {
		if (jited_linfo[i] >= (void *)ip)
			break;
		idx = i;
	}

	if (idx == -1)
		return -ENOENT;

	bpf_get_linfo_file_line(btf, &linfo[idx], filep, linep, nump);
	return 0;
}

struct walk_stack_ctx {
	struct bpf_prog *prog;
};

static bool find_from_stack_cb(void *cookie, u64 ip, u64 sp, u64 bp)
{
	struct walk_stack_ctx *ctxp = cookie;
	struct bpf_prog *prog;

	/*
	 * The RCU read lock is held to safely traverse the latch tree, but we
	 * don't need its protection when accessing the prog, since it has an
	 * active stack frame on the current stack trace, and won't disappear.
	 */
	rcu_read_lock();
	prog = bpf_prog_ksym_find(ip);
	rcu_read_unlock();
	if (!prog)
		return true;
	/* Make sure we return the main prog if we found a subprog */
	ctxp->prog = prog->aux->main_prog_aux->prog;
	return false;
}

struct bpf_prog *bpf_prog_find_from_stack(void)
{
	struct walk_stack_ctx ctx = {};

	arch_bpf_stack_walk(find_from_stack_cb, &ctx);
	return ctx.prog;
}

#endif
