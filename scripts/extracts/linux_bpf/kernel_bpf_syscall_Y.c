// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 25/37



static enum bpf_prog_type
attach_type_to_prog_type(enum bpf_attach_type attach_type)
{
	switch (attach_type) {
	case BPF_CGROUP_INET_INGRESS:
	case BPF_CGROUP_INET_EGRESS:
		return BPF_PROG_TYPE_CGROUP_SKB;
	case BPF_CGROUP_INET_SOCK_CREATE:
	case BPF_CGROUP_INET_SOCK_RELEASE:
	case BPF_CGROUP_INET4_POST_BIND:
	case BPF_CGROUP_INET6_POST_BIND:
		return BPF_PROG_TYPE_CGROUP_SOCK;
	case BPF_CGROUP_INET4_BIND:
	case BPF_CGROUP_INET6_BIND:
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
		return BPF_PROG_TYPE_CGROUP_SOCK_ADDR;
	case BPF_CGROUP_SOCK_OPS:
		return BPF_PROG_TYPE_SOCK_OPS;
	case BPF_CGROUP_DEVICE:
		return BPF_PROG_TYPE_CGROUP_DEVICE;
	case BPF_SK_MSG_VERDICT:
		return BPF_PROG_TYPE_SK_MSG;
	case BPF_SK_SKB_STREAM_PARSER:
	case BPF_SK_SKB_STREAM_VERDICT:
	case BPF_SK_SKB_VERDICT:
		return BPF_PROG_TYPE_SK_SKB;
	case BPF_LIRC_MODE2:
		return BPF_PROG_TYPE_LIRC_MODE2;
	case BPF_FLOW_DISSECTOR:
		return BPF_PROG_TYPE_FLOW_DISSECTOR;
	case BPF_CGROUP_SYSCTL:
		return BPF_PROG_TYPE_CGROUP_SYSCTL;
	case BPF_CGROUP_GETSOCKOPT:
	case BPF_CGROUP_SETSOCKOPT:
		return BPF_PROG_TYPE_CGROUP_SOCKOPT;
	case BPF_TRACE_ITER:
	case BPF_TRACE_RAW_TP:
	case BPF_TRACE_FENTRY:
	case BPF_TRACE_FEXIT:
	case BPF_TRACE_FSESSION:
	case BPF_MODIFY_RETURN:
		return BPF_PROG_TYPE_TRACING;
	case BPF_LSM_MAC:
		return BPF_PROG_TYPE_LSM;
	case BPF_SK_LOOKUP:
		return BPF_PROG_TYPE_SK_LOOKUP;
	case BPF_XDP:
		return BPF_PROG_TYPE_XDP;
	case BPF_LSM_CGROUP:
		return BPF_PROG_TYPE_LSM;
	case BPF_TCX_INGRESS:
	case BPF_TCX_EGRESS:
	case BPF_NETKIT_PRIMARY:
	case BPF_NETKIT_PEER:
		return BPF_PROG_TYPE_SCHED_CLS;
	default:
		return BPF_PROG_TYPE_UNSPEC;
	}
}

static int bpf_prog_attach_check_attach_type(const struct bpf_prog *prog,
					     enum bpf_attach_type attach_type)
{
	enum bpf_prog_type ptype;

	switch (prog->type) {
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
	case BPF_PROG_TYPE_SK_LOOKUP:
		return attach_type == prog->expected_attach_type ? 0 : -EINVAL;
	case BPF_PROG_TYPE_CGROUP_SKB:
		if (!bpf_token_capable(prog->aux->token, CAP_NET_ADMIN))
			/* cg-skb progs can be loaded by unpriv user.
			 * check permissions at attach time.
			 */
			return -EPERM;

		ptype = attach_type_to_prog_type(attach_type);
		if (prog->type != ptype)
			return -EINVAL;

		return prog->enforce_expected_attach_type &&
			prog->expected_attach_type != attach_type ?
			-EINVAL : 0;
	case BPF_PROG_TYPE_EXT:
		return 0;
	case BPF_PROG_TYPE_NETFILTER:
		if (attach_type != BPF_NETFILTER)
			return -EINVAL;
		return 0;
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_TRACEPOINT:
		if (attach_type != BPF_PERF_EVENT)
			return -EINVAL;
		return 0;
	case BPF_PROG_TYPE_KPROBE:
		if (prog->expected_attach_type == BPF_TRACE_KPROBE_MULTI &&
		    attach_type != BPF_TRACE_KPROBE_MULTI)
			return -EINVAL;
		if (prog->expected_attach_type == BPF_TRACE_KPROBE_SESSION &&
		    attach_type != BPF_TRACE_KPROBE_SESSION)
			return -EINVAL;
		if (prog->expected_attach_type == BPF_TRACE_UPROBE_MULTI &&
		    attach_type != BPF_TRACE_UPROBE_MULTI)
			return -EINVAL;
		if (prog->expected_attach_type == BPF_TRACE_UPROBE_SESSION &&
		    attach_type != BPF_TRACE_UPROBE_SESSION)
			return -EINVAL;
		if (attach_type != BPF_PERF_EVENT &&
		    attach_type != BPF_TRACE_KPROBE_MULTI &&
		    attach_type != BPF_TRACE_KPROBE_SESSION &&
		    attach_type != BPF_TRACE_UPROBE_MULTI &&
		    attach_type != BPF_TRACE_UPROBE_SESSION)
			return -EINVAL;
		return 0;
	case BPF_PROG_TYPE_SCHED_CLS:
		if (attach_type != BPF_TCX_INGRESS &&
		    attach_type != BPF_TCX_EGRESS &&
		    attach_type != BPF_NETKIT_PRIMARY &&
		    attach_type != BPF_NETKIT_PEER)
			return -EINVAL;
		return 0;
	default:
		ptype = attach_type_to_prog_type(attach_type);
		if (ptype == BPF_PROG_TYPE_UNSPEC || ptype != prog->type)
			return -EINVAL;
		return 0;
	}
}

static bool is_cgroup_prog_type(enum bpf_prog_type ptype, enum bpf_attach_type atype,
				bool check_atype)
{
	switch (ptype) {
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SKB:
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_SOCK_OPS:
		return true;
	case BPF_PROG_TYPE_LSM:
		return check_atype ? atype == BPF_LSM_CGROUP : true;
	default:
		return false;
	}
}

#define BPF_PROG_ATTACH_LAST_FIELD expected_revision