// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 32/37



static int bpf_map_do_batch(const union bpf_attr *attr,
			    union bpf_attr __user *uattr,
			    int cmd)
{
	bool has_read  = cmd == BPF_MAP_LOOKUP_BATCH ||
			 cmd == BPF_MAP_LOOKUP_AND_DELETE_BATCH;
	bool has_write = cmd != BPF_MAP_LOOKUP_BATCH;
	struct bpf_map *map;
	int err;

	if (CHECK_ATTR(BPF_MAP_BATCH))
		return -EINVAL;

	CLASS(fd, f)(attr->batch.map_fd);

	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (has_write)
		bpf_map_write_active_inc(map);
	if (has_read && !(map_get_sys_perms(map, f) & FMODE_CAN_READ)) {
		err = -EPERM;
		goto err_put;
	}
	if (has_write && !(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	if (cmd == BPF_MAP_LOOKUP_BATCH)
		BPF_DO_BATCH(map->ops->map_lookup_batch, map, attr, uattr);
	else if (cmd == BPF_MAP_LOOKUP_AND_DELETE_BATCH)
		BPF_DO_BATCH(map->ops->map_lookup_and_delete_batch, map, attr, uattr);
	else if (cmd == BPF_MAP_UPDATE_BATCH)
		BPF_DO_BATCH(map->ops->map_update_batch, map, fd_file(f), attr, uattr);
	else
		BPF_DO_BATCH(map->ops->map_delete_batch, map, attr, uattr);
err_put:
	if (has_write) {
		maybe_wait_bpf_programs(map);
		bpf_map_write_active_dec(map);
	}
	return err;
}

#define BPF_LINK_CREATE_LAST_FIELD link_create.uprobe_multi.pid
static int link_create(union bpf_attr *attr, bpfptr_t uattr)
{
	struct bpf_prog *prog;
	int ret;

	if (CHECK_ATTR(BPF_LINK_CREATE))
		return -EINVAL;

	if (attr->link_create.attach_type == BPF_STRUCT_OPS)
		return bpf_struct_ops_link_create(attr);

	prog = bpf_prog_get(attr->link_create.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	ret = bpf_prog_attach_check_attach_type(prog,
						attr->link_create.attach_type);
	if (ret)
		goto out;

	switch (prog->type) {
	case BPF_PROG_TYPE_CGROUP_SKB:
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_SOCK_OPS:
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
		ret = cgroup_bpf_link_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_EXT:
		ret = bpf_tracing_prog_attach(prog,
					      attr->link_create.target_fd,
					      attr->link_create.target_btf_id,
					      attr->link_create.tracing.cookie,
					      attr->link_create.attach_type);
		break;
	case BPF_PROG_TYPE_LSM:
	case BPF_PROG_TYPE_TRACING:
		if (attr->link_create.attach_type != prog->expected_attach_type) {
			ret = -EINVAL;
			goto out;
		}
		if (prog->expected_attach_type == BPF_TRACE_RAW_TP)
			ret = bpf_raw_tp_link_attach(prog, NULL, attr->link_create.tracing.cookie,
						     attr->link_create.attach_type);
		else if (prog->expected_attach_type == BPF_TRACE_ITER)
			ret = bpf_iter_link_attach(attr, uattr, prog);
		else if (prog->expected_attach_type == BPF_LSM_CGROUP)
			ret = cgroup_bpf_link_attach(attr, prog);
		else
			ret = bpf_tracing_prog_attach(prog,
						      attr->link_create.target_fd,
						      attr->link_create.target_btf_id,
						      attr->link_create.tracing.cookie,
						      attr->link_create.attach_type);
		break;
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
	case BPF_PROG_TYPE_SK_LOOKUP:
		ret = netns_bpf_link_create(attr, prog);
		break;
	case BPF_PROG_TYPE_SK_MSG:
	case BPF_PROG_TYPE_SK_SKB:
		ret = sock_map_link_create(attr, prog);
		break;
#ifdef CONFIG_NET
	case BPF_PROG_TYPE_XDP:
		ret = bpf_xdp_link_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_SCHED_CLS:
		if (attr->link_create.attach_type == BPF_TCX_INGRESS ||
		    attr->link_create.attach_type == BPF_TCX_EGRESS)
			ret = tcx_link_attach(attr, prog);
		else
			ret = netkit_link_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_NETFILTER:
		ret = bpf_nf_link_attach(attr, prog);
		break;
#endif
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_TRACEPOINT:
		ret = bpf_perf_link_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_KPROBE:
		if (attr->link_create.attach_type == BPF_PERF_EVENT)
			ret = bpf_perf_link_attach(attr, prog);
		else if (attr->link_create.attach_type == BPF_TRACE_KPROBE_MULTI ||
			 attr->link_create.attach_type == BPF_TRACE_KPROBE_SESSION)
			ret = bpf_kprobe_multi_link_attach(attr, prog);
		else if (attr->link_create.attach_type == BPF_TRACE_UPROBE_MULTI ||
			 attr->link_create.attach_type == BPF_TRACE_UPROBE_SESSION)
			ret = bpf_uprobe_multi_link_attach(attr, prog);
		break;
	default:
		ret = -EINVAL;
	}

out:
	if (ret < 0)
		bpf_prog_put(prog);
	return ret;
}

static int link_update_map(struct bpf_link *link, union bpf_attr *attr)
{
	struct bpf_map *new_map, *old_map = NULL;
	int ret;

	new_map = bpf_map_get(attr->link_update.new_map_fd);
	if (IS_ERR(new_map))
		return PTR_ERR(new_map);

	if (attr->link_update.flags & BPF_F_REPLACE) {
		old_map = bpf_map_get(attr->link_update.old_map_fd);
		if (IS_ERR(old_map)) {
			ret = PTR_ERR(old_map);
			goto out_put;
		}
	} else if (attr->link_update.old_map_fd) {
		ret = -EINVAL;
		goto out_put;
	}

	ret = link->ops->update_map(link, new_map, old_map);