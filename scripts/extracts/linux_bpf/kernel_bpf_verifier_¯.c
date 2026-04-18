// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 111/132



	/* check whether implicit source operand (register R6) is readable */
	err = check_reg_arg(env, ctx_reg, SRC_OP);
	if (err)
		return err;

	/* Disallow usage of BPF_LD_[ABS|IND] with reference tracking, as
	 * gen_ld_abs() may terminate the program at runtime, leading to
	 * reference leak.
	 */
	err = check_resource_leak(env, false, true, "BPF_LD_[ABS|IND]");
	if (err)
		return err;

	if (regs[ctx_reg].type != PTR_TO_CTX) {
		verbose(env,
			"at the time of BPF_LD_ABS|IND R6 != pointer to skb\n");
		return -EINVAL;
	}

	if (mode == BPF_IND) {
		/* check explicit source operand */
		err = check_reg_arg(env, insn->src_reg, SRC_OP);
		if (err)
			return err;
	}

	err = check_ptr_off_reg(env, &regs[ctx_reg], ctx_reg);
	if (err < 0)
		return err;

	/* reset caller saved regs to unreadable */
	for (i = 0; i < CALLER_SAVED_REGS; i++) {
		bpf_mark_reg_not_init(env, &regs[caller_saved[i]]);
		check_reg_arg(env, caller_saved[i], DST_OP_NO_MARK);
	}

	/* mark destination R0 register as readable, since it contains
	 * the value fetched from the packet.
	 * Already marked as written above.
	 */
	mark_reg_unknown(env, regs, BPF_REG_0);
	/* ld_abs load up to 32-bit skb data. */
	regs[BPF_REG_0].subreg_def = env->insn_idx + 1;
	/*
	 * See bpf_gen_ld_abs() which emits a hidden BPF_EXIT with r0=0
	 * which must be explored by the verifier when in a subprog.
	 */
	if (env->cur_state->curframe) {
		struct bpf_verifier_state *branch;

		mark_reg_scratched(env, BPF_REG_0);
		branch = push_stack(env, env->insn_idx + 1, env->insn_idx, false);
		if (IS_ERR(branch))
			return PTR_ERR(branch);
		mark_reg_known_zero(env, regs, BPF_REG_0);
		err = prepare_func_exit(env, &env->insn_idx);
		if (err)
			return err;
		env->insn_idx--;
	}
	return 0;
}


static bool return_retval_range(struct bpf_verifier_env *env, struct bpf_retval_range *range)
{
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);

	/* Default return value range. */
	*range = retval_range(0, 1);

	switch (prog_type) {
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
		switch (env->prog->expected_attach_type) {
		case BPF_CGROUP_UDP4_RECVMSG:
		case BPF_CGROUP_UDP6_RECVMSG:
		case BPF_CGROUP_UNIX_RECVMSG:
		case BPF_CGROUP_INET4_GETPEERNAME:
		case BPF_CGROUP_INET6_GETPEERNAME:
		case BPF_CGROUP_UNIX_GETPEERNAME:
		case BPF_CGROUP_INET4_GETSOCKNAME:
		case BPF_CGROUP_INET6_GETSOCKNAME:
		case BPF_CGROUP_UNIX_GETSOCKNAME:
			*range = retval_range(1, 1);
			break;
		case BPF_CGROUP_INET4_BIND:
		case BPF_CGROUP_INET6_BIND:
			*range = retval_range(0, 3);
			break;
		default:
			break;
		}
		break;
	case BPF_PROG_TYPE_CGROUP_SKB:
		if (env->prog->expected_attach_type == BPF_CGROUP_INET_EGRESS)
			*range = retval_range(0, 3);
		break;
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_SOCK_OPS:
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
		break;
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
		if (!env->prog->aux->attach_btf_id)
			return false;
		*range = retval_range(0, 0);
		break;
	case BPF_PROG_TYPE_TRACING:
		switch (env->prog->expected_attach_type) {
		case BPF_TRACE_FENTRY:
		case BPF_TRACE_FEXIT:
		case BPF_TRACE_FSESSION:
			*range = retval_range(0, 0);
			break;
		case BPF_TRACE_RAW_TP:
		case BPF_MODIFY_RETURN:
			return false;
		case BPF_TRACE_ITER:
		default:
			break;
		}
		break;
	case BPF_PROG_TYPE_KPROBE:
		switch (env->prog->expected_attach_type) {
		case BPF_TRACE_KPROBE_SESSION:
		case BPF_TRACE_UPROBE_SESSION:
			break;
		default:
			return false;
		}
		break;
	case BPF_PROG_TYPE_SK_LOOKUP:
		*range = retval_range(SK_DROP, SK_PASS);
		break;

	case BPF_PROG_TYPE_LSM:
		if (env->prog->expected_attach_type != BPF_LSM_CGROUP) {
			/* no range found, any return value is allowed */
			if (!get_func_retval_range(env->prog, range))
				return false;
			/* no restricted range, any return value is allowed */
			if (range->minval == S32_MIN && range->maxval == S32_MAX)
				return false;
			range->return_32bit = true;
		} else if (!env->prog->aux->attach_func_proto->type) {
			/* Make sure programs that attach to void
			 * hooks don't try to modify return value.
			 */
			*range = retval_range(1, 1);
		}
		break;

	case BPF_PROG_TYPE_NETFILTER:
		*range = retval_range(NF_DROP, NF_ACCEPT);
		break;
	case BPF_PROG_TYPE_STRUCT_OPS:
		*range = retval_range(0, 0);
		break;
	case BPF_PROG_TYPE_EXT:
		/* freplace program can return anything as its return value
		 * depends on the to-be-replaced kernel func or bpf program.
		 */
	default:
		return false;
	}

	/* Continue calculating. */

	return true;
}

static bool program_returns_void(struct bpf_verifier_env *env)
{
	const struct bpf_prog *prog = env->prog;
	enum bpf_prog_type prog_type = prog->type;