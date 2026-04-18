// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 18/20



	aux = container_of(work, struct bpf_prog_aux, work);
#ifdef CONFIG_BPF_SYSCALL
	bpf_free_kfunc_btf_tab(aux->kfunc_btf_tab);
	bpf_prog_stream_free(aux->prog);
#endif
#ifdef CONFIG_CGROUP_BPF
	if (aux->cgroup_atype != CGROUP_BPF_ATTACH_TYPE_INVALID)
		bpf_cgroup_atype_put(aux->cgroup_atype);
#endif
	bpf_free_used_maps(aux);
	bpf_free_used_btfs(aux);
	bpf_prog_disassoc_struct_ops(aux->prog);
	if (bpf_prog_is_dev_bound(aux))
		bpf_prog_dev_bound_destroy(aux->prog);
#ifdef CONFIG_PERF_EVENTS
	if (aux->prog->has_callchain_buf)
		put_callchain_buffers();
#endif
	if (aux->dst_trampoline)
		bpf_trampoline_put(aux->dst_trampoline);
	for (i = 0; i < aux->real_func_cnt; i++) {
		/* We can just unlink the subprog poke descriptor table as
		 * it was originally linked to the main program and is also
		 * released along with it.
		 */
		aux->func[i]->aux->poke_tab = NULL;
		bpf_jit_free(aux->func[i]);
	}
	if (aux->real_func_cnt) {
		kfree(aux->func);
		bpf_prog_unlock_free(aux->prog);
	} else {
		bpf_jit_free(aux->prog);
	}
}

void bpf_prog_free(struct bpf_prog *fp)
{
	struct bpf_prog_aux *aux = fp->aux;

	if (aux->dst_prog)
		bpf_prog_put(aux->dst_prog);
	bpf_token_put(aux->token);
	INIT_WORK(&aux->work, bpf_prog_free_deferred);
	schedule_work(&aux->work);
}
EXPORT_SYMBOL_GPL(bpf_prog_free);

/* RNG for unprivileged user space with separated state from prandom_u32(). */
static DEFINE_PER_CPU(struct rnd_state, bpf_user_rnd_state);

void bpf_user_rnd_init_once(void)
{
	prandom_init_once(&bpf_user_rnd_state);
}

BPF_CALL_0(bpf_user_rnd_u32)
{
	/* Should someone ever have the rather unwise idea to use some
	 * of the registers passed into this function, then note that
	 * this function is called from native eBPF and classic-to-eBPF
	 * transformations. Register assignments from both sides are
	 * different, f.e. classic always sets fn(ctx, A, X) here.
	 */
	struct rnd_state *state;
	u32 res;

	state = &get_cpu_var(bpf_user_rnd_state);
	res = prandom_u32_state(state);
	put_cpu_var(bpf_user_rnd_state);

	return res;
}

BPF_CALL_0(bpf_get_raw_cpu_id)
{
	return raw_smp_processor_id();
}

/* Weak definitions of helper functions in case we don't have bpf syscall. */
const struct bpf_func_proto bpf_map_lookup_elem_proto __weak;
const struct bpf_func_proto bpf_map_update_elem_proto __weak;
const struct bpf_func_proto bpf_map_delete_elem_proto __weak;
const struct bpf_func_proto bpf_map_push_elem_proto __weak;
const struct bpf_func_proto bpf_map_pop_elem_proto __weak;
const struct bpf_func_proto bpf_map_peek_elem_proto __weak;
const struct bpf_func_proto bpf_map_lookup_percpu_elem_proto __weak;
const struct bpf_func_proto bpf_spin_lock_proto __weak;
const struct bpf_func_proto bpf_spin_unlock_proto __weak;
const struct bpf_func_proto bpf_jiffies64_proto __weak;

const struct bpf_func_proto bpf_get_prandom_u32_proto __weak;
const struct bpf_func_proto bpf_get_smp_processor_id_proto __weak;
const struct bpf_func_proto bpf_get_numa_node_id_proto __weak;
const struct bpf_func_proto bpf_ktime_get_ns_proto __weak;
const struct bpf_func_proto bpf_ktime_get_boot_ns_proto __weak;
const struct bpf_func_proto bpf_ktime_get_coarse_ns_proto __weak;
const struct bpf_func_proto bpf_ktime_get_tai_ns_proto __weak;

const struct bpf_func_proto bpf_get_current_pid_tgid_proto __weak;
const struct bpf_func_proto bpf_get_current_uid_gid_proto __weak;
const struct bpf_func_proto bpf_get_current_comm_proto __weak;
const struct bpf_func_proto bpf_get_current_cgroup_id_proto __weak;
const struct bpf_func_proto bpf_get_current_ancestor_cgroup_id_proto __weak;
const struct bpf_func_proto bpf_get_local_storage_proto __weak;
const struct bpf_func_proto bpf_get_ns_current_pid_tgid_proto __weak;
const struct bpf_func_proto bpf_snprintf_btf_proto __weak;
const struct bpf_func_proto bpf_seq_printf_btf_proto __weak;
const struct bpf_func_proto bpf_set_retval_proto __weak;
const struct bpf_func_proto bpf_get_retval_proto __weak;

const struct bpf_func_proto * __weak bpf_get_trace_printk_proto(void)
{
	return NULL;
}

const struct bpf_func_proto * __weak bpf_get_trace_vprintk_proto(void)
{
	return NULL;
}

const struct bpf_func_proto * __weak bpf_get_perf_event_read_value_proto(void)
{
	return NULL;
}

u64 __weak
bpf_event_output(struct bpf_map *map, u64 flags, void *meta, u64 meta_size,
		 void *ctx, u64 ctx_size, bpf_ctx_copy_t ctx_copy)
{
	return -ENOTSUPP;
}
EXPORT_SYMBOL_GPL(bpf_event_output);

/* Always built-in helper functions. */
const struct bpf_func_proto bpf_tail_call_proto = {
	/* func is unused for tail_call, we set it to pass the
	 * get_helper_proto check
	 */
	.func		= BPF_PTR_POISON,
	.gpl_only	= false,
	.ret_type	= RET_VOID,
	.arg1_type	= ARG_PTR_TO_CTX,
	.arg2_type	= ARG_CONST_MAP_PTR,
	.arg3_type	= ARG_ANYTHING,
};