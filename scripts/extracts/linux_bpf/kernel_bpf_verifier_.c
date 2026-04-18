// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 87/132



	if (sleepable && !in_sleepable_context(env)) {
		verbose(env, "kernel func %s is sleepable within %s\n",
			func_name, non_sleepable_context_description(env));
		return -EACCES;
	}

	if (in_rbtree_lock_required_cb(env) && (rcu_lock || rcu_unlock)) {
		verbose(env, "Calling bpf_rcu_read_{lock,unlock} in unnecessary rbtree callback\n");
		return -EACCES;
	}

	if (is_kfunc_rcu_protected(&meta) && !in_rcu_cs(env)) {
		verbose(env, "kernel func %s requires RCU critical section protection\n", func_name);
		return -EACCES;
	}

	/* In case of release function, we get register number of refcounted
	 * PTR_TO_BTF_ID in bpf_kfunc_arg_meta, do the release now.
	 */
	if (meta.release_regno) {
		struct bpf_reg_state *reg = &regs[meta.release_regno];

		if (meta.initialized_dynptr.ref_obj_id) {
			err = unmark_stack_slots_dynptr(env, reg);
		} else {
			err = release_reference(env, reg->ref_obj_id);
			if (err)
				verbose(env, "kfunc %s#%d reference has not been acquired before\n",
					func_name, meta.func_id);
		}
		if (err)
			return err;
	}

	if (is_bpf_list_push_kfunc(meta.func_id) || is_bpf_rbtree_add_kfunc(meta.func_id)) {
		release_ref_obj_id = regs[BPF_REG_2].ref_obj_id;
		insn_aux->insert_off = regs[BPF_REG_2].var_off.value;
		insn_aux->kptr_struct_meta = btf_find_struct_meta(meta.arg_btf, meta.arg_btf_id);
		err = ref_convert_owning_non_owning(env, release_ref_obj_id);
		if (err) {
			verbose(env, "kfunc %s#%d conversion of owning ref to non-owning failed\n",
				func_name, meta.func_id);
			return err;
		}

		err = release_reference(env, release_ref_obj_id);
		if (err) {
			verbose(env, "kfunc %s#%d reference has not been acquired before\n",
				func_name, meta.func_id);
			return err;
		}
	}

	if (meta.func_id == special_kfunc_list[KF_bpf_throw]) {
		if (!bpf_jit_supports_exceptions()) {
			verbose(env, "JIT does not support calling kfunc %s#%d\n",
				func_name, meta.func_id);
			return -ENOTSUPP;
		}
		env->seen_exception = true;

		/* In the case of the default callback, the cookie value passed
		 * to bpf_throw becomes the return value of the program.
		 */
		if (!env->exception_callback_subprog) {
			err = check_return_code(env, BPF_REG_1, "R1");
			if (err < 0)
				return err;
		}
	}

	for (i = 0; i < CALLER_SAVED_REGS; i++) {
		u32 regno = caller_saved[i];

		bpf_mark_reg_not_init(env, &regs[regno]);
		regs[regno].subreg_def = DEF_NOT_SUBREG;
	}

	/* Check return type */
	t = btf_type_skip_modifiers(desc_btf, meta.func_proto->type, NULL);

	if (is_kfunc_acquire(&meta) && !btf_type_is_struct_ptr(meta.btf, t)) {
		if (meta.btf != btf_vmlinux ||
		    (!is_bpf_obj_new_kfunc(meta.func_id) &&
		     !is_bpf_percpu_obj_new_kfunc(meta.func_id) &&
		     !is_bpf_refcount_acquire_kfunc(meta.func_id))) {
			verbose(env, "acquire kernel function does not return PTR_TO_BTF_ID\n");
			return -EINVAL;
		}
	}

	if (btf_type_is_scalar(t)) {
		mark_reg_unknown(env, regs, BPF_REG_0);
		if (meta.btf == btf_vmlinux && (meta.func_id == special_kfunc_list[KF_bpf_res_spin_lock] ||
		    meta.func_id == special_kfunc_list[KF_bpf_res_spin_lock_irqsave]))
			__mark_reg_const_zero(env, &regs[BPF_REG_0]);
		mark_btf_func_reg_size(env, BPF_REG_0, t->size);
	} else if (btf_type_is_ptr(t)) {
		ptr_type = btf_type_skip_modifiers(desc_btf, t->type, &ptr_type_id);
		err = check_special_kfunc(env, &meta, regs, insn_aux, ptr_type, desc_btf);
		if (err) {
			if (err < 0)
				return err;
		} else if (btf_type_is_void(ptr_type)) {
			/* kfunc returning 'void *' is equivalent to returning scalar */
			mark_reg_unknown(env, regs, BPF_REG_0);
		} else if (!__btf_type_is_struct(ptr_type)) {
			if (!meta.r0_size) {
				__u32 sz;

				if (!IS_ERR(btf_resolve_size(desc_btf, ptr_type, &sz))) {
					meta.r0_size = sz;
					meta.r0_rdonly = true;
				}
			}
			if (!meta.r0_size) {
				ptr_type_name = btf_name_by_offset(desc_btf,
								   ptr_type->name_off);
				verbose(env,
					"kernel function %s returns pointer type %s %s is not supported\n",
					func_name,
					btf_type_str(ptr_type),
					ptr_type_name);
				return -EINVAL;
			}

			mark_reg_known_zero(env, regs, BPF_REG_0);
			regs[BPF_REG_0].type = PTR_TO_MEM;
			regs[BPF_REG_0].mem_size = meta.r0_size;

			if (meta.r0_rdonly)
				regs[BPF_REG_0].type |= MEM_RDONLY;

			/* Ensures we don't access the memory after a release_reference() */
			if (meta.ref_obj_id)
				regs[BPF_REG_0].ref_obj_id = meta.ref_obj_id;

			if (is_kfunc_rcu_protected(&meta))
				regs[BPF_REG_0].type |= MEM_RCU;
		} else {
			enum bpf_reg_type type = PTR_TO_BTF_ID;