// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 70/132



	switch (base_type(ret_type)) {
	case RET_INTEGER:
		/* sets type to SCALAR_VALUE */
		mark_reg_unknown(env, regs, BPF_REG_0);
		break;
	case RET_VOID:
		regs[BPF_REG_0].type = NOT_INIT;
		break;
	case RET_PTR_TO_MAP_VALUE:
		/* There is no offset yet applied, variable or fixed */
		mark_reg_known_zero(env, regs, BPF_REG_0);
		/* remember map_ptr, so that check_map_access()
		 * can check 'value_size' boundary of memory access
		 * to map element returned from bpf_map_lookup_elem()
		 */
		if (meta.map.ptr == NULL) {
			verifier_bug(env, "unexpected null map_ptr");
			return -EFAULT;
		}

		if (func_id == BPF_FUNC_map_lookup_elem &&
		    can_elide_value_nullness(meta.map.ptr->map_type) &&
		    meta.const_map_key >= 0 &&
		    meta.const_map_key < meta.map.ptr->max_entries)
			ret_flag &= ~PTR_MAYBE_NULL;

		regs[BPF_REG_0].map_ptr = meta.map.ptr;
		regs[BPF_REG_0].map_uid = meta.map.uid;
		regs[BPF_REG_0].type = PTR_TO_MAP_VALUE | ret_flag;
		if (!type_may_be_null(ret_flag) &&
		    btf_record_has_field(meta.map.ptr->record, BPF_SPIN_LOCK | BPF_RES_SPIN_LOCK)) {
			regs[BPF_REG_0].id = ++env->id_gen;
		}
		break;
	case RET_PTR_TO_SOCKET:
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_SOCKET | ret_flag;
		break;
	case RET_PTR_TO_SOCK_COMMON:
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_SOCK_COMMON | ret_flag;
		break;
	case RET_PTR_TO_TCP_SOCK:
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_TCP_SOCK | ret_flag;
		break;
	case RET_PTR_TO_MEM:
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_MEM | ret_flag;
		regs[BPF_REG_0].mem_size = meta.mem_size;
		break;
	case RET_PTR_TO_MEM_OR_BTF_ID:
	{
		const struct btf_type *t;

		mark_reg_known_zero(env, regs, BPF_REG_0);
		t = btf_type_skip_modifiers(meta.ret_btf, meta.ret_btf_id, NULL);
		if (!btf_type_is_struct(t)) {
			u32 tsize;
			const struct btf_type *ret;
			const char *tname;

			/* resolve the type size of ksym. */
			ret = btf_resolve_size(meta.ret_btf, t, &tsize);
			if (IS_ERR(ret)) {
				tname = btf_name_by_offset(meta.ret_btf, t->name_off);
				verbose(env, "unable to resolve the size of type '%s': %ld\n",
					tname, PTR_ERR(ret));
				return -EINVAL;
			}
			regs[BPF_REG_0].type = PTR_TO_MEM | ret_flag;
			regs[BPF_REG_0].mem_size = tsize;
		} else {
			if (returns_cpu_specific_alloc_ptr) {
				regs[BPF_REG_0].type = PTR_TO_BTF_ID | MEM_ALLOC | MEM_RCU;
			} else {
				/* MEM_RDONLY may be carried from ret_flag, but it
				 * doesn't apply on PTR_TO_BTF_ID. Fold it, otherwise
				 * it will confuse the check of PTR_TO_BTF_ID in
				 * check_mem_access().
				 */
				ret_flag &= ~MEM_RDONLY;
				regs[BPF_REG_0].type = PTR_TO_BTF_ID | ret_flag;
			}

			regs[BPF_REG_0].btf = meta.ret_btf;
			regs[BPF_REG_0].btf_id = meta.ret_btf_id;
		}
		break;
	}
	case RET_PTR_TO_BTF_ID:
	{
		struct btf *ret_btf;
		int ret_btf_id;

		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_BTF_ID | ret_flag;
		if (func_id == BPF_FUNC_kptr_xchg) {
			ret_btf = meta.kptr_field->kptr.btf;
			ret_btf_id = meta.kptr_field->kptr.btf_id;
			if (!btf_is_kernel(ret_btf)) {
				regs[BPF_REG_0].type |= MEM_ALLOC;
				if (meta.kptr_field->type == BPF_KPTR_PERCPU)
					regs[BPF_REG_0].type |= MEM_PERCPU;
			}
		} else {
			if (fn->ret_btf_id == BPF_PTR_POISON) {
				verifier_bug(env, "func %s has non-overwritten BPF_PTR_POISON return type",
					     func_id_name(func_id));
				return -EFAULT;
			}
			ret_btf = btf_vmlinux;
			ret_btf_id = *fn->ret_btf_id;
		}
		if (ret_btf_id == 0) {
			verbose(env, "invalid return type %u of func %s#%d\n",
				base_type(ret_type), func_id_name(func_id),
				func_id);
			return -EINVAL;
		}
		regs[BPF_REG_0].btf = ret_btf;
		regs[BPF_REG_0].btf_id = ret_btf_id;
		break;
	}
	default:
		verbose(env, "unknown return type %u of func %s#%d\n",
			base_type(ret_type), func_id_name(func_id), func_id);
		return -EINVAL;
	}

	if (type_may_be_null(regs[BPF_REG_0].type))
		regs[BPF_REG_0].id = ++env->id_gen;

	if (helper_multiple_ref_obj_use(func_id, meta.map.ptr)) {
		verifier_bug(env, "func %s#%d sets ref_obj_id more than once",
			     func_id_name(func_id), func_id);
		return -EFAULT;
	}

	if (is_dynptr_ref_function(func_id))
		regs[BPF_REG_0].dynptr_id = meta.dynptr_id;

	if (is_ptr_cast_function(func_id) || is_dynptr_ref_function(func_id)) {
		/* For release_reference() */
		regs[BPF_REG_0].ref_obj_id = meta.ref_obj_id;
	} else if (is_acquire_function(func_id, meta.map.ptr)) {
		int id = acquire_reference(env, insn_idx);

		if (id < 0)
			return id;
		/* For mark_ptr_or_null_reg() */
		regs[BPF_REG_0].id = id;
		/* For release_reference() */
		regs[BPF_REG_0].ref_obj_id = id;
	}

	err = do_refine_retval_range(env, regs, fn->ret_type, func_id, &meta);
	if (err)
		return err;

	err = check_map_func_compatibility(env, meta.map.ptr, func_id);
	if (err)
		return err;