// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 76/132



	/* Enforce strict type matching for calls to kfuncs that are acquiring
	 * or releasing a reference, or are no-cast aliases. We do _not_
	 * enforce strict matching for kfuncs by default,
	 * as we want to enable BPF programs to pass types that are bitwise
	 * equivalent without forcing them to explicitly cast with something
	 * like bpf_cast_to_kern_ctx().
	 *
	 * For example, say we had a type like the following:
	 *
	 * struct bpf_cpumask {
	 *	cpumask_t cpumask;
	 *	refcount_t usage;
	 * };
	 *
	 * Note that as specified in <linux/cpumask.h>, cpumask_t is typedef'ed
	 * to a struct cpumask, so it would be safe to pass a struct
	 * bpf_cpumask * to a kfunc expecting a struct cpumask *.
	 *
	 * The philosophy here is similar to how we allow scalars of different
	 * types to be passed to kfuncs as long as the size is the same. The
	 * only difference here is that we're simply allowing
	 * btf_struct_ids_match() to walk the struct at the 0th offset, and
	 * resolve types.
	 */
	if ((is_kfunc_release(meta) && reg->ref_obj_id) ||
	    btf_type_ids_nocast_alias(&env->log, reg_btf, reg_ref_id, meta->btf, ref_id))
		strict_type_match = true;

	WARN_ON_ONCE(is_kfunc_release(meta) && !tnum_is_const(reg->var_off));

	reg_ref_t = btf_type_skip_modifiers(reg_btf, reg_ref_id, &reg_ref_id);
	reg_ref_tname = btf_name_by_offset(reg_btf, reg_ref_t->name_off);
	struct_same = btf_struct_ids_match(&env->log, reg_btf, reg_ref_id, reg->var_off.value,
					   meta->btf, ref_id, strict_type_match);
	/* If kfunc is accepting a projection type (ie. __sk_buff), it cannot
	 * actually use it -- it must cast to the underlying type. So we allow
	 * caller to pass in the underlying type.
	 */
	taking_projection = btf_is_projection_of(ref_tname, reg_ref_tname);
	if (!taking_projection && !struct_same) {
		verbose(env, "kernel function %s args#%d expected pointer to %s %s but R%d has a pointer to %s %s\n",
			meta->func_name, argno, btf_type_str(ref_t), ref_tname, argno + 1,
			btf_type_str(reg_ref_t), reg_ref_tname);
		return -EINVAL;
	}
	return 0;
}

static int process_irq_flag(struct bpf_verifier_env *env, int regno,
			     struct bpf_kfunc_call_arg_meta *meta)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	int err, kfunc_class = IRQ_NATIVE_KFUNC;
	bool irq_save;

	if (meta->func_id == special_kfunc_list[KF_bpf_local_irq_save] ||
	    meta->func_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave]) {
		irq_save = true;
		if (meta->func_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave])
			kfunc_class = IRQ_LOCK_KFUNC;
	} else if (meta->func_id == special_kfunc_list[KF_bpf_local_irq_restore] ||
		   meta->func_id == special_kfunc_list[KF_bpf_res_spin_unlock_irqrestore]) {
		irq_save = false;
		if (meta->func_id == special_kfunc_list[KF_bpf_res_spin_unlock_irqrestore])
			kfunc_class = IRQ_LOCK_KFUNC;
	} else {
		verifier_bug(env, "unknown irq flags kfunc");
		return -EFAULT;
	}

	if (irq_save) {
		if (!is_irq_flag_reg_valid_uninit(env, reg)) {
			verbose(env, "expected uninitialized irq flag as arg#%d\n", regno - 1);
			return -EINVAL;
		}

		err = check_mem_access(env, env->insn_idx, regno, 0, BPF_DW, BPF_WRITE, -1, false, false);
		if (err)
			return err;

		err = mark_stack_slot_irq_flag(env, meta, reg, env->insn_idx, kfunc_class);
		if (err)
			return err;
	} else {
		err = is_irq_flag_reg_valid_init(env, reg);
		if (err) {
			verbose(env, "expected an initialized irq flag as arg#%d\n", regno - 1);
			return err;
		}

		err = mark_irq_flag_read(env, reg);
		if (err)
			return err;

		err = unmark_stack_slot_irq_flag(env, reg, kfunc_class);
		if (err)
			return err;
	}
	return 0;
}


static int ref_set_non_owning(struct bpf_verifier_env *env, struct bpf_reg_state *reg)
{
	struct btf_record *rec = reg_btf_record(reg);

	if (!env->cur_state->active_locks) {
		verifier_bug(env, "%s w/o active lock", __func__);
		return -EFAULT;
	}

	if (type_flag(reg->type) & NON_OWN_REF) {
		verifier_bug(env, "NON_OWN_REF already set");
		return -EFAULT;
	}

	reg->type |= NON_OWN_REF;
	if (rec->refcount_off >= 0)
		reg->type |= MEM_RCU;

	return 0;
}

static int ref_convert_owning_non_owning(struct bpf_verifier_env *env, u32 ref_obj_id)
{
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_func_state *unused;
	struct bpf_reg_state *reg;
	int i;

	if (!ref_obj_id) {
		verifier_bug(env, "ref_obj_id is zero for owning -> non-owning conversion");
		return -EFAULT;
	}

	for (i = 0; i < state->acquired_refs; i++) {
		if (state->refs[i].id != ref_obj_id)
			continue;

		/* Clear ref_obj_id here so release_reference doesn't clobber
		 * the whole reg
		 */
		bpf_for_each_reg_in_vstate(env->cur_state, unused, reg, ({
			if (reg->ref_obj_id == ref_obj_id) {
				reg->ref_obj_id = 0;
				ref_set_non_owning(env, reg);
			}
		}));
		return 0;
	}

	verifier_bug(env, "ref state missing for ref_obj_id");
	return -EFAULT;
}