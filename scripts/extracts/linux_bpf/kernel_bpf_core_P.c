// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 16/20



	/* eBPF JITs can rewrite the program in case constant
	 * blinding is active. However, in case of error during
	 * blinding, bpf_int_jit_compile() must always return a
	 * valid program, which in this case would simply not
	 * be JITed, but falls back to the interpreter.
	 */
	if (!bpf_prog_is_offloaded(fp->aux)) {
		*err = bpf_prog_alloc_jited_linfo(fp);
		if (*err)
			return fp;

		fp = bpf_prog_jit_compile(env, fp);
		bpf_prog_jit_attempt_done(fp);
		if (!fp->jited && jit_needed) {
			*err = -ENOTSUPP;
			return fp;
		}
	} else {
		*err = bpf_prog_offload_compile(fp);
		if (*err)
			return fp;
	}

finalize:
	*err = bpf_prog_lock_ro(fp);
	if (*err)
		return fp;

	/* The tail call compatibility check can only be done at
	 * this late stage as we need to determine, if we deal
	 * with JITed or non JITed program concatenations and not
	 * all eBPF JITs might immediately support all features.
	 */
	*err = bpf_check_tail_call(fp);

	return fp;
}

/**
 *	bpf_prog_select_runtime - select exec runtime for BPF program
 *	@fp: bpf_prog populated with BPF program
 *	@err: pointer to error variable
 *
 * Try to JIT eBPF program, if JIT is not available, use interpreter.
 * The BPF program will be executed via bpf_prog_run() function.
 *
 * Return: the &fp argument along with &err set to 0 for success or
 * a negative errno code on failure
 */
struct bpf_prog *bpf_prog_select_runtime(struct bpf_prog *fp, int *err)
{
	return __bpf_prog_select_runtime(NULL, fp, err);
}
EXPORT_SYMBOL_GPL(bpf_prog_select_runtime);

static unsigned int __bpf_prog_ret1(const void *ctx,
				    const struct bpf_insn *insn)
{
	return 1;
}

static struct bpf_prog_dummy {
	struct bpf_prog prog;
} dummy_bpf_prog = {
	.prog = {
		.bpf_func = __bpf_prog_ret1,
	},
};

struct bpf_prog_array bpf_empty_prog_array = {
	.items = {
		{ .prog = NULL },
	},
};
EXPORT_SYMBOL(bpf_empty_prog_array);

struct bpf_prog_array *bpf_prog_array_alloc(u32 prog_cnt, gfp_t flags)
{
	struct bpf_prog_array *p;

	if (prog_cnt)
		p = kzalloc_flex(*p, items, prog_cnt + 1, flags);
	else
		p = &bpf_empty_prog_array;

	return p;
}

void bpf_prog_array_free(struct bpf_prog_array *progs)
{
	if (!progs || progs == &bpf_empty_prog_array)
		return;
	kfree_rcu(progs, rcu);
}

static void __bpf_prog_array_free_sleepable_cb(struct rcu_head *rcu)
{
	struct bpf_prog_array *progs;

	/*
	 * RCU Tasks Trace grace period implies RCU grace period, there is no
	 * need to call kfree_rcu(), just call kfree() directly.
	 */
	progs = container_of(rcu, struct bpf_prog_array, rcu);
	kfree(progs);
}

void bpf_prog_array_free_sleepable(struct bpf_prog_array *progs)
{
	if (!progs || progs == &bpf_empty_prog_array)
		return;
	call_rcu_tasks_trace(&progs->rcu, __bpf_prog_array_free_sleepable_cb);
}

int bpf_prog_array_length(struct bpf_prog_array *array)
{
	struct bpf_prog_array_item *item;
	u32 cnt = 0;

	for (item = array->items; item->prog; item++)
		if (item->prog != &dummy_bpf_prog.prog)
			cnt++;
	return cnt;
}

bool bpf_prog_array_is_empty(struct bpf_prog_array *array)
{
	struct bpf_prog_array_item *item;

	for (item = array->items; item->prog; item++)
		if (item->prog != &dummy_bpf_prog.prog)
			return false;
	return true;
}

static bool bpf_prog_array_copy_core(struct bpf_prog_array *array,
				     u32 *prog_ids,
				     u32 request_cnt)
{
	struct bpf_prog_array_item *item;
	int i = 0;

	for (item = array->items; item->prog; item++) {
		if (item->prog == &dummy_bpf_prog.prog)
			continue;
		prog_ids[i] = item->prog->aux->id;
		if (++i == request_cnt) {
			item++;
			break;
		}
	}

	return !!(item->prog);
}

int bpf_prog_array_copy_to_user(struct bpf_prog_array *array,
				__u32 __user *prog_ids, u32 cnt)
{
	unsigned long err = 0;
	bool nospc;
	u32 *ids;

	/* users of this function are doing:
	 * cnt = bpf_prog_array_length();
	 * if (cnt > 0)
	 *     bpf_prog_array_copy_to_user(..., cnt);
	 * so below kcalloc doesn't need extra cnt > 0 check.
	 */
	ids = kcalloc(cnt, sizeof(u32), GFP_USER | __GFP_NOWARN);
	if (!ids)
		return -ENOMEM;
	nospc = bpf_prog_array_copy_core(array, ids, cnt);
	err = copy_to_user(prog_ids, ids, cnt * sizeof(u32));
	kfree(ids);
	if (err)
		return -EFAULT;
	if (nospc)
		return -ENOSPC;
	return 0;
}

void bpf_prog_array_delete_safe(struct bpf_prog_array *array,
				struct bpf_prog *old_prog)
{
	struct bpf_prog_array_item *item;

	for (item = array->items; item->prog; item++)
		if (item->prog == old_prog) {
			WRITE_ONCE(item->prog, &dummy_bpf_prog.prog);
			break;
		}
}