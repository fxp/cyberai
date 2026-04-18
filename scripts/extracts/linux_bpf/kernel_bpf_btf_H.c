// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 40/55



	/* check for PTR_TO_RDONLY_BUF_OR_NULL or PTR_TO_RDWR_BUF_OR_NULL */
	for (i = 0; i < prog->aux->ctx_arg_info_size; i++) {
		const struct bpf_ctx_arg_aux *ctx_arg_info = &prog->aux->ctx_arg_info[i];
		u32 type, flag;

		type = base_type(ctx_arg_info->reg_type);
		flag = type_flag(ctx_arg_info->reg_type);
		if (ctx_arg_info->offset == off && type == PTR_TO_BUF &&
		    (flag & PTR_MAYBE_NULL)) {
			info->reg_type = ctx_arg_info->reg_type;
			return true;
		}
	}

	/*
	 * If it's a single or multilevel pointer, except a pointer
	 * to a structure, it's the same as scalar from the verifier
	 * safety POV. Multilevel pointers to structures are treated as
	 * scalars. The verifier lacks the context to infer the size of
	 * their target memory regions. Either way, no further pointer
	 * walking is allowed.
	 */
	if (!btf_type_is_struct_ptr(btf, t))
		return true;

	/* this is a pointer to another type */
	for (i = 0; i < prog->aux->ctx_arg_info_size; i++) {
		const struct bpf_ctx_arg_aux *ctx_arg_info = &prog->aux->ctx_arg_info[i];

		if (ctx_arg_info->offset == off) {
			if (!ctx_arg_info->btf_id) {
				bpf_log(log,"invalid btf_id for context argument offset %u\n", off);
				return false;
			}

			info->reg_type = ctx_arg_info->reg_type;
			info->btf = ctx_arg_info->btf ? : btf_vmlinux;
			info->btf_id = ctx_arg_info->btf_id;
			info->ref_obj_id = ctx_arg_info->ref_obj_id;
			return true;
		}
	}

	info->reg_type = PTR_TO_BTF_ID;
	if (prog_args_trusted(prog))
		info->reg_type |= PTR_TRUSTED;

	if (btf_param_match_suffix(btf, &args[arg], "__nullable"))
		info->reg_type |= PTR_MAYBE_NULL;

	if (prog->expected_attach_type == BPF_TRACE_RAW_TP) {
		struct btf *btf = prog->aux->attach_btf;
		const struct btf_type *t;
		const char *tname;

		/* BTF lookups cannot fail, return false on error */
		t = btf_type_by_id(btf, prog->aux->attach_btf_id);
		if (!t)
			return false;
		tname = btf_name_by_offset(btf, t->name_off);
		if (!tname)
			return false;
		/* Checked by bpf_check_attach_target */
		tname += sizeof("btf_trace_") - 1;
		for (i = 0; i < ARRAY_SIZE(raw_tp_null_args); i++) {
			/* Is this a func with potential NULL args? */
			if (strcmp(tname, raw_tp_null_args[i].func))
				continue;
			if (raw_tp_null_args[i].mask & (0x1ULL << (arg * 4)))
				info->reg_type |= PTR_MAYBE_NULL;
			/* Is the current arg IS_ERR? */
			if (raw_tp_null_args[i].mask & (0x2ULL << (arg * 4)))
				ptr_err_raw_tp = true;
			break;
		}
		/* If we don't know NULL-ness specification and the tracepoint
		 * is coming from a loadable module, be conservative and mark
		 * argument as PTR_MAYBE_NULL.
		 */
		if (i == ARRAY_SIZE(raw_tp_null_args) && btf_is_module(btf))
			info->reg_type |= PTR_MAYBE_NULL;
	}

	if (tgt_prog) {
		enum bpf_prog_type tgt_type;

		if (tgt_prog->type == BPF_PROG_TYPE_EXT)
			tgt_type = tgt_prog->aux->saved_dst_prog_type;
		else
			tgt_type = tgt_prog->type;

		ret = btf_translate_to_vmlinux(log, btf, t, tgt_type, arg);
		if (ret > 0) {
			info->btf = btf_vmlinux;
			info->btf_id = ret;
			return true;
		} else {
			return false;
		}
	}

	info->btf = btf;
	info->btf_id = t->type;
	t = btf_type_by_id(btf, t->type);

	if (btf_type_is_type_tag(t) && !btf_type_kflag(t)) {
		tag_value = __btf_name_by_offset(btf, t->name_off);
		if (strcmp(tag_value, "user") == 0)
			info->reg_type |= MEM_USER;
		if (strcmp(tag_value, "percpu") == 0)
			info->reg_type |= MEM_PERCPU;
	}

	/* skip modifiers */
	while (btf_type_is_modifier(t)) {
		info->btf_id = t->type;
		t = btf_type_by_id(btf, t->type);
	}
	if (!btf_type_is_struct(t)) {
		bpf_log(log,
			"func '%s' arg%d type %s is not a struct\n",
			tname, arg, btf_type_str(t));
		return false;
	}
	bpf_log(log, "func '%s' arg%d has btf_id %d type %s '%s'\n",
		tname, arg, info->btf_id, btf_type_str(t),
		__btf_name_by_offset(btf, t->name_off));

	/* Perform all checks on the validity of type for this argument, but if
	 * we know it can be IS_ERR at runtime, scrub pointer type and mark as
	 * scalar.
	 */
	if (ptr_err_raw_tp) {
		bpf_log(log, "marking pointer arg%d as scalar as it may encode error", arg);
		info->reg_type = SCALAR_VALUE;
	}
	return true;
}
EXPORT_SYMBOL_GPL(btf_ctx_access);

enum bpf_struct_walk_result {
	/* < 0 error */
	WALK_SCALAR = 0,
	WALK_PTR,
	WALK_PTR_UNTRUSTED,
	WALK_STRUCT,
};

static int btf_struct_walk(struct bpf_verifier_log *log, const struct btf *btf,
			   const struct btf_type *t, int off, int size,
			   u32 *next_btf_id, enum bpf_type_flag *flag,
			   const char **field_name)
{
	u32 i, moff, mtrue_end, msize = 0, total_nelems = 0;
	const struct btf_type *mtype, *elem_type = NULL;
	const struct btf_member *member;
	const char *tname, *mname, *tag_value;
	u32 vlen, elem_id, mid;

again:
	if (btf_type_is_modifier(t))
		t = btf_type_skip_modifiers(btf, t->type, NULL);
	tname = __btf_name_by_offset(btf, t->name_off);
	if (!btf_type_is_struct(t)) {
		bpf_log(log, "Type '%s' is not a struct\n", tname);
		return -EINVAL;
	}