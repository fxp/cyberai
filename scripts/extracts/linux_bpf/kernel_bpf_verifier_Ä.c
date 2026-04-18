// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 132/132



		memcpy(env->prog->aux->used_maps, env->used_maps,
		       sizeof(env->used_maps[0]) * env->used_map_cnt);
		env->prog->aux->used_map_cnt = env->used_map_cnt;
	}
	if (env->used_btf_cnt) {
		/* if program passed verifier, update used_btfs in bpf_prog_aux */
		env->prog->aux->used_btfs = kmalloc_objs(env->used_btfs[0],
							 env->used_btf_cnt,
							 GFP_KERNEL_ACCOUNT);
		if (!env->prog->aux->used_btfs) {
			ret = -ENOMEM;
			goto err_release_maps;
		}

		memcpy(env->prog->aux->used_btfs, env->used_btfs,
		       sizeof(env->used_btfs[0]) * env->used_btf_cnt);
		env->prog->aux->used_btf_cnt = env->used_btf_cnt;
	}
	if (env->used_map_cnt || env->used_btf_cnt) {
		/* program is valid. Convert pseudo bpf_ld_imm64 into generic
		 * bpf_ld_imm64 instructions
		 */
		convert_pseudo_ld_imm64(env);
	}

	adjust_btf_func(env);

	/* extension progs temporarily inherit the attach_type of their targets
	   for verification purposes, so set it back to zero before returning
	 */
	if (env->prog->type == BPF_PROG_TYPE_EXT)
		env->prog->expected_attach_type = 0;

	env->prog = __bpf_prog_select_runtime(env, env->prog, &ret);

err_release_maps:
	if (ret)
		release_insn_arrays(env);
	if (!env->prog->aux->used_maps)
		/* if we didn't copy map pointers into bpf_prog_info, release
		 * them now. Otherwise free_used_maps() will release them.
		 */
		release_maps(env);
	if (!env->prog->aux->used_btfs)
		release_btfs(env);

	*prog = env->prog;

	module_put(env->attach_btf_mod);
err_unlock:
	if (!is_priv)
		mutex_unlock(&bpf_verifier_lock);
	bpf_clear_insn_aux_data(env, 0, env->prog->len);
	vfree(env->insn_aux_data);
err_free_env:
	bpf_stack_liveness_free(env);
	kvfree(env->cfg.insn_postorder);
	kvfree(env->scc_info);
	kvfree(env->succ);
	kvfree(env->gotox_tmp_buf);
	kvfree(env);
	return ret;
}
