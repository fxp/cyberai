// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 50/55



	/* Grow set */
	set = krealloc(tab->sets[hook],
		       struct_size(set, pairs, set_cnt + add_set->cnt),
		       GFP_KERNEL | __GFP_NOWARN);
	if (!set) {
		ret = -ENOMEM;
		goto end;
	}

	/* For newly allocated set, initialize set->cnt to 0 */
	if (!tab->sets[hook])
		set->cnt = 0;
	tab->sets[hook] = set;

	/* Concatenate the two sets */
	memcpy(set->pairs + set->cnt, add_set->pairs, add_set->cnt * sizeof(set->pairs[0]));
	/* Now that the set is copied, update with relocated BTF ids */
	for (i = set->cnt; i < set->cnt + add_set->cnt; i++)
		set->pairs[i].id = btf_relocate_id(btf, set->pairs[i].id);

	set->cnt += add_set->cnt;

	sort(set->pairs, set->cnt, sizeof(set->pairs[0]), btf_id_cmp_func, NULL);

	if (add_filter) {
		hook_filter = &tab->hook_filters[hook];
		hook_filter->filters[hook_filter->nr_filters++] = kset->filter;
	}
	return 0;
end:
	btf_free_kfunc_set_tab(btf);
	return ret;
}

static u32 *btf_kfunc_id_set_contains(const struct btf *btf,
				      enum btf_kfunc_hook hook,
				      u32 kfunc_btf_id)
{
	struct btf_id_set8 *set;
	u32 *id;

	if (hook >= BTF_KFUNC_HOOK_MAX)
		return NULL;
	if (!btf->kfunc_set_tab)
		return NULL;
	set = btf->kfunc_set_tab->sets[hook];
	if (!set)
		return NULL;
	id = btf_id_set8_contains(set, kfunc_btf_id);
	if (!id)
		return NULL;
	/* The flags for BTF ID are located next to it */
	return id + 1;
}

static bool __btf_kfunc_is_allowed(const struct btf *btf,
				   enum btf_kfunc_hook hook,
				   u32 kfunc_btf_id,
				   const struct bpf_prog *prog)
{
	struct btf_kfunc_hook_filter *hook_filter;
	int i;

	if (hook >= BTF_KFUNC_HOOK_MAX)
		return false;
	if (!btf->kfunc_set_tab)
		return false;

	hook_filter = &btf->kfunc_set_tab->hook_filters[hook];
	for (i = 0; i < hook_filter->nr_filters; i++) {
		if (hook_filter->filters[i](prog, kfunc_btf_id))
			return false;
	}

	return true;
}

static int bpf_prog_type_to_kfunc_hook(enum bpf_prog_type prog_type)
{
	switch (prog_type) {
	case BPF_PROG_TYPE_UNSPEC:
		return BTF_KFUNC_HOOK_COMMON;
	case BPF_PROG_TYPE_XDP:
		return BTF_KFUNC_HOOK_XDP;
	case BPF_PROG_TYPE_SCHED_CLS:
		return BTF_KFUNC_HOOK_TC;
	case BPF_PROG_TYPE_STRUCT_OPS:
		return BTF_KFUNC_HOOK_STRUCT_OPS;
	case BPF_PROG_TYPE_TRACING:
	case BPF_PROG_TYPE_TRACEPOINT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_LSM:
		return BTF_KFUNC_HOOK_TRACING;
	case BPF_PROG_TYPE_SYSCALL:
		return BTF_KFUNC_HOOK_SYSCALL;
	case BPF_PROG_TYPE_CGROUP_SKB:
	case BPF_PROG_TYPE_CGROUP_SOCK:
	case BPF_PROG_TYPE_CGROUP_DEVICE:
	case BPF_PROG_TYPE_CGROUP_SOCK_ADDR:
	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
	case BPF_PROG_TYPE_CGROUP_SYSCTL:
	case BPF_PROG_TYPE_SOCK_OPS:
		return BTF_KFUNC_HOOK_CGROUP;
	case BPF_PROG_TYPE_SCHED_ACT:
		return BTF_KFUNC_HOOK_SCHED_ACT;
	case BPF_PROG_TYPE_SK_SKB:
		return BTF_KFUNC_HOOK_SK_SKB;
	case BPF_PROG_TYPE_SOCKET_FILTER:
		return BTF_KFUNC_HOOK_SOCKET_FILTER;
	case BPF_PROG_TYPE_LWT_OUT:
	case BPF_PROG_TYPE_LWT_IN:
	case BPF_PROG_TYPE_LWT_XMIT:
	case BPF_PROG_TYPE_LWT_SEG6LOCAL:
		return BTF_KFUNC_HOOK_LWT;
	case BPF_PROG_TYPE_NETFILTER:
		return BTF_KFUNC_HOOK_NETFILTER;
	case BPF_PROG_TYPE_KPROBE:
		return BTF_KFUNC_HOOK_KPROBE;
	default:
		return BTF_KFUNC_HOOK_MAX;
	}
}

bool btf_kfunc_is_allowed(const struct btf *btf,
			  u32 kfunc_btf_id,
			  const struct bpf_prog *prog)
{
	enum bpf_prog_type prog_type = resolve_prog_type(prog);
	enum btf_kfunc_hook hook;
	u32 *kfunc_flags;

	kfunc_flags = btf_kfunc_id_set_contains(btf, BTF_KFUNC_HOOK_COMMON, kfunc_btf_id);
	if (kfunc_flags && __btf_kfunc_is_allowed(btf, BTF_KFUNC_HOOK_COMMON, kfunc_btf_id, prog))
		return true;

	hook = bpf_prog_type_to_kfunc_hook(prog_type);
	kfunc_flags = btf_kfunc_id_set_contains(btf, hook, kfunc_btf_id);
	if (kfunc_flags && __btf_kfunc_is_allowed(btf, hook, kfunc_btf_id, prog))
		return true;

	return false;
}

/* Caution:
 * Reference to the module (obtained using btf_try_get_module) corresponding to
 * the struct btf *MUST* be held when calling this function from verifier
 * context. This is usually true as we stash references in prog's kfunc_btf_tab;
 * keeping the reference for the duration of the call provides the necessary
 * protection for looking up a well-formed btf->kfunc_set_tab.
 */
u32 *btf_kfunc_flags(const struct btf *btf, u32 kfunc_btf_id, const struct bpf_prog *prog)
{
	enum bpf_prog_type prog_type = resolve_prog_type(prog);
	enum btf_kfunc_hook hook;
	u32 *kfunc_flags;

	kfunc_flags = btf_kfunc_id_set_contains(btf, BTF_KFUNC_HOOK_COMMON, kfunc_btf_id);
	if (kfunc_flags)
		return kfunc_flags;

	hook = bpf_prog_type_to_kfunc_hook(prog_type);
	return btf_kfunc_id_set_contains(btf, hook, kfunc_btf_id);
}

u32 *btf_kfunc_is_modify_return(const struct btf *btf, u32 kfunc_btf_id,
				const struct bpf_prog *prog)
{
	if (!__btf_kfunc_is_allowed(btf, BTF_KFUNC_HOOK_FMODRET, kfunc_btf_id, prog))
		return NULL;

	return btf_kfunc_id_set_contains(btf, BTF_KFUNC_HOOK_FMODRET, kfunc_btf_id);
}