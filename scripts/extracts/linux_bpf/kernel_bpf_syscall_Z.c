// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 26/37



#define BPF_F_ATTACH_MASK_BASE	\
	(BPF_F_ALLOW_OVERRIDE |	\
	 BPF_F_ALLOW_MULTI |	\
	 BPF_F_REPLACE |	\
	 BPF_F_PREORDER)

#define BPF_F_ATTACH_MASK_MPROG	\
	(BPF_F_REPLACE |	\
	 BPF_F_BEFORE |		\
	 BPF_F_AFTER |		\
	 BPF_F_ID |		\
	 BPF_F_LINK)

static int bpf_prog_attach(const union bpf_attr *attr)
{
	enum bpf_prog_type ptype;
	struct bpf_prog *prog;
	int ret;

	if (CHECK_ATTR(BPF_PROG_ATTACH))
		return -EINVAL;

	ptype = attach_type_to_prog_type(attr->attach_type);
	if (ptype == BPF_PROG_TYPE_UNSPEC)
		return -EINVAL;
	if (bpf_mprog_supported(ptype)) {
		if (attr->attach_flags & ~BPF_F_ATTACH_MASK_MPROG)
			return -EINVAL;
	} else if (is_cgroup_prog_type(ptype, 0, false)) {
		if (attr->attach_flags & ~(BPF_F_ATTACH_MASK_BASE | BPF_F_ATTACH_MASK_MPROG))
			return -EINVAL;
	} else {
		if (attr->attach_flags & ~BPF_F_ATTACH_MASK_BASE)
			return -EINVAL;
		if (attr->relative_fd ||
		    attr->expected_revision)
			return -EINVAL;
	}

	prog = bpf_prog_get_type(attr->attach_bpf_fd, ptype);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	if (bpf_prog_attach_check_attach_type(prog, attr->attach_type)) {
		bpf_prog_put(prog);
		return -EINVAL;
	}

	if (is_cgroup_prog_type(ptype, prog->expected_attach_type, true)) {
		ret = cgroup_bpf_prog_attach(attr, ptype, prog);
		goto out;
	}

	switch (ptype) {
	case BPF_PROG_TYPE_SK_SKB:
	case BPF_PROG_TYPE_SK_MSG:
		ret = sock_map_get_from_fd(attr, prog);
		break;
	case BPF_PROG_TYPE_LIRC_MODE2:
		ret = lirc_prog_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
		ret = netns_bpf_prog_attach(attr, prog);
		break;
	case BPF_PROG_TYPE_SCHED_CLS:
		if (attr->attach_type == BPF_TCX_INGRESS ||
		    attr->attach_type == BPF_TCX_EGRESS)
			ret = tcx_prog_attach(attr, prog);
		else
			ret = netkit_prog_attach(attr, prog);
		break;
	default:
		ret = -EINVAL;
	}
out:
	if (ret)
		bpf_prog_put(prog);
	return ret;
}

#define BPF_PROG_DETACH_LAST_FIELD expected_revision

static int bpf_prog_detach(const union bpf_attr *attr)
{
	struct bpf_prog *prog = NULL;
	enum bpf_prog_type ptype;
	int ret;

	if (CHECK_ATTR(BPF_PROG_DETACH))
		return -EINVAL;

	ptype = attach_type_to_prog_type(attr->attach_type);
	if (bpf_mprog_supported(ptype)) {
		if (ptype == BPF_PROG_TYPE_UNSPEC)
			return -EINVAL;
		if (attr->attach_flags & ~BPF_F_ATTACH_MASK_MPROG)
			return -EINVAL;
		if (attr->attach_bpf_fd) {
			prog = bpf_prog_get_type(attr->attach_bpf_fd, ptype);
			if (IS_ERR(prog))
				return PTR_ERR(prog);
		} else if (!bpf_mprog_detach_empty(ptype)) {
			return -EPERM;
		}
	} else if (is_cgroup_prog_type(ptype, 0, false)) {
		if (attr->attach_flags || attr->relative_fd)
			return -EINVAL;
	} else if (attr->attach_flags ||
		   attr->relative_fd ||
		   attr->expected_revision) {
		return -EINVAL;
	}

	switch (ptype) {
	case BPF_PROG_TYPE_SK_MSG:
	case BPF_PROG_TYPE_SK_SKB:
		ret = sock_map_prog_detach(attr, ptype);
		break;
	case BPF_PROG_TYPE_LIRC_MODE2:
		ret = lirc_prog_detach(attr);
		break;
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
		ret = netns_bpf_prog_detach(attr, ptype);
		break;
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SKB:
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_SOCK_OPS:
	case BPF_PROG_TYPE_LSM:
		ret = cgroup_bpf_prog_detach(attr, ptype);
		break;
	case BPF_PROG_TYPE_SCHED_CLS:
		if (attr->attach_type == BPF_TCX_INGRESS ||
		    attr->attach_type == BPF_TCX_EGRESS)
			ret = tcx_prog_detach(attr, prog);
		else
			ret = netkit_prog_detach(attr, prog);
		break;
	default:
		ret = -EINVAL;
	}

	if (prog)
		bpf_prog_put(prog);
	return ret;
}

#define BPF_PROG_QUERY_LAST_FIELD query.revision

static int bpf_prog_query(const union bpf_attr *attr,
			  union bpf_attr __user *uattr)
{
	if (!bpf_net_capable())
		return -EPERM;
	if (CHECK_ATTR(BPF_PROG_QUERY))
		return -EINVAL;
	if (attr->query.query_flags & ~BPF_F_QUERY_EFFECTIVE)
		return -EINVAL;