// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 43/55



	*ret_type = btf_type_by_id(btf, 0);
	if (!btf_id)
		/* void */
		return 0;
	t = btf_type_by_id(btf, btf_id);
	while (t && btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);
	if (!t)
		return -EINVAL;
	*ret_type = t;
	if (btf_type_is_ptr(t))
		/* kernel size of pointer. Not BPF's size of pointer*/
		return sizeof(void *);
	if (btf_type_is_int(t) || btf_is_any_enum(t) || btf_type_is_struct(t))
		return t->size;
	return -EINVAL;
}

static u8 __get_type_fmodel_flags(const struct btf_type *t)
{
	u8 flags = 0;

	if (btf_type_is_struct(t))
		flags |= BTF_FMODEL_STRUCT_ARG;
	if (btf_type_is_signed_int(t))
		flags |= BTF_FMODEL_SIGNED_ARG;

	return flags;
}

int btf_distill_func_proto(struct bpf_verifier_log *log,
			   struct btf *btf,
			   const struct btf_type *func,
			   const char *tname,
			   struct btf_func_model *m)
{
	const struct btf_param *args;
	const struct btf_type *t;
	u32 i, nargs;
	int ret;

	if (!func) {
		/* BTF function prototype doesn't match the verifier types.
		 * Fall back to MAX_BPF_FUNC_REG_ARGS u64 args.
		 */
		for (i = 0; i < MAX_BPF_FUNC_REG_ARGS; i++) {
			m->arg_size[i] = 8;
			m->arg_flags[i] = 0;
		}
		m->ret_size = 8;
		m->ret_flags = 0;
		m->nr_args = MAX_BPF_FUNC_REG_ARGS;
		return 0;
	}
	args = (const struct btf_param *)(func + 1);
	nargs = btf_type_vlen(func);
	if (nargs > MAX_BPF_FUNC_ARGS) {
		bpf_log(log,
			"The function %s has %d arguments. Too many.\n",
			tname, nargs);
		return -EINVAL;
	}
	ret = __get_type_size(btf, func->type, &t);
	if (ret < 0 || btf_type_is_struct(t)) {
		bpf_log(log,
			"The function %s return type %s is unsupported.\n",
			tname, btf_type_str(t));
		return -EINVAL;
	}
	m->ret_size = ret;
	m->ret_flags = __get_type_fmodel_flags(t);

	for (i = 0; i < nargs; i++) {
		if (i == nargs - 1 && args[i].type == 0) {
			bpf_log(log,
				"The function %s with variable args is unsupported.\n",
				tname);
			return -EINVAL;
		}
		ret = __get_type_size(btf, args[i].type, &t);

		/* No support of struct argument size greater than 16 bytes */
		if (ret < 0 || ret > 16) {
			bpf_log(log,
				"The function %s arg%d type %s is unsupported.\n",
				tname, i, btf_type_str(t));
			return -EINVAL;
		}
		if (ret == 0) {
			bpf_log(log,
				"The function %s has malformed void argument.\n",
				tname);
			return -EINVAL;
		}
		m->arg_size[i] = ret;
		m->arg_flags[i] = __get_type_fmodel_flags(t);
	}
	m->nr_args = nargs;
	return 0;
}

/* Compare BTFs of two functions assuming only scalars and pointers to context.
 * t1 points to BTF_KIND_FUNC in btf1
 * t2 points to BTF_KIND_FUNC in btf2
 * Returns:
 * EINVAL - function prototype mismatch
 * EFAULT - verifier bug
 * 0 - 99% match. The last 1% is validated by the verifier.
 */
static int btf_check_func_type_match(struct bpf_verifier_log *log,
				     struct btf *btf1, const struct btf_type *t1,
				     struct btf *btf2, const struct btf_type *t2)
{
	const struct btf_param *args1, *args2;
	const char *fn1, *fn2, *s1, *s2;
	u32 nargs1, nargs2, i;

	fn1 = btf_name_by_offset(btf1, t1->name_off);
	fn2 = btf_name_by_offset(btf2, t2->name_off);

	if (btf_func_linkage(t1) != BTF_FUNC_GLOBAL) {
		bpf_log(log, "%s() is not a global function\n", fn1);
		return -EINVAL;
	}
	if (btf_func_linkage(t2) != BTF_FUNC_GLOBAL) {
		bpf_log(log, "%s() is not a global function\n", fn2);
		return -EINVAL;
	}

	t1 = btf_type_by_id(btf1, t1->type);
	if (!t1 || !btf_type_is_func_proto(t1))
		return -EFAULT;
	t2 = btf_type_by_id(btf2, t2->type);
	if (!t2 || !btf_type_is_func_proto(t2))
		return -EFAULT;

	args1 = (const struct btf_param *)(t1 + 1);
	nargs1 = btf_type_vlen(t1);
	args2 = (const struct btf_param *)(t2 + 1);
	nargs2 = btf_type_vlen(t2);

	if (nargs1 != nargs2) {
		bpf_log(log, "%s() has %d args while %s() has %d args\n",
			fn1, nargs1, fn2, nargs2);
		return -EINVAL;
	}

	t1 = btf_type_skip_modifiers(btf1, t1->type, NULL);
	t2 = btf_type_skip_modifiers(btf2, t2->type, NULL);
	if (t1->info != t2->info) {
		bpf_log(log,
			"Return type %s of %s() doesn't match type %s of %s()\n",
			btf_type_str(t1), fn1,
			btf_type_str(t2), fn2);
		return -EINVAL;
	}

	for (i = 0; i < nargs1; i++) {
		t1 = btf_type_skip_modifiers(btf1, args1[i].type, NULL);
		t2 = btf_type_skip_modifiers(btf2, args2[i].type, NULL);

		if (t1->info != t2->info) {
			bpf_log(log, "arg%d in %s() is %s while %s() has %s\n",
				i, fn1, btf_type_str(t1),
				fn2, btf_type_str(t2));
			return -EINVAL;
		}
		if (btf_type_has_size(t1) && t1->size != t2->size) {
			bpf_log(log,
				"arg%d in %s() has size %d while %s() has %d\n",
				i, fn1, t1->size,
				fn2, t2->size);
			return -EINVAL;
		}