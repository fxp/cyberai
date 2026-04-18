// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 28/132



/* When register 'dst_regno' is assigned some values from stack[min_off,
 * max_off), we set the register's type according to the types of the
 * respective stack slots. If all the stack values are known to be zeros, then
 * so is the destination reg. Otherwise, the register is considered to be
 * SCALAR. This function does not deal with register filling; the caller must
 * ensure that all spilled registers in the stack range have been marked as
 * read.
 */
static void mark_reg_stack_read(struct bpf_verifier_env *env,
				/* func where src register points to */
				struct bpf_func_state *ptr_state,
				int min_off, int max_off, int dst_regno)
{
	struct bpf_verifier_state *vstate = env->cur_state;
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	int i, slot, spi;
	u8 *stype;
	int zeros = 0;

	for (i = min_off; i < max_off; i++) {
		slot = -i - 1;
		spi = slot / BPF_REG_SIZE;
		mark_stack_slot_scratched(env, spi);
		stype = ptr_state->stack[spi].slot_type;
		if (stype[slot % BPF_REG_SIZE] != STACK_ZERO)
			break;
		zeros++;
	}
	if (zeros == max_off - min_off) {
		/* Any access_size read into register is zero extended,
		 * so the whole register == const_zero.
		 */
		__mark_reg_const_zero(env, &state->regs[dst_regno]);
	} else {
		/* have read misc data from the stack */
		mark_reg_unknown(env, state->regs, dst_regno);
	}
}

/* Read the stack at 'off' and put the results into the register indicated by
 * 'dst_regno'. It handles reg filling if the addressed stack slot is a
 * spilled reg.
 *
 * 'dst_regno' can be -1, meaning that the read value is not going to a
 * register.
 *
 * The access is assumed to be within the current stack bounds.
 */
static int check_stack_read_fixed_off(struct bpf_verifier_env *env,
				      /* func where src register points to */
				      struct bpf_func_state *reg_state,
				      int off, int size, int dst_regno)
{
	struct bpf_verifier_state *vstate = env->cur_state;
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	int i, slot = -off - 1, spi = slot / BPF_REG_SIZE;
	struct bpf_reg_state *reg;
	u8 *stype, type;
	int insn_flags = insn_stack_access_flags(reg_state->frameno, spi);

	stype = reg_state->stack[spi].slot_type;
	reg = &reg_state->stack[spi].spilled_ptr;

	mark_stack_slot_scratched(env, spi);
	check_fastcall_stack_contract(env, state, env->insn_idx, off);

	if (bpf_is_spilled_reg(&reg_state->stack[spi])) {
		u8 spill_size = 1;

		for (i = BPF_REG_SIZE - 1; i > 0 && stype[i - 1] == STACK_SPILL; i--)
			spill_size++;

		if (size != BPF_REG_SIZE || spill_size != BPF_REG_SIZE) {
			if (reg->type != SCALAR_VALUE) {
				verbose_linfo(env, env->insn_idx, "; ");
				verbose(env, "invalid size of register fill\n");
				return -EACCES;
			}

			if (dst_regno < 0)
				return 0;

			if (size <= spill_size &&
			    bpf_stack_narrow_access_ok(off, size, spill_size)) {
				/* The earlier check_reg_arg() has decided the
				 * subreg_def for this insn.  Save it first.
				 */
				s32 subreg_def = state->regs[dst_regno].subreg_def;

				if (env->bpf_capable && size == 4 && spill_size == 4 &&
				    get_reg_width(reg) <= 32)
					/* Ensure stack slot has an ID to build a relation
					 * with the destination register on fill.
					 */
					assign_scalar_id_before_mov(env, reg);
				copy_register_state(&state->regs[dst_regno], reg);
				state->regs[dst_regno].subreg_def = subreg_def;

				/* Break the relation on a narrowing fill.
				 * coerce_reg_to_size will adjust the boundaries.
				 */
				if (get_reg_width(reg) > size * BITS_PER_BYTE)
					clear_scalar_id(&state->regs[dst_regno]);
			} else {
				int spill_cnt = 0, zero_cnt = 0;

				for (i = 0; i < size; i++) {
					type = stype[(slot - i) % BPF_REG_SIZE];
					if (type == STACK_SPILL) {
						spill_cnt++;
						continue;
					}
					if (type == STACK_MISC)
						continue;
					if (type == STACK_ZERO) {
						zero_cnt++;
						continue;
					}
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