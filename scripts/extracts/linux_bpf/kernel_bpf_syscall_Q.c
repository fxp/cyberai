// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 17/37



	prog->orig_prog = NULL;
	prog->jited = 0;

	atomic64_set(&prog->aux->refcnt, 1);

	if (bpf_prog_is_dev_bound(prog->aux)) {
		err = bpf_prog_dev_bound_init(prog, attr);
		if (err)
			goto free_prog;
	}

	if (type == BPF_PROG_TYPE_EXT && dst_prog &&
	    bpf_prog_is_dev_bound(dst_prog->aux)) {
		err = bpf_prog_dev_bound_inherit(prog, dst_prog);
		if (err)
			goto free_prog;
	}

	/*
	 * Bookkeeping for managing the program attachment chain.
	 *
	 * It might be tempting to set attach_tracing_prog flag at the attachment
	 * time, but this will not prevent from loading bunch of tracing prog
	 * first, then attach them one to another.
	 *
	 * The flag attach_tracing_prog is set for the whole program lifecycle, and
	 * doesn't have to be cleared in bpf_tracing_link_release, since tracing
	 * programs cannot change attachment target.
	 */
	if (type == BPF_PROG_TYPE_TRACING && dst_prog &&
	    dst_prog->type == BPF_PROG_TYPE_TRACING) {
		prog->aux->attach_tracing_prog = true;
	}

	/* find program type: socket_filter vs tracing_filter */
	err = find_prog_type(type, prog);
	if (err < 0)
		goto free_prog;

	prog->aux->load_time = ktime_get_boottime_ns();
	err = bpf_obj_name_cpy(prog->aux->name, attr->prog_name,
			       sizeof(attr->prog_name));
	if (err < 0)
		goto free_prog;

	err = security_bpf_prog_load(prog, attr, token, uattr.is_kernel);
	if (err)
		goto free_prog_sec;

	/* run eBPF verifier */
	err = bpf_check(&prog, attr, uattr, uattr_size);
	if (err < 0)
		goto free_used_maps;

	err = bpf_prog_mark_insn_arrays_ready(prog);
	if (err < 0)
		goto free_used_maps;

	err = bpf_prog_alloc_id(prog);
	if (err)
		goto free_used_maps;

	/* Upon success of bpf_prog_alloc_id(), the BPF prog is
	 * effectively publicly exposed. However, retrieving via
	 * bpf_prog_get_fd_by_id() will take another reference,
	 * therefore it cannot be gone underneath us.
	 *
	 * Only for the time /after/ successful bpf_prog_new_fd()
	 * and before returning to userspace, we might just hold
	 * one reference and any parallel close on that fd could
	 * rip everything out. Hence, below notifications must
	 * happen before bpf_prog_new_fd().
	 *
	 * Also, any failure handling from this point onwards must
	 * be using bpf_prog_put() given the program is exposed.
	 */
	bpf_prog_kallsyms_add(prog);
	perf_event_bpf_event(prog, PERF_BPF_EVENT_PROG_LOAD, 0);
	bpf_audit_prog(prog, BPF_AUDIT_LOAD);

	err = bpf_prog_new_fd(prog);
	if (err < 0)
		bpf_prog_put(prog);
	return err;

free_used_maps:
	/* In case we have subprogs, we need to wait for a grace
	 * period before we can tear down JIT memory since symbols
	 * are already exposed under kallsyms.
	 */
	__bpf_prog_put_noref(prog, prog->aux->real_func_cnt);
	return err;

free_prog_sec:
	security_bpf_prog_free(prog);
free_prog:
	free_uid(prog->aux->user);
	if (prog->aux->attach_btf)
		btf_put(prog->aux->attach_btf);
	bpf_prog_free(prog);
put_token:
	bpf_token_put(token);
	return err;
}

#define BPF_OBJ_LAST_FIELD path_fd

static int bpf_obj_pin(const union bpf_attr *attr)
{
	int path_fd;

	if (CHECK_ATTR(BPF_OBJ) || attr->file_flags & ~BPF_F_PATH_FD)
		return -EINVAL;

	/* path_fd has to be accompanied by BPF_F_PATH_FD flag */
	if (!(attr->file_flags & BPF_F_PATH_FD) && attr->path_fd)
		return -EINVAL;

	path_fd = attr->file_flags & BPF_F_PATH_FD ? attr->path_fd : AT_FDCWD;
	return bpf_obj_pin_user(attr->bpf_fd, path_fd,
				u64_to_user_ptr(attr->pathname));
}

static int bpf_obj_get(const union bpf_attr *attr)
{
	int path_fd;

	if (CHECK_ATTR(BPF_OBJ) || attr->bpf_fd != 0 ||
	    attr->file_flags & ~(BPF_OBJ_FLAG_MASK | BPF_F_PATH_FD))
		return -EINVAL;

	/* path_fd has to be accompanied by BPF_F_PATH_FD flag */
	if (!(attr->file_flags & BPF_F_PATH_FD) && attr->path_fd)
		return -EINVAL;

	path_fd = attr->file_flags & BPF_F_PATH_FD ? attr->path_fd : AT_FDCWD;
	return bpf_obj_get_user(path_fd, u64_to_user_ptr(attr->pathname),
				attr->file_flags);
}

/* bpf_link_init_sleepable() allows to specify whether BPF link itself has
 * "sleepable" semantics, which normally would mean that BPF link's attach
 * hook can dereference link or link's underlying program for some time after
 * detachment due to RCU Tasks Trace-based lifetime protection scheme.
 * BPF program itself can be non-sleepable, yet, because it's transitively
 * reachable through BPF link, its freeing has to be delayed until after RCU
 * Tasks Trace GP.
 */
void bpf_link_init_sleepable(struct bpf_link *link, enum bpf_link_type type,
			     const struct bpf_link_ops *ops, struct bpf_prog *prog,
			     enum bpf_attach_type attach_type, bool sleepable)
{
	WARN_ON(ops->dealloc && ops->dealloc_deferred);
	atomic64_set(&link->refcnt, 1);
	link->type = type;
	link->sleepable = sleepable;
	link->id = 0;
	link->ops = ops;
	link->prog = prog;
	link->attach_type = attach_type;
}