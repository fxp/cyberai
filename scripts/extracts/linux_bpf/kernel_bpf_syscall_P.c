// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 16/37



		err = bpf_insn_array_ready(prog->aux->used_maps[i]);
		if (err)
			return err;
	}

	return 0;
}

/* last field in 'union bpf_attr' used by this command */
#define BPF_PROG_LOAD_LAST_FIELD keyring_id

static int bpf_prog_load(union bpf_attr *attr, bpfptr_t uattr, u32 uattr_size)
{
	enum bpf_prog_type type = attr->prog_type;
	struct bpf_prog *prog, *dst_prog = NULL;
	struct btf *attach_btf = NULL;
	struct bpf_token *token = NULL;
	bool bpf_cap;
	int err;
	char license[128];

	if (CHECK_ATTR(BPF_PROG_LOAD))
		return -EINVAL;

	if (attr->prog_flags & ~(BPF_F_STRICT_ALIGNMENT |
				 BPF_F_ANY_ALIGNMENT |
				 BPF_F_TEST_STATE_FREQ |
				 BPF_F_SLEEPABLE |
				 BPF_F_TEST_RND_HI32 |
				 BPF_F_XDP_HAS_FRAGS |
				 BPF_F_XDP_DEV_BOUND_ONLY |
				 BPF_F_TEST_REG_INVARIANTS |
				 BPF_F_TOKEN_FD))
		return -EINVAL;

	bpf_prog_load_fixup_attach_type(attr);

	if (attr->prog_flags & BPF_F_TOKEN_FD) {
		token = bpf_token_get_from_fd(attr->prog_token_fd);
		if (IS_ERR(token))
			return PTR_ERR(token);
		/* if current token doesn't grant prog loading permissions,
		 * then we can't use this token, so ignore it and rely on
		 * system-wide capabilities checks
		 */
		if (!bpf_token_allow_cmd(token, BPF_PROG_LOAD) ||
		    !bpf_token_allow_prog_type(token, attr->prog_type,
					       attr->expected_attach_type)) {
			bpf_token_put(token);
			token = NULL;
		}
	}

	bpf_cap = bpf_token_capable(token, CAP_BPF);
	err = -EPERM;

	if (!IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) &&
	    (attr->prog_flags & BPF_F_ANY_ALIGNMENT) &&
	    !bpf_cap)
		goto put_token;

	/* Intent here is for unprivileged_bpf_disabled to block BPF program
	 * creation for unprivileged users; other actions depend
	 * on fd availability and access to bpffs, so are dependent on
	 * object creation success. Even with unprivileged BPF disabled,
	 * capability checks are still carried out for these
	 * and other operations.
	 */
	if (sysctl_unprivileged_bpf_disabled && !bpf_cap)
		goto put_token;

	if (attr->insn_cnt == 0 ||
	    attr->insn_cnt > (bpf_cap ? BPF_COMPLEXITY_LIMIT_INSNS : BPF_MAXINSNS)) {
		err = -E2BIG;
		goto put_token;
	}
	if (type != BPF_PROG_TYPE_SOCKET_FILTER &&
	    type != BPF_PROG_TYPE_CGROUP_SKB &&
	    !bpf_cap)
		goto put_token;

	if (is_net_admin_prog_type(type) && !bpf_token_capable(token, CAP_NET_ADMIN))
		goto put_token;
	if (is_perfmon_prog_type(type) && !bpf_token_capable(token, CAP_PERFMON))
		goto put_token;

	/* attach_prog_fd/attach_btf_obj_fd can specify fd of either bpf_prog
	 * or btf, we need to check which one it is
	 */
	if (attr->attach_prog_fd) {
		dst_prog = bpf_prog_get(attr->attach_prog_fd);
		if (IS_ERR(dst_prog)) {
			dst_prog = NULL;
			attach_btf = btf_get_by_fd(attr->attach_btf_obj_fd);
			if (IS_ERR(attach_btf)) {
				err = -EINVAL;
				goto put_token;
			}
			if (!btf_is_kernel(attach_btf)) {
				/* attaching through specifying bpf_prog's BTF
				 * objects directly might be supported eventually
				 */
				btf_put(attach_btf);
				err = -ENOTSUPP;
				goto put_token;
			}
		}
	} else if (attr->attach_btf_id) {
		/* fall back to vmlinux BTF, if BTF type ID is specified */
		attach_btf = bpf_get_btf_vmlinux();
		if (IS_ERR(attach_btf)) {
			err = PTR_ERR(attach_btf);
			goto put_token;
		}
		if (!attach_btf) {
			err = -EINVAL;
			goto put_token;
		}
		btf_get(attach_btf);
	}

	if (bpf_prog_load_check_attach(type, attr->expected_attach_type,
				       attach_btf, attr->attach_btf_id,
				       dst_prog)) {
		if (dst_prog)
			bpf_prog_put(dst_prog);
		if (attach_btf)
			btf_put(attach_btf);
		err = -EINVAL;
		goto put_token;
	}

	/* plain bpf_prog allocation */
	prog = bpf_prog_alloc(bpf_prog_size(attr->insn_cnt), GFP_USER);
	if (!prog) {
		if (dst_prog)
			bpf_prog_put(dst_prog);
		if (attach_btf)
			btf_put(attach_btf);
		err = -EINVAL;
		goto put_token;
	}

	prog->expected_attach_type = attr->expected_attach_type;
	prog->sleepable = !!(attr->prog_flags & BPF_F_SLEEPABLE);
	prog->aux->attach_btf = attach_btf;
	prog->aux->attach_btf_id = attr->attach_btf_id;
	prog->aux->dst_prog = dst_prog;
	prog->aux->dev_bound = !!attr->prog_ifindex;
	prog->aux->xdp_has_frags = attr->prog_flags & BPF_F_XDP_HAS_FRAGS;

	/* move token into prog->aux, reuse taken refcnt */
	prog->aux->token = token;
	token = NULL;

	prog->aux->user = get_current_user();
	prog->len = attr->insn_cnt;

	err = -EFAULT;
	if (copy_from_bpfptr(prog->insns,
			     make_bpfptr(attr->insns, uattr.is_kernel),
			     bpf_prog_insn_size(prog)) != 0)
		goto free_prog;
	/* copy eBPF program license from user space */
	if (strncpy_from_bpfptr(license,
				make_bpfptr(attr->license, uattr.is_kernel),
				sizeof(license) - 1) < 0)
		goto free_prog;
	license[sizeof(license) - 1] = 0;

	/* eBPF programs must be GPL compatible to use GPL-ed functions */
	prog->gpl_compatible = license_is_gpl_compatible(license) ? 1 : 0;

	if (attr->signature) {
		err = bpf_prog_verify_signature(prog, attr, uattr.is_kernel);
		if (err)
			goto free_prog;
	}