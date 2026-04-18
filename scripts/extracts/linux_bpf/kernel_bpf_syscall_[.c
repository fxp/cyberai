// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 27/37



	switch (attr->query.attach_type) {
	case BPF_CGROUP_INET_INGRESS:
	case BPF_CGROUP_INET_EGRESS:
	case BPF_CGROUP_INET_SOCK_CREATE:
	case BPF_CGROUP_INET_SOCK_RELEASE:
	case BPF_CGROUP_INET4_BIND:
	case BPF_CGROUP_INET6_BIND:
	case BPF_CGROUP_INET4_POST_BIND:
	case BPF_CGROUP_INET6_POST_BIND:
	case BPF_CGROUP_INET4_CONNECT:
	case BPF_CGROUP_INET6_CONNECT:
	case BPF_CGROUP_UNIX_CONNECT:
	case BPF_CGROUP_INET4_GETPEERNAME:
	case BPF_CGROUP_INET6_GETPEERNAME:
	case BPF_CGROUP_UNIX_GETPEERNAME:
	case BPF_CGROUP_INET4_GETSOCKNAME:
	case BPF_CGROUP_INET6_GETSOCKNAME:
	case BPF_CGROUP_UNIX_GETSOCKNAME:
	case BPF_CGROUP_UDP4_SENDMSG:
	case BPF_CGROUP_UDP6_SENDMSG:
	case BPF_CGROUP_UNIX_SENDMSG:
	case BPF_CGROUP_UDP4_RECVMSG:
	case BPF_CGROUP_UDP6_RECVMSG:
	case BPF_CGROUP_UNIX_RECVMSG:
	case BPF_CGROUP_SOCK_OPS:
	case BPF_CGROUP_DEVICE:
	case BPF_CGROUP_SYSCTL:
	case BPF_CGROUP_GETSOCKOPT:
	case BPF_CGROUP_SETSOCKOPT:
	case BPF_LSM_CGROUP:
		return cgroup_bpf_prog_query(attr, uattr);
	case BPF_LIRC_MODE2:
		return lirc_prog_query(attr, uattr);
	case BPF_FLOW_DISSECTOR:
	case BPF_SK_LOOKUP:
		return netns_bpf_prog_query(attr, uattr);
	case BPF_SK_SKB_STREAM_PARSER:
	case BPF_SK_SKB_STREAM_VERDICT:
	case BPF_SK_MSG_VERDICT:
	case BPF_SK_SKB_VERDICT:
		return sock_map_bpf_prog_query(attr, uattr);
	case BPF_TCX_INGRESS:
	case BPF_TCX_EGRESS:
		return tcx_prog_query(attr, uattr);
	case BPF_NETKIT_PRIMARY:
	case BPF_NETKIT_PEER:
		return netkit_prog_query(attr, uattr);
	default:
		return -EINVAL;
	}
}

#define BPF_PROG_TEST_RUN_LAST_FIELD test.batch_size

static int bpf_prog_test_run(const union bpf_attr *attr,
			     union bpf_attr __user *uattr)
{
	struct bpf_prog *prog;
	int ret = -ENOTSUPP;

	if (CHECK_ATTR(BPF_PROG_TEST_RUN))
		return -EINVAL;

	if ((attr->test.ctx_size_in && !attr->test.ctx_in) ||
	    (!attr->test.ctx_size_in && attr->test.ctx_in))
		return -EINVAL;

	if ((attr->test.ctx_size_out && !attr->test.ctx_out) ||
	    (!attr->test.ctx_size_out && attr->test.ctx_out))
		return -EINVAL;

	prog = bpf_prog_get(attr->test.prog_fd);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	if (prog->aux->ops->test_run)
		ret = prog->aux->ops->test_run(prog, attr, uattr);

	bpf_prog_put(prog);
	return ret;
}

#define BPF_OBJ_GET_NEXT_ID_LAST_FIELD next_id

static int bpf_obj_get_next_id(const union bpf_attr *attr,
			       union bpf_attr __user *uattr,
			       struct idr *idr,
			       spinlock_t *lock)
{
	u32 next_id = attr->start_id;
	int err = 0;

	if (CHECK_ATTR(BPF_OBJ_GET_NEXT_ID) || next_id >= INT_MAX)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	next_id++;
	spin_lock_bh(lock);
	if (!idr_get_next(idr, &next_id))
		err = -ENOENT;
	spin_unlock_bh(lock);

	if (!err)
		err = put_user(next_id, &uattr->next_id);

	return err;
}

struct bpf_map *bpf_map_get_curr_or_next(u32 *id)
{
	struct bpf_map *map;

	spin_lock_bh(&map_idr_lock);
again:
	map = idr_get_next(&map_idr, id);
	if (map) {
		map = __bpf_map_inc_not_zero(map, false);
		if (IS_ERR(map)) {
			(*id)++;
			goto again;
		}
	}
	spin_unlock_bh(&map_idr_lock);

	return map;
}

struct bpf_prog *bpf_prog_get_curr_or_next(u32 *id)
{
	struct bpf_prog *prog;

	spin_lock_bh(&prog_idr_lock);
again:
	prog = idr_get_next(&prog_idr, id);
	if (prog) {
		prog = bpf_prog_inc_not_zero(prog);
		if (IS_ERR(prog)) {
			(*id)++;
			goto again;
		}
	}
	spin_unlock_bh(&prog_idr_lock);

	return prog;
}

#define BPF_PROG_GET_FD_BY_ID_LAST_FIELD prog_id

struct bpf_prog *bpf_prog_by_id(u32 id)
{
	struct bpf_prog *prog;

	if (!id)
		return ERR_PTR(-ENOENT);

	spin_lock_bh(&prog_idr_lock);
	prog = idr_find(&prog_idr, id);
	if (prog)
		prog = bpf_prog_inc_not_zero(prog);
	else
		prog = ERR_PTR(-ENOENT);
	spin_unlock_bh(&prog_idr_lock);
	return prog;
}

static int bpf_prog_get_fd_by_id(const union bpf_attr *attr)
{
	struct bpf_prog *prog;
	u32 id = attr->prog_id;
	int fd;

	if (CHECK_ATTR(BPF_PROG_GET_FD_BY_ID))
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	prog = bpf_prog_by_id(id);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	fd = bpf_prog_new_fd(prog);
	if (fd < 0)
		bpf_prog_put(prog);

	return fd;
}

#define BPF_MAP_GET_FD_BY_ID_LAST_FIELD open_flags

static int bpf_map_get_fd_by_id(const union bpf_attr *attr)
{
	struct bpf_map *map;
	u32 id = attr->map_id;
	int f_flags;
	int fd;

	if (CHECK_ATTR(BPF_MAP_GET_FD_BY_ID) ||
	    attr->open_flags & ~BPF_OBJ_FLAG_MASK)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	f_flags = bpf_get_file_flag(attr->open_flags);
	if (f_flags < 0)
		return f_flags;

	spin_lock_bh(&map_idr_lock);
	map = idr_find(&map_idr, id);
	if (map)
		map = __bpf_map_inc_not_zero(map, true);
	else
		map = ERR_PTR(-ENOENT);
	spin_unlock_bh(&map_idr_lock);

	if (IS_ERR(map))
		return PTR_ERR(map);

	fd = bpf_map_new_fd(map, f_flags);
	if (fd < 0)
		bpf_map_put_with_uref(map);

	return fd;
}