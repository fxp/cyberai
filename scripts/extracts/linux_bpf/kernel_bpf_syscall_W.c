// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 23/37



	if (type == BPF_FD_TYPE_URETPROBE)
		info->perf_event.type = BPF_PERF_EVENT_URETPROBE;
	else
		info->perf_event.type = BPF_PERF_EVENT_UPROBE;
	info->perf_event.uprobe.name_len = ulen;
	info->perf_event.uprobe.offset = offset;
	info->perf_event.uprobe.cookie = event->bpf_cookie;
	info->perf_event.uprobe.ref_ctr_offset = ref_ctr_offset;
	return 0;
}

static void bpf_perf_link_fdinfo_uprobe(const struct perf_event *event,
					struct seq_file *seq)
{
	const char *name;
	int err;
	u32 prog_id, type;
	u64 offset, ref_ctr_offset;
	unsigned long missed;

	err = bpf_get_perf_event_info(event, &prog_id, &type, &name,
				      &offset, &ref_ctr_offset, &missed);
	if (err)
		return;

	seq_printf(seq,
		   "name:\t%s\n"
		   "offset:\t%#llx\n"
		   "ref_ctr_offset:\t%#llx\n"
		   "event_type:\t%s\n"
		   "cookie:\t%llu\n",
		   name, offset, ref_ctr_offset,
		   type == BPF_FD_TYPE_URETPROBE ?  "uretprobe" : "uprobe",
		   event->bpf_cookie);
}
#endif

static int bpf_perf_link_fill_probe(const struct perf_event *event,
				    struct bpf_link_info *info)
{
#ifdef CONFIG_KPROBE_EVENTS
	if (event->tp_event->flags & TRACE_EVENT_FL_KPROBE)
		return bpf_perf_link_fill_kprobe(event, info);
#endif
#ifdef CONFIG_UPROBE_EVENTS
	if (event->tp_event->flags & TRACE_EVENT_FL_UPROBE)
		return bpf_perf_link_fill_uprobe(event, info);
#endif
	return -EOPNOTSUPP;
}

static int bpf_perf_link_fill_tracepoint(const struct perf_event *event,
					 struct bpf_link_info *info)
{
	char __user *uname;
	u32 ulen;
	int err;

	uname = u64_to_user_ptr(info->perf_event.tracepoint.tp_name);
	ulen = info->perf_event.tracepoint.name_len;
	err = bpf_perf_link_fill_common(event, uname, &ulen, NULL, NULL, NULL, NULL);
	if (err)
		return err;

	info->perf_event.type = BPF_PERF_EVENT_TRACEPOINT;
	info->perf_event.tracepoint.name_len = ulen;
	info->perf_event.tracepoint.cookie = event->bpf_cookie;
	return 0;
}

static int bpf_perf_link_fill_perf_event(const struct perf_event *event,
					 struct bpf_link_info *info)
{
	info->perf_event.event.type = event->attr.type;
	info->perf_event.event.config = event->attr.config;
	info->perf_event.event.cookie = event->bpf_cookie;
	info->perf_event.type = BPF_PERF_EVENT_EVENT;
	return 0;
}

static int bpf_perf_link_fill_link_info(const struct bpf_link *link,
					struct bpf_link_info *info)
{
	struct bpf_perf_link *perf_link;
	const struct perf_event *event;

	perf_link = container_of(link, struct bpf_perf_link, link);
	event = perf_get_event(perf_link->perf_file);
	if (IS_ERR(event))
		return PTR_ERR(event);

	switch (event->prog->type) {
	case BPF_PROG_TYPE_PERF_EVENT:
		return bpf_perf_link_fill_perf_event(event, info);
	case BPF_PROG_TYPE_TRACEPOINT:
		return bpf_perf_link_fill_tracepoint(event, info);
	case BPF_PROG_TYPE_KPROBE:
		return bpf_perf_link_fill_probe(event, info);
	default:
		return -EOPNOTSUPP;
	}
}

static void bpf_perf_event_link_show_fdinfo(const struct perf_event *event,
					    struct seq_file *seq)
{
	seq_printf(seq,
		   "type:\t%u\n"
		   "config:\t%llu\n"
		   "event_type:\t%s\n"
		   "cookie:\t%llu\n",
		   event->attr.type, event->attr.config,
		   "event", event->bpf_cookie);
}

static void bpf_tracepoint_link_show_fdinfo(const struct perf_event *event,
					    struct seq_file *seq)
{
	int err;
	const char *name;
	u32 prog_id;

	err = bpf_get_perf_event_info(event, &prog_id, NULL, &name, NULL,
				      NULL, NULL);
	if (err)
		return;

	seq_printf(seq,
		   "tp_name:\t%s\n"
		   "event_type:\t%s\n"
		   "cookie:\t%llu\n",
		   name, "tracepoint", event->bpf_cookie);
}

static void bpf_probe_link_show_fdinfo(const struct perf_event *event,
				       struct seq_file *seq)
{
#ifdef CONFIG_KPROBE_EVENTS
	if (event->tp_event->flags & TRACE_EVENT_FL_KPROBE)
		return bpf_perf_link_fdinfo_kprobe(event, seq);
#endif

#ifdef CONFIG_UPROBE_EVENTS
	if (event->tp_event->flags & TRACE_EVENT_FL_UPROBE)
		return bpf_perf_link_fdinfo_uprobe(event, seq);
#endif
}

static void bpf_perf_link_show_fdinfo(const struct bpf_link *link,
				      struct seq_file *seq)
{
	struct bpf_perf_link *perf_link;
	const struct perf_event *event;

	perf_link = container_of(link, struct bpf_perf_link, link);
	event = perf_get_event(perf_link->perf_file);
	if (IS_ERR(event))
		return;

	switch (event->prog->type) {
	case BPF_PROG_TYPE_PERF_EVENT:
		return bpf_perf_event_link_show_fdinfo(event, seq);
	case BPF_PROG_TYPE_TRACEPOINT:
		return bpf_tracepoint_link_show_fdinfo(event, seq);
	case BPF_PROG_TYPE_KPROBE:
		return bpf_probe_link_show_fdinfo(event, seq);
	default:
		return;
	}
}

static const struct bpf_link_ops bpf_perf_link_lops = {
	.release = bpf_perf_link_release,
	.dealloc = bpf_perf_link_dealloc,
	.fill_link_info = bpf_perf_link_fill_link_info,
	.show_fdinfo = bpf_perf_link_show_fdinfo,
};

static int bpf_perf_link_attach(const union bpf_attr *attr, struct bpf_prog *prog)
{
	struct bpf_link_primer link_primer;
	struct bpf_perf_link *link;
	struct perf_event *event;
	struct file *perf_file;
	int err;