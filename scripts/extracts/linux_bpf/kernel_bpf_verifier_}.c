// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 61/132



static bool check_btf_id_ok(const struct bpf_func_proto *fn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fn->arg_type); i++) {
		if (base_type(fn->arg_type[i]) == ARG_PTR_TO_BTF_ID)
			return !!fn->arg_btf_id[i];
		if (base_type(fn->arg_type[i]) == ARG_PTR_TO_SPIN_LOCK)
			return fn->arg_btf_id[i] == BPF_PTR_POISON;
		if (base_type(fn->arg_type[i]) != ARG_PTR_TO_BTF_ID && fn->arg_btf_id[i] &&
		    /* arg_btf_id and arg_size are in a union. */
		    (base_type(fn->arg_type[i]) != ARG_PTR_TO_MEM ||
		     !(fn->arg_type[i] & MEM_FIXED_SIZE)))
			return false;
	}

	return true;
}

static bool check_mem_arg_rw_flag_ok(const struct bpf_func_proto *fn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fn->arg_type); i++) {
		enum bpf_arg_type arg_type = fn->arg_type[i];

		if (base_type(arg_type) != ARG_PTR_TO_MEM)
			continue;
		if (!(arg_type & (MEM_WRITE | MEM_RDONLY)))
			return false;
	}

	return true;
}

static int check_func_proto(const struct bpf_func_proto *fn)
{
	return check_raw_mode_ok(fn) &&
	       check_arg_pair_ok(fn) &&
	       check_mem_arg_rw_flag_ok(fn) &&
	       check_btf_id_ok(fn) ? 0 : -EINVAL;
}

/* Packet data might have moved, any old PTR_TO_PACKET[_META,_END]
 * are now invalid, so turn them into unknown SCALAR_VALUE.
 *
 * This also applies to dynptr slices belonging to skb and xdp dynptrs,
 * since these slices point to packet data.
 */
static void clear_all_pkt_pointers(struct bpf_verifier_env *env)
{
	struct bpf_func_state *state;
	struct bpf_reg_state *reg;

	bpf_for_each_reg_in_vstate(env->cur_state, state, reg, ({
		if (reg_is_pkt_pointer_any(reg) || reg_is_dynptr_slice_pkt(reg))
			mark_reg_invalid(env, reg);
	}));
}

enum {
	AT_PKT_END = -1,
	BEYOND_PKT_END = -2,
};

static void mark_pkt_end(struct bpf_verifier_state *vstate, int regn, bool range_open)
{
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	struct bpf_reg_state *reg = &state->regs[regn];

	if (reg->type != PTR_TO_PACKET)
		/* PTR_TO_PACKET_META is not supported yet */
		return;

	/* The 'reg' is pkt > pkt_end or pkt >= pkt_end.
	 * How far beyond pkt_end it goes is unknown.
	 * if (!range_open) it's the case of pkt >= pkt_end
	 * if (range_open) it's the case of pkt > pkt_end
	 * hence this pointer is at least 1 byte bigger than pkt_end
	 */
	if (range_open)
		reg->range = BEYOND_PKT_END;
	else
		reg->range = AT_PKT_END;
}

static int release_reference_nomark(struct bpf_verifier_state *state, int ref_obj_id)
{
	int i;

	for (i = 0; i < state->acquired_refs; i++) {
		if (state->refs[i].type != REF_TYPE_PTR)
			continue;
		if (state->refs[i].id == ref_obj_id) {
			release_reference_state(state, i);
			return 0;
		}
	}
	return -EINVAL;
}

/* The pointer with the specified id has released its reference to kernel
 * resources. Identify all copies of the same pointer and clear the reference.
 *
 * This is the release function corresponding to acquire_reference(). Idempotent.
 */
static int release_reference(struct bpf_verifier_env *env, int ref_obj_id)
{
	struct bpf_verifier_state *vstate = env->cur_state;
	struct bpf_func_state *state;
	struct bpf_reg_state *reg;
	int err;

	err = release_reference_nomark(vstate, ref_obj_id);
	if (err)
		return err;

	bpf_for_each_reg_in_vstate(vstate, state, reg, ({
		if (reg->ref_obj_id == ref_obj_id)
			mark_reg_invalid(env, reg);
	}));

	return 0;
}

static void invalidate_non_owning_refs(struct bpf_verifier_env *env)
{
	struct bpf_func_state *unused;
	struct bpf_reg_state *reg;

	bpf_for_each_reg_in_vstate(env->cur_state, unused, reg, ({
		if (type_is_non_owning_ref(reg->type))
			mark_reg_invalid(env, reg);
	}));
}

static void clear_caller_saved_regs(struct bpf_verifier_env *env,
				    struct bpf_reg_state *regs)
{
	int i;

	/* after the call registers r0 - r5 were scratched */
	for (i = 0; i < CALLER_SAVED_REGS; i++) {
		bpf_mark_reg_not_init(env, &regs[caller_saved[i]]);
		__check_reg_arg(env, regs, caller_saved[i], DST_OP_NO_MARK);
	}
}

typedef int (*set_callee_state_fn)(struct bpf_verifier_env *env,
				   struct bpf_func_state *caller,
				   struct bpf_func_state *callee,
				   int insn_idx);

static int set_callee_state(struct bpf_verifier_env *env,
			    struct bpf_func_state *caller,
			    struct bpf_func_state *callee, int insn_idx);

static int setup_func_entry(struct bpf_verifier_env *env, int subprog, int callsite,
			    set_callee_state_fn set_callee_state_cb,
			    struct bpf_verifier_state *state)
{
	struct bpf_func_state *caller, *callee;
	int err;

	if (state->curframe + 1 >= MAX_CALL_FRAMES) {
		verbose(env, "the call stack of %d frames is too deep\n",
			state->curframe + 2);
		return -E2BIG;
	}

	if (state->frame[state->curframe + 1]) {
		verifier_bug(env, "Frame %d already allocated", state->curframe + 1);
		return -EFAULT;
	}

	caller = state->frame[state->curframe];
	callee = kzalloc_obj(*callee, GFP_KERNEL_ACCOUNT);
	if (!callee)
		return -ENOMEM;
	state->frame[state->curframe + 1] = callee;