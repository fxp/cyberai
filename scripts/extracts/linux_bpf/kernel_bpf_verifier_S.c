// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 51/132



		err = mark_stack_slots_iter(env, meta, reg, insn_idx, meta->btf, btf_id, nr_slots);
		if (err)
			return err;
	} else {
		/* iter_next() or iter_destroy(), as well as any kfunc
		 * accepting iter argument, expect initialized iter state
		 */
		err = is_iter_reg_valid_init(env, reg, meta->btf, btf_id, nr_slots);
		switch (err) {
		case 0:
			break;
		case -EINVAL:
			verbose(env, "expected an initialized iter_%s as arg #%d\n",
				iter_type_str(meta->btf, btf_id), regno - 1);
			return err;
		case -EPROTO:
			verbose(env, "expected an RCU CS when using %s\n", meta->func_name);
			return err;
		default:
			return err;
		}

		spi = iter_get_spi(env, reg, nr_slots);
		if (spi < 0)
			return spi;

		err = mark_iter_read(env, reg, spi, nr_slots);
		if (err)
			return err;

		/* remember meta->iter info for process_iter_next_call() */
		meta->iter.spi = spi;
		meta->iter.frameno = reg->frameno;
		meta->ref_obj_id = iter_ref_obj_id(env, reg, spi);

		if (is_iter_destroy_kfunc(meta)) {
			err = unmark_stack_slots_iter(env, reg, nr_slots);
			if (err)
				return err;
		}
	}

	return 0;
}

/* Look for a previous loop entry at insn_idx: nearest parent state
 * stopped at insn_idx with callsites matching those in cur->frame.
 */
static struct bpf_verifier_state *find_prev_entry(struct bpf_verifier_env *env,
						  struct bpf_verifier_state *cur,
						  int insn_idx)
{
	struct bpf_verifier_state_list *sl;
	struct bpf_verifier_state *st;
	struct list_head *pos, *head;

	/* Explored states are pushed in stack order, most recent states come first */
	head = bpf_explored_state(env, insn_idx);
	list_for_each(pos, head) {
		sl = container_of(pos, struct bpf_verifier_state_list, node);
		/* If st->branches != 0 state is a part of current DFS verification path,
		 * hence cur & st for a loop.
		 */
		st = &sl->state;
		if (st->insn_idx == insn_idx && st->branches && same_callsites(st, cur) &&
		    st->dfs_depth < cur->dfs_depth)
			return st;
	}

	return NULL;
}

/*
 * Check if scalar registers are exact for the purpose of not widening.
 * More lenient than regs_exact()
 */
static bool scalars_exact_for_widen(const struct bpf_reg_state *rold,
				    const struct bpf_reg_state *rcur)
{
	return !memcmp(rold, rcur, offsetof(struct bpf_reg_state, id));
}

static void maybe_widen_reg(struct bpf_verifier_env *env,
			    struct bpf_reg_state *rold, struct bpf_reg_state *rcur)
{
	if (rold->type != SCALAR_VALUE)
		return;
	if (rold->type != rcur->type)
		return;
	if (rold->precise || rcur->precise || scalars_exact_for_widen(rold, rcur))
		return;
	__mark_reg_unknown(env, rcur);
}

static int widen_imprecise_scalars(struct bpf_verifier_env *env,
				   struct bpf_verifier_state *old,
				   struct bpf_verifier_state *cur)
{
	struct bpf_func_state *fold, *fcur;
	int i, fr, num_slots;

	for (fr = old->curframe; fr >= 0; fr--) {
		fold = old->frame[fr];
		fcur = cur->frame[fr];

		for (i = 0; i < MAX_BPF_REG; i++)
			maybe_widen_reg(env,
					&fold->regs[i],
					&fcur->regs[i]);

		num_slots = min(fold->allocated_stack / BPF_REG_SIZE,
				fcur->allocated_stack / BPF_REG_SIZE);
		for (i = 0; i < num_slots; i++) {
			if (!bpf_is_spilled_reg(&fold->stack[i]) ||
			    !bpf_is_spilled_reg(&fcur->stack[i]))
				continue;

			maybe_widen_reg(env,
					&fold->stack[i].spilled_ptr,
					&fcur->stack[i].spilled_ptr);
		}
	}
	return 0;
}

static struct bpf_reg_state *get_iter_from_state(struct bpf_verifier_state *cur_st,
						 struct bpf_kfunc_call_arg_meta *meta)
{
	int iter_frameno = meta->iter.frameno;
	int iter_spi = meta->iter.spi;

	return &cur_st->frame[iter_frameno]->stack[iter_spi].spilled_ptr;
}