// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 126/132



	if (!btf_id) {
		bpf_log(log, "Tracing programs must provide btf_id\n");
		return -EINVAL;
	}
	btf = tgt_prog ? tgt_prog->aux->btf : prog->aux->attach_btf;
	if (!btf) {
		bpf_log(log,
			"Tracing program can only be attached to another program annotated with BTF\n");
		return -EINVAL;
	}
	t = btf_type_by_id(btf, btf_id);
	if (!t) {
		bpf_log(log, "attach_btf_id %u is invalid\n", btf_id);
		return -EINVAL;
	}
	tname = btf_name_by_offset(btf, t->name_off);
	if (!tname) {
		bpf_log(log, "attach_btf_id %u doesn't have a name\n", btf_id);
		return -EINVAL;
	}
	if (tgt_prog) {
		struct bpf_prog_aux *aux = tgt_prog->aux;
		bool tgt_changes_pkt_data;
		bool tgt_might_sleep;

		if (bpf_prog_is_dev_bound(prog->aux) &&
		    !bpf_prog_dev_bound_match(prog, tgt_prog)) {
			bpf_log(log, "Target program bound device mismatch");
			return -EINVAL;
		}

		for (i = 0; i < aux->func_info_cnt; i++)
			if (aux->func_info[i].type_id == btf_id) {
				subprog = i;
				break;
			}
		if (subprog == -1) {
			bpf_log(log, "Subprog %s doesn't exist\n", tname);
			return -EINVAL;
		}
		if (aux->func && aux->func[subprog]->aux->exception_cb) {
			bpf_log(log,
				"%s programs cannot attach to exception callback\n",
				prog_extension ? "Extension" : "Tracing");
			return -EINVAL;
		}
		conservative = aux->func_info_aux[subprog].unreliable;
		if (prog_extension) {
			if (conservative) {
				bpf_log(log,
					"Cannot replace static functions\n");
				return -EINVAL;
			}
			if (!prog->jit_requested) {
				bpf_log(log,
					"Extension programs should be JITed\n");
				return -EINVAL;
			}
			tgt_changes_pkt_data = aux->func
					       ? aux->func[subprog]->aux->changes_pkt_data
					       : aux->changes_pkt_data;
			if (prog->aux->changes_pkt_data && !tgt_changes_pkt_data) {
				bpf_log(log,
					"Extension program changes packet data, while original does not\n");
				return -EINVAL;
			}

			tgt_might_sleep = aux->func
					  ? aux->func[subprog]->aux->might_sleep
					  : aux->might_sleep;
			if (prog->aux->might_sleep && !tgt_might_sleep) {
				bpf_log(log,
					"Extension program may sleep, while original does not\n");
				return -EINVAL;
			}
		}
		if (!tgt_prog->jited) {
			bpf_log(log, "Can attach to only JITed progs\n");
			return -EINVAL;
		}
		if (prog_tracing) {
			if (aux->attach_tracing_prog) {
				/*
				 * Target program is an fentry/fexit which is already attached
				 * to another tracing program. More levels of nesting
				 * attachment are not allowed.
				 */
				bpf_log(log, "Cannot nest tracing program attach more than once\n");
				return -EINVAL;
			}
		} else if (tgt_prog->type == prog->type) {
			/*
			 * To avoid potential call chain cycles, prevent attaching of a
			 * program extension to another extension. It's ok to attach
			 * fentry/fexit to extension program.
			 */
			bpf_log(log, "Cannot recursively attach\n");
			return -EINVAL;
		}
		if (tgt_prog->type == BPF_PROG_TYPE_TRACING &&
		    prog_extension &&
		    (tgt_prog->expected_attach_type == BPF_TRACE_FENTRY ||
		     tgt_prog->expected_attach_type == BPF_TRACE_FEXIT ||
		     tgt_prog->expected_attach_type == BPF_TRACE_FSESSION)) {
			/* Program extensions can extend all program types
			 * except fentry/fexit. The reason is the following.
			 * The fentry/fexit programs are used for performance
			 * analysis, stats and can be attached to any program
			 * type. When extension program is replacing XDP function
			 * it is necessary to allow performance analysis of all
			 * functions. Both original XDP program and its program
			 * extension. Hence attaching fentry/fexit to
			 * BPF_PROG_TYPE_EXT is allowed. If extending of
			 * fentry/fexit was allowed it would be possible to create
			 * long call chain fentry->extension->fentry->extension
			 * beyond reasonable stack size. Hence extending fentry
			 * is not allowed.
			 */
			bpf_log(log, "Cannot extend fentry/fexit/fsession\n");
			return -EINVAL;
		}
	} else {
		if (prog_extension) {
			bpf_log(log, "Cannot replace kernel functions\n");
			return -EINVAL;
		}
	}

	switch (prog->expected_attach_type) {
	case BPF_TRACE_RAW_TP:
		if (tgt_prog) {
			bpf_log(log,
				"Only FENTRY/FEXIT/FSESSION progs are attachable to another BPF prog\n");
			return -EINVAL;
		}
		if (!btf_type_is_typedef(t)) {
			bpf_log(log, "attach_btf_id %u is not a typedef\n",
				btf_id);
			return -EINVAL;
		}
		if (strncmp(prefix, tname, sizeof(prefix) - 1)) {
			bpf_log(log, "attach_btf_id %u points to wrong type name %s\n",
				btf_id, tname);
			return -EINVAL;
		}
		tname += sizeof(prefix) - 1;

		/* The func_proto of "btf_trace_##tname" is generated from typedef without argument
		 * names. Thus using bpf_raw_event_map to get argument names.
		 */
		btp = bpf_get_raw_tracepoint(tname);
		if (!btp)
			return -EINVAL;
		fname = kallsyms_lookup((unsigned long)btp->bpf_func, NULL, NULL, NULL,
					trace_symbol);
		bpf_put_raw_tracepoint(btp);