// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 24/37



	if (attr->link_create.flags)
		return -EINVAL;

	perf_file = perf_event_get(attr->link_create.target_fd);
	if (IS_ERR(perf_file))
		return PTR_ERR(perf_file);

	link = kzalloc_obj(*link, GFP_USER);
	if (!link) {
		err = -ENOMEM;
		goto out_put_file;
	}
	bpf_link_init(&link->link, BPF_LINK_TYPE_PERF_EVENT, &bpf_perf_link_lops, prog,
		      attr->link_create.attach_type);
	link->perf_file = perf_file;

	err = bpf_link_prime(&link->link, &link_primer);
	if (err) {
		kfree(link);
		goto out_put_file;
	}

	event = perf_file->private_data;
	err = perf_event_set_bpf_prog(event, prog, attr->link_create.perf_event.bpf_cookie);
	if (err) {
		bpf_link_cleanup(&link_primer);
		goto out_put_file;
	}
	/* perf_event_set_bpf_prog() doesn't take its own refcnt on prog */
	bpf_prog_inc(prog);

	return bpf_link_settle(&link_primer);

out_put_file:
	fput(perf_file);
	return err;
}
#else
static int bpf_perf_link_attach(const union bpf_attr *attr, struct bpf_prog *prog)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_PERF_EVENTS */

static int bpf_raw_tp_link_attach(struct bpf_prog *prog,
				  const char __user *user_tp_name, u64 cookie,
				  enum bpf_attach_type attach_type)
{
	struct bpf_link_primer link_primer;
	struct bpf_raw_tp_link *link;
	struct bpf_raw_event_map *btp;
	const char *tp_name;
	char buf[128];
	int err;

	switch (prog->type) {
	case BPF_PROG_TYPE_TRACING:
	case BPF_PROG_TYPE_EXT:
	case BPF_PROG_TYPE_LSM:
		if (user_tp_name)
			/* The attach point for this category of programs
			 * should be specified via btf_id during program load.
			 */
			return -EINVAL;
		if (prog->type == BPF_PROG_TYPE_TRACING &&
		    prog->expected_attach_type == BPF_TRACE_RAW_TP) {
			tp_name = prog->aux->attach_func_name;
			break;
		}
		return bpf_tracing_prog_attach(prog, 0, 0, 0, attach_type);
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE:
		if (strncpy_from_user(buf, user_tp_name, sizeof(buf) - 1) < 0)
			return -EFAULT;
		buf[sizeof(buf) - 1] = 0;
		tp_name = buf;
		break;
	default:
		return -EINVAL;
	}

	btp = bpf_get_raw_tracepoint(tp_name);
	if (!btp)
		return -ENOENT;

	link = kzalloc_obj(*link, GFP_USER);
	if (!link) {
		err = -ENOMEM;
		goto out_put_btp;
	}
	bpf_link_init_sleepable(&link->link, BPF_LINK_TYPE_RAW_TRACEPOINT,
				&bpf_raw_tp_link_lops, prog, attach_type,
				tracepoint_is_faultable(btp->tp));
	link->btp = btp;
	link->cookie = cookie;

	err = bpf_link_prime(&link->link, &link_primer);
	if (err) {
		kfree(link);
		goto out_put_btp;
	}

	err = bpf_probe_register(link->btp, link);
	if (err) {
		bpf_link_cleanup(&link_primer);
		goto out_put_btp;
	}

	return bpf_link_settle(&link_primer);

out_put_btp:
	bpf_put_raw_tracepoint(btp);
	return err;
}

#define BPF_RAW_TRACEPOINT_OPEN_LAST_FIELD raw_tracepoint.cookie

static int bpf_raw_tracepoint_open(const union bpf_attr *attr)
{
	struct bpf_prog *prog;
	void __user *tp_name;
	__u64 cookie;
	int fd;

	if (CHECK_ATTR(BPF_RAW_TRACEPOINT_OPEN))
		return -EINVAL;

	prog = bpf_prog_get(attr->raw_tracepoint.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	tp_name = u64_to_user_ptr(attr->raw_tracepoint.name);
	cookie = attr->raw_tracepoint.cookie;
	fd = bpf_raw_tp_link_attach(prog, tp_name, cookie, prog->expected_attach_type);
	if (fd < 0)
		bpf_prog_put(prog);
	return fd;
}