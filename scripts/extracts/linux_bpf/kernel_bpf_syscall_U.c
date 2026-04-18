// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 21/37



	/* There are a few possible cases here:
	 *
	 * - if prog->aux->dst_trampoline is set, the program was just loaded
	 *   and not yet attached to anything, so we can use the values stored
	 *   in prog->aux
	 *
	 * - if prog->aux->dst_trampoline is NULL, the program has already been
	 *   attached to a target and its initial target was cleared (below)
	 *
	 * - if tgt_prog != NULL, the caller specified tgt_prog_fd +
	 *   target_btf_id using the link_create API.
	 *
	 * - if tgt_prog == NULL when this function was called using the old
	 *   raw_tracepoint_open API, and we need a target from prog->aux
	 *
	 * - if prog->aux->dst_trampoline and tgt_prog is NULL, the program
	 *   was detached and is going for re-attachment.
	 *
	 * - if prog->aux->dst_trampoline is NULL and tgt_prog and prog->aux->attach_btf
	 *   are NULL, then program was already attached and user did not provide
	 *   tgt_prog_fd so we have no way to find out or create trampoline
	 */
	if (!prog->aux->dst_trampoline && !tgt_prog) {
		/*
		 * Allow re-attach for TRACING and LSM programs. If it's
		 * currently linked, bpf_trampoline_link_prog will fail.
		 * EXT programs need to specify tgt_prog_fd, so they
		 * re-attach in separate code path.
		 */
		if (prog->type != BPF_PROG_TYPE_TRACING &&
		    prog->type != BPF_PROG_TYPE_LSM) {
			err = -EINVAL;
			goto out_unlock;
		}
		/* We can allow re-attach only if we have valid attach_btf. */
		if (!prog->aux->attach_btf) {
			err = -EINVAL;
			goto out_unlock;
		}
		btf_id = prog->aux->attach_btf_id;
		key = bpf_trampoline_compute_key(NULL, prog->aux->attach_btf, btf_id);
	}

	if (!prog->aux->dst_trampoline ||
	    (key && key != prog->aux->dst_trampoline->key)) {
		/* If there is no saved target, or the specified target is
		 * different from the destination specified at load time, we
		 * need a new trampoline and a check for compatibility
		 */
		struct bpf_attach_target_info tgt_info = {};

		err = bpf_check_attach_target(NULL, prog, tgt_prog, btf_id,
					      &tgt_info);
		if (err)
			goto out_unlock;

		if (tgt_info.tgt_mod) {
			module_put(prog->aux->mod);
			prog->aux->mod = tgt_info.tgt_mod;
		}

		tr = bpf_trampoline_get(key, &tgt_info);
		if (!tr) {
			err = -ENOMEM;
			goto out_unlock;
		}
	} else {
		/* The caller didn't specify a target, or the target was the
		 * same as the destination supplied during program load. This
		 * means we can reuse the trampoline and reference from program
		 * load time, and there is no need to allocate a new one. This
		 * can only happen once for any program, as the saved values in
		 * prog->aux are cleared below.
		 */
		tr = prog->aux->dst_trampoline;
		tgt_prog = prog->aux->dst_prog;
	}
	/*
	 * It is to prevent modifying struct pt_regs via kprobe_write_ctx=true
	 * freplace prog. Without this check, kprobe_write_ctx=true freplace
	 * prog is allowed to attach to kprobe_write_ctx=false kprobe prog, and
	 * then modify the registers of the kprobe prog's target kernel
	 * function.
	 *
	 * This also blocks the combination of uprobe+freplace, because it is
	 * unable to recognize the use of the tgt_prog as an uprobe or a kprobe
	 * by tgt_prog itself. At attach time, uprobe/kprobe is recognized by
	 * the target perf event flags in __perf_event_set_bpf_prog().
	 */
	if (prog->type == BPF_PROG_TYPE_EXT &&
	    prog->aux->kprobe_write_ctx != tgt_prog->aux->kprobe_write_ctx) {
		err = -EINVAL;
		goto out_unlock;
	}

	err = bpf_link_prime(&link->link.link, &link_primer);
	if (err)
		goto out_unlock;

	err = bpf_trampoline_link_prog(&link->link, tr, tgt_prog);
	if (err) {
		bpf_link_cleanup(&link_primer);
		link = NULL;
		goto out_unlock;
	}

	link->tgt_prog = tgt_prog;
	link->trampoline = tr;

	/* Always clear the trampoline and target prog from prog->aux to make
	 * sure the original attach destination is not kept alive after a
	 * program is (re-)attached to another target.
	 */
	if (prog->aux->dst_prog &&
	    (tgt_prog_fd || tr != prog->aux->dst_trampoline))
		/* got extra prog ref from syscall, or attaching to different prog */
		bpf_prog_put(prog->aux->dst_prog);
	if (prog->aux->dst_trampoline && tr != prog->aux->dst_trampoline)
		/* we allocated a new trampoline, so free the old one */
		bpf_trampoline_put(prog->aux->dst_trampoline);

	prog->aux->dst_prog = NULL;
	prog->aux->dst_trampoline = NULL;
	mutex_unlock(&prog->aux->dst_mutex);

	return bpf_link_settle(&link_primer);
out_unlock:
	if (tr && tr != prog->aux->dst_trampoline)
		bpf_trampoline_put(tr);
	mutex_unlock(&prog->aux->dst_mutex);
	kfree(link);
out_put_prog:
	if (tgt_prog_fd && tgt_prog)
		bpf_prog_put(tgt_prog);
	return err;
}

static void bpf_raw_tp_link_release(struct bpf_link *link)
{
	struct bpf_raw_tp_link *raw_tp =
		container_of(link, struct bpf_raw_tp_link, link);

	bpf_probe_unregister(raw_tp->btp, raw_tp);
	bpf_put_raw_tracepoint(raw_tp->btp);
}