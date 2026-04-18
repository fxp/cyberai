// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 36/37



		if (attr->test.ctx_size_in < prog->aux->max_ctx_offset ||
		    attr->test.ctx_size_in > U16_MAX) {
			bpf_prog_put(prog);
			return -EINVAL;
		}

		run_ctx.bpf_cookie = 0;
		if (!__bpf_prog_enter_sleepable_recur(prog, &run_ctx)) {
			/* recursion detected */
			__bpf_prog_exit_sleepable_recur(prog, 0, &run_ctx);
			bpf_prog_put(prog);
			return -EBUSY;
		}
		attr->test.retval = bpf_prog_run(prog, (void *) (long) attr->test.ctx_in);
		__bpf_prog_exit_sleepable_recur(prog, 0 /* bpf_prog_run does runtime stats */,
						&run_ctx);
		bpf_prog_put(prog);
		return 0;
#endif
	default:
		return ____bpf_sys_bpf(cmd, attr, size);
	}
}
EXPORT_SYMBOL_NS(kern_sys_bpf, "BPF_INTERNAL");

static const struct bpf_func_proto bpf_sys_bpf_proto = {
	.func		= bpf_sys_bpf,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_ANYTHING,
	.arg2_type	= ARG_PTR_TO_MEM | MEM_RDONLY,
	.arg3_type	= ARG_CONST_SIZE,
};

const struct bpf_func_proto * __weak
tracing_prog_func_proto(enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	return bpf_base_func_proto(func_id, prog);
}

BPF_CALL_1(bpf_sys_close, u32, fd)
{
	/* When bpf program calls this helper there should not be
	 * an fdget() without matching completed fdput().
	 * This helper is allowed in the following callchain only:
	 * sys_bpf->prog_test_run->bpf_prog->bpf_sys_close
	 */
	return close_fd(fd);
}

static const struct bpf_func_proto bpf_sys_close_proto = {
	.func		= bpf_sys_close,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_ANYTHING,
};

BPF_CALL_4(bpf_kallsyms_lookup_name, const char *, name, int, name_sz, int, flags, u64 *, res)
{
	*res = 0;
	if (flags)
		return -EINVAL;

	if (name_sz <= 1 || name[name_sz - 1])
		return -EINVAL;

	if (!bpf_dump_raw_ok(current_cred()))
		return -EPERM;

	*res = kallsyms_lookup_name(name);
	return *res ? 0 : -ENOENT;
}

static const struct bpf_func_proto bpf_kallsyms_lookup_name_proto = {
	.func		= bpf_kallsyms_lookup_name,
	.gpl_only	= false,
	.ret_type	= RET_INTEGER,
	.arg1_type	= ARG_PTR_TO_MEM | MEM_RDONLY,
	.arg2_type	= ARG_CONST_SIZE_OR_ZERO,
	.arg3_type	= ARG_ANYTHING,
	.arg4_type	= ARG_PTR_TO_FIXED_SIZE_MEM | MEM_UNINIT | MEM_WRITE | MEM_ALIGNED,
	.arg4_size	= sizeof(u64),
};

static const struct bpf_func_proto *
syscall_prog_func_proto(enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	switch (func_id) {
	case BPF_FUNC_sys_bpf:
		return !bpf_token_capable(prog->aux->token, CAP_PERFMON)
		       ? NULL : &bpf_sys_bpf_proto;
	case BPF_FUNC_btf_find_by_name_kind:
		return &bpf_btf_find_by_name_kind_proto;
	case BPF_FUNC_sys_close:
		return &bpf_sys_close_proto;
	case BPF_FUNC_kallsyms_lookup_name:
		return &bpf_kallsyms_lookup_name_proto;
	default:
		return tracing_prog_func_proto(func_id, prog);
	}
}

const struct bpf_verifier_ops bpf_syscall_verifier_ops = {
	.get_func_proto  = syscall_prog_func_proto,
	.is_valid_access = syscall_prog_is_valid_access,
};

const struct bpf_prog_ops bpf_syscall_prog_ops = {
	.test_run = bpf_prog_test_run_syscall,
};

#ifdef CONFIG_SYSCTL
static int bpf_stats_handler(const struct ctl_table *table, int write,
			     void *buffer, size_t *lenp, loff_t *ppos)
{
	struct static_key *key = (struct static_key *)table->data;
	static int saved_val;
	int val, ret;
	struct ctl_table tmp = {
		.data   = &val,
		.maxlen = sizeof(val),
		.mode   = table->mode,
		.extra1 = SYSCTL_ZERO,
		.extra2 = SYSCTL_ONE,
	};

	if (write && !capable(CAP_SYS_ADMIN))
		return -EPERM;

	mutex_lock(&bpf_stats_enabled_mutex);
	val = saved_val;
	ret = proc_dointvec_minmax(&tmp, write, buffer, lenp, ppos);
	if (write && !ret && val != saved_val) {
		if (val)
			static_key_slow_inc(key);
		else
			static_key_slow_dec(key);
		saved_val = val;
	}
	mutex_unlock(&bpf_stats_enabled_mutex);
	return ret;
}

void __weak unpriv_ebpf_notify(int new_state)
{
}

static int bpf_unpriv_handler(const struct ctl_table *table, int write,
			      void *buffer, size_t *lenp, loff_t *ppos)
{
	int ret, unpriv_enable = *(int *)table->data;
	bool locked_state = unpriv_enable == 1;
	struct ctl_table tmp = *table;

	if (write && !capable(CAP_SYS_ADMIN))
		return -EPERM;

	tmp.data = &unpriv_enable;
	ret = proc_dointvec_minmax(&tmp, write, buffer, lenp, ppos);
	if (write && !ret) {
		if (locked_state && unpriv_enable != 1)
			return -EPERM;
		*(int *)table->data = unpriv_enable;
	}

	if (write)
		unpriv_ebpf_notify(unpriv_enable);

	return ret;
}

static const struct ctl_table bpf_syscall_table[] = {
	{
		.procname	= "unprivileged_bpf_disabled",
		.data		= &sysctl_unprivileged_bpf_disabled,
		.maxlen		= sizeof(sysctl_unprivileged_bpf_disabled),
		.mode		= 0644,
		.proc_handler	= bpf_unpriv_handler,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_TWO,
	},
	{
		.procname	= "bpf_stats_enabled",
		.data		= &bpf_stats_enabled_key.key,
		.mode		= 0644,
		.proc_handler	= bpf_stats_handler,
	},
};