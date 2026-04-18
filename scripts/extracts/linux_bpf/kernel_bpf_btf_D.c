// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 36/55



static int btf_validate_prog_ctx_type(struct bpf_verifier_log *log, const struct btf *btf,
				      const struct btf_type *t, int arg,
				      enum bpf_prog_type prog_type,
				      enum bpf_attach_type attach_type)
{
	const struct btf_type *ctx_type;
	const char *tname, *ctx_tname;

	if (!btf_is_ptr(t)) {
		bpf_log(log, "arg#%d type isn't a pointer\n", arg);
		return -EINVAL;
	}
	t = btf_type_by_id(btf, t->type);

	/* KPROBE and PERF_EVENT programs allow bpf_user_pt_regs_t typedef */
	if (prog_type == BPF_PROG_TYPE_KPROBE || prog_type == BPF_PROG_TYPE_PERF_EVENT) {
		while (btf_type_is_modifier(t) && !btf_type_is_typedef(t))
			t = btf_type_by_id(btf, t->type);

		if (btf_type_is_typedef(t)) {
			tname = btf_name_by_offset(btf, t->name_off);
			if (tname && strcmp(tname, "bpf_user_pt_regs_t") == 0)
				return 0;
		}
	}

	/* all other program types don't use typedefs for context type */
	while (btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);

	/* `void *ctx __arg_ctx` is always valid */
	if (btf_type_is_void(t))
		return 0;

	tname = btf_name_by_offset(btf, t->name_off);
	if (str_is_empty(tname)) {
		bpf_log(log, "arg#%d type doesn't have a name\n", arg);
		return -EINVAL;
	}

	/* special cases */
	switch (prog_type) {
	case BPF_PROG_TYPE_KPROBE:
		if (__btf_type_is_struct(t) && strcmp(tname, "pt_regs") == 0)
			return 0;
		break;
	case BPF_PROG_TYPE_PERF_EVENT:
		if (__builtin_types_compatible_p(bpf_user_pt_regs_t, struct pt_regs) &&
		    __btf_type_is_struct(t) && strcmp(tname, "pt_regs") == 0)
			return 0;
		if (__builtin_types_compatible_p(bpf_user_pt_regs_t, struct user_pt_regs) &&
		    __btf_type_is_struct(t) && strcmp(tname, "user_pt_regs") == 0)
			return 0;
		if (__builtin_types_compatible_p(bpf_user_pt_regs_t, struct user_regs_struct) &&
		    __btf_type_is_struct(t) && strcmp(tname, "user_regs_struct") == 0)
			return 0;
		break;
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE:
		/* allow u64* as ctx */
		if (btf_is_int(t) && t->size == 8)
			return 0;
		break;
	case BPF_PROG_TYPE_TRACING:
		switch (attach_type) {
		case BPF_TRACE_RAW_TP:
			/* tp_btf program is TRACING, so need special case here */
			if (__btf_type_is_struct(t) &&
			    strcmp(tname, "bpf_raw_tracepoint_args") == 0)
				return 0;
			/* allow u64* as ctx */
			if (btf_is_int(t) && t->size == 8)
				return 0;
			break;
		case BPF_TRACE_ITER:
			/* allow struct bpf_iter__xxx types only */
			if (__btf_type_is_struct(t) &&
			    strncmp(tname, "bpf_iter__", sizeof("bpf_iter__") - 1) == 0)
				return 0;
			break;
		case BPF_TRACE_FENTRY:
		case BPF_TRACE_FEXIT:
		case BPF_MODIFY_RETURN:
		case BPF_TRACE_FSESSION:
			/* allow u64* as ctx */
			if (btf_is_int(t) && t->size == 8)
				return 0;
			break;
		default:
			break;
		}
		break;
	case BPF_PROG_TYPE_LSM:
	case BPF_PROG_TYPE_STRUCT_OPS:
		/* allow u64* as ctx */
		if (btf_is_int(t) && t->size == 8)
			return 0;
		break;
	case BPF_PROG_TYPE_TRACEPOINT:
	case BPF_PROG_TYPE_SYSCALL:
	case BPF_PROG_TYPE_EXT:
		return 0; /* anything goes */
	default:
		break;
	}

	ctx_type = find_canonical_prog_ctx_type(prog_type);
	if (!ctx_type) {
		/* should not happen */
		bpf_log(log, "btf_vmlinux is malformed\n");
		return -EINVAL;
	}

	/* resolve typedefs and check that underlying structs are matching as well */
	while (btf_type_is_modifier(ctx_type))
		ctx_type = btf_type_by_id(btf_vmlinux, ctx_type->type);

	/* if program type doesn't have distinctly named struct type for
	 * context, then __arg_ctx argument can only be `void *`, which we
	 * already checked above
	 */
	if (!__btf_type_is_struct(ctx_type)) {
		bpf_log(log, "arg#%d should be void pointer\n", arg);
		return -EINVAL;
	}

	ctx_tname = btf_name_by_offset(btf_vmlinux, ctx_type->name_off);
	if (!__btf_type_is_struct(t) || strcmp(ctx_tname, tname) != 0) {
		bpf_log(log, "arg#%d should be `struct %s *`\n", arg, ctx_tname);
		return -EINVAL;
	}

	return 0;
}

static int btf_translate_to_vmlinux(struct bpf_verifier_log *log,
				     struct btf *btf,
				     const struct btf_type *t,
				     enum bpf_prog_type prog_type,
				     int arg)
{
	if (!btf_is_prog_ctx_type(log, btf, t, prog_type, arg))
		return -ENOENT;
	return find_kern_ctx_type_id(prog_type);
}

int get_kern_ctx_btf_id(struct bpf_verifier_log *log, enum bpf_prog_type prog_type)
{
	const struct btf_member *kctx_member;
	const struct btf_type *conv_struct;
	const struct btf_type *kctx_type;
	u32 kctx_type_id;

	conv_struct = bpf_ctx_convert.t;
	/* get member for kernel ctx type */
	kctx_member = btf_type_member(conv_struct) + bpf_ctx_convert_map[prog_type] * 2 + 1;
	kctx_type_id = kctx_member->type;
	kctx_type = btf_type_by_id(btf_vmlinux, kctx_type_id);
	if (!btf_type_is_struct(kctx_type)) {
		bpf_log(log, "kern ctx type id %u is not a struct\n", kctx_type_id);
		return -EINVAL;
	}

	return kctx_type_id;
}

BTF_ID_LIST_SINGLE(bpf_ctx_convert_btf_id, struct, bpf_ctx_convert)