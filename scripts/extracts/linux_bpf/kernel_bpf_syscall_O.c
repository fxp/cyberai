// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 15/37



		switch (prog_type) {
		case BPF_PROG_TYPE_TRACING:
		case BPF_PROG_TYPE_LSM:
		case BPF_PROG_TYPE_STRUCT_OPS:
		case BPF_PROG_TYPE_EXT:
			break;
		default:
			return -EINVAL;
		}
	}

	if (attach_btf && (!btf_id || dst_prog))
		return -EINVAL;

	if (dst_prog && prog_type != BPF_PROG_TYPE_TRACING &&
	    prog_type != BPF_PROG_TYPE_EXT)
		return -EINVAL;

	switch (prog_type) {
	case BPF_PROG_TYPE_CGROUP_SOCK:
		switch (expected_attach_type) {
		case BPF_CGROUP_INET_SOCK_CREATE:
		case BPF_CGROUP_INET_SOCK_RELEASE:
		case BPF_CGROUP_INET4_POST_BIND:
		case BPF_CGROUP_INET6_POST_BIND:
			return 0;
		default:
			return -EINVAL;
		}
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
		switch (expected_attach_type) {
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
			return 0;
		default:
			return -EINVAL;
		}
	case BPF_PROG_TYPE_CGROUP_SKB:
		switch (expected_attach_type) {
		case BPF_CGROUP_INET_INGRESS:
		case BPF_CGROUP_INET_EGRESS:
			return 0;
		default:
			return -EINVAL;
		}
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
		switch (expected_attach_type) {
		case BPF_CGROUP_SETSOCKOPT:
		case BPF_CGROUP_GETSOCKOPT:
			return 0;
		default:
			return -EINVAL;
		}
	case BPF_PROG_TYPE_SK_LOOKUP:
		if (expected_attach_type == BPF_SK_LOOKUP)
			return 0;
		return -EINVAL;
	case BPF_PROG_TYPE_SK_REUSEPORT:
		switch (expected_attach_type) {
		case BPF_SK_REUSEPORT_SELECT:
		case BPF_SK_REUSEPORT_SELECT_OR_MIGRATE:
			return 0;
		default:
			return -EINVAL;
		}
	case BPF_PROG_TYPE_NETFILTER:
		if (expected_attach_type == BPF_NETFILTER)
			return 0;
		return -EINVAL;
	case BPF_PROG_TYPE_SYSCALL:
	case BPF_PROG_TYPE_EXT:
		if (expected_attach_type)
			return -EINVAL;
		fallthrough;
	default:
		return 0;
	}
}

static bool is_net_admin_prog_type(enum bpf_prog_type prog_type)
{
	switch (prog_type) {
	case BPF_PROG_TYPE_SCHED_CLS:
	case BPF_PROG_TYPE_SCHED_ACT:
	case BPF_PROG_TYPE_XDP:
	case BPF_PROG_TYPE_LWT_IN:
	case BPF_PROG_TYPE_LWT_OUT:
	case BPF_PROG_TYPE_LWT_XMIT:
	case BPF_PROG_TYPE_LWT_SEG6LOCAL:
	case BPF_PROG_TYPE_SK_SKB:
	case BPF_PROG_TYPE_SK_MSG:
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_SOCK_OPS:
	case BPF_PROG_TYPE_EXT: /* extends any prog */
	case BPF_PROG_TYPE_NETFILTER:
		return true;
	case BPF_PROG_TYPE_CGROUP_SKB:
		/* always unpriv */
	case BPF_PROG_TYPE_SK_REUSEPORT:
		/* equivalent to SOCKET_FILTER. need CAP_BPF only */
	default:
		return false;
	}
}

static bool is_perfmon_prog_type(enum bpf_prog_type prog_type)
{
	switch (prog_type) {
	case BPF_PROG_TYPE_KPROBE:
	case BPF_PROG_TYPE_TRACEPOINT:
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE:
	case BPF_PROG_TYPE_TRACING:
	case BPF_PROG_TYPE_LSM:
	case BPF_PROG_TYPE_STRUCT_OPS: /* has access to struct sock */
	case BPF_PROG_TYPE_EXT: /* extends any prog */
		return true;
	default:
		return false;
	}
}

static int bpf_prog_verify_signature(struct bpf_prog *prog, union bpf_attr *attr,
				     bool is_kernel)
{
	bpfptr_t usig = make_bpfptr(attr->signature, is_kernel);
	struct bpf_dynptr_kern sig_ptr, insns_ptr;
	struct bpf_key *key = NULL;
	void *sig;
	int err = 0;

	/*
	 * Don't attempt to use kmalloc_large or vmalloc for signatures.
	 * Practical signature for BPF program should be below this limit.
	 */
	if (attr->signature_size > KMALLOC_MAX_CACHE_SIZE)
		return -EINVAL;

	if (system_keyring_id_check(attr->keyring_id) == 0)
		key = bpf_lookup_system_key(attr->keyring_id);
	else
		key = bpf_lookup_user_key(attr->keyring_id, 0);

	if (!key)
		return -EINVAL;

	sig = kvmemdup_bpfptr(usig, attr->signature_size);
	if (IS_ERR(sig)) {
		bpf_key_put(key);
		return PTR_ERR(sig);
	}

	bpf_dynptr_init(&sig_ptr, sig, BPF_DYNPTR_TYPE_LOCAL, 0,
			attr->signature_size);
	bpf_dynptr_init(&insns_ptr, prog->insnsi, BPF_DYNPTR_TYPE_LOCAL, 0,
			prog->len * sizeof(struct bpf_insn));

	err = bpf_verify_pkcs7_signature((struct bpf_dynptr *)&insns_ptr,
					 (struct bpf_dynptr *)&sig_ptr, key);

	bpf_key_put(key);
	kvfree(sig);
	return err;
}

static int bpf_prog_mark_insn_arrays_ready(struct bpf_prog *prog)
{
	int err;
	int i;

	for (i = 0; i < prog->aux->used_map_cnt; i++) {
		if (prog->aux->used_maps[i]->map_type != BPF_MAP_TYPE_INSN_ARRAY)
			continue;