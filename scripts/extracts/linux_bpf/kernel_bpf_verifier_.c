// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 85/132



		ret_t = btf_type_by_id(ret_btf, ret_btf_id);
		if (!ret_t || !__btf_type_is_struct(ret_t)) {
			verbose(env, "bpf_obj_new/bpf_percpu_obj_new type ID argument must be of a struct\n");
			return -EINVAL;
		}

		if (is_bpf_percpu_obj_new_kfunc(meta->func_id)) {
			if (ret_t->size > BPF_GLOBAL_PERCPU_MA_MAX_SIZE) {
				verbose(env, "bpf_percpu_obj_new type size (%d) is greater than %d\n",
					ret_t->size, BPF_GLOBAL_PERCPU_MA_MAX_SIZE);
				return -EINVAL;
			}

			if (!bpf_global_percpu_ma_set) {
				mutex_lock(&bpf_percpu_ma_lock);
				if (!bpf_global_percpu_ma_set) {
					/* Charge memory allocated with bpf_global_percpu_ma to
					 * root memcg. The obj_cgroup for root memcg is NULL.
					 */
					err = bpf_mem_alloc_percpu_init(&bpf_global_percpu_ma, NULL);
					if (!err)
						bpf_global_percpu_ma_set = true;
				}
				mutex_unlock(&bpf_percpu_ma_lock);
				if (err)
					return err;
			}

			mutex_lock(&bpf_percpu_ma_lock);
			err = bpf_mem_alloc_percpu_unit_init(&bpf_global_percpu_ma, ret_t->size);
			mutex_unlock(&bpf_percpu_ma_lock);
			if (err)
				return err;
		}

		struct_meta = btf_find_struct_meta(ret_btf, ret_btf_id);
		if (is_bpf_percpu_obj_new_kfunc(meta->func_id)) {
			if (!__btf_type_is_scalar_struct(env, ret_btf, ret_t, 0)) {
				verbose(env, "bpf_percpu_obj_new type ID argument must be of a struct of scalars\n");
				return -EINVAL;
			}

			if (struct_meta) {
				verbose(env, "bpf_percpu_obj_new type ID argument must not contain special fields\n");
				return -EINVAL;
			}
		}

		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_BTF_ID | MEM_ALLOC;
		regs[BPF_REG_0].btf = ret_btf;
		regs[BPF_REG_0].btf_id = ret_btf_id;
		if (is_bpf_percpu_obj_new_kfunc(meta->func_id))
			regs[BPF_REG_0].type |= MEM_PERCPU;

		insn_aux->obj_new_size = ret_t->size;
		insn_aux->kptr_struct_meta = struct_meta;
	} else if (is_bpf_refcount_acquire_kfunc(meta->func_id)) {
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_BTF_ID | MEM_ALLOC;
		regs[BPF_REG_0].btf = meta->arg_btf;
		regs[BPF_REG_0].btf_id = meta->arg_btf_id;

		insn_aux->kptr_struct_meta =
			btf_find_struct_meta(meta->arg_btf,
					     meta->arg_btf_id);
	} else if (is_list_node_type(ptr_type)) {
		struct btf_field *field = meta->arg_list_head.field;

		mark_reg_graph_node(regs, BPF_REG_0, &field->graph_root);
	} else if (is_rbtree_node_type(ptr_type)) {
		struct btf_field *field = meta->arg_rbtree_root.field;

		mark_reg_graph_node(regs, BPF_REG_0, &field->graph_root);
	} else if (meta->func_id == special_kfunc_list[KF_bpf_cast_to_kern_ctx]) {
		mark_reg_known_zero(env, regs, BPF_REG_0);
		regs[BPF_REG_0].type = PTR_TO_BTF_ID | PTR_TRUSTED;
		regs[BPF_REG_0].btf = desc_btf;
		regs[BPF_REG_0].btf_id = meta->ret_btf_id;
	} else if (meta->func_id == special_kfunc_list[KF_bpf_rdonly_cast]) {
		ret_t = btf_type_by_id(desc_btf, meta->arg_constant.value);
		if (!ret_t) {
			verbose(env, "Unknown type ID %lld passed to kfunc bpf_rdonly_cast\n",
				meta->arg_constant.value);
			return -EINVAL;
		} else if (btf_type_is_struct(ret_t)) {
			mark_reg_known_zero(env, regs, BPF_REG_0);
			regs[BPF_REG_0].type = PTR_TO_BTF_ID | PTR_UNTRUSTED;
			regs[BPF_REG_0].btf = desc_btf;
			regs[BPF_REG_0].btf_id = meta->arg_constant.value;
		} else if (btf_type_is_void(ret_t)) {
			mark_reg_known_zero(env, regs, BPF_REG_0);
			regs[BPF_REG_0].type = PTR_TO_MEM | MEM_RDONLY | PTR_UNTRUSTED;
			regs[BPF_REG_0].mem_size = 0;
		} else {
			verbose(env,
				"kfunc bpf_rdonly_cast type ID argument must be of a struct or void\n");
			return -EINVAL;
		}
	} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_slice] ||
		   meta->func_id == special_kfunc_list[KF_bpf_dynptr_slice_rdwr]) {
		enum bpf_type_flag type_flag = get_dynptr_type_flag(meta->initialized_dynptr.type);

		mark_reg_known_zero(env, regs, BPF_REG_0);

		if (!meta->arg_constant.found) {
			verifier_bug(env, "bpf_dynptr_slice(_rdwr) no constant size");
			return -EFAULT;
		}

		regs[BPF_REG_0].mem_size = meta->arg_constant.value;

		/* PTR_MAYBE_NULL will be added when is_kfunc_ret_null is checked */
		regs[BPF_REG_0].type = PTR_TO_MEM | type_flag;

		if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_slice]) {
			regs[BPF_REG_0].type |= MEM_RDONLY;
		} else {
			/* this will set env->seen_direct_write to true */
			if (!may_access_direct_pkt_data(env, NULL, BPF_WRITE)) {
				verbose(env, "the prog does not allow writes to packet data\n");
				return -EINVAL;
			}
		}

		if (!meta->initialized_dynptr.id) {
			verifier_bug(env, "no dynptr id");
			return -EFAULT;
		}
		regs[BPF_REG_0].dynptr_id = meta->initialized_dynptr.id;

		/* we don't need to set BPF_REG_0's ref obj id
		 * because packet slices are not refcounted (see
		 * dynptr_type_refcounted)
		 */
	} else {
		return 0;
	}

	return 1;
}