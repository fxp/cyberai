// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 38/55



static bool prog_args_trusted(const struct bpf_prog *prog)
{
	enum bpf_attach_type atype = prog->expected_attach_type;

	switch (prog->type) {
	case BPF_PROG_TYPE_TRACING:
		return atype == BPF_TRACE_RAW_TP || atype == BPF_TRACE_ITER;
	case BPF_PROG_TYPE_LSM:
		return bpf_lsm_is_trusted(prog);
	case BPF_PROG_TYPE_STRUCT_OPS:
		return true;
	default:
		return false;
	}
}

int btf_ctx_arg_offset(const struct btf *btf, const struct btf_type *func_proto,
		       u32 arg_no)
{
	const struct btf_param *args;
	const struct btf_type *t;
	int off = 0, i;
	u32 sz;

	args = btf_params(func_proto);
	for (i = 0; i < arg_no; i++) {
		t = btf_type_by_id(btf, args[i].type);
		t = btf_resolve_size(btf, t, &sz);
		if (IS_ERR(t))
			return PTR_ERR(t);
		off += roundup(sz, 8);
	}

	return off;
}

struct bpf_raw_tp_null_args {
	const char *func;
	u64 mask;
};

static const struct bpf_raw_tp_null_args raw_tp_null_args[] = {
	/* sched */
	{ "sched_pi_setprio", 0x10 },
	/* ... from sched_numa_pair_template event class */
	{ "sched_stick_numa", 0x100 },
	{ "sched_swap_numa", 0x100 },
	/* afs */
	{ "afs_make_fs_call", 0x10 },
	{ "afs_make_fs_calli", 0x10 },
	{ "afs_make_fs_call1", 0x10 },
	{ "afs_make_fs_call2", 0x10 },
	{ "afs_protocol_error", 0x1 },
	{ "afs_flock_ev", 0x10 },
	/* cachefiles */
	{ "cachefiles_lookup", 0x1 | 0x200 },
	{ "cachefiles_unlink", 0x1 },
	{ "cachefiles_rename", 0x1 },
	{ "cachefiles_prep_read", 0x1 },
	{ "cachefiles_mark_active", 0x1 },
	{ "cachefiles_mark_failed", 0x1 },
	{ "cachefiles_mark_inactive", 0x1 },
	{ "cachefiles_vfs_error", 0x1 },
	{ "cachefiles_io_error", 0x1 },
	{ "cachefiles_ondemand_open", 0x1 },
	{ "cachefiles_ondemand_copen", 0x1 },
	{ "cachefiles_ondemand_close", 0x1 },
	{ "cachefiles_ondemand_read", 0x1 },
	{ "cachefiles_ondemand_cread", 0x1 },
	{ "cachefiles_ondemand_fd_write", 0x1 },
	{ "cachefiles_ondemand_fd_release", 0x1 },
	/* ext4, from ext4__mballoc event class */
	{ "ext4_mballoc_discard", 0x10 },
	{ "ext4_mballoc_free", 0x10 },
	/* fib */
	{ "fib_table_lookup", 0x100 },
	/* filelock */
	/* ... from filelock_lock event class */
	{ "posix_lock_inode", 0x10 },
	{ "fcntl_setlk", 0x10 },
	{ "locks_remove_posix", 0x10 },
	{ "flock_lock_inode", 0x10 },
	/* ... from filelock_lease event class */
	{ "break_lease_noblock", 0x10 },
	{ "break_lease_block", 0x10 },
	{ "break_lease_unblock", 0x10 },
	{ "generic_delete_lease", 0x10 },
	{ "time_out_leases", 0x10 },
	/* host1x */
	{ "host1x_cdma_push_gather", 0x10000 },
	/* huge_memory */
	{ "mm_khugepaged_scan_pmd", 0x10 },
	{ "mm_collapse_huge_page_isolate", 0x1 },
	{ "mm_khugepaged_scan_file", 0x10 },
	{ "mm_khugepaged_collapse_file", 0x10 },
	/* kmem */
	{ "mm_page_alloc", 0x1 },
	{ "mm_page_pcpu_drain", 0x1 },
	/* .. from mm_page event class */
	{ "mm_page_alloc_zone_locked", 0x1 },
	/* netfs */
	{ "netfs_failure", 0x10 },
	/* power */
	{ "device_pm_callback_start", 0x10 },
	/* qdisc */
	{ "qdisc_dequeue", 0x1000 },
	/* rxrpc */
	{ "rxrpc_recvdata", 0x1 },
	{ "rxrpc_resend", 0x10 },
	{ "rxrpc_tq", 0x10 },
	{ "rxrpc_client", 0x1 },
	/* skb */
	{"kfree_skb", 0x1000},
	/* sunrpc */
	{ "xs_stream_read_data", 0x1 },
	/* ... from xprt_cong_event event class */
	{ "xprt_reserve_cong", 0x10 },
	{ "xprt_release_cong", 0x10 },
	{ "xprt_get_cong", 0x10 },
	{ "xprt_put_cong", 0x10 },
	/* tcp */
	{ "tcp_send_reset", 0x11 },
	{ "tcp_sendmsg_locked", 0x100 },
	/* tegra_apb_dma */
	{ "tegra_dma_tx_status", 0x100 },
	/* timer_migration */
	{ "tmigr_update_events", 0x1 },
	/* writeback, from writeback_folio_template event class */
	{ "writeback_dirty_folio", 0x10 },
	{ "folio_wait_writeback", 0x10 },
	/* rdma */
	{ "mr_integ_alloc", 0x2000 },
	/* bpf_testmod */
	{ "bpf_testmod_test_read", 0x0 },
	/* amdgpu */
	{ "amdgpu_vm_bo_map", 0x1 },
	{ "amdgpu_vm_bo_unmap", 0x1 },
	/* netfs */
	{ "netfs_folioq", 0x1 },
	/* xfs from xfs_defer_pending_class */
	{ "xfs_defer_create_intent", 0x1 },
	{ "xfs_defer_cancel_list", 0x1 },
	{ "xfs_defer_pending_finish", 0x1 },
	{ "xfs_defer_pending_abort", 0x1 },
	{ "xfs_defer_relog_intent", 0x1 },
	{ "xfs_defer_isolate_paused", 0x1 },
	{ "xfs_defer_item_pause", 0x1 },
	{ "xfs_defer_item_unpause", 0x1 },
	/* xfs from xfs_defer_pending_item_class */
	{ "xfs_defer_add_item", 0x1 },
	{ "xfs_defer_cancel_item", 0x1 },
	{ "xfs_defer_finish_item", 0x1 },
	/* xfs from xfs_icwalk_class */
	{ "xfs_ioc_free_eofblocks", 0x10 },
	{ "xfs_blockgc_free_space", 0x10 },
	/* xfs from xfs_btree_cur_class */
	{ "xfs_btree_updkeys", 0x100 },
	{ "xfs_btree_overlapped_query_range", 0x100 },
	/* xfs from xfs_imap_class*/
	{ "xfs_map_blocks_found", 0x10000 },
	{ "xfs_map_blocks_alloc", 0x10000 },
	{ "xfs_iomap_alloc", 0x1000 },
	{ "xfs_iomap_found", 0x1000 },
	/* xfs from xfs_fs_class */
	{ "xfs_inodegc_flush", 0x1 },
	{ "xfs_inodegc_push", 0x1 },
	{ "xfs_inodegc_start", 0x1 },
	{ "xfs_inodegc_stop", 0x1 },
	{ "xfs_inodegc_queue", 0x1 },
	{ "xfs_inodegc_throttle", 0x1 },
	{ "xfs_fs_sync_fs", 0x1 },
	{ "xfs_blockgc_start", 0x1 },
	{ "xfs_blockgc_stop