// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 80/132



			if (is_ret_buf_sz) {
				if (meta->r0_size) {
					verbose(env, "2 or more rdonly/rdwr_buf_size parameters for kfunc");
					return -EINVAL;
				}

				if (!tnum_is_const(reg->var_off)) {
					verbose(env, "R%d is not a const\n", regno);
					return -EINVAL;
				}

				meta->r0_size = reg->var_off.value;
				ret = mark_chain_precision(env, regno);
				if (ret)
					return ret;
			}
			continue;
		}

		if (!btf_type_is_ptr(t)) {
			verbose(env, "Unrecognized arg#%d type %s\n", i, btf_type_str(t));
			return -EINVAL;
		}

		if ((bpf_register_is_null(reg) || type_may_be_null(reg->type)) &&
		    !is_kfunc_arg_nullable(meta->btf, &args[i])) {
			verbose(env, "Possibly NULL pointer passed to trusted arg%d\n", i);
			return -EACCES;
		}

		if (reg->ref_obj_id) {
			if (is_kfunc_release(meta) && meta->ref_obj_id) {
				verifier_bug(env, "more than one arg with ref_obj_id R%d %u %u",
					     regno, reg->ref_obj_id,
					     meta->ref_obj_id);
				return -EFAULT;
			}
			meta->ref_obj_id = reg->ref_obj_id;
			if (is_kfunc_release(meta))
				meta->release_regno = regno;
		}

		ref_t = btf_type_skip_modifiers(btf, t->type, &ref_id);
		ref_tname = btf_name_by_offset(btf, ref_t->name_off);

		kf_arg_type = get_kfunc_ptr_arg_type(env, meta, t, ref_t, ref_tname, args, i, nargs);
		if (kf_arg_type < 0)
			return kf_arg_type;

		switch (kf_arg_type) {
		case KF_ARG_PTR_TO_NULL:
			continue;
		case KF_ARG_PTR_TO_MAP:
			if (!reg->map_ptr) {
				verbose(env, "pointer in R%d isn't map pointer\n", regno);
				return -EINVAL;
			}
			if (meta->map.ptr && (reg->map_ptr->record->wq_off >= 0 ||
					      reg->map_ptr->record->task_work_off >= 0)) {
				/* Use map_uid (which is unique id of inner map) to reject:
				 * inner_map1 = bpf_map_lookup_elem(outer_map, key1)
				 * inner_map2 = bpf_map_lookup_elem(outer_map, key2)
				 * if (inner_map1 && inner_map2) {
				 *     wq = bpf_map_lookup_elem(inner_map1);
				 *     if (wq)
				 *         // mismatch would have been allowed
				 *         bpf_wq_init(wq, inner_map2);
				 * }
				 *
				 * Comparing map_ptr is enough to distinguish normal and outer maps.
				 */
				if (meta->map.ptr != reg->map_ptr ||
				    meta->map.uid != reg->map_uid) {
					if (reg->map_ptr->record->task_work_off >= 0) {
						verbose(env,
							"bpf_task_work pointer in R2 map_uid=%d doesn't match map pointer in R3 map_uid=%d\n",
							meta->map.uid, reg->map_uid);
						return -EINVAL;
					}
					verbose(env,
						"workqueue pointer in R1 map_uid=%d doesn't match map pointer in R2 map_uid=%d\n",
						meta->map.uid, reg->map_uid);
					return -EINVAL;
				}
			}
			meta->map.ptr = reg->map_ptr;
			meta->map.uid = reg->map_uid;
			fallthrough;
		case KF_ARG_PTR_TO_ALLOC_BTF_ID:
		case KF_ARG_PTR_TO_BTF_ID:
			if (!is_trusted_reg(reg)) {
				if (!is_kfunc_rcu(meta)) {
					verbose(env, "R%d must be referenced or trusted\n", regno);
					return -EINVAL;
				}
				if (!is_rcu_reg(reg)) {
					verbose(env, "R%d must be a rcu pointer\n", regno);
					return -EINVAL;
				}
			}
			fallthrough;
		case KF_ARG_PTR_TO_DYNPTR:
		case KF_ARG_PTR_TO_ITER:
		case KF_ARG_PTR_TO_LIST_HEAD:
		case KF_ARG_PTR_TO_LIST_NODE:
		case KF_ARG_PTR_TO_RB_ROOT:
		case KF_ARG_PTR_TO_RB_NODE:
		case KF_ARG_PTR_TO_MEM:
		case KF_ARG_PTR_TO_MEM_SIZE:
		case KF_ARG_PTR_TO_CALLBACK:
		case KF_ARG_PTR_TO_REFCOUNTED_KPTR:
		case KF_ARG_PTR_TO_CONST_STR:
		case KF_ARG_PTR_TO_WORKQUEUE:
		case KF_ARG_PTR_TO_TIMER:
		case KF_ARG_PTR_TO_TASK_WORK:
		case KF_ARG_PTR_TO_IRQ_FLAG:
		case KF_ARG_PTR_TO_RES_SPIN_LOCK:
			break;
		case KF_ARG_PTR_TO_CTX:
			arg_type = ARG_PTR_TO_CTX;
			break;
		default:
			verifier_bug(env, "unknown kfunc arg type %d", kf_arg_type);
			return -EFAULT;
		}

		if (is_kfunc_release(meta) && reg->ref_obj_id)
			arg_type |= OBJ_RELEASE;
		ret = check_func_arg_reg_off(env, reg, regno, arg_type);
		if (ret < 0)
			return ret;

		switch (kf_arg_type) {
		case KF_ARG_PTR_TO_CTX:
			if (reg->type != PTR_TO_CTX) {
				verbose(env, "arg#%d expected pointer to ctx, but got %s\n",
					i, reg_type_str(env, reg->type));
				return -EINVAL;
			}