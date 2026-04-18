// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 39/132



static void set_sext32_default_val(struct bpf_reg_state *reg, int size)
{
	if (size == 1) {
		reg->s32_min_value = S8_MIN;
		reg->s32_max_value = S8_MAX;
	} else {
		/* size == 2 */
		reg->s32_min_value = S16_MIN;
		reg->s32_max_value = S16_MAX;
	}
	reg->u32_min_value = 0;
	reg->u32_max_value = U32_MAX;
	reg->var_off = tnum_subreg(tnum_unknown);
}

static void coerce_subreg_to_size_sx(struct bpf_reg_state *reg, int size)
{
	s32 init_s32_max, init_s32_min, s32_max, s32_min, u32_val;
	u32 top_smax_value, top_smin_value;
	u32 num_bits = size * 8;

	if (tnum_is_const(reg->var_off)) {
		u32_val = reg->var_off.value;
		if (size == 1)
			reg->var_off = tnum_const((s8)u32_val);
		else
			reg->var_off = tnum_const((s16)u32_val);

		u32_val = reg->var_off.value;
		reg->s32_min_value = reg->s32_max_value = u32_val;
		reg->u32_min_value = reg->u32_max_value = u32_val;
		return;
	}

	top_smax_value = ((u32)reg->s32_max_value >> num_bits) << num_bits;
	top_smin_value = ((u32)reg->s32_min_value >> num_bits) << num_bits;

	if (top_smax_value != top_smin_value)
		goto out;

	/* find the s32_min and s32_min after sign extension */
	if (size == 1) {
		init_s32_max = (s8)reg->s32_max_value;
		init_s32_min = (s8)reg->s32_min_value;
	} else {
		/* size == 2 */
		init_s32_max = (s16)reg->s32_max_value;
		init_s32_min = (s16)reg->s32_min_value;
	}
	s32_max = max(init_s32_max, init_s32_min);
	s32_min = min(init_s32_max, init_s32_min);

	if ((s32_min >= 0) == (s32_max >= 0)) {
		reg->s32_min_value = s32_min;
		reg->s32_max_value = s32_max;
		reg->u32_min_value = (u32)s32_min;
		reg->u32_max_value = (u32)s32_max;
		reg->var_off = tnum_subreg(tnum_range(s32_min, s32_max));
		return;
	}

out:
	set_sext32_default_val(reg, size);
}

bool bpf_map_is_rdonly(const struct bpf_map *map)
{
	/* A map is considered read-only if the following condition are true:
	 *
	 * 1) BPF program side cannot change any of the map content. The
	 *    BPF_F_RDONLY_PROG flag is throughout the lifetime of a map
	 *    and was set at map creation time.
	 * 2) The map value(s) have been initialized from user space by a
	 *    loader and then "frozen", such that no new map update/delete
	 *    operations from syscall side are possible for the rest of
	 *    the map's lifetime from that point onwards.
	 * 3) Any parallel/pending map update/delete operations from syscall
	 *    side have been completed. Only after that point, it's safe to
	 *    assume that map value(s) are immutable.
	 */
	return (map->map_flags & BPF_F_RDONLY_PROG) &&
	       READ_ONCE(map->frozen) &&
	       !bpf_map_write_active(map);
}

int bpf_map_direct_read(struct bpf_map *map, int off, int size, u64 *val,
			bool is_ldsx)
{
	void *ptr;
	u64 addr;
	int err;

	err = map->ops->map_direct_value_addr(map, &addr, off);
	if (err)
		return err;
	ptr = (void *)(long)addr + off;

	switch (size) {
	case sizeof(u8):
		*val = is_ldsx ? (s64)*(s8 *)ptr : (u64)*(u8 *)ptr;
		break;
	case sizeof(u16):
		*val = is_ldsx ? (s64)*(s16 *)ptr : (u64)*(u16 *)ptr;
		break;
	case sizeof(u32):
		*val = is_ldsx ? (s64)*(s32 *)ptr : (u64)*(u32 *)ptr;
		break;
	case sizeof(u64):
		*val = *(u64 *)ptr;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define BTF_TYPE_SAFE_RCU(__type)  __PASTE(__type, __safe_rcu)
#define BTF_TYPE_SAFE_RCU_OR_NULL(__type)  __PASTE(__type, __safe_rcu_or_null)
#define BTF_TYPE_SAFE_TRUSTED(__type)  __PASTE(__type, __safe_trusted)
#define BTF_TYPE_SAFE_TRUSTED_OR_NULL(__type)  __PASTE(__type, __safe_trusted_or_null)

/*
 * Allow list few fields as RCU trusted or full trusted.
 * This logic doesn't allow mix tagging and will be removed once GCC supports
 * btf_type_tag.
 */

/* RCU trusted: these fields are trusted in RCU CS and never NULL */
BTF_TYPE_SAFE_RCU(struct task_struct) {
	const cpumask_t *cpus_ptr;
	struct css_set __rcu *cgroups;
	struct task_struct __rcu *real_parent;
	struct task_struct *group_leader;
};

BTF_TYPE_SAFE_RCU(struct cgroup) {
	/* cgrp->kn is always accessible as documented in kernel/cgroup/cgroup.c */
	struct kernfs_node *kn;
};

BTF_TYPE_SAFE_RCU(struct css_set) {
	struct cgroup *dfl_cgrp;
};

BTF_TYPE_SAFE_RCU(struct cgroup_subsys_state) {
	struct cgroup *cgroup;
};

/* RCU trusted: these fields are trusted in RCU CS and can be NULL */
BTF_TYPE_SAFE_RCU_OR_NULL(struct mm_struct) {
	struct file __rcu *exe_file;
#ifdef CONFIG_MEMCG
	struct task_struct __rcu *owner;
#endif
};

/* skb->sk, req->sk are not RCU protected, but we mark them as such
 * because bpf prog accessible sockets are SOCK_RCU_FREE.
 */
BTF_TYPE_SAFE_RCU_OR_NULL(struct sk_buff) {
	struct sock *sk;
};

BTF_TYPE_SAFE_RCU_OR_NULL(struct request_sock) {
	struct sock *sk;
};

/* full trusted: these fields are trusted even outside of RCU CS and never NULL */
BTF_TYPE_SAFE_TRUSTED(struct bpf_iter_meta) {
	struct seq_file *seq;
};

BTF_TYPE_SAFE_TRUSTED(struct bpf_iter__task) {
	struct bpf_iter_meta *meta;
	struct task_struct *task;
};