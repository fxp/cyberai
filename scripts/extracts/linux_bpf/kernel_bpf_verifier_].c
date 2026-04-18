// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 29/132



				if (spill_cnt == size &&
				    tnum_is_const(reg->var_off) && reg->var_off.value == 0) {
					__mark_reg_const_zero(env, &state->regs[dst_regno]);
					/* this IS register fill, so keep insn_flags */
				} else if (zero_cnt == size) {
					/* similarly to mark_reg_stack_read(), preserve zeroes */
					__mark_reg_const_zero(env, &state->regs[dst_regno]);
					insn_flags = 0; /* not restoring original register state */
				} else {
					mark_reg_unknown(env, state->regs, dst_regno);
					insn_flags = 0; /* not restoring original register state */
				}
			}
		} else if (dst_regno >= 0) {
			/* restore register state from stack */
			if (env->bpf_capable)
				/* Ensure stack slot has an ID to build a relation
				 * with the destination register on fill.
				 */
				assign_scalar_id_before_mov(env, reg);
			copy_register_state(&state->regs[dst_regno], reg);
			/* mark reg as written since spilled pointer state likely
			 * has its liveness marks cleared by is_state_visited()
			 * which resets stack/reg liveness for state transitions
			 */
		} else if (__is_pointer_value(env->allow_ptr_leaks, reg)) {
			/* If dst_regno==-1, the caller is asking us whether
			 * it is acceptable to use this value as a SCALAR_VALUE
			 * (e.g. for XADD).
			 * We must not allow unprivileged callers to do that
			 * with spilled pointers.
			 */
			verbose(env, "leaking pointer from stack off %d\n",
				off);
			return -EACCES;
		}
	} else {
		for (i = 0; i < size; i++) {
			type = stype[(slot - i) % BPF_REG_SIZE];
			if (type == STACK_MISC)
				continue;
			if (type == STACK_ZERO)
				continue;
			if (type == STACK_INVALID && env->allow_uninit_stack)
				continue;
			if (type == STACK_POISON) {
				verbose(env, "reading from stack off %d+%d size %d, slot poisoned by dead code elimination\n",
					off, i, size);
			} else {
				verbose(env, "invalid read from stack off %d+%d size %d\n",
					off, i, size);
			}
			return -EACCES;
		}
		if (dst_regno >= 0)
			mark_reg_stack_read(env, reg_state, off, off + size, dst_regno);
		insn_flags = 0; /* we are not restoring spilled register */
	}
	if (insn_flags)
		return bpf_push_jmp_history(env, env->cur_state, insn_flags, 0);
	return 0;
}

enum bpf_access_src {
	ACCESS_DIRECT = 1,  /* the access is performed by an instruction */
	ACCESS_HELPER = 2,  /* the access is performed by a helper */
};

static int check_stack_range_initialized(struct bpf_verifier_env *env,
					 int regno, int off, int access_size,
					 bool zero_size_allowed,
					 enum bpf_access_type type,
					 struct bpf_call_arg_meta *meta);

static struct bpf_reg_state *reg_state(struct bpf_verifier_env *env, int regno)
{
	return cur_regs(env) + regno;
}

/* Read the stack at 'ptr_regno + off' and put the result into the register
 * 'dst_regno'.
 * 'off' includes the pointer register's fixed offset(i.e. 'ptr_regno.off'),
 * but not its variable offset.
 * 'size' is assumed to be <= reg size and the access is assumed to be aligned.
 *
 * As opposed to check_stack_read_fixed_off, this function doesn't deal with
 * filling registers (i.e. reads of spilled register cannot be detected when
 * the offset is not fixed). We conservatively mark 'dst_regno' as containing
 * SCALAR_VALUE. That's why we assert that the 'ptr_regno' has a variable
 * offset; for a fixed offset check_stack_read_fixed_off should be used
 * instead.
 */
static int check_stack_read_var_off(struct bpf_verifier_env *env,
				    int ptr_regno, int off, int size, int dst_regno)
{
	/* The state of the source register. */
	struct bpf_reg_state *reg = reg_state(env, ptr_regno);
	struct bpf_func_state *ptr_state = bpf_func(env, reg);
	int err;
	int min_off, max_off;

	/* Note that we pass a NULL meta, so raw access will not be permitted.
	 */
	err = check_stack_range_initialized(env, ptr_regno, off, size,
					    false, BPF_READ, NULL);
	if (err)
		return err;

	min_off = reg->smin_value + off;
	max_off = reg->smax_value + off;
	mark_reg_stack_read(env, ptr_state, min_off, max_off + size, dst_regno);
	check_fastcall_stack_contract(env, ptr_state, env->insn_idx, min_off);
	return 0;
}

/* check_stack_read dispatches to check_stack_read_fixed_off or
 * check_stack_read_var_off.
 *
 * The caller must ensure that the offset falls within the allocated stack
 * bounds.
 *
 * 'dst_regno' is a register which will receive the value from the stack. It
 * can be -1, meaning that the read value is not going to a register.
 */
static int check_stack_read(struct bpf_verifier_env *env,
			    int ptr_regno, int off, int size,
			    int dst_regno)
{
	struct bpf_reg_state *reg = reg_state(env, ptr_regno);
	struct bpf_func_state *state = bpf_func(env, reg);
	int err;
	/* Some accesses are only permitted with a static offset. */
	bool var_off = !tnum_is_const(reg->var_off);