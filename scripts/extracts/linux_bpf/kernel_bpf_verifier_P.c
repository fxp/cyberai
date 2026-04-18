// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 48/132



/* Implementation details:
 * bpf_map_lookup returns PTR_TO_MAP_VALUE_OR_NULL.
 * bpf_obj_new returns PTR_TO_BTF_ID | MEM_ALLOC | PTR_MAYBE_NULL.
 * Two bpf_map_lookups (even with the same key) will have different reg->id.
 * Two separate bpf_obj_new will also have different reg->id.
 * For traditional PTR_TO_MAP_VALUE or PTR_TO_BTF_ID | MEM_ALLOC, the verifier
 * clears reg->id after value_or_null->value transition, since the verifier only
 * cares about the range of access to valid map value pointer and doesn't care
 * about actual address of the map element.
 * For maps with 'struct bpf_spin_lock' inside map value the verifier keeps
 * reg->id > 0 after value_or_null->value transition. By doing so
 * two bpf_map_lookups will be considered two different pointers that
 * point to different bpf_spin_locks. Likewise for pointers to allocated objects
 * returned from bpf_obj_new.
 * The verifier allows taking only one bpf_spin_lock at a time to avoid
 * dead-locks.
 * Since only one bpf_spin_lock is allowed the checks are simpler than
 * reg_is_refcounted() logic. The verifier needs to remember only
 * one spin_lock instead of array of acquired_refs.
 * env->cur_state->active_locks remembers which map value element or allocated
 * object got locked and clears it after bpf_spin_unlock.
 */
static int process_spin_lock(struct bpf_verifier_env *env, int regno, int flags)
{
	bool is_lock = flags & PROCESS_SPIN_LOCK, is_res_lock = flags & PROCESS_RES_LOCK;
	const char *lock_str = is_res_lock ? "bpf_res_spin" : "bpf_spin";
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct bpf_verifier_state *cur = env->cur_state;
	bool is_const = tnum_is_const(reg->var_off);
	bool is_irq = flags & PROCESS_LOCK_IRQ;
	u64 val = reg->var_off.value;
	struct bpf_map *map = NULL;
	struct btf *btf = NULL;
	struct btf_record *rec;
	u32 spin_lock_off;
	int err;

	if (!is_const) {
		verbose(env,
			"R%d doesn't have constant offset. %s_lock has to be at the constant offset\n",
			regno, lock_str);
		return -EINVAL;
	}
	if (reg->type == PTR_TO_MAP_VALUE) {
		map = reg->map_ptr;
		if (!map->btf) {
			verbose(env,
				"map '%s' has to have BTF in order to use %s_lock\n",
				map->name, lock_str);
			return -EINVAL;
		}
	} else {
		btf = reg->btf;
	}

	rec = reg_btf_record(reg);
	if (!btf_record_has_field(rec, is_res_lock ? BPF_RES_SPIN_LOCK : BPF_SPIN_LOCK)) {
		verbose(env, "%s '%s' has no valid %s_lock\n", map ? "map" : "local",
			map ? map->name : "kptr", lock_str);
		return -EINVAL;
	}
	spin_lock_off = is_res_lock ? rec->res_spin_lock_off : rec->spin_lock_off;
	if (spin_lock_off != val) {
		verbose(env, "off %lld doesn't point to 'struct %s_lock' that is at %d\n",
			val, lock_str, spin_lock_off);
		return -EINVAL;
	}
	if (is_lock) {
		void *ptr;
		int type;

		if (map)
			ptr = map;
		else
			ptr = btf;

		if (!is_res_lock && cur->active_locks) {
			if (find_lock_state(env->cur_state, REF_TYPE_LOCK, 0, NULL)) {
				verbose(env,
					"Locking two bpf_spin_locks are not allowed\n");
				return -EINVAL;
			}
		} else if (is_res_lock && cur->active_locks) {
			if (find_lock_state(env->cur_state, REF_TYPE_RES_LOCK | REF_TYPE_RES_LOCK_IRQ, reg->id, ptr)) {
				verbose(env, "Acquiring the same lock again, AA deadlock detected\n");
				return -EINVAL;
			}
		}

		if (is_res_lock && is_irq)
			type = REF_TYPE_RES_LOCK_IRQ;
		else if (is_res_lock)
			type = REF_TYPE_RES_LOCK;
		else
			type = REF_TYPE_LOCK;
		err = acquire_lock_state(env, env->insn_idx, type, reg->id, ptr);
		if (err < 0) {
			verbose(env, "Failed to acquire lock state\n");
			return err;
		}
	} else {
		void *ptr;
		int type;

		if (map)
			ptr = map;
		else
			ptr = btf;

		if (!cur->active_locks) {
			verbose(env, "%s_unlock without taking a lock\n", lock_str);
			return -EINVAL;
		}

		if (is_res_lock && is_irq)
			type = REF_TYPE_RES_LOCK_IRQ;
		else if (is_res_lock)
			type = REF_TYPE_RES_LOCK;
		else
			type = REF_TYPE_LOCK;
		if (!find_lock_state(cur, type, reg->id, ptr)) {
			verbose(env, "%s_unlock of different lock\n", lock_str);
			return -EINVAL;
		}
		if (reg->id != cur->active_lock_id || ptr != cur->active_lock_ptr) {
			verbose(env, "%s_unlock cannot be out of order\n", lock_str);
			return -EINVAL;
		}
		if (release_lock_state(cur, type, reg->id, ptr)) {
			verbose(env, "%s_unlock of different lock\n", lock_str);
			return -EINVAL;
		}

		invalidate_non_owning_refs(env);
	}
	return 0;
}

/* Check if @regno is a pointer to a specific field in a map value */
static int check_map_field_pointer(struct bpf_verifier_env *env, u32 regno,
				   enum btf_field_type field_type,
				   struct bpf_map_desc *map_desc)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	bool is_const = tnum_is_const(reg->var_off);
	struct bpf_map *map = reg->map_ptr;
	u64 val = reg->var_off.value;
	const char *struct_name = btf_field_type_name(field_type);
	int field_off = -1;