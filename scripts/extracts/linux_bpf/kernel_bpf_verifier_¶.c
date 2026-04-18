// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 118/132



		/* Reduce verification complexity by stopping speculative path
		 * verification when a nospec is encountered.
		 */
		if (state->speculative && insn_aux->nospec)
			goto process_bpf_exit;

		err = do_check_insn(env, &do_print_state);
		if (error_recoverable_with_nospec(err) && state->speculative) {
			/* Prevent this speculative path from ever reaching the
			 * insn that would have been unsafe to execute.
			 */
			insn_aux->nospec = true;
			/* If it was an ADD/SUB insn, potentially remove any
			 * markings for alu sanitization.
			 */
			insn_aux->alu_state = 0;
			goto process_bpf_exit;
		} else if (err < 0) {
			return err;
		} else if (err == PROCESS_BPF_EXIT) {
			goto process_bpf_exit;
		} else if (err == INSN_IDX_UPDATED) {
		} else if (err == 0) {
			env->insn_idx++;
		}

		if (state->speculative && insn_aux->nospec_result) {
			/* If we are on a path that performed a jump-op, this
			 * may skip a nospec patched-in after the jump. This can
			 * currently never happen because nospec_result is only
			 * used for the write-ops
			 * `*(size*)(dst_reg+off)=src_reg|imm32` and helper
			 * calls. These must never skip the following insn
			 * (i.e., bpf_insn_successors()'s opcode_info.can_jump
			 * is false). Still, add a warning to document this in
			 * case nospec_result is used elsewhere in the future.
			 *
			 * All non-branch instructions have a single
			 * fall-through edge. For these, nospec_result should
			 * already work.
			 */
			if (verifier_bug_if((BPF_CLASS(insn->code) == BPF_JMP ||
					     BPF_CLASS(insn->code) == BPF_JMP32) &&
					    BPF_OP(insn->code) != BPF_CALL, env,
					    "speculation barrier after jump instruction may not have the desired effect"))
				return -EFAULT;
process_bpf_exit:
			mark_verifier_state_scratched(env);
			err = bpf_update_branch_counts(env, env->cur_state);
			if (err)
				return err;
			err = pop_stack(env, &prev_insn_idx, &env->insn_idx,
					pop_log);
			if (err < 0) {
				if (err != -ENOENT)
					return err;
				break;
			} else {
				do_print_state = true;
				continue;
			}
		}
	}

	return 0;
}

static int find_btf_percpu_datasec(struct btf *btf)
{
	const struct btf_type *t;
	const char *tname;
	int i, n;

	/*
	 * Both vmlinux and module each have their own ".data..percpu"
	 * DATASECs in BTF. So for module's case, we need to skip vmlinux BTF
	 * types to look at only module's own BTF types.
	 */
	n = btf_nr_types(btf);
	for (i = btf_named_start_id(btf, true); i < n; i++) {
		t = btf_type_by_id(btf, i);
		if (BTF_INFO_KIND(t->info) != BTF_KIND_DATASEC)
			continue;

		tname = btf_name_by_offset(btf, t->name_off);
		if (!strcmp(tname, ".data..percpu"))
			return i;
	}

	return -ENOENT;
}

/*
 * Add btf to the env->used_btfs array. If needed, refcount the
 * corresponding kernel module. To simplify caller's logic
 * in case of error or if btf was added before the function
 * decreases the btf refcount.
 */
static int __add_used_btf(struct bpf_verifier_env *env, struct btf *btf)
{
	struct btf_mod_pair *btf_mod;
	int ret = 0;
	int i;

	/* check whether we recorded this BTF (and maybe module) already */
	for (i = 0; i < env->used_btf_cnt; i++)
		if (env->used_btfs[i].btf == btf)
			goto ret_put;

	if (env->used_btf_cnt >= MAX_USED_BTFS) {
		verbose(env, "The total number of btfs per program has reached the limit of %u\n",
			MAX_USED_BTFS);
		ret = -E2BIG;
		goto ret_put;
	}

	btf_mod = &env->used_btfs[env->used_btf_cnt];
	btf_mod->btf = btf;
	btf_mod->module = NULL;

	/* if we reference variables from kernel module, bump its refcount */
	if (btf_is_module(btf)) {
		btf_mod->module = btf_try_get_module(btf);
		if (!btf_mod->module) {
			ret = -ENXIO;
			goto ret_put;
		}
	}

	env->used_btf_cnt++;
	return 0;

ret_put:
	/* Either error or this BTF was already added */
	btf_put(btf);
	return ret;
}

/* replace pseudo btf_id with kernel symbol address */
static int __check_pseudo_btf_id(struct bpf_verifier_env *env,
				 struct bpf_insn *insn,
				 struct bpf_insn_aux_data *aux,
				 struct btf *btf)
{
	const struct btf_var_secinfo *vsi;
	const struct btf_type *datasec;
	const struct btf_type *t;
	const char *sym_name;
	bool percpu = false;
	u32 type, id = insn->imm;
	s32 datasec_id;
	u64 addr;
	int i;

	t = btf_type_by_id(btf, id);
	if (!t) {
		verbose(env, "ldimm64 insn specifies invalid btf_id %d.\n", id);
		return -ENOENT;
	}

	if (!btf_type_is_var(t) && !btf_type_is_func(t)) {
		verbose(env, "pseudo btf_id %d in ldimm64 isn't KIND_VAR or KIND_FUNC\n", id);
		return -EINVAL;
	}

	sym_name = btf_name_by_offset(btf, t->name_off);
	addr = kallsyms_lookup_name(sym_name);
	if (!addr) {
		verbose(env, "ldimm64 failed to find the address for kernel symbol '%s'.\n",
			sym_name);
		return -ENOENT;
	}
	insn[0].imm = (u32)addr;
	insn[1].imm = addr >> 32;

	if (btf_type_is_func(t)) {
		aux->btf_var.reg_type = PTR_TO_MEM | MEM_RDONLY;
		aux->btf_var.mem_size = 0;
		return 0;
	}