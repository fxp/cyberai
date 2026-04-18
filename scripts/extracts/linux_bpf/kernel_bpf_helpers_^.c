// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/helpers.c
// Segment 30/30



	ret = register_btf_kfunc_id_set(BPF_PROG_TYPE_TRACING, &generic_kfunc_set);
	ret = ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_SCHED_CLS, &generic_kfunc_set);
	ret = ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_XDP, &generic_kfunc_set);
	ret = ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_STRUCT_OPS, &generic_kfunc_set);
	ret = ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_SYSCALL, &generic_kfunc_set);
	ret = ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_CGROUP_SKB, &generic_kfunc_set);
	ret = ret ?: register_btf_id_dtor_kfuncs(generic_dtors,
						  ARRAY_SIZE(generic_dtors),
						  THIS_MODULE);
	return ret ?: register_btf_kfunc_id_set(BPF_PROG_TYPE_UNSPEC, &common_kfunc_set);
}

late_initcall(kfunc_init);

/* Get a pointer to dynptr data up to len bytes for read only access. If
 * the dynptr doesn't have continuous data up to len bytes, return NULL.
 */
const void *__bpf_dynptr_data(const struct bpf_dynptr_kern *ptr, u64 len)
{
	const struct bpf_dynptr *p = (struct bpf_dynptr *)ptr;

	return bpf_dynptr_slice(p, 0, NULL, len);
}

/* Get a pointer to dynptr data up to len bytes for read write access. If
 * the dynptr doesn't have continuous data up to len bytes, or the dynptr
 * is read only, return NULL.
 */
void *__bpf_dynptr_data_rw(const struct bpf_dynptr_kern *ptr, u64 len)
{
	if (__bpf_dynptr_is_rdonly(ptr))
		return NULL;
	return (void *)__bpf_dynptr_data(ptr, len);
}

void bpf_map_free_internal_structs(struct bpf_map *map, void *val)
{
	if (btf_record_has_field(map->record, BPF_TIMER))
		bpf_obj_free_timer(map->record, val);
	if (btf_record_has_field(map->record, BPF_WORKQUEUE))
		bpf_obj_free_workqueue(map->record, val);
	if (btf_record_has_field(map->record, BPF_TASK_WORK))
		bpf_obj_free_task_work(map->record, val);
}
