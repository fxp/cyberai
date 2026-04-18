// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 39/55

", 0x1 },
	{ "xfs_blockgc_worker", 0x1 },
	{ "xfs_blockgc_flush_all", 0x1 },
	/* xfs_scrub */
	{ "xchk_nlinks_live_update", 0x10 },
	/* xfs_scrub from xchk_metapath_class */
	{ "xchk_metapath_lookup", 0x100 },
	/* nfsd */
	{ "nfsd_dirent", 0x1 },
	{ "nfsd_file_acquire", 0x1001 },
	{ "nfsd_file_insert_err", 0x1 },
	{ "nfsd_file_cons_err", 0x1 },
	/* nfs4 */
	{ "nfs4_setup_sequence", 0x1 },
	{ "pnfs_update_layout", 0x10000 },
	{ "nfs4_inode_callback_event", 0x200 },
	{ "nfs4_inode_stateid_callback_event", 0x200 },
	/* nfs from pnfs_layout_event */
	{ "pnfs_mds_fallback_pg_init_read", 0x10000 },
	{ "pnfs_mds_fallback_pg_init_write", 0x10000 },
	{ "pnfs_mds_fallback_pg_get_mirror_count", 0x10000 },
	{ "pnfs_mds_fallback_read_done", 0x10000 },
	{ "pnfs_mds_fallback_write_done", 0x10000 },
	{ "pnfs_mds_fallback_read_pagelist", 0x10000 },
	{ "pnfs_mds_fallback_write_pagelist", 0x10000 },
	/* coda */
	{ "coda_dec_pic_run", 0x10 },
	{ "coda_dec_pic_done", 0x10 },
	/* cfg80211 */
	{ "cfg80211_scan_done", 0x11 },
	{ "rdev_set_coalesce", 0x10 },
	{ "cfg80211_report_wowlan_wakeup", 0x100 },
	{ "cfg80211_inform_bss_frame", 0x100 },
	{ "cfg80211_michael_mic_failure", 0x10000 },
	/* cfg80211 from wiphy_work_event */
	{ "wiphy_work_queue", 0x10 },
	{ "wiphy_work_run", 0x10 },
	{ "wiphy_work_cancel", 0x10 },
	{ "wiphy_work_flush", 0x10 },
	/* hugetlbfs */
	{ "hugetlbfs_alloc_inode", 0x10 },
	/* spufs */
	{ "spufs_context", 0x10 },
	/* kvm_hv */
	{ "kvm_page_fault_enter", 0x100 },
	/* dpu */
	{ "dpu_crtc_setup_mixer", 0x100 },
	/* binder */
	{ "binder_transaction", 0x100 },
	/* bcachefs */
	{ "btree_path_free", 0x100 },
	/* hfi1_tx */
	{ "hfi1_sdma_progress", 0x1000 },
	/* iptfs */
	{ "iptfs_ingress_postq_event", 0x1000 },
	/* neigh */
	{ "neigh_update", 0x10 },
	/* snd_firewire_lib */
	{ "amdtp_packet", 0x100 },
};

bool btf_ctx_access(int off, int size, enum bpf_access_type type,
		    const struct bpf_prog *prog,
		    struct bpf_insn_access_aux *info)
{
	const struct btf_type *t = prog->aux->attach_func_proto;
	struct bpf_prog *tgt_prog = prog->aux->dst_prog;
	struct btf *btf = bpf_prog_get_target_btf(prog);
	const char *tname = prog->aux->attach_func_name;
	struct bpf_verifier_log *log = info->log;
	const struct btf_param *args;
	bool ptr_err_raw_tp = false;
	const char *tag_value;
	u32 nr_args, arg;
	int i, ret;

	if (off % 8) {
		bpf_log(log, "func '%s' offset %d is not multiple of 8\n",
			tname, off);
		return false;
	}
	arg = btf_ctx_arg_idx(btf, t, off);
	args = (const struct btf_param *)(t + 1);
	/* if (t == NULL) Fall back to default BPF prog with
	 * MAX_BPF_FUNC_REG_ARGS u64 arguments.
	 */
	nr_args = t ? btf_type_vlen(t) : MAX_BPF_FUNC_REG_ARGS;
	if (prog->aux->attach_btf_trace) {
		/* skip first 'void *__data' argument in btf_trace_##name typedef */
		args++;
		nr_args--;
	}

	if (arg > nr_args) {
		bpf_log(log, "func '%s' doesn't have %d-th argument\n",
			tname, arg + 1);
		return false;
	}

	if (arg == nr_args) {
		switch (prog->expected_attach_type) {
		case BPF_LSM_MAC:
			/* mark we are accessing the return value */
			info->is_retval = true;
			fallthrough;
		case BPF_LSM_CGROUP:
		case BPF_TRACE_FEXIT:
		case BPF_TRACE_FSESSION:
			/* When LSM programs are attached to void LSM hooks
			 * they use FEXIT trampolines and when attached to
			 * int LSM hooks, they use MODIFY_RETURN trampolines.
			 *
			 * While the LSM programs are BPF_MODIFY_RETURN-like
			 * the check:
			 *
			 *	if (ret_type != 'int')
			 *		return -EINVAL;
			 *
			 * is _not_ done here. This is still safe as LSM hooks
			 * have only void and int return types.
			 */
			if (!t)
				return true;
			t = btf_type_by_id(btf, t->type);
			break;
		case BPF_MODIFY_RETURN:
			/* For now the BPF_MODIFY_RETURN can only be attached to
			 * functions that return an int.
			 */
			if (!t)
				return false;

			t = btf_type_skip_modifiers(btf, t->type, NULL);
			if (!btf_type_is_small_int(t)) {
				bpf_log(log,
					"ret type %s not allowed for fmod_ret\n",
					btf_type_str(t));
				return false;
			}
			break;
		default:
			bpf_log(log, "func '%s' doesn't have %d-th argument\n",
				tname, arg + 1);
			return false;
		}
	} else {
		if (!t)
			/* Default prog with MAX_BPF_FUNC_REG_ARGS args */
			return true;
		t = btf_type_by_id(btf, args[arg].type);
	}

	/* skip modifiers */
	while (btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);
	if (btf_type_is_small_int(t) || btf_is_any_enum(t) || btf_type_is_struct(t))
		/* accessing a scalar */
		return true;
	if (!btf_type_is_ptr(t)) {
		bpf_log(log,
			"func '%s' arg%d '%s' has type %s. Only pointer access is allowed\n",
			tname, arg,
			__btf_name_by_offset(btf, t->name_off),
			btf_type_str(t));
		return false;
	}

	if (size != sizeof(u64)) {
		bpf_log(log, "func '%s' size %d must be 8\n",
			tname, size);
		return false;
	}