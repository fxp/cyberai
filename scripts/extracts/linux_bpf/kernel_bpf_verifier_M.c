// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 45/132



	if (!atomic_ptr_type_ok(env, insn->dst_reg, insn)) {
		verbose(env, "BPF_ATOMIC stores into R%d %s is not allowed\n",
			insn->dst_reg,
			reg_type_str(env, reg_state(env, insn->dst_reg)->type));
		return -EACCES;
	}

	if (insn->imm & BPF_FETCH) {
		if (insn->imm == BPF_CMPXCHG)
			load_reg = BPF_REG_0;
		else
			load_reg = insn->src_reg;

		/* check and record load of old value */
		err = check_reg_arg(env, load_reg, DST_OP);
		if (err)
			return err;
	} else {
		/* This instruction accesses a memory location but doesn't
		 * actually load it into a register.
		 */
		load_reg = -1;
	}

	/* Check whether we can read the memory, with second call for fetch
	 * case to simulate the register fill.
	 */
	err = check_mem_access(env, env->insn_idx, insn->dst_reg, insn->off,
			       BPF_SIZE(insn->code), BPF_READ, -1, true, false);
	if (!err && load_reg >= 0)
		err = check_mem_access(env, env->insn_idx, insn->dst_reg,
				       insn->off, BPF_SIZE(insn->code),
				       BPF_READ, load_reg, true, false);
	if (err)
		return err;

	if (is_arena_reg(env, insn->dst_reg)) {
		err = save_aux_ptr_type(env, PTR_TO_ARENA, false);
		if (err)
			return err;
	}
	/* Check whether we can write into the same memory. */
	err = check_mem_access(env, env->insn_idx, insn->dst_reg, insn->off,
			       BPF_SIZE(insn->code), BPF_WRITE, -1, true, false);
	if (err)
		return err;
	return 0;
}

static int check_atomic_load(struct bpf_verifier_env *env,
			     struct bpf_insn *insn)
{
	int err;

	err = check_load_mem(env, insn, true, false, false, "atomic_load");
	if (err)
		return err;

	if (!atomic_ptr_type_ok(env, insn->src_reg, insn)) {
		verbose(env, "BPF_ATOMIC loads from R%d %s is not allowed\n",
			insn->src_reg,
			reg_type_str(env, reg_state(env, insn->src_reg)->type));
		return -EACCES;
	}

	return 0;
}

static int check_atomic_store(struct bpf_verifier_env *env,
			      struct bpf_insn *insn)
{
	int err;

	err = check_store_reg(env, insn, true);
	if (err)
		return err;

	if (!atomic_ptr_type_ok(env, insn->dst_reg, insn)) {
		verbose(env, "BPF_ATOMIC stores into R%d %s is not allowed\n",
			insn->dst_reg,
			reg_type_str(env, reg_state(env, insn->dst_reg)->type));
		return -EACCES;
	}

	return 0;
}

static int check_atomic(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	switch (insn->imm) {
	case BPF_ADD:
	case BPF_ADD | BPF_FETCH:
	case BPF_AND:
	case BPF_AND | BPF_FETCH:
	case BPF_OR:
	case BPF_OR | BPF_FETCH:
	case BPF_XOR:
	case BPF_XOR | BPF_FETCH:
	case BPF_XCHG:
	case BPF_CMPXCHG:
		return check_atomic_rmw(env, insn);
	case BPF_LOAD_ACQ:
		if (BPF_SIZE(insn->code) == BPF_DW && BITS_PER_LONG != 64) {
			verbose(env,
				"64-bit load-acquires are only supported on 64-bit arches\n");
			return -EOPNOTSUPP;
		}
		return check_atomic_load(env, insn);
	case BPF_STORE_REL:
		if (BPF_SIZE(insn->code) == BPF_DW && BITS_PER_LONG != 64) {
			verbose(env,
				"64-bit store-releases are only supported on 64-bit arches\n");
			return -EOPNOTSUPP;
		}
		return check_atomic_store(env, insn);
	default:
		verbose(env, "BPF_ATOMIC uses invalid atomic opcode %02x\n",
			insn->imm);
		return -EINVAL;
	}
}

/* When register 'regno' is used to read the stack (either directly or through
 * a helper function) make sure that it's within stack boundary and, depending
 * on the access type and privileges, that all elements of the stack are
 * initialized.
 *
 * All registers that have been spilled on the stack in the slots within the
 * read offsets are marked as read.
 */
static int check_stack_range_initialized(
		struct bpf_verifier_env *env, int regno, int off,
		int access_size, bool zero_size_allowed,
		enum bpf_access_type type, struct bpf_call_arg_meta *meta)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct bpf_func_state *state = bpf_func(env, reg);
	int err, min_off, max_off, i, j, slot, spi;
	/* Some accesses can write anything into the stack, others are
	 * read-only.
	 */
	bool clobber = type == BPF_WRITE;
	/*
	 * Negative access_size signals global subprog/kfunc arg check where
	 * STACK_POISON slots are acceptable. static stack liveness
	 * might have determined that subprog doesn't read them,
	 * but BTF based global subprog validation isn't accurate enough.
	 */
	bool allow_poison = access_size < 0 || clobber;

	access_size = abs(access_size);

	if (access_size == 0 && !zero_size_allowed) {
		verbose(env, "invalid zero-sized read\n");
		return -EACCES;
	}

	err = check_stack_access_within_bounds(env, regno, off, access_size, type);
	if (err)
		return err;


	if (tnum_is_const(reg->var_off)) {
		min_off = max_off = reg->var_off.value + off;
	} else {
		/* Variable offset is prohibited for unprivileged mode for
		 * simplicity since it requires corresponding support in
		 * Spectre masking for stack ALU.
		 * See also retrieve_ptr_limit().
		 */
		if (!env->bypass_spec_v1) {
			char tn_buf[48];