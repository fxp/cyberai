// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 14/37



		st = per_cpu_ptr(prog->stats, cpu);
		do {
			start = u64_stats_fetch_begin(&st->syncp);
			tnsecs = u64_stats_read(&st->nsecs);
			tcnt = u64_stats_read(&st->cnt);
			tmisses = u64_stats_read(&st->misses);
		} while (u64_stats_fetch_retry(&st->syncp, start));
		nsecs += tnsecs;
		cnt += tcnt;
		misses += tmisses;
	}
	stats->nsecs = nsecs;
	stats->cnt = cnt;
	stats->misses = misses;
}

#ifdef CONFIG_PROC_FS
static void bpf_prog_show_fdinfo(struct seq_file *m, struct file *filp)
{
	const struct bpf_prog *prog = filp->private_data;
	char prog_tag[sizeof(prog->tag) * 2 + 1] = { };
	struct bpf_prog_kstats stats;

	bpf_prog_get_stats(prog, &stats);
	bin2hex(prog_tag, prog->tag, sizeof(prog->tag));
	seq_printf(m,
		   "prog_type:\t%u\n"
		   "prog_jited:\t%u\n"
		   "prog_tag:\t%s\n"
		   "memlock:\t%llu\n"
		   "prog_id:\t%u\n"
		   "run_time_ns:\t%llu\n"
		   "run_cnt:\t%llu\n"
		   "recursion_misses:\t%llu\n"
		   "verified_insns:\t%u\n",
		   prog->type,
		   prog->jited,
		   prog_tag,
		   prog->pages * 1ULL << PAGE_SHIFT,
		   prog->aux->id,
		   stats.nsecs,
		   stats.cnt,
		   stats.misses,
		   prog->aux->verified_insns);
}
#endif

const struct file_operations bpf_prog_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= bpf_prog_show_fdinfo,
#endif
	.release	= bpf_prog_release,
	.read		= bpf_dummy_read,
	.write		= bpf_dummy_write,
};

int bpf_prog_new_fd(struct bpf_prog *prog)
{
	int ret;

	ret = security_bpf_prog(prog);
	if (ret < 0)
		return ret;

	return anon_inode_getfd("bpf-prog", &bpf_prog_fops, prog,
				O_RDWR | O_CLOEXEC);
}

void bpf_prog_add(struct bpf_prog *prog, int i)
{
	atomic64_add(i, &prog->aux->refcnt);
}
EXPORT_SYMBOL_GPL(bpf_prog_add);

void bpf_prog_sub(struct bpf_prog *prog, int i)
{
	/* Only to be used for undoing previous bpf_prog_add() in some
	 * error path. We still know that another entity in our call
	 * path holds a reference to the program, thus atomic_sub() can
	 * be safely used in such cases!
	 */
	WARN_ON(atomic64_sub_return(i, &prog->aux->refcnt) == 0);
}
EXPORT_SYMBOL_GPL(bpf_prog_sub);

void bpf_prog_inc(struct bpf_prog *prog)
{
	atomic64_inc(&prog->aux->refcnt);
}
EXPORT_SYMBOL_GPL(bpf_prog_inc);

/* prog_idr_lock should have been held */
struct bpf_prog *bpf_prog_inc_not_zero(struct bpf_prog *prog)
{
	int refold;

	refold = atomic64_fetch_add_unless(&prog->aux->refcnt, 1, 0);

	if (!refold)
		return ERR_PTR(-ENOENT);

	return prog;
}
EXPORT_SYMBOL_GPL(bpf_prog_inc_not_zero);

bool bpf_prog_get_ok(struct bpf_prog *prog,
			    enum bpf_prog_type *attach_type, bool attach_drv)
{
	/* not an attachment, just a refcount inc, always allow */
	if (!attach_type)
		return true;

	if (prog->type != *attach_type)
		return false;
	if (bpf_prog_is_offloaded(prog->aux) && !attach_drv)
		return false;

	return true;
}

static struct bpf_prog *__bpf_prog_get(u32 ufd, enum bpf_prog_type *attach_type,
				       bool attach_drv)
{
	CLASS(fd, f)(ufd);
	struct bpf_prog *prog;

	if (fd_empty(f))
		return ERR_PTR(-EBADF);
	if (fd_file(f)->f_op != &bpf_prog_fops)
		return ERR_PTR(-EINVAL);

	prog = fd_file(f)->private_data;
	if (!bpf_prog_get_ok(prog, attach_type, attach_drv))
		return ERR_PTR(-EINVAL);

	bpf_prog_inc(prog);
	return prog;
}

struct bpf_prog *bpf_prog_get(u32 ufd)
{
	return __bpf_prog_get(ufd, NULL, false);
}

struct bpf_prog *bpf_prog_get_type_dev(u32 ufd, enum bpf_prog_type type,
				       bool attach_drv)
{
	return __bpf_prog_get(ufd, &type, attach_drv);
}
EXPORT_SYMBOL_GPL(bpf_prog_get_type_dev);

/* Initially all BPF programs could be loaded w/o specifying
 * expected_attach_type. Later for some of them specifying expected_attach_type
 * at load time became required so that program could be validated properly.
 * Programs of types that are allowed to be loaded both w/ and w/o (for
 * backward compatibility) expected_attach_type, should have the default attach
 * type assigned to expected_attach_type for the latter case, so that it can be
 * validated later at attach time.
 *
 * bpf_prog_load_fixup_attach_type() sets expected_attach_type in @attr if
 * prog type requires it but has some attach types that have to be backward
 * compatible.
 */
static void bpf_prog_load_fixup_attach_type(union bpf_attr *attr)
{
	switch (attr->prog_type) {
	case BPF_PROG_TYPE_CGROUP_SOCK:
		/* Unfortunately BPF_ATTACH_TYPE_UNSPEC enumeration doesn't
		 * exist so checking for non-zero is the way to go here.
		 */
		if (!attr->expected_attach_type)
			attr->expected_attach_type =
				BPF_CGROUP_INET_SOCK_CREATE;
		break;
	case BPF_PROG_TYPE_SK_REUSEPORT:
		if (!attr->expected_attach_type)
			attr->expected_attach_type =
				BPF_SK_REUSEPORT_SELECT;
		break;
	}
}

static int
bpf_prog_load_check_attach(enum bpf_prog_type prog_type,
			   enum bpf_attach_type expected_attach_type,
			   struct btf *attach_btf, u32 btf_id,
			   struct bpf_prog *dst_prog)
{
	if (btf_id) {
		if (btf_id > BTF_MAX_TYPE)
			return -EINVAL;

		if (!attach_btf && !dst_prog)
			return -EINVAL;