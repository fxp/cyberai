// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 106/132



	err = reg_bounds_sanity_check(env, &env->true_reg1, "true_reg1");
	err = err ?: reg_bounds_sanity_check(env, &env->true_reg2, "true_reg2");
	err = err ?: reg_bounds_sanity_check(env, &env->false_reg1, "false_reg1");
	err = err ?: reg_bounds_sanity_check(env, &env->false_reg2, "false_reg2");
	return err;
}

static void mark_ptr_or_null_reg(struct bpf_func_state *state,
				 struct bpf_reg_state *reg, u32 id,
				 bool is_null)
{
	if (type_may_be_null(reg->type) && reg->id == id &&
	    (is_rcu_reg(reg) || !WARN_ON_ONCE(!reg->id))) {
		/* Old offset should have been known-zero, because we don't
		 * allow pointer arithmetic on pointers that might be NULL.
		 * If we see this happening, don't convert the register.
		 *
		 * But in some cases, some helpers that return local kptrs
		 * advance offset for the returned pointer. In those cases,
		 * it is fine to expect to see reg->var_off.
		 */
		if (!(type_is_ptr_alloc_obj(reg->type) || type_is_non_owning_ref(reg->type)) &&
		    WARN_ON_ONCE(!tnum_equals_const(reg->var_off, 0)))
			return;
		if (is_null) {
			/* We don't need id and ref_obj_id from this point
			 * onwards anymore, thus we should better reset it,
			 * so that state pruning has chances to take effect.
			 */
			__mark_reg_known_zero(reg);
			reg->type = SCALAR_VALUE;

			return;
		}

		mark_ptr_not_null_reg(reg);

		if (!reg_may_point_to_spin_lock(reg)) {
			/* For not-NULL ptr, reg->ref_obj_id will be reset
			 * in release_reference().
			 *
			 * reg->id is still used by spin_lock ptr. Other
			 * than spin_lock ptr type, reg->id can be reset.
			 */
			reg->id = 0;
		}
	}
}

/* The logic is similar to find_good_pkt_pointers(), both could eventually
 * be folded together at some point.
 */
static void mark_ptr_or_null_regs(struct bpf_verifier_state *vstate, u32 regno,
				  bool is_null)
{
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	struct bpf_reg_state *regs = state->regs, *reg;
	u32 ref_obj_id = regs[regno].ref_obj_id;
	u32 id = regs[regno].id;

	if (ref_obj_id && ref_obj_id == id && is_null)
		/* regs[regno] is in the " == NULL" branch.
		 * No one could have freed the reference state before
		 * doing the NULL check.
		 */
		WARN_ON_ONCE(release_reference_nomark(vstate, id));

	bpf_for_each_reg_in_vstate(vstate, state, reg, ({
		mark_ptr_or_null_reg(state, reg, id, is_null);
	}));
}

static bool try_match_pkt_pointers(const struct bpf_insn *insn,
				   struct bpf_reg_state *dst_reg,
				   struct bpf_reg_state *src_reg,
				   struct bpf_verifier_state *this_branch,
				   struct bpf_verifier_state *other_branch)
{
	if (BPF_SRC(insn->code) != BPF_X)
		return false;

	/* Pointers are always 64-bit. */
	if (BPF_CLASS(insn->code) == BPF_JMP32)
		return false;