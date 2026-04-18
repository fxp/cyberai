// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 131/132



	/* user could have requested verbose verifier output
	 * and supplied buffer to store the verification trace
	 */
	ret = bpf_vlog_init(&env->log, attr->log_level,
			    (char __user *) (unsigned long) attr->log_buf,
			    attr->log_size);
	if (ret)
		goto err_unlock;

	ret = process_fd_array(env, attr, uattr);
	if (ret)
		goto skip_full_check;

	mark_verifier_state_clean(env);

	if (IS_ERR(btf_vmlinux)) {
		/* Either gcc or pahole or kernel are broken. */
		verbose(env, "in-kernel BTF is malformed\n");
		ret = PTR_ERR(btf_vmlinux);
		goto skip_full_check;
	}

	env->strict_alignment = !!(attr->prog_flags & BPF_F_STRICT_ALIGNMENT);
	if (!IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS))
		env->strict_alignment = true;
	if (attr->prog_flags & BPF_F_ANY_ALIGNMENT)
		env->strict_alignment = false;

	if (is_priv)
		env->test_state_freq = attr->prog_flags & BPF_F_TEST_STATE_FREQ;
	env->test_reg_invariants = attr->prog_flags & BPF_F_TEST_REG_INVARIANTS;

	env->explored_states = kvzalloc_objs(struct list_head,
					     state_htab_size(env),
					     GFP_KERNEL_ACCOUNT);
	ret = -ENOMEM;
	if (!env->explored_states)
		goto skip_full_check;

	for (i = 0; i < state_htab_size(env); i++)
		INIT_LIST_HEAD(&env->explored_states[i]);
	INIT_LIST_HEAD(&env->free_list);

	ret = bpf_check_btf_info_early(env, attr, uattr);
	if (ret < 0)
		goto skip_full_check;

	ret = add_subprog_and_kfunc(env);
	if (ret < 0)
		goto skip_full_check;

	ret = check_subprogs(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_check_btf_info(env, attr, uattr);
	if (ret < 0)
		goto skip_full_check;

	ret = check_and_resolve_insns(env);
	if (ret < 0)
		goto skip_full_check;

	if (bpf_prog_is_offloaded(env->prog->aux)) {
		ret = bpf_prog_offload_verifier_prep(env->prog);
		if (ret)
			goto skip_full_check;
	}

	ret = bpf_check_cfg(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_compute_postorder(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_stack_liveness_init(env);
	if (ret)
		goto skip_full_check;

	ret = check_attach_btf_id(env);
	if (ret)
		goto skip_full_check;

	ret = bpf_compute_const_regs(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_prune_dead_branches(env);
	if (ret < 0)
		goto skip_full_check;

	ret = sort_subprogs_topo(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_compute_scc(env);
	if (ret < 0)
		goto skip_full_check;

	ret = bpf_compute_live_registers(env);
	if (ret < 0)
		goto skip_full_check;

	ret = mark_fastcall_patterns(env);
	if (ret < 0)
		goto skip_full_check;

	ret = do_check_main(env);
	ret = ret ?: do_check_subprogs(env);

	if (ret == 0 && bpf_prog_is_offloaded(env->prog->aux))
		ret = bpf_prog_offload_finalize(env);

skip_full_check:
	kvfree(env->explored_states);

	/* might decrease stack depth, keep it before passes that
	 * allocate additional slots.
	 */
	if (ret == 0)
		ret = bpf_remove_fastcall_spills_fills(env);

	if (ret == 0)
		ret = check_max_stack_depth(env);

	/* instruction rewrites happen after this point */
	if (ret == 0)
		ret = bpf_optimize_bpf_loop(env);

	if (is_priv) {
		if (ret == 0)
			bpf_opt_hard_wire_dead_code_branches(env);
		if (ret == 0)
			ret = bpf_opt_remove_dead_code(env);
		if (ret == 0)
			ret = bpf_opt_remove_nops(env);
	} else {
		if (ret == 0)
			sanitize_dead_code(env);
	}

	if (ret == 0)
		/* program is valid, convert *(u32*)(ctx + off) accesses */
		ret = bpf_convert_ctx_accesses(env);

	if (ret == 0)
		ret = bpf_do_misc_fixups(env);

	/* do 32-bit optimization after insn patching has done so those patched
	 * insns could be handled correctly.
	 */
	if (ret == 0 && !bpf_prog_is_offloaded(env->prog->aux)) {
		ret = bpf_opt_subreg_zext_lo32_rnd_hi32(env, attr);
		env->prog->aux->verifier_zext = bpf_jit_needs_zext() ? !ret
								     : false;
	}

	if (ret == 0)
		ret = bpf_fixup_call_args(env);

	env->verification_time = ktime_get_ns() - start_time;
	print_verification_stats(env);
	env->prog->aux->verified_insns = env->insn_processed;

	/* preserve original error even if log finalization is successful */
	err = bpf_vlog_finalize(&env->log, &log_true_size);
	if (err)
		ret = err;

	if (uattr_size >= offsetofend(union bpf_attr, log_true_size) &&
	    copy_to_bpfptr_offset(uattr, offsetof(union bpf_attr, log_true_size),
				  &log_true_size, sizeof(log_true_size))) {
		ret = -EFAULT;
		goto err_release_maps;
	}

	if (ret)
		goto err_release_maps;

	if (env->used_map_cnt) {
		/* if program passed verifier, update used_maps in bpf_prog_info */
		env->prog->aux->used_maps = kmalloc_objs(env->used_maps[0],
							 env->used_map_cnt,
							 GFP_KERNEL_ACCOUNT);

		if (!env->prog->aux->used_maps) {
			ret = -ENOMEM;
			goto err_release_maps;
		}