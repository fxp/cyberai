// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 102/132



						if (no_sext)
							assign_scalar_id_before_mov(env, src_reg);
						copy_register_state(dst_reg, src_reg);
						if (!no_sext)
							clear_scalar_id(dst_reg);
						dst_reg->subreg_def = env->insn_idx + 1;
						coerce_subreg_to_size_sx(dst_reg, insn->off >> 3);
					}
				} else {
					mark_reg_unknown(env, regs,
							 insn->dst_reg);
				}
				zext_32_to_64(dst_reg);
				reg_bounds_sync(dst_reg);
			}
		} else {
			/* case: R = imm
			 * remember the value we stored into this reg
			 */
			/* clear any state __mark_reg_known doesn't set */
			mark_reg_unknown(env, regs, insn->dst_reg);
			regs[insn->dst_reg].type = SCALAR_VALUE;
			if (BPF_CLASS(insn->code) == BPF_ALU64) {
				__mark_reg_known(regs + insn->dst_reg,
						 insn->imm);
			} else {
				__mark_reg_known(regs + insn->dst_reg,
						 (u32)insn->imm);
			}
		}

	} else {	/* all other ALU ops: and, sub, xor, add, ... */

		if (BPF_SRC(insn->code) == BPF_X) {
			/* check src1 operand */
			err = check_reg_arg(env, insn->src_reg, SRC_OP);
			if (err)
				return err;
		}

		/* check src2 operand */
		err = check_reg_arg(env, insn->dst_reg, SRC_OP);
		if (err)
			return err;

		if ((opcode == BPF_MOD || opcode == BPF_DIV) &&
		    BPF_SRC(insn->code) == BPF_K && insn->imm == 0) {
			verbose(env, "div by zero\n");
			return -EINVAL;
		}

		if ((opcode == BPF_LSH || opcode == BPF_RSH ||
		     opcode == BPF_ARSH) && BPF_SRC(insn->code) == BPF_K) {
			int size = BPF_CLASS(insn->code) == BPF_ALU64 ? 64 : 32;

			if (insn->imm < 0 || insn->imm >= size) {
				verbose(env, "invalid shift %d\n", insn->imm);
				return -EINVAL;
			}
		}

		/* check dest operand */
		err = check_reg_arg(env, insn->dst_reg, DST_OP_NO_MARK);
		err = err ?: adjust_reg_min_max_vals(env, insn);
		if (err)
			return err;
	}

	return reg_bounds_sanity_check(env, &regs[insn->dst_reg], "alu");
}

static void find_good_pkt_pointers(struct bpf_verifier_state *vstate,
				   struct bpf_reg_state *dst_reg,
				   enum bpf_reg_type type,
				   bool range_right_open)
{
	struct bpf_func_state *state;
	struct bpf_reg_state *reg;
	int new_range;

	if (dst_reg->umax_value == 0 && range_right_open)
		/* This doesn't give us any range */
		return;

	if (dst_reg->umax_value > MAX_PACKET_OFF)
		/* Risk of overflow.  For instance, ptr + (1<<63) may be less
		 * than pkt_end, but that's because it's also less than pkt.
		 */
		return;

	new_range = dst_reg->umax_value;
	if (range_right_open)
		new_range++;

	/* Examples for register markings:
	 *
	 * pkt_data in dst register:
	 *
	 *   r2 = r3;
	 *   r2 += 8;
	 *   if (r2 > pkt_end) goto <handle exception>
	 *   <access okay>
	 *
	 *   r2 = r3;
	 *   r2 += 8;
	 *   if (r2 < pkt_end) goto <access okay>
	 *   <handle exception>
	 *
	 *   Where:
	 *     r2 == dst_reg, pkt_end == src_reg
	 *     r2=pkt(id=n,off=8,r=0)
	 *     r3=pkt(id=n,off=0,r=0)
	 *
	 * pkt_data in src register:
	 *
	 *   r2 = r3;
	 *   r2 += 8;
	 *   if (pkt_end >= r2) goto <access okay>
	 *   <handle exception>
	 *
	 *   r2 = r3;
	 *   r2 += 8;
	 *   if (pkt_end <= r2) goto <handle exception>
	 *   <access okay>
	 *
	 *   Where:
	 *     pkt_end == dst_reg, r2 == src_reg
	 *     r2=pkt(id=n,off=8,r=0)
	 *     r3=pkt(id=n,off=0,r=0)
	 *
	 * Find register r3 and mark its range as r3=pkt(id=n,off=0,r=8)
	 * or r3=pkt(id=n,off=0,r=8-1), so that range of bytes [r3, r3 + 8)
	 * and [r3, r3 + 8-1) respectively is safe to access depending on
	 * the check.
	 */

	/* If our ids match, then we must have the same max_value.  And we
	 * don't care about the other reg's fixed offset, since if it's too big
	 * the range won't allow anything.
	 * dst_reg->umax_value is known < MAX_PACKET_OFF, therefore it fits in a u16.
	 */
	bpf_for_each_reg_in_vstate(vstate, state, reg, ({
		if (reg->type == type && reg->id == dst_reg->id)
			/* keep the maximum range already checked */
			reg->range = max(reg->range, new_range);
	}));
}

static void regs_refine_cond_op(struct bpf_reg_state *reg1, struct bpf_reg_state *reg2,
				u8 opcode, bool is_jmp32);
static u8 rev_opcode(u8 opcode);

/*
 * Learn more information about live branches by simulating refinement on both branches.
 * regs_refine_cond_op() is sound, so producing ill-formed register bounds for the branch means
 * that branch is dead.
 */
static int simulate_both_branches_taken(struct bpf_verifier_env *env, u8 opcode, bool is_jmp32)
{
	/* Fallthrough (FALSE) branch */
	regs_refine_cond_op(&env->false_reg1, &env->false_reg2, rev_opcode(opcode), is_jmp32);
	reg_bounds_sync(&env->false_reg1);
	reg_bounds_sync(&env->false_reg2);
	/*
	 * If there is a range bounds violation in *any* of the abstract values in either
	 * reg_states in the FALSE branch (i.e. reg1, reg2), the FALSE branch must be dead. Only
	 * TRUE branch will be taken.
	 */
	if (range_bounds_violation(&env->false_reg1) || range_bounds_violation(&env->false_reg2))
		return 1;