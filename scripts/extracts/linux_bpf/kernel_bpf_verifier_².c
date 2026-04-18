// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 114/132



	/* A bitmask specifying which caller saved registers are clobbered
	 * by a call to a helper/kfunc *as if* this helper/kfunc follows
	 * bpf_fastcall contract:
	 * - includes R0 if function is non-void;
	 * - includes R1-R5 if corresponding parameter has is described
	 *   in the function prototype.
	 */
	clobbered_regs_mask = GENMASK(cs.num_params, cs.is_void ? 1 : 0);
	/* e.g. if helper call clobbers r{0,1}, expect r{2,3,4,5} in the pattern */
	expected_regs_mask = ~clobbered_regs_mask & ALL_CALLER_SAVED_REGS;

	/* match pairs of form:
	 *
	 * *(u64 *)(r10 - Y) = rX   (where Y % 8 == 0)
	 * ...
	 * call %[to_be_inlined]
	 * ...
	 * rX = *(u64 *)(r10 - Y)
	 */
	for (i = 1, off = lowest_off; i <= ARRAY_SIZE(caller_saved); ++i, off += BPF_REG_SIZE) {
		if (insn_idx - i < 0 || insn_idx + i >= env->prog->len)
			break;
		stx = &insns[insn_idx - i];
		ldx = &insns[insn_idx + i];
		/* must be a stack spill/fill pair */
		if (stx->code != (BPF_STX | BPF_MEM | BPF_DW) ||
		    ldx->code != (BPF_LDX | BPF_MEM | BPF_DW) ||
		    stx->dst_reg != BPF_REG_10 ||
		    ldx->src_reg != BPF_REG_10)
			break;
		/* must be a spill/fill for the same reg */
		if (stx->src_reg != ldx->dst_reg)
			break;
		/* must be one of the previously unseen registers */
		if ((BIT(stx->src_reg) & expected_regs_mask) == 0)
			break;
		/* must be a spill/fill for the same expected offset,
		 * no need to check offset alignment, BPF_DW stack access
		 * is always 8-byte aligned.
		 */
		if (stx->off != off || ldx->off != off)
			break;
		expected_regs_mask &= ~BIT(stx->src_reg);
		env->insn_aux_data[insn_idx - i].fastcall_pattern = 1;
		env->insn_aux_data[insn_idx + i].fastcall_pattern = 1;
	}
	if (i == 1)
		return;

	/* Conditionally set 'fastcall_spills_num' to allow forward
	 * compatibility when more helper functions are marked as
	 * bpf_fastcall at compile time than current kernel supports, e.g:
	 *
	 *   1: *(u64 *)(r10 - 8) = r1
	 *   2: call A                  ;; assume A is bpf_fastcall for current kernel
	 *   3: r1 = *(u64 *)(r10 - 8)
	 *   4: *(u64 *)(r10 - 8) = r1
	 *   5: call B                  ;; assume B is not bpf_fastcall for current kernel
	 *   6: r1 = *(u64 *)(r10 - 8)
	 *
	 * There is no need to block bpf_fastcall rewrite for such program.
	 * Set 'fastcall_pattern' for both calls to keep check_fastcall_stack_contract() happy,
	 * don't set 'fastcall_spills_num' for call B so that remove_fastcall_spills_fills()
	 * does not remove spill/fill pair {4,6}.
	 */
	if (cs.fastcall)
		env->insn_aux_data[insn_idx].fastcall_spills_num = i - 1;
	else
		subprog->keep_fastcall_stack = 1;
	subprog->fastcall_stack_off = min(subprog->fastcall_stack_off, off);
}

static int mark_fastcall_patterns(struct bpf_verifier_env *env)
{
	struct bpf_subprog_info *subprog = env->subprog_info;
	struct bpf_insn *insn;
	s16 lowest_off;
	int s, i;

	for (s = 0; s < env->subprog_cnt; ++s, ++subprog) {
		/* find lowest stack spill offset used in this subprog */
		lowest_off = 0;
		for (i = subprog->start; i < (subprog + 1)->start; ++i) {
			insn = env->prog->insnsi + i;
			if (insn->code != (BPF_STX | BPF_MEM | BPF_DW) ||
			    insn->dst_reg != BPF_REG_10)
				continue;
			lowest_off = min(lowest_off, insn->off);
		}
		/* use this offset to find fastcall patterns */
		for (i = subprog->start; i < (subprog + 1)->start; ++i) {
			insn = env->prog->insnsi + i;
			if (insn->code != (BPF_JMP | BPF_CALL))
				continue;
			mark_fastcall_pattern_for_call(env, subprog, i, lowest_off);
		}
	}
	return 0;
}

static void adjust_btf_func(struct bpf_verifier_env *env)
{
	struct bpf_prog_aux *aux = env->prog->aux;
	int i;

	if (!aux->func_info)
		return;

	/* func_info is not available for hidden subprogs */
	for (i = 0; i < env->subprog_cnt - env->hidden_subprog_cnt; i++)
		aux->func_info[i].insn_off = env->subprog_info[i].start;
}

/* Find id in idset and increment its count, or add new entry */
static void idset_cnt_inc(struct bpf_idset *idset, u32 id)
{
	u32 i;

	for (i = 0; i < idset->num_ids; i++) {
		if (idset->entries[i].id == id) {
			idset->entries[i].cnt++;
			return;
		}
	}
	/* New id */
	if (idset->num_ids < BPF_ID_MAP_SIZE) {
		idset->entries[idset->num_ids].id = id;
		idset->entries[idset->num_ids].cnt = 1;
		idset->num_ids++;
	}
}

/* Find id in idset and return its count, or 0 if not found */
static u32 idset_cnt_get(struct bpf_idset *idset, u32 id)
{
	u32 i;

	for (i = 0; i < idset->num_ids; i++) {
		if (idset->entries[i].id == id)
			return idset->entries[i].cnt;
	}
	return 0;
}

/*
 * Clear singular scalar ids in a state.
 * A register with a non-zero id is called singular if no other register shares
 * the same base id. Such registers can be treated as independent (id=0).
 */
void bpf_clear_singular_ids(struct bpf_verifier_env *env,
			    struct bpf_verifier_state *st)
{
	struct bpf_idset *idset = &env->idset_scratch;
	struct bpf_func_state *func;
	struct bpf_reg_state *reg;

	idset->num_ids = 0;