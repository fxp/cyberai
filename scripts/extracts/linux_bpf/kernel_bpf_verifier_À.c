// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 128/132



/* fexit and fmod_ret can't be used to attach to __noreturn functions.
 * Currently, we must manually list all __noreturn functions here. Once a more
 * robust solution is implemented, this workaround can be removed.
 */
BTF_SET_START(noreturn_deny)
#ifdef CONFIG_IA32_EMULATION
BTF_ID(func, __ia32_sys_exit)
BTF_ID(func, __ia32_sys_exit_group)
#endif
#ifdef CONFIG_KUNIT
BTF_ID(func, __kunit_abort)
BTF_ID(func, kunit_try_catch_throw)
#endif
#ifdef CONFIG_MODULES
BTF_ID(func, __module_put_and_kthread_exit)
#endif
#ifdef CONFIG_X86_64
BTF_ID(func, __x64_sys_exit)
BTF_ID(func, __x64_sys_exit_group)
#endif
BTF_ID(func, do_exit)
BTF_ID(func, do_group_exit)
BTF_ID(func, kthread_complete_and_exit)
BTF_ID(func, make_task_dead)
BTF_SET_END(noreturn_deny)

static bool can_be_sleepable(struct bpf_prog *prog)
{
	if (prog->type == BPF_PROG_TYPE_TRACING) {
		switch (prog->expected_attach_type) {
		case BPF_TRACE_FENTRY:
		case BPF_TRACE_FEXIT:
		case BPF_MODIFY_RETURN:
		case BPF_TRACE_ITER:
		case BPF_TRACE_FSESSION:
			return true;
		default:
			return false;
		}
	}
	return prog->type == BPF_PROG_TYPE_LSM ||
	       prog->type == BPF_PROG_TYPE_KPROBE /* only for uprobes */ ||
	       prog->type == BPF_PROG_TYPE_STRUCT_OPS;
}

static int check_attach_btf_id(struct bpf_verifier_env *env)
{
	struct bpf_prog *prog = env->prog;
	struct bpf_prog *tgt_prog = prog->aux->dst_prog;
	struct bpf_attach_target_info tgt_info = {};
	u32 btf_id = prog->aux->attach_btf_id;
	struct bpf_trampoline *tr;
	int ret;
	u64 key;

	if (prog->type == BPF_PROG_TYPE_SYSCALL) {
		if (prog->sleepable)
			/* attach_btf_id checked to be zero already */
			return 0;
		verbose(env, "Syscall programs can only be sleepable\n");
		return -EINVAL;
	}

	if (prog->sleepable && !can_be_sleepable(prog)) {
		verbose(env, "Only fentry/fexit/fsession/fmod_ret, lsm, iter, uprobe, and struct_ops programs can be sleepable\n");
		return -EINVAL;
	}

	if (prog->type == BPF_PROG_TYPE_STRUCT_OPS)
		return check_struct_ops_btf_id(env);

	if (prog->type != BPF_PROG_TYPE_TRACING &&
	    prog->type != BPF_PROG_TYPE_LSM &&
	    prog->type != BPF_PROG_TYPE_EXT)
		return 0;

	ret = bpf_check_attach_target(&env->log, prog, tgt_prog, btf_id, &tgt_info);
	if (ret)
		return ret;

	if (tgt_prog && prog->type == BPF_PROG_TYPE_EXT) {
		/* to make freplace equivalent to their targets, they need to
		 * inherit env->ops and expected_attach_type for the rest of the
		 * verification
		 */
		env->ops = bpf_verifier_ops[tgt_prog->type];
		prog->expected_attach_type = tgt_prog->expected_attach_type;
	}

	/* store info about the attachment target that will be used later */
	prog->aux->attach_func_proto = tgt_info.tgt_type;
	prog->aux->attach_func_name = tgt_info.tgt_name;
	prog->aux->mod = tgt_info.tgt_mod;

	if (tgt_prog) {
		prog->aux->saved_dst_prog_type = tgt_prog->type;
		prog->aux->saved_dst_attach_type = tgt_prog->expected_attach_type;
	}

	if (prog->expected_attach_type == BPF_TRACE_RAW_TP) {
		prog->aux->attach_btf_trace = true;
		return 0;
	} else if (prog->expected_attach_type == BPF_TRACE_ITER) {
		return bpf_iter_prog_supported(prog);
	}

	if (prog->type == BPF_PROG_TYPE_LSM) {
		ret = bpf_lsm_verify_prog(&env->log, prog);
		if (ret < 0)
			return ret;
	} else if (prog->type == BPF_PROG_TYPE_TRACING &&
		   btf_id_set_contains(&btf_id_deny, btf_id)) {
		verbose(env, "Attaching tracing programs to function '%s' is rejected.\n",
			tgt_info.tgt_name);
		return -EINVAL;
	} else if ((prog->expected_attach_type == BPF_TRACE_FEXIT ||
		   prog->expected_attach_type == BPF_TRACE_FSESSION ||
		   prog->expected_attach_type == BPF_MODIFY_RETURN) &&
		   btf_id_set_contains(&noreturn_deny, btf_id)) {
		verbose(env, "Attaching fexit/fsession/fmod_ret to __noreturn function '%s' is rejected.\n",
			tgt_info.tgt_name);
		return -EINVAL;
	}

	key = bpf_trampoline_compute_key(tgt_prog, prog->aux->attach_btf, btf_id);
	tr = bpf_trampoline_get(key, &tgt_info);
	if (!tr)
		return -ENOMEM;

	if (tgt_prog && tgt_prog->aux->tail_call_reachable)
		tr->flags = BPF_TRAMP_F_TAIL_CALL_CTX;

	prog->aux->dst_trampoline = tr;
	return 0;
}

struct btf *bpf_get_btf_vmlinux(void)
{
	if (!btf_vmlinux && IS_ENABLED(CONFIG_DEBUG_INFO_BTF)) {
		mutex_lock(&bpf_verifier_lock);
		if (!btf_vmlinux)
			btf_vmlinux = btf_parse_vmlinux();
		mutex_unlock(&bpf_verifier_lock);
	}
	return btf_vmlinux;
}

/*
 * The add_fd_from_fd_array() is executed only if fd_array_cnt is non-zero. In
 * this case expect that every file descriptor in the array is either a map or
 * a BTF. Everything else is considered to be trash.
 */
static int add_fd_from_fd_array(struct bpf_verifier_env *env, int fd)
{
	struct bpf_map *map;
	struct btf *btf;
	CLASS(fd, f)(fd);
	int err;

	map = __bpf_map_get(f);
	if (!IS_ERR(map)) {
		err = __add_used_map(env, map);
		if (err < 0)
			return err;
		return 0;
	}

	btf = __btf_get_by_fd(f);
	if (!IS_ERR(btf)) {
		btf_get(btf);
		return __add_used_btf(env, btf);
	}