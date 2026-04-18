// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 125/132



	member_off = __btf_member_bit_offset(t, member) / 8;
	err = bpf_struct_ops_supported(st_ops, member_off);
	if (err) {
		verbose(env, "attach to unsupported member %s of struct %s\n",
			mname, st_ops->name);
		return err;
	}

	if (st_ops->check_member) {
		err = st_ops->check_member(t, member, prog);

		if (err) {
			verbose(env, "attach to unsupported member %s of struct %s\n",
				mname, st_ops->name);
			return err;
		}
	}

	if (prog->aux->priv_stack_requested && !bpf_jit_supports_private_stack()) {
		verbose(env, "Private stack not supported by jit\n");
		return -EACCES;
	}

	for (i = 0; i < st_ops_desc->arg_info[member_idx].cnt; i++) {
		if (st_ops_desc->arg_info[member_idx].info[i].refcounted) {
			has_refcounted_arg = true;
			break;
		}
	}

	/* Tail call is not allowed for programs with refcounted arguments since we
	 * cannot guarantee that valid refcounted kptrs will be passed to the callee.
	 */
	for (i = 0; i < env->subprog_cnt; i++) {
		if (has_refcounted_arg && env->subprog_info[i].has_tail_call) {
			verbose(env, "program with __ref argument cannot tail call\n");
			return -EINVAL;
		}
	}

	prog->aux->st_ops = st_ops;
	prog->aux->attach_st_ops_member_off = member_off;

	prog->aux->attach_func_proto = func_proto;
	prog->aux->attach_func_name = mname;
	env->ops = st_ops->verifier_ops;

	return bpf_prog_ctx_arg_info_init(prog, st_ops_desc->arg_info[member_idx].info,
					  st_ops_desc->arg_info[member_idx].cnt);
}
#define SECURITY_PREFIX "security_"

#ifdef CONFIG_FUNCTION_ERROR_INJECTION

/* list of non-sleepable functions that are otherwise on
 * ALLOW_ERROR_INJECTION list
 */
BTF_SET_START(btf_non_sleepable_error_inject)
/* Three functions below can be called from sleepable and non-sleepable context.
 * Assume non-sleepable from bpf safety point of view.
 */
BTF_ID(func, __filemap_add_folio)
#ifdef CONFIG_FAIL_PAGE_ALLOC
BTF_ID(func, should_fail_alloc_page)
#endif
#ifdef CONFIG_FAILSLAB
BTF_ID(func, should_failslab)
#endif
BTF_SET_END(btf_non_sleepable_error_inject)

static int check_non_sleepable_error_inject(u32 btf_id)
{
	return btf_id_set_contains(&btf_non_sleepable_error_inject, btf_id);
}

static int check_attach_sleepable(u32 btf_id, unsigned long addr, const char *func_name)
{
	/* fentry/fexit/fmod_ret progs can be sleepable if they are
	 * attached to ALLOW_ERROR_INJECTION and are not in denylist.
	 */
	if (!check_non_sleepable_error_inject(btf_id) &&
	    within_error_injection_list(addr))
		return 0;

	return -EINVAL;
}

static int check_attach_modify_return(unsigned long addr, const char *func_name)
{
	if (within_error_injection_list(addr) ||
	    !strncmp(SECURITY_PREFIX, func_name, sizeof(SECURITY_PREFIX) - 1))
		return 0;

	return -EINVAL;
}

#else

/* Unfortunately, the arch-specific prefixes are hard-coded in arch syscall code
 * so we need to hard-code them, too. Ftrace has arch_syscall_match_sym_name()
 * but that just compares two concrete function names.
 */
static bool has_arch_syscall_prefix(const char *func_name)
{
#if defined(__x86_64__)
	return !strncmp(func_name, "__x64_", 6);
#elif defined(__i386__)
	return !strncmp(func_name, "__ia32_", 7);
#elif defined(__s390x__)
	return !strncmp(func_name, "__s390x_", 8);
#elif defined(__aarch64__)
	return !strncmp(func_name, "__arm64_", 8);
#elif defined(__riscv)
	return !strncmp(func_name, "__riscv_", 8);
#elif defined(__powerpc__) || defined(__powerpc64__)
	return !strncmp(func_name, "sys_", 4);
#elif defined(__loongarch__)
	return !strncmp(func_name, "sys_", 4);
#else
	return false;
#endif
}

/* Without error injection, allow sleepable and fmod_ret progs on syscalls. */

static int check_attach_sleepable(u32 btf_id, unsigned long addr, const char *func_name)
{
	if (has_arch_syscall_prefix(func_name))
		return 0;

	return -EINVAL;
}

static int check_attach_modify_return(unsigned long addr, const char *func_name)
{
	if (has_arch_syscall_prefix(func_name) ||
	    !strncmp(SECURITY_PREFIX, func_name, sizeof(SECURITY_PREFIX) - 1))
		return 0;

	return -EINVAL;
}

#endif /* CONFIG_FUNCTION_ERROR_INJECTION */

int bpf_check_attach_target(struct bpf_verifier_log *log,
			    const struct bpf_prog *prog,
			    const struct bpf_prog *tgt_prog,
			    u32 btf_id,
			    struct bpf_attach_target_info *tgt_info)
{
	bool prog_extension = prog->type == BPF_PROG_TYPE_EXT;
	bool prog_tracing = prog->type == BPF_PROG_TYPE_TRACING;
	char trace_symbol[KSYM_SYMBOL_LEN];
	const char prefix[] = "btf_trace_";
	struct bpf_raw_event_map *btp;
	int ret = 0, subprog = -1, i;
	const struct btf_type *t;
	bool conservative = true;
	const char *tname, *fname;
	struct btf *btf;
	long addr = 0;
	struct module *mod = NULL;