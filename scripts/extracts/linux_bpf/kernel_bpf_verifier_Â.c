// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 130/132



		insn_buf[0] = BPF_MOV64_IMM(BPF_REG_1, obj_new_size);
		insn_buf[1] = addr[0];
		insn_buf[2] = addr[1];
		insn_buf[3] = *insn;
		*cnt = 4;
	} else if (is_bpf_obj_drop_kfunc(desc->func_id) ||
		   is_bpf_percpu_obj_drop_kfunc(desc->func_id) ||
		   is_bpf_refcount_acquire_kfunc(desc->func_id)) {
		struct btf_struct_meta *kptr_struct_meta = env->insn_aux_data[insn_idx].kptr_struct_meta;
		struct bpf_insn addr[2] = { BPF_LD_IMM64(BPF_REG_2, (long)kptr_struct_meta) };

		if (is_bpf_percpu_obj_drop_kfunc(desc->func_id) && kptr_struct_meta) {
			verifier_bug(env, "NULL kptr_struct_meta expected at insn_idx %d",
				     insn_idx);
			return -EFAULT;
		}

		if (is_bpf_refcount_acquire_kfunc(desc->func_id) && !kptr_struct_meta) {
			verifier_bug(env, "kptr_struct_meta expected at insn_idx %d",
				     insn_idx);
			return -EFAULT;
		}

		insn_buf[0] = addr[0];
		insn_buf[1] = addr[1];
		insn_buf[2] = *insn;
		*cnt = 3;
	} else if (is_bpf_list_push_kfunc(desc->func_id) ||
		   is_bpf_rbtree_add_kfunc(desc->func_id)) {
		struct btf_struct_meta *kptr_struct_meta = env->insn_aux_data[insn_idx].kptr_struct_meta;
		int struct_meta_reg = BPF_REG_3;
		int node_offset_reg = BPF_REG_4;

		/* rbtree_add has extra 'less' arg, so args-to-fixup are in diff regs */
		if (is_bpf_rbtree_add_kfunc(desc->func_id)) {
			struct_meta_reg = BPF_REG_4;
			node_offset_reg = BPF_REG_5;
		}

		if (!kptr_struct_meta) {
			verifier_bug(env, "kptr_struct_meta expected at insn_idx %d",
				     insn_idx);
			return -EFAULT;
		}

		__fixup_collection_insert_kfunc(&env->insn_aux_data[insn_idx], struct_meta_reg,
						node_offset_reg, insn, insn_buf, cnt);
	} else if (desc->func_id == special_kfunc_list[KF_bpf_cast_to_kern_ctx] ||
		   desc->func_id == special_kfunc_list[KF_bpf_rdonly_cast]) {
		insn_buf[0] = BPF_MOV64_REG(BPF_REG_0, BPF_REG_1);
		*cnt = 1;
	} else if (desc->func_id == special_kfunc_list[KF_bpf_session_is_return] &&
		   env->prog->expected_attach_type == BPF_TRACE_FSESSION) {
		/*
		 * inline the bpf_session_is_return() for fsession:
		 *   bool bpf_session_is_return(void *ctx)
		 *   {
		 *       return (((u64 *)ctx)[-1] >> BPF_TRAMP_IS_RETURN_SHIFT) & 1;
		 *   }
		 */
		insn_buf[0] = BPF_LDX_MEM(BPF_DW, BPF_REG_0, BPF_REG_1, -8);
		insn_buf[1] = BPF_ALU64_IMM(BPF_RSH, BPF_REG_0, BPF_TRAMP_IS_RETURN_SHIFT);
		insn_buf[2] = BPF_ALU64_IMM(BPF_AND, BPF_REG_0, 1);
		*cnt = 3;
	} else if (desc->func_id == special_kfunc_list[KF_bpf_session_cookie] &&
		   env->prog->expected_attach_type == BPF_TRACE_FSESSION) {
		/*
		 * inline bpf_session_cookie() for fsession:
		 *   __u64 *bpf_session_cookie(void *ctx)
		 *   {
		 *       u64 off = (((u64 *)ctx)[-1] >> BPF_TRAMP_COOKIE_INDEX_SHIFT) & 0xFF;
		 *       return &((u64 *)ctx)[-off];
		 *   }
		 */
		insn_buf[0] = BPF_LDX_MEM(BPF_DW, BPF_REG_0, BPF_REG_1, -8);
		insn_buf[1] = BPF_ALU64_IMM(BPF_RSH, BPF_REG_0, BPF_TRAMP_COOKIE_INDEX_SHIFT);
		insn_buf[2] = BPF_ALU64_IMM(BPF_AND, BPF_REG_0, 0xFF);
		insn_buf[3] = BPF_ALU64_IMM(BPF_LSH, BPF_REG_0, 3);
		insn_buf[4] = BPF_ALU64_REG(BPF_SUB, BPF_REG_0, BPF_REG_1);
		insn_buf[5] = BPF_ALU64_IMM(BPF_NEG, BPF_REG_0, 0);
		*cnt = 6;
	}

	if (env->insn_aux_data[insn_idx].arg_prog) {
		u32 regno = env->insn_aux_data[insn_idx].arg_prog;
		struct bpf_insn ld_addrs[2] = { BPF_LD_IMM64(regno, (long)env->prog->aux) };
		int idx = *cnt;

		insn_buf[idx++] = ld_addrs[0];
		insn_buf[idx++] = ld_addrs[1];
		insn_buf[idx++] = *insn;
		*cnt = idx;
	}
	return 0;
}

int bpf_check(struct bpf_prog **prog, union bpf_attr *attr, bpfptr_t uattr, __u32 uattr_size)
{
	u64 start_time = ktime_get_ns();
	struct bpf_verifier_env *env;
	int i, len, ret = -EINVAL, err;
	u32 log_true_size;
	bool is_priv;

	BTF_TYPE_EMIT(enum bpf_features);

	/* no program is valid */
	if (ARRAY_SIZE(bpf_verifier_ops) == 0)
		return -EINVAL;

	/* 'struct bpf_verifier_env' can be global, but since it's not small,
	 * allocate/free it every time bpf_check() is called
	 */
	env = kvzalloc_obj(struct bpf_verifier_env, GFP_KERNEL_ACCOUNT);
	if (!env)
		return -ENOMEM;

	env->bt.env = env;

	len = (*prog)->len;
	env->insn_aux_data =
		vzalloc(array_size(sizeof(struct bpf_insn_aux_data), len));
	ret = -ENOMEM;
	if (!env->insn_aux_data)
		goto err_free_env;
	for (i = 0; i < len; i++)
		env->insn_aux_data[i].orig_idx = i;
	env->succ = bpf_iarray_realloc(NULL, 2);
	if (!env->succ)
		goto err_free_env;
	env->prog = *prog;
	env->ops = bpf_verifier_ops[env->prog->type];

	env->allow_ptr_leaks = bpf_allow_ptr_leaks(env->prog->aux->token);
	env->allow_uninit_stack = bpf_allow_uninit_stack(env->prog->aux->token);
	env->bypass_spec_v1 = bpf_bypass_spec_v1(env->prog->aux->token);
	env->bypass_spec_v4 = bpf_bypass_spec_v4(env->prog->aux->token);
	env->bpf_capable = is_priv = bpf_token_capable(env->prog->aux->token, CAP_BPF);

	bpf_get_btf_vmlinux();

	/* grab the mutex to protect few globals used by verifier */
	if (!is_priv)
		mutex_lock(&bpf_verifier_lock);