// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 22/37



static void bpf_raw_tp_link_dealloc(struct bpf_link *link)
{
	struct bpf_raw_tp_link *raw_tp =
		container_of(link, struct bpf_raw_tp_link, link);

	kfree(raw_tp);
}

static void bpf_raw_tp_link_show_fdinfo(const struct bpf_link *link,
					struct seq_file *seq)
{
	struct bpf_raw_tp_link *raw_tp_link =
		container_of(link, struct bpf_raw_tp_link, link);

	seq_printf(seq,
		   "tp_name:\t%s\n"
		   "cookie:\t%llu\n",
		   raw_tp_link->btp->tp->name,
		   raw_tp_link->cookie);
}

static int bpf_copy_to_user(char __user *ubuf, const char *buf, u32 ulen,
			    u32 len)
{
	if (ulen >= len + 1) {
		if (copy_to_user(ubuf, buf, len + 1))
			return -EFAULT;
	} else {
		char zero = '\0';

		if (copy_to_user(ubuf, buf, ulen - 1))
			return -EFAULT;
		if (put_user(zero, ubuf + ulen - 1))
			return -EFAULT;
		return -ENOSPC;
	}

	return 0;
}

static int bpf_raw_tp_link_fill_link_info(const struct bpf_link *link,
					  struct bpf_link_info *info)
{
	struct bpf_raw_tp_link *raw_tp_link =
		container_of(link, struct bpf_raw_tp_link, link);
	char __user *ubuf = u64_to_user_ptr(info->raw_tracepoint.tp_name);
	const char *tp_name = raw_tp_link->btp->tp->name;
	u32 ulen = info->raw_tracepoint.tp_name_len;
	size_t tp_len = strlen(tp_name);

	if (!ulen ^ !ubuf)
		return -EINVAL;

	info->raw_tracepoint.tp_name_len = tp_len + 1;
	info->raw_tracepoint.cookie = raw_tp_link->cookie;

	if (!ubuf)
		return 0;

	return bpf_copy_to_user(ubuf, tp_name, ulen, tp_len);
}

static const struct bpf_link_ops bpf_raw_tp_link_lops = {
	.release = bpf_raw_tp_link_release,
	.dealloc_deferred = bpf_raw_tp_link_dealloc,
	.show_fdinfo = bpf_raw_tp_link_show_fdinfo,
	.fill_link_info = bpf_raw_tp_link_fill_link_info,
};

#ifdef CONFIG_PERF_EVENTS
struct bpf_perf_link {
	struct bpf_link link;
	struct file *perf_file;
};

static void bpf_perf_link_release(struct bpf_link *link)
{
	struct bpf_perf_link *perf_link = container_of(link, struct bpf_perf_link, link);
	struct perf_event *event = perf_link->perf_file->private_data;

	perf_event_free_bpf_prog(event);
	fput(perf_link->perf_file);
}

static void bpf_perf_link_dealloc(struct bpf_link *link)
{
	struct bpf_perf_link *perf_link = container_of(link, struct bpf_perf_link, link);

	kfree(perf_link);
}

static int bpf_perf_link_fill_common(const struct perf_event *event,
				     char __user *uname, u32 *ulenp,
				     u64 *probe_offset, u64 *probe_addr,
				     u32 *fd_type, unsigned long *missed)
{
	const char *buf;
	u32 prog_id, ulen;
	size_t len;
	int err;

	ulen = *ulenp;
	if (!ulen ^ !uname)
		return -EINVAL;

	err = bpf_get_perf_event_info(event, &prog_id, fd_type, &buf,
				      probe_offset, probe_addr, missed);
	if (err)
		return err;

	if (buf) {
		len = strlen(buf);
		*ulenp = len + 1;
	} else {
		*ulenp = 1;
	}
	if (!uname)
		return 0;

	if (buf) {
		err = bpf_copy_to_user(uname, buf, ulen, len);
		if (err)
			return err;
	} else {
		char zero = '\0';

		if (put_user(zero, uname))
			return -EFAULT;
	}
	return 0;
}

#ifdef CONFIG_KPROBE_EVENTS
static int bpf_perf_link_fill_kprobe(const struct perf_event *event,
				     struct bpf_link_info *info)
{
	unsigned long missed;
	char __user *uname;
	u64 addr, offset;
	u32 ulen, type;
	int err;

	uname = u64_to_user_ptr(info->perf_event.kprobe.func_name);
	ulen = info->perf_event.kprobe.name_len;
	err = bpf_perf_link_fill_common(event, uname, &ulen, &offset, &addr,
					&type, &missed);
	if (err)
		return err;
	if (type == BPF_FD_TYPE_KRETPROBE)
		info->perf_event.type = BPF_PERF_EVENT_KRETPROBE;
	else
		info->perf_event.type = BPF_PERF_EVENT_KPROBE;
	info->perf_event.kprobe.name_len = ulen;
	info->perf_event.kprobe.offset = offset;
	info->perf_event.kprobe.missed = missed;
	if (!kallsyms_show_value(current_cred()))
		addr = 0;
	info->perf_event.kprobe.addr = addr;
	info->perf_event.kprobe.cookie = event->bpf_cookie;
	return 0;
}

static void bpf_perf_link_fdinfo_kprobe(const struct perf_event *event,
					struct seq_file *seq)
{
	const char *name;
	int err;
	u32 prog_id, type;
	u64 offset, addr;
	unsigned long missed;

	err = bpf_get_perf_event_info(event, &prog_id, &type, &name,
				      &offset, &addr, &missed);
	if (err)
		return;

	seq_printf(seq,
		   "name:\t%s\n"
		   "offset:\t%#llx\n"
		   "missed:\t%lu\n"
		   "addr:\t%#llx\n"
		   "event_type:\t%s\n"
		   "cookie:\t%llu\n",
		   name, offset, missed, addr,
		   type == BPF_FD_TYPE_KRETPROBE ?  "kretprobe" : "kprobe",
		   event->bpf_cookie);
}
#endif

#ifdef CONFIG_UPROBE_EVENTS
static int bpf_perf_link_fill_uprobe(const struct perf_event *event,
				     struct bpf_link_info *info)
{
	u64 ref_ctr_offset, offset;
	char __user *uname;
	u32 ulen, type;
	int err;

	uname = u64_to_user_ptr(info->perf_event.uprobe.file_name);
	ulen = info->perf_event.uprobe.name_len;
	err = bpf_perf_link_fill_common(event, uname, &ulen, &offset, &ref_ctr_offset,
					&type, NULL);
	if (err)
		return err;