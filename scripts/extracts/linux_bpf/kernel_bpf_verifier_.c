// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 77/132



/* Implementation details:
 *
 * Each register points to some region of memory, which we define as an
 * allocation. Each allocation may embed a bpf_spin_lock which protects any
 * special BPF objects (bpf_list_head, bpf_rb_root, etc.) part of the same
 * allocation. The lock and the data it protects are colocated in the same
 * memory region.
 *
 * Hence, everytime a register holds a pointer value pointing to such
 * allocation, the verifier preserves a unique reg->id for it.
 *
 * The verifier remembers the lock 'ptr' and the lock 'id' whenever
 * bpf_spin_lock is called.
 *
 * To enable this, lock state in the verifier captures two values:
 *	active_lock.ptr = Register's type specific pointer
 *	active_lock.id  = A unique ID for each register pointer value
 *
 * Currently, PTR_TO_MAP_VALUE and PTR_TO_BTF_ID | MEM_ALLOC are the two
 * supported register types.
 *
 * The active_lock.ptr in case of map values is the reg->map_ptr, and in case of
 * allocated objects is the reg->btf pointer.
 *
 * The active_lock.id is non-unique for maps supporting direct_value_addr, as we
 * can establish the provenance of the map value statically for each distinct
 * lookup into such maps. They always contain a single map value hence unique
 * IDs for each pseudo load pessimizes the algorithm and rejects valid programs.
 *
 * So, in case of global variables, they use array maps with max_entries = 1,
 * hence their active_lock.ptr becomes map_ptr and id = 0 (since they all point
 * into the same map value as max_entries is 1, as described above).
 *
 * In case of inner map lookups, the inner map pointer has same map_ptr as the
 * outer map pointer (in verifier context), but each lookup into an inner map
 * assigns a fresh reg->id to the lookup, so while lookups into distinct inner
 * maps from the same outer map share the same map_ptr as active_lock.ptr, they
 * will get different reg->id assigned to each lookup, hence different
 * active_lock.id.
 *
 * In case of allocated objects, active_lock.ptr is the reg->btf, and the
 * reg->id is a unique ID preserved after the NULL pointer check on the pointer
 * returned from bpf_obj_new. Each allocation receives a new reg->id.
 */
static int check_reg_allocation_locked(struct bpf_verifier_env *env, struct bpf_reg_state *reg)
{
	struct bpf_reference_state *s;
	void *ptr;
	u32 id;

	switch ((int)reg->type) {
	case PTR_TO_MAP_VALUE:
		ptr = reg->map_ptr;
		break;
	case PTR_TO_BTF_ID | MEM_ALLOC:
		ptr = reg->btf;
		break;
	default:
		verifier_bug(env, "unknown reg type for lock check");
		return -EFAULT;
	}
	id = reg->id;

	if (!env->cur_state->active_locks)
		return -EINVAL;
	s = find_lock_state(env->cur_state, REF_TYPE_LOCK_MASK, id, ptr);
	if (!s) {
		verbose(env, "held lock and object are not in the same allocation\n");
		return -EINVAL;
	}
	return 0;
}

static bool is_bpf_list_api_kfunc(u32 btf_id)
{
	return is_bpf_list_push_kfunc(btf_id) ||
	       btf_id == special_kfunc_list[KF_bpf_list_pop_front] ||
	       btf_id == special_kfunc_list[KF_bpf_list_pop_back] ||
	       btf_id == special_kfunc_list[KF_bpf_list_front] ||
	       btf_id == special_kfunc_list[KF_bpf_list_back];
}

static bool is_bpf_rbtree_api_kfunc(u32 btf_id)
{
	return is_bpf_rbtree_add_kfunc(btf_id) ||
	       btf_id == special_kfunc_list[KF_bpf_rbtree_remove] ||
	       btf_id == special_kfunc_list[KF_bpf_rbtree_first] ||
	       btf_id == special_kfunc_list[KF_bpf_rbtree_root] ||
	       btf_id == special_kfunc_list[KF_bpf_rbtree_left] ||
	       btf_id == special_kfunc_list[KF_bpf_rbtree_right];
}

static bool is_bpf_iter_num_api_kfunc(u32 btf_id)
{
	return btf_id == special_kfunc_list[KF_bpf_iter_num_new] ||
	       btf_id == special_kfunc_list[KF_bpf_iter_num_next] ||
	       btf_id == special_kfunc_list[KF_bpf_iter_num_destroy];
}

static bool is_bpf_graph_api_kfunc(u32 btf_id)
{
	return is_bpf_list_api_kfunc(btf_id) ||
	       is_bpf_rbtree_api_kfunc(btf_id) ||
	       is_bpf_refcount_acquire_kfunc(btf_id);
}

static bool is_bpf_res_spin_lock_kfunc(u32 btf_id)
{
	return btf_id == special_kfunc_list[KF_bpf_res_spin_lock] ||
	       btf_id == special_kfunc_list[KF_bpf_res_spin_unlock] ||
	       btf_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave] ||
	       btf_id == special_kfunc_list[KF_bpf_res_spin_unlock_irqrestore];
}

static bool is_bpf_arena_kfunc(u32 btf_id)
{
	return btf_id == special_kfunc_list[KF_bpf_arena_alloc_pages] ||
	       btf_id == special_kfunc_list[KF_bpf_arena_free_pages] ||
	       btf_id == special_kfunc_list[KF_bpf_arena_reserve_pages];
}

static bool is_bpf_stream_kfunc(u32 btf_id)
{
	return btf_id == special_kfunc_list[KF_bpf_stream_vprintk] ||
	       btf_id == special_kfunc_list[KF_bpf_stream_print_stack];
}

static bool kfunc_spin_allowed(u32 btf_id)
{
	return is_bpf_graph_api_kfunc(btf_id) || is_bpf_iter_num_api_kfunc(btf_id) ||
	       is_bpf_res_spin_lock_kfunc(btf_id) || is_bpf_arena_kfunc(btf_id) ||
	       is_bpf_stream_kfunc(btf_id);
}