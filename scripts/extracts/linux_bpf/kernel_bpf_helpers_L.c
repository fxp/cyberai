// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 12/30



const struct bpf_func_proto *
bpf_base_func_proto(enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	switch (func_id) {
	case BPF_FUNC_map_lookup_elem:
		return &bpf_map_lookup_elem_proto;
	case BPF_FUNC_map_update_elem:
		return &bpf_map_update_elem_proto;
	case BPF_FUNC_map_delete_elem:
		return &bpf_map_delete_elem_proto;
	case BPF_FUNC_map_push_elem:
		return &bpf_map_push_elem_proto;
	case BPF_FUNC_map_pop_elem:
		return &bpf_map_pop_elem_proto;
	case BPF_FUNC_map_peek_elem:
		return &bpf_map_peek_elem_proto;
	case BPF_FUNC_map_lookup_percpu_elem:
		return &bpf_map_lookup_percpu_elem_proto;
	case BPF_FUNC_get_prandom_u32:
		return &bpf_get_prandom_u32_proto;
	case BPF_FUNC_get_smp_processor_id:
		return &bpf_get_raw_smp_processor_id_proto;
	case BPF_FUNC_get_numa_node_id:
		return &bpf_get_numa_node_id_proto;
	case BPF_FUNC_tail_call:
		return &bpf_tail_call_proto;
	case BPF_FUNC_ktime_get_ns:
		return &bpf_ktime_get_ns_proto;
	case BPF_FUNC_ktime_get_boot_ns:
		return &bpf_ktime_get_boot_ns_proto;
	case BPF_FUNC_ktime_get_tai_ns:
		return &bpf_ktime_get_tai_ns_proto;
	case BPF_FUNC_ringbuf_output:
		return &bpf_ringbuf_output_proto;
	case BPF_FUNC_ringbuf_reserve:
		return &bpf_ringbuf_reserve_proto;
	case BPF_FUNC_ringbuf_submit:
		return &bpf_ringbuf_submit_proto;
	case BPF_FUNC_ringbuf_discard:
		return &bpf_ringbuf_discard_proto;
	case BPF_FUNC_ringbuf_query:
		return &bpf_ringbuf_query_proto;
	case BPF_FUNC_strncmp:
		return &bpf_strncmp_proto;
	case BPF_FUNC_strtol:
		return &bpf_strtol_proto;
	case BPF_FUNC_strtoul:
		return &bpf_strtoul_proto;
	case BPF_FUNC_get_current_pid_tgid:
		return &bpf_get_current_pid_tgid_proto;
	case BPF_FUNC_get_ns_current_pid_tgid:
		return &bpf_get_ns_current_pid_tgid_proto;
	case BPF_FUNC_get_current_uid_gid:
		return &bpf_get_current_uid_gid_proto;
	default:
		break;
	}

	if (!bpf_token_capable(prog->aux->token, CAP_BPF))
		return NULL;

	switch (func_id) {
	case BPF_FUNC_spin_lock:
		return &bpf_spin_lock_proto;
	case BPF_FUNC_spin_unlock:
		return &bpf_spin_unlock_proto;
	case BPF_FUNC_jiffies64:
		return &bpf_jiffies64_proto;
	case BPF_FUNC_per_cpu_ptr:
		return &bpf_per_cpu_ptr_proto;
	case BPF_FUNC_this_cpu_ptr:
		return &bpf_this_cpu_ptr_proto;
	case BPF_FUNC_timer_init:
		return &bpf_timer_init_proto;
	case BPF_FUNC_timer_set_callback:
		return &bpf_timer_set_callback_proto;
	case BPF_FUNC_timer_start:
		return &bpf_timer_start_proto;
	case BPF_FUNC_timer_cancel:
		return &bpf_timer_cancel_proto;
	case BPF_FUNC_kptr_xchg:
		return &bpf_kptr_xchg_proto;
	case BPF_FUNC_for_each_map_elem:
		return &bpf_for_each_map_elem_proto;
	case BPF_FUNC_loop:
		return &bpf_loop_proto;
	case BPF_FUNC_user_ringbuf_drain:
		return &bpf_user_ringbuf_drain_proto;
	case BPF_FUNC_ringbuf_reserve_dynptr:
		return &bpf_ringbuf_reserve_dynptr_proto;
	case BPF_FUNC_ringbuf_submit_dynptr:
		return &bpf_ringbuf_submit_dynptr_proto;
	case BPF_FUNC_ringbuf_discard_dynptr:
		return &bpf_ringbuf_discard_dynptr_proto;
	case BPF_FUNC_dynptr_from_mem:
		return &bpf_dynptr_from_mem_proto;
	case BPF_FUNC_dynptr_read:
		return &bpf_dynptr_read_proto;
	case BPF_FUNC_dynptr_write:
		return &bpf_dynptr_write_proto;
	case BPF_FUNC_dynptr_data:
		return &bpf_dynptr_data_proto;
#ifdef CONFIG_CGROUPS
	case BPF_FUNC_cgrp_storage_get:
		return &bpf_cgrp_storage_get_proto;
	case BPF_FUNC_cgrp_storage_delete:
		return &bpf_cgrp_storage_delete_proto;
	case BPF_FUNC_get_current_cgroup_id:
		return &bpf_get_current_cgroup_id_proto;
	case BPF_FUNC_get_current_ancestor_cgroup_id:
		return &bpf_get_current_ancestor_cgroup_id_proto;
	case BPF_FUNC_current_task_under_cgroup:
		return &bpf_current_task_under_cgroup_proto;
#endif
#ifdef CONFIG_CGROUP_NET_CLASSID
	case BPF_FUNC_get_cgroup_classid:
		return &bpf_get_cgroup_classid_curr_proto;
#endif
	case BPF_FUNC_task_storage_get:
		return &bpf_task_storage_get_proto;
	case BPF_FUNC_task_storage_delete:
		return &bpf_task_storage_delete_proto;
	default:
		break;
	}

	if (!bpf_token_capable(prog->aux->token, CAP_PERFMON))
		return NULL;