// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 81/132



			if (meta->func_id == special_kfunc_list[KF_bpf_cast_to_kern_ctx]) {
				ret = get_kern_ctx_btf_id(&env->log, resolve_prog_type(env->prog));
				if (ret < 0)
					return -EINVAL;
				meta->ret_btf_id  = ret;
			}
			break;
		case KF_ARG_PTR_TO_ALLOC_BTF_ID:
			if (reg->type == (PTR_TO_BTF_ID | MEM_ALLOC)) {
				if (!is_bpf_obj_drop_kfunc(meta->func_id)) {
					verbose(env, "arg#%d expected for bpf_obj_drop()\n", i);
					return -EINVAL;
				}
			} else if (reg->type == (PTR_TO_BTF_ID | MEM_ALLOC | MEM_PERCPU)) {
				if (!is_bpf_percpu_obj_drop_kfunc(meta->func_id)) {
					verbose(env, "arg#%d expected for bpf_percpu_obj_drop()\n", i);
					return -EINVAL;
				}
			} else {
				verbose(env, "arg#%d expected pointer to allocated object\n", i);
				return -EINVAL;
			}
			if (!reg->ref_obj_id) {
				verbose(env, "allocated object must be referenced\n");
				return -EINVAL;
			}
			if (meta->btf == btf_vmlinux) {
				meta->arg_btf = reg->btf;
				meta->arg_btf_id = reg->btf_id;
			}
			break;
		case KF_ARG_PTR_TO_DYNPTR:
		{
			enum bpf_arg_type dynptr_arg_type = ARG_PTR_TO_DYNPTR;
			int clone_ref_obj_id = 0;

			if (reg->type == CONST_PTR_TO_DYNPTR)
				dynptr_arg_type |= MEM_RDONLY;

			if (is_kfunc_arg_uninit(btf, &args[i]))
				dynptr_arg_type |= MEM_UNINIT;

			if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_from_skb]) {
				dynptr_arg_type |= DYNPTR_TYPE_SKB;
			} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_from_xdp]) {
				dynptr_arg_type |= DYNPTR_TYPE_XDP;
			} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_from_skb_meta]) {
				dynptr_arg_type |= DYNPTR_TYPE_SKB_META;
			} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_from_file]) {
				dynptr_arg_type |= DYNPTR_TYPE_FILE;
			} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_file_discard]) {
				dynptr_arg_type |= DYNPTR_TYPE_FILE;
				meta->release_regno = regno;
			} else if (meta->func_id == special_kfunc_list[KF_bpf_dynptr_clone] &&
				   (dynptr_arg_type & MEM_UNINIT)) {
				enum bpf_dynptr_type parent_type = meta->initialized_dynptr.type;

				if (parent_type == BPF_DYNPTR_TYPE_INVALID) {
					verifier_bug(env, "no dynptr type for parent of clone");
					return -EFAULT;
				}

				dynptr_arg_type |= (unsigned int)get_dynptr_type_flag(parent_type);
				clone_ref_obj_id = meta->initialized_dynptr.ref_obj_id;
				if (dynptr_type_refcounted(parent_type) && !clone_ref_obj_id) {
					verifier_bug(env, "missing ref obj id for parent of clone");
					return -EFAULT;
				}
			}

			ret = process_dynptr_func(env, regno, insn_idx, dynptr_arg_type, clone_ref_obj_id);
			if (ret < 0)
				return ret;

			if (!(dynptr_arg_type & MEM_UNINIT)) {
				int id = dynptr_id(env, reg);

				if (id < 0) {
					verifier_bug(env, "failed to obtain dynptr id");
					return id;
				}
				meta->initialized_dynptr.id = id;
				meta->initialized_dynptr.type = dynptr_get_type(env, reg);
				meta->initialized_dynptr.ref_obj_id = dynptr_ref_obj_id(env, reg);
			}