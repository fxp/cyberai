// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 46/132



			tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
			verbose(env, "R%d variable offset stack access prohibited for !root, var_off=%s\n",
				regno, tn_buf);
			return -EACCES;
		}
		/* Only initialized buffer on stack is allowed to be accessed
		 * with variable offset. With uninitialized buffer it's hard to
		 * guarantee that whole memory is marked as initialized on
		 * helper return since specific bounds are unknown what may
		 * cause uninitialized stack leaking.
		 */
		if (meta && meta->raw_mode)
			meta = NULL;

		min_off = reg->smin_value + off;
		max_off = reg->smax_value + off;
	}

	if (meta && meta->raw_mode) {
		/* Ensure we won't be overwriting dynptrs when simulating byte
		 * by byte access in check_helper_call using meta.access_size.
		 * This would be a problem if we have a helper in the future
		 * which takes:
		 *
		 *	helper(uninit_mem, len, dynptr)
		 *
		 * Now, uninint_mem may overlap with dynptr pointer. Hence, it
		 * may end up writing to dynptr itself when touching memory from
		 * arg 1. This can be relaxed on a case by case basis for known
		 * safe cases, but reject due to the possibilitiy of aliasing by
		 * default.
		 */
		for (i = min_off; i < max_off + access_size; i++) {
			int stack_off = -i - 1;

			spi = bpf_get_spi(i);
			/* raw_mode may write past allocated_stack */
			if (state->allocated_stack <= stack_off)
				continue;
			if (state->stack[spi].slot_type[stack_off % BPF_REG_SIZE] == STACK_DYNPTR) {
				verbose(env, "potential write to dynptr at off=%d disallowed\n", i);
				return -EACCES;
			}
		}
		meta->access_size = access_size;
		meta->regno = regno;
		return 0;
	}

	for (i = min_off; i < max_off + access_size; i++) {
		u8 *stype;

		slot = -i - 1;
		spi = slot / BPF_REG_SIZE;
		if (state->allocated_stack <= slot) {
			verbose(env, "allocated_stack too small\n");
			return -EFAULT;
		}

		stype = &state->stack[spi].slot_type[slot % BPF_REG_SIZE];
		if (*stype == STACK_MISC)
			goto mark;
		if ((*stype == STACK_ZERO) ||
		    (*stype == STACK_INVALID && env->allow_uninit_stack)) {
			if (clobber) {
				/* helper can write anything into the stack */
				*stype = STACK_MISC;
			}
			goto mark;
		}

		if (bpf_is_spilled_reg(&state->stack[spi]) &&
		    (state->stack[spi].spilled_ptr.type == SCALAR_VALUE ||
		     env->allow_ptr_leaks)) {
			if (clobber) {
				__mark_reg_unknown(env, &state->stack[spi].spilled_ptr);
				for (j = 0; j < BPF_REG_SIZE; j++)
					scrub_spilled_slot(&state->stack[spi].slot_type[j]);
			}
			goto mark;
		}

		if (*stype == STACK_POISON) {
			if (allow_poison)
				goto mark;
			verbose(env, "reading from stack R%d off %d+%d size %d, slot poisoned by dead code elimination\n",
				regno, min_off, i - min_off, access_size);
		} else if (tnum_is_const(reg->var_off)) {
			verbose(env, "invalid read from stack R%d off %d+%d size %d\n",
				regno, min_off, i - min_off, access_size);
		} else {
			char tn_buf[48];

			tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
			verbose(env, "invalid read from stack R%d var_off %s+%d size %d\n",
				regno, tn_buf, i - min_off, access_size);
		}
		return -EACCES;
mark:
		;
	}
	return 0;
}

static int check_helper_mem_access(struct bpf_verifier_env *env, int regno,
				   int access_size, enum bpf_access_type access_type,
				   bool zero_size_allowed,
				   struct bpf_call_arg_meta *meta)
{
	struct bpf_reg_state *regs = cur_regs(env), *reg = &regs[regno];
	u32 *max_access;

	switch (base_type(reg->type)) {
	case PTR_TO_PACKET:
	case PTR_TO_PACKET_META:
		return check_packet_access(env, regno, 0, access_size,
					   zero_size_allowed);
	case PTR_TO_MAP_KEY:
		if (access_type == BPF_WRITE) {
			verbose(env, "R%d cannot write into %s\n", regno,
				reg_type_str(env, reg->type));
			return -EACCES;
		}
		return check_mem_region_access(env, regno, 0, access_size,
					       reg->map_ptr->key_size, false);
	case PTR_TO_MAP_VALUE:
		if (check_map_access_type(env, regno, 0, access_size, access_type))
			return -EACCES;
		return check_map_access(env, regno, 0, access_size,
					zero_size_allowed, ACCESS_HELPER);
	case PTR_TO_MEM:
		if (type_is_rdonly_mem(reg->type)) {
			if (access_type == BPF_WRITE) {
				verbose(env, "R%d cannot write into %s\n", regno,
					reg_type_str(env, reg->type));
				return -EACCES;
			}
		}
		return check_mem_region_access(env, regno, 0,
					       access_size, reg->mem_size,
					       zero_size_allowed);
	case PTR_TO_BUF:
		if (type_is_rdonly_mem(reg->type)) {
			if (access_type == BPF_WRITE) {
				verbose(env, "R%d cannot write into %s\n", regno,
					reg_type_str(env, reg->type));
				return -EACCES;
			}