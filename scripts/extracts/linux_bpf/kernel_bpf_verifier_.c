// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 69/132



	switch (func_id) {
	case BPF_FUNC_tail_call:
		err = check_resource_leak(env, false, true, "tail_call");
		if (err)
			return err;
		break;
	case BPF_FUNC_get_local_storage:
		/* check that flags argument in get_local_storage(map, flags) is 0,
		 * this is required because get_local_storage() can't return an error.
		 */
		if (!bpf_register_is_null(&regs[BPF_REG_2])) {
			verbose(env, "get_local_storage() doesn't support non-zero flags\n");
			return -EINVAL;
		}
		break;
	case BPF_FUNC_for_each_map_elem:
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_map_elem_callback_state);
		break;
	case BPF_FUNC_timer_set_callback:
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_timer_callback_state);
		break;
	case BPF_FUNC_find_vma:
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_find_vma_callback_state);
		break;
	case BPF_FUNC_snprintf:
		err = check_bpf_snprintf_call(env, regs);
		break;
	case BPF_FUNC_loop:
		update_loop_inline_state(env, meta.subprogno);
		/* Verifier relies on R1 value to determine if bpf_loop() iteration
		 * is finished, thus mark it precise.
		 */
		err = mark_chain_precision(env, BPF_REG_1);
		if (err)
			return err;
		if (cur_func(env)->callback_depth < regs[BPF_REG_1].umax_value) {
			err = push_callback_call(env, insn, insn_idx, meta.subprogno,
						 set_loop_callback_state);
		} else {
			cur_func(env)->callback_depth = 0;
			if (env->log.level & BPF_LOG_LEVEL2)
				verbose(env, "frame%d bpf_loop iteration limit reached\n",
					env->cur_state->curframe);
		}
		break;
	case BPF_FUNC_dynptr_from_mem:
		if (regs[BPF_REG_1].type != PTR_TO_MAP_VALUE) {
			verbose(env, "Unsupported reg type %s for bpf_dynptr_from_mem data\n",
				reg_type_str(env, regs[BPF_REG_1].type));
			return -EACCES;
		}
		break;
	case BPF_FUNC_set_retval:
		if (prog_type == BPF_PROG_TYPE_LSM &&
		    env->prog->expected_attach_type == BPF_LSM_CGROUP) {
			if (!env->prog->aux->attach_func_proto->type) {
				/* Make sure programs that attach to void
				 * hooks don't try to modify return value.
				 */
				verbose(env, "BPF_LSM_CGROUP that attach to void LSM hooks can't modify return value!\n");
				return -EINVAL;
			}
		}
		break;
	case BPF_FUNC_dynptr_data:
	{
		struct bpf_reg_state *reg;
		int id, ref_obj_id;

		reg = get_dynptr_arg_reg(env, fn, regs);
		if (!reg)
			return -EFAULT;


		if (meta.dynptr_id) {
			verifier_bug(env, "meta.dynptr_id already set");
			return -EFAULT;
		}
		if (meta.ref_obj_id) {
			verifier_bug(env, "meta.ref_obj_id already set");
			return -EFAULT;
		}

		id = dynptr_id(env, reg);
		if (id < 0) {
			verifier_bug(env, "failed to obtain dynptr id");
			return id;
		}

		ref_obj_id = dynptr_ref_obj_id(env, reg);
		if (ref_obj_id < 0) {
			verifier_bug(env, "failed to obtain dynptr ref_obj_id");
			return ref_obj_id;
		}

		meta.dynptr_id = id;
		meta.ref_obj_id = ref_obj_id;

		break;
	}
	case BPF_FUNC_dynptr_write:
	{
		enum bpf_dynptr_type dynptr_type;
		struct bpf_reg_state *reg;

		reg = get_dynptr_arg_reg(env, fn, regs);
		if (!reg)
			return -EFAULT;

		dynptr_type = dynptr_get_type(env, reg);
		if (dynptr_type == BPF_DYNPTR_TYPE_INVALID)
			return -EFAULT;

		if (dynptr_type == BPF_DYNPTR_TYPE_SKB ||
		    dynptr_type == BPF_DYNPTR_TYPE_SKB_META)
			/* this will trigger clear_all_pkt_pointers(), which will
			 * invalidate all dynptr slices associated with the skb
			 */
			changes_data = true;

		break;
	}
	case BPF_FUNC_per_cpu_ptr:
	case BPF_FUNC_this_cpu_ptr:
	{
		struct bpf_reg_state *reg = &regs[BPF_REG_1];
		const struct btf_type *type;

		if (reg->type & MEM_RCU) {
			type = btf_type_by_id(reg->btf, reg->btf_id);
			if (!type || !btf_type_is_struct(type)) {
				verbose(env, "Helper has invalid btf/btf_id in R1\n");
				return -EFAULT;
			}
			returns_cpu_specific_alloc_ptr = true;
			env->insn_aux_data[insn_idx].call_with_percpu_alloc_ptr = true;
		}
		break;
	}
	case BPF_FUNC_user_ringbuf_drain:
		err = push_callback_call(env, insn, insn_idx, meta.subprogno,
					 set_user_ringbuf_callback_state);
		break;
	}

	if (err)
		return err;

	/* reset caller saved regs */
	for (i = 0; i < CALLER_SAVED_REGS; i++) {
		bpf_mark_reg_not_init(env, &regs[caller_saved[i]]);
		check_reg_arg(env, caller_saved[i], DST_OP_NO_MARK);
	}

	/* helper call returns 64-bit value. */
	regs[BPF_REG_0].subreg_def = DEF_NOT_SUBREG;

	/* update return register (already marked as written above) */
	ret_type = fn->ret_type;
	ret_flag = type_flag(ret_type);