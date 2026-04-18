// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 19/37



static void bpf_link_show_fdinfo(struct seq_file *m, struct file *filp)
{
	const struct bpf_link *link = filp->private_data;
	const struct bpf_prog *prog = link->prog;
	enum bpf_link_type type = link->type;
	char prog_tag[sizeof(prog->tag) * 2 + 1] = { };

	if (type < ARRAY_SIZE(bpf_link_type_strs) && bpf_link_type_strs[type]) {
		if (link->type == BPF_LINK_TYPE_KPROBE_MULTI)
			seq_printf(m, "link_type:\t%s\n", link->flags == BPF_F_KPROBE_MULTI_RETURN ?
				   "kretprobe_multi" : "kprobe_multi");
		else if (link->type == BPF_LINK_TYPE_UPROBE_MULTI)
			seq_printf(m, "link_type:\t%s\n", link->flags == BPF_F_UPROBE_MULTI_RETURN ?
				   "uretprobe_multi" : "uprobe_multi");
		else
			seq_printf(m, "link_type:\t%s\n", bpf_link_type_strs[type]);
	} else {
		WARN_ONCE(1, "missing BPF_LINK_TYPE(...) for link type %u\n", type);
		seq_printf(m, "link_type:\t<%u>\n", type);
	}
	seq_printf(m, "link_id:\t%u\n", link->id);

	if (prog) {
		bin2hex(prog_tag, prog->tag, sizeof(prog->tag));
		seq_printf(m,
			   "prog_tag:\t%s\n"
			   "prog_id:\t%u\n",
			   prog_tag,
			   prog->aux->id);
	}
	if (link->ops->show_fdinfo)
		link->ops->show_fdinfo(link, m);
}
#endif

static __poll_t bpf_link_poll(struct file *file, struct poll_table_struct *pts)
{
	struct bpf_link *link = file->private_data;

	return link->ops->poll(file, pts);
}

static const struct file_operations bpf_link_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= bpf_link_show_fdinfo,
#endif
	.release	= bpf_link_release,
	.read		= bpf_dummy_read,
	.write		= bpf_dummy_write,
};

static const struct file_operations bpf_link_fops_poll = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= bpf_link_show_fdinfo,
#endif
	.release	= bpf_link_release,
	.read		= bpf_dummy_read,
	.write		= bpf_dummy_write,
	.poll		= bpf_link_poll,
};

static int bpf_link_alloc_id(struct bpf_link *link)
{
	int id;

	idr_preload(GFP_KERNEL);
	spin_lock_bh(&link_idr_lock);
	id = idr_alloc_cyclic(&link_idr, link, 1, INT_MAX, GFP_ATOMIC);
	spin_unlock_bh(&link_idr_lock);
	idr_preload_end();

	return id;
}

/* Prepare bpf_link to be exposed to user-space by allocating anon_inode file,
 * reserving unused FD and allocating ID from link_idr. This is to be paired
 * with bpf_link_settle() to install FD and ID and expose bpf_link to
 * user-space, if bpf_link is successfully attached. If not, bpf_link and
 * pre-allocated resources are to be freed with bpf_cleanup() call. All the
 * transient state is passed around in struct bpf_link_primer.
 * This is preferred way to create and initialize bpf_link, especially when
 * there are complicated and expensive operations in between creating bpf_link
 * itself and attaching it to BPF hook. By using bpf_link_prime() and
 * bpf_link_settle() kernel code using bpf_link doesn't have to perform
 * expensive (and potentially failing) roll back operations in a rare case
 * that file, FD, or ID can't be allocated.
 */
int bpf_link_prime(struct bpf_link *link, struct bpf_link_primer *primer)
{
	struct file *file;
	int fd, id;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return fd;


	id = bpf_link_alloc_id(link);
	if (id < 0) {
		put_unused_fd(fd);
		return id;
	}

	file = anon_inode_getfile("bpf_link",
				  link->ops->poll ? &bpf_link_fops_poll : &bpf_link_fops,
				  link, O_CLOEXEC);
	if (IS_ERR(file)) {
		bpf_link_free_id(id);
		put_unused_fd(fd);
		return PTR_ERR(file);
	}

	primer->link = link;
	primer->file = file;
	primer->fd = fd;
	primer->id = id;
	return 0;
}

int bpf_link_settle(struct bpf_link_primer *primer)
{
	/* make bpf_link fetchable by ID */
	spin_lock_bh(&link_idr_lock);
	primer->link->id = primer->id;
	spin_unlock_bh(&link_idr_lock);
	/* make bpf_link fetchable by FD */
	fd_install(primer->fd, primer->file);
	/* pass through installed FD */
	return primer->fd;
}

int bpf_link_new_fd(struct bpf_link *link)
{
	return anon_inode_getfd("bpf-link",
				link->ops->poll ? &bpf_link_fops_poll : &bpf_link_fops,
				link, O_CLOEXEC);
}

struct bpf_link *bpf_link_get_from_fd(u32 ufd)
{
	CLASS(fd, f)(ufd);
	struct bpf_link *link;

	if (fd_empty(f))
		return ERR_PTR(-EBADF);
	if (fd_file(f)->f_op != &bpf_link_fops && fd_file(f)->f_op != &bpf_link_fops_poll)
		return ERR_PTR(-EINVAL);

	link = fd_file(f)->private_data;
	bpf_link_inc(link);
	return link;
}
EXPORT_SYMBOL_NS(bpf_link_get_from_fd, "BPF_INTERNAL");

static void bpf_tracing_link_release(struct bpf_link *link)
{
	struct bpf_tracing_link *tr_link =
		container_of(link, struct bpf_tracing_link, link.link);

	WARN_ON_ONCE(bpf_trampoline_unlink_prog(&tr_link->link,
						tr_link->trampoline,
						tr_link->tgt_prog));

	bpf_trampoline_put(tr_link->trampoline);

	/* tgt_prog is NULL if target is a kernel function */
	if (tr_link->tgt_prog)
		bpf_prog_put(tr_link->tgt_prog);
}

static void bpf_tracing_link_dealloc(struct bpf_link *link)
{
	struct bpf_tracing_link *tr_link =
		container_of(link, struct bpf_tracing_link, link.link);

	kfree(tr_link);
}