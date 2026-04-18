// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 45/55



/* Process BTF of a function to produce high-level expectation of function
 * arguments (like ARG_PTR_TO_CTX, or ARG_PTR_TO_MEM, etc). This information
 * is cached in subprog info for reuse.
 * Returns:
 * EFAULT - there is a verifier bug. Abort verification.
 * EINVAL - cannot convert BTF.
 * 0 - Successfully processed BTF and constructed argument expectations.
 */
int btf_prepare_func_args(struct bpf_verifier_env *env, int subprog)
{
	bool is_global = subprog_aux(env, subprog)->linkage == BTF_FUNC_GLOBAL;
	struct bpf_subprog_info *sub = subprog_info(env, subprog);
	struct bpf_verifier_log *log = &env->log;
	struct bpf_prog *prog = env->prog;
	enum bpf_prog_type prog_type = prog->type;
	struct btf *btf = prog->aux->btf;
	const struct btf_param *args;
	const struct btf_type *t, *ref_t, *fn_t;
	u32 i, nargs, btf_id;
	const char *tname;

	if (sub->args_cached)
		return 0;

	if (!prog->aux->func_info) {
		verifier_bug(env, "func_info undefined");
		return -EFAULT;
	}

	btf_id = prog->aux->func_info[subprog].type_id;
	if (!btf_id) {
		if (!is_global) /* not fatal for static funcs */
			return -EINVAL;
		bpf_log(log, "Global functions need valid BTF\n");
		return -EFAULT;
	}

	fn_t = btf_type_by_id(btf, btf_id);
	if (!fn_t || !btf_type_is_func(fn_t)) {
		/* These checks were already done by the verifier while loading
		 * struct bpf_func_info
		 */
		bpf_log(log, "BTF of func#%d doesn't point to KIND_FUNC\n",
			subprog);
		return -EFAULT;
	}
	tname = btf_name_by_offset(btf, fn_t->name_off);

	if (prog->aux->func_info_aux[subprog].unreliable) {
		verifier_bug(env, "unreliable BTF for function %s()", tname);
		return -EFAULT;
	}
	if (prog_type == BPF_PROG_TYPE_EXT)
		prog_type = prog->aux->dst_prog->type;

	t = btf_type_by_id(btf, fn_t->type);
	if (!t || !btf_type_is_func_proto(t)) {
		bpf_log(log, "Invalid type of function %s()\n", tname);
		return -EFAULT;
	}
	args = (const struct btf_param *)(t + 1);
	nargs = btf_type_vlen(t);
	if (nargs > MAX_BPF_FUNC_REG_ARGS) {
		if (!is_global)
			return -EINVAL;
		bpf_log(log, "Global function %s() with %d > %d args. Buggy compiler.\n",
			tname, nargs, MAX_BPF_FUNC_REG_ARGS);
		return -EINVAL;
	}
	/* check that function is void or returns int, exception cb also requires this */
	t = btf_type_by_id(btf, t->type);
	while (btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);
	if (!btf_type_is_void(t) && !btf_type_is_int(t) && !btf_is_any_enum(t)) {
		if (!is_global)
			return -EINVAL;
		bpf_log(log,
			"Global function %s() return value not void or scalar. "
			"Only those are supported.\n",
			tname);
		return -EINVAL;
	}

	/* Convert BTF function arguments into verifier types.
	 * Only PTR_TO_CTX and SCALAR are supported atm.
	 */
	for (i = 0; i < nargs; i++) {
		u32 tags = 0;
		int id = btf_named_start_id(btf, false) - 1;

		/* 'arg:<tag>' decl_tag takes precedence over derivation of
		 * register type from BTF type itself
		 */
		while ((id = btf_find_next_decl_tag(btf, fn_t, i, "arg:", id)) > 0) {
			const struct btf_type *tag_t = btf_type_by_id(btf, id);
			const char *tag = __btf_name_by_offset(btf, tag_t->name_off) + 4;

			/* disallow arg tags in static subprogs */
			if (!is_global) {
				bpf_log(log, "arg#%d type tag is not supported in static functions\n", i);
				return -EOPNOTSUPP;
			}

			if (strcmp(tag, "ctx") == 0) {
				tags |= ARG_TAG_CTX;
			} else if (strcmp(tag, "trusted") == 0) {
				tags |= ARG_TAG_TRUSTED;
			} else if (strcmp(tag, "untrusted") == 0) {
				tags |= ARG_TAG_UNTRUSTED;
			} else if (strcmp(tag, "nonnull") == 0) {
				tags |= ARG_TAG_NONNULL;
			} else if (strcmp(tag, "nullable") == 0) {
				tags |= ARG_TAG_NULLABLE;
			} else if (strcmp(tag, "arena") == 0) {
				tags |= ARG_TAG_ARENA;
			} else {
				bpf_log(log, "arg#%d has unsupported set of tags\n", i);
				return -EOPNOTSUPP;
			}
		}
		if (id != -ENOENT) {
			bpf_log(log, "arg#%d type tag fetching failure: %d\n", i, id);
			return id;
		}

		t = btf_type_by_id(btf, args[i].type);
		while (btf_type_is_modifier(t))
			t = btf_type_by_id(btf, t->type);
		if (!btf_type_is_ptr(t))
			goto skip_pointer;

		if ((tags & ARG_TAG_CTX) || btf_is_prog_ctx_type(log, btf, t, prog_type, i)) {
			if (tags & ~ARG_TAG_CTX) {
				bpf_log(log, "arg#%d has invalid combination of tags\n", i);
				return -EINVAL;
			}
			if ((tags & ARG_TAG_CTX) &&
			    btf_validate_prog_ctx_type(log, btf, t, i, prog_type,
						       prog->expected_attach_type))
				return -EINVAL;
			sub->args[i].arg_type = ARG_PTR_TO_CTX;
			continue;
		}
		if (btf_is_dynptr_ptr(btf, t)) {
			if (tags) {
				bpf_log(log, "arg#%d has invalid combination of tags\n", i);
				return -EINVAL;
			}
			sub->args[i].arg_type = ARG_PTR_TO_DYNPTR | MEM_RDONLY;
			continue;
		}
		if (tags & ARG_TAG_TRUSTED) {
			int kern_type_id;

			if (tags & ARG_TAG_NONNULL) {
				bpf_log(log, "arg#%d has invalid combination of tags\n", i);
				return -EINVAL;
			}