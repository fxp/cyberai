// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 41/132



	} else if (is_trusted_reg(reg) || is_rcu_reg(reg)) {
		/* By default any pointer obtained from walking a trusted pointer is no
		 * longer trusted, unless the field being accessed has explicitly been
		 * marked as inheriting its parent's state of trust (either full or RCU).
		 * For example:
		 * 'cgroups' pointer is untrusted if task->cgroups dereference
		 * happened in a sleepable program outside of bpf_rcu_read_lock()
		 * section. In a non-sleepable program it's trusted while in RCU CS (aka MEM_RCU).
		 * Note bpf_rcu_read_unlock() converts MEM_RCU pointers to PTR_UNTRUSTED.
		 *
		 * A regular RCU-protected pointer with __rcu tag can also be deemed
		 * trusted if we are in an RCU CS. Such pointer can be NULL.
		 */
		if (type_is_trusted(env, reg, field_name, btf_id)) {
			flag |= PTR_TRUSTED;
		} else if (type_is_trusted_or_null(env, reg, field_name, btf_id)) {
			flag |= PTR_TRUSTED | PTR_MAYBE_NULL;
		} else if (in_rcu_cs(env) && !type_may_be_null(reg->type)) {
			if (type_is_rcu(env, reg, field_name, btf_id)) {
				/* ignore __rcu tag and mark it MEM_RCU */
				flag |= MEM_RCU;
			} else if (flag & MEM_RCU ||
				   type_is_rcu_or_null(env, reg, field_name, btf_id)) {
				/* __rcu tagged pointers can be NULL */
				flag |= MEM_RCU | PTR_MAYBE_NULL;

				/* We always trust them */
				if (type_is_rcu_or_null(env, reg, field_name, btf_id) &&
				    flag & PTR_UNTRUSTED)
					flag &= ~PTR_UNTRUSTED;
			} else if (flag & (MEM_PERCPU | MEM_USER)) {
				/* keep as-is */
			} else {
				/* walking unknown pointers yields old deprecated PTR_TO_BTF_ID */
				clear_trusted_flags(&flag);
			}
		} else {
			/*
			 * If not in RCU CS or MEM_RCU pointer can be NULL then
			 * aggressively mark as untrusted otherwise such
			 * pointers will be plain PTR_TO_BTF_ID without flags
			 * and will be allowed to be passed into helpers for
			 * compat reasons.
			 */
			flag = PTR_UNTRUSTED;
		}
	} else {
		/* Old compat. Deprecated */
		clear_trusted_flags(&flag);
	}

	if (atype == BPF_READ && value_regno >= 0) {
		ret = mark_btf_ld_reg(env, regs, value_regno, ret, reg->btf, btf_id, flag);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int check_ptr_to_map_access(struct bpf_verifier_env *env,
				   struct bpf_reg_state *regs,
				   int regno, int off, int size,
				   enum bpf_access_type atype,
				   int value_regno)
{
	struct bpf_reg_state *reg = regs + regno;
	struct bpf_map *map = reg->map_ptr;
	struct bpf_reg_state map_reg;
	enum bpf_type_flag flag = 0;
	const struct btf_type *t;
	const char *tname;
	u32 btf_id;
	int ret;

	if (!btf_vmlinux) {
		verbose(env, "map_ptr access not supported without CONFIG_DEBUG_INFO_BTF\n");
		return -ENOTSUPP;
	}

	if (!map->ops->map_btf_id || !*map->ops->map_btf_id) {
		verbose(env, "map_ptr access not supported for map type %d\n",
			map->map_type);
		return -ENOTSUPP;
	}

	t = btf_type_by_id(btf_vmlinux, *map->ops->map_btf_id);
	tname = btf_name_by_offset(btf_vmlinux, t->name_off);

	if (!env->allow_ptr_leaks) {
		verbose(env,
			"'struct %s' access is allowed only to CAP_PERFMON and CAP_SYS_ADMIN\n",
			tname);
		return -EPERM;
	}

	if (off < 0) {
		verbose(env, "R%d is %s invalid negative access: off=%d\n",
			regno, tname, off);
		return -EACCES;
	}

	if (atype != BPF_READ) {
		verbose(env, "only read from %s is supported\n", tname);
		return -EACCES;
	}

	/* Simulate access to a PTR_TO_BTF_ID */
	memset(&map_reg, 0, sizeof(map_reg));
	ret = mark_btf_ld_reg(env, &map_reg, 0, PTR_TO_BTF_ID,
			      btf_vmlinux, *map->ops->map_btf_id, 0);
	if (ret < 0)
		return ret;
	ret = btf_struct_access(&env->log, &map_reg, off, size, atype, &btf_id, &flag, NULL);
	if (ret < 0)
		return ret;

	if (value_regno >= 0) {
		ret = mark_btf_ld_reg(env, regs, value_regno, ret, btf_vmlinux, btf_id, flag);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* Check that the stack access at the given offset is within bounds. The
 * maximum valid offset is -1.
 *
 * The minimum valid offset is -MAX_BPF_STACK for writes, and
 * -state->allocated_stack for reads.
 */
static int check_stack_slot_within_bounds(struct bpf_verifier_env *env,
                                          s64 off,
                                          struct bpf_func_state *state,
                                          enum bpf_access_type t)
{
	int min_valid_off;

	if (t == BPF_WRITE || env->allow_uninit_stack)
		min_valid_off = -MAX_BPF_STACK;
	else
		min_valid_off = -state->allocated_stack;

	if (off < min_valid_off || off > -1)
		return -EACCES;
	return 0;
}