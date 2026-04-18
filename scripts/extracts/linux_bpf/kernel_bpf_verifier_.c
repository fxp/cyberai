// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 82/132



			break;
		}
		case KF_ARG_PTR_TO_ITER:
			if (meta->func_id == special_kfunc_list[KF_bpf_iter_css_task_new]) {
				if (!check_css_task_iter_allowlist(env)) {
					verbose(env, "css_task_iter is only allowed in bpf_lsm, bpf_iter and sleepable progs\n");
					return -EINVAL;
				}
			}
			ret = process_iter_arg(env, regno, insn_idx, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_LIST_HEAD:
			if (reg->type != PTR_TO_MAP_VALUE &&
			    reg->type != (PTR_TO_BTF_ID | MEM_ALLOC)) {
				verbose(env, "arg#%d expected pointer to map value or allocated object\n", i);
				return -EINVAL;
			}
			if (reg->type == (PTR_TO_BTF_ID | MEM_ALLOC) && !reg->ref_obj_id) {
				verbose(env, "allocated object must be referenced\n");
				return -EINVAL;
			}
			ret = process_kf_arg_ptr_to_list_head(env, reg, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_RB_ROOT:
			if (reg->type != PTR_TO_MAP_VALUE &&
			    reg->type != (PTR_TO_BTF_ID | MEM_ALLOC)) {
				verbose(env, "arg#%d expected pointer to map value or allocated object\n", i);
				return -EINVAL;
			}
			if (reg->type == (PTR_TO_BTF_ID | MEM_ALLOC) && !reg->ref_obj_id) {
				verbose(env, "allocated object must be referenced\n");
				return -EINVAL;
			}
			ret = process_kf_arg_ptr_to_rbtree_root(env, reg, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_LIST_NODE:
			if (reg->type != (PTR_TO_BTF_ID | MEM_ALLOC)) {
				verbose(env, "arg#%d expected pointer to allocated object\n", i);
				return -EINVAL;
			}
			if (!reg->ref_obj_id) {
				verbose(env, "allocated object must be referenced\n");
				return -EINVAL;
			}
			ret = process_kf_arg_ptr_to_list_node(env, reg, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_RB_NODE:
			if (is_bpf_rbtree_add_kfunc(meta->func_id)) {
				if (reg->type != (PTR_TO_BTF_ID | MEM_ALLOC)) {
					verbose(env, "arg#%d expected pointer to allocated object\n", i);
					return -EINVAL;
				}
				if (!reg->ref_obj_id) {
					verbose(env, "allocated object must be referenced\n");
					return -EINVAL;
				}
			} else {
				if (!type_is_non_owning_ref(reg->type) && !reg->ref_obj_id) {
					verbose(env, "%s can only take non-owning or refcounted bpf_rb_node pointer\n", func_name);
					return -EINVAL;
				}
				if (in_rbtree_lock_required_cb(env)) {
					verbose(env, "%s not allowed in rbtree cb\n", func_name);
					return -EINVAL;
				}
			}

			ret = process_kf_arg_ptr_to_rbtree_node(env, reg, regno, meta);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_MAP:
			/* If argument has '__map' suffix expect 'struct bpf_map *' */
			ref_id = *reg2btf_ids[CONST_PTR_TO_MAP];
			ref_t = btf_type_by_id(btf_vmlinux, ref_id);
			ref_tname = btf_name_by_offset(btf, ref_t->name_off);
			fallthrough;
		case KF_ARG_PTR_TO_BTF_ID:
			/* Only base_type is checked, further checks are done here */
			if ((base_type(reg->type) != PTR_TO_BTF_ID ||
			     (bpf_type_has_unsafe_modifiers(reg->type) && !is_rcu_reg(reg))) &&
			    !reg2btf_ids[base_type(reg->type)]) {
				verbose(env, "arg#%d is %s ", i, reg_type_str(env, reg->type));
				verbose(env, "expected %s or socket\n",
					reg_type_str(env, base_type(reg->type) |
							  (type_flag(reg->type) & BPF_REG_TRUSTED_MODIFIERS)));
				return -EINVAL;
			}
			ret = process_kf_arg_ptr_to_btf_id(env, reg, ref_t, ref_tname, ref_id, meta, i);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_MEM:
			resolve_ret = btf_resolve_size(btf, ref_t, &type_size);
			if (IS_ERR(resolve_ret)) {
				verbose(env, "arg#%d reference type('%s %s') size cannot be determined: %ld\n",
					i, btf_type_str(ref_t), ref_tname, PTR_ERR(resolve_ret));
				return -EINVAL;
			}
			ret = check_mem_reg(env, reg, regno, type_size);
			if (ret < 0)
				return ret;
			break;
		case KF_ARG_PTR_TO_MEM_SIZE:
		{
			struct bpf_reg_state *buff_reg = &regs[regno];
			const struct btf_param *buff_arg = &args[i];
			struct bpf_reg_state *size_reg = &regs[regno + 1];
			const struct btf_param *size_arg = &args[i + 1];

			if (!bpf_register_is_null(buff_reg) || !is_kfunc_arg_nullable(meta->btf, buff_arg)) {
				ret = check_kfunc_mem_size_reg(env, size_reg, regno + 1);
				if (ret < 0) {
					verbose(env, "arg#%d arg#%d memory, len pair leads to invalid memory access\n", i, i + 1);
					return ret;
				}
			}

			if (is_kfunc_arg_const_mem_size(meta->btf, size_arg, size_reg)) {
				if (meta->arg_constant.found) {
					verifier_bug(env, "only one constant argument permitted");
					return -EFAULT;
				}
				if (!tnum_is_const(size_reg->var_off)) {
					verbose(env, "R%d must be a known constant\n", regno + 1);
					return -EINVAL;
				}
				meta->arg_constant.found = true;
				meta->arg_constant.value = size_reg->var_off.value;
			}