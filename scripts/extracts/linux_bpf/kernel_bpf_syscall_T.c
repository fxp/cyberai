// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 20/37



static void bpf_tracing_link_show_fdinfo(const struct bpf_link *link,
					 struct seq_file *seq)
{
	struct bpf_tracing_link *tr_link =
		container_of(link, struct bpf_tracing_link, link.link);
	u32 target_btf_id, target_obj_id;

	bpf_trampoline_unpack_key(tr_link->trampoline->key,
				  &target_obj_id, &target_btf_id);
	seq_printf(seq,
		   "attach_type:\t%d\n"
		   "target_obj_id:\t%u\n"
		   "target_btf_id:\t%u\n"
		   "cookie:\t%llu\n",
		   link->attach_type,
		   target_obj_id,
		   target_btf_id,
		   tr_link->link.cookie);
}

static int bpf_tracing_link_fill_link_info(const struct bpf_link *link,
					   struct bpf_link_info *info)
{
	struct bpf_tracing_link *tr_link =
		container_of(link, struct bpf_tracing_link, link.link);

	info->tracing.attach_type = link->attach_type;
	info->tracing.cookie = tr_link->link.cookie;
	bpf_trampoline_unpack_key(tr_link->trampoline->key,
				  &info->tracing.target_obj_id,
				  &info->tracing.target_btf_id);

	return 0;
}

static const struct bpf_link_ops bpf_tracing_link_lops = {
	.release = bpf_tracing_link_release,
	.dealloc = bpf_tracing_link_dealloc,
	.show_fdinfo = bpf_tracing_link_show_fdinfo,
	.fill_link_info = bpf_tracing_link_fill_link_info,
};

static int bpf_tracing_prog_attach(struct bpf_prog *prog,
				   int tgt_prog_fd,
				   u32 btf_id,
				   u64 bpf_cookie,
				   enum bpf_attach_type attach_type)
{
	struct bpf_link_primer link_primer;
	struct bpf_prog *tgt_prog = NULL;
	struct bpf_trampoline *tr = NULL;
	struct bpf_tracing_link *link;
	u64 key = 0;
	int err;

	switch (prog->type) {
	case BPF_PROG_TYPE_TRACING:
		if (prog->expected_attach_type != BPF_TRACE_FENTRY &&
		    prog->expected_attach_type != BPF_TRACE_FEXIT &&
		    prog->expected_attach_type != BPF_TRACE_FSESSION &&
		    prog->expected_attach_type != BPF_MODIFY_RETURN) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	case BPF_PROG_TYPE_EXT:
		if (prog->expected_attach_type != 0) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	case BPF_PROG_TYPE_LSM:
		if (prog->expected_attach_type != BPF_LSM_MAC) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	default:
		err = -EINVAL;
		goto out_put_prog;
	}

	if (!!tgt_prog_fd != !!btf_id) {
		err = -EINVAL;
		goto out_put_prog;
	}

	if (tgt_prog_fd) {
		/*
		 * For now we only allow new targets for BPF_PROG_TYPE_EXT. If this
		 * part would be changed to implement the same for
		 * BPF_PROG_TYPE_TRACING, do not forget to update the way how
		 * attach_tracing_prog flag is set.
		 */
		if (prog->type != BPF_PROG_TYPE_EXT) {
			err = -EINVAL;
			goto out_put_prog;
		}

		tgt_prog = bpf_prog_get(tgt_prog_fd);
		if (IS_ERR(tgt_prog)) {
			err = PTR_ERR(tgt_prog);
			tgt_prog = NULL;
			goto out_put_prog;
		}

		key = bpf_trampoline_compute_key(tgt_prog, NULL, btf_id);
	}

	if (prog->expected_attach_type == BPF_TRACE_FSESSION) {
		struct bpf_fsession_link *fslink;

		fslink = kzalloc_obj(*fslink, GFP_USER);
		if (fslink) {
			bpf_link_init(&fslink->fexit.link, BPF_LINK_TYPE_TRACING,
				      &bpf_tracing_link_lops, prog, attach_type);
			fslink->fexit.cookie = bpf_cookie;
			link = &fslink->link;
		} else {
			link = NULL;
		}
	} else {
		link = kzalloc_obj(*link, GFP_USER);
	}
	if (!link) {
		err = -ENOMEM;
		goto out_put_prog;
	}
	bpf_link_init(&link->link.link, BPF_LINK_TYPE_TRACING,
		      &bpf_tracing_link_lops, prog, attach_type);

	link->link.cookie = bpf_cookie;

	mutex_lock(&prog->aux->dst_mutex);