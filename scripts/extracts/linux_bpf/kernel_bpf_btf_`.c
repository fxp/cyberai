// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 32/55



	env->resolve_mode = RESOLVE_TBD;
	env_stack_push(env, t, type_id);
	while (!err && (v = env_stack_peak(env))) {
		env->log_type_id = v->type_id;
		err = btf_type_ops(v->t)->resolve(env, v);
	}

	env->log_type_id = type_id;
	if (err == -E2BIG) {
		btf_verifier_log_type(env, t,
				      "Exceeded max resolving depth:%u",
				      MAX_RESOLVE_DEPTH);
	} else if (err == -EEXIST) {
		btf_verifier_log_type(env, t, "Loop detected");
	}

	/* Final sanity check */
	if (!err && !btf_resolve_valid(env, t, type_id)) {
		btf_verifier_log_type(env, t, "Invalid resolve state");
		err = -EINVAL;
	}

	env->log_type_id = save_log_type_id;
	return err;
}

static int btf_check_all_types(struct btf_verifier_env *env)
{
	struct btf *btf = env->btf;
	const struct btf_type *t;
	u32 type_id, i;
	int err;

	err = env_resolve_init(env);
	if (err)
		return err;

	env->phase++;
	for (i = btf->base_btf ? 0 : 1; i < btf->nr_types; i++) {
		type_id = btf->start_id + i;
		t = btf_type_by_id(btf, type_id);

		env->log_type_id = type_id;
		if (btf_type_needs_resolve(t) &&
		    !env_type_is_resolved(env, type_id)) {
			err = btf_resolve(env, t, type_id);
			if (err)
				return err;
		}

		if (btf_type_is_func_proto(t)) {
			err = btf_func_proto_check(env, t);
			if (err)
				return err;
		}
	}

	return 0;
}

static int btf_parse_type_sec(struct btf_verifier_env *env)
{
	const struct btf_header *hdr = &env->btf->hdr;
	int err;

	/* Type section must align to 4 bytes */
	if (hdr->type_off & (sizeof(u32) - 1)) {
		btf_verifier_log(env, "Unaligned type_off");
		return -EINVAL;
	}

	if (!env->btf->base_btf && !hdr->type_len) {
		btf_verifier_log(env, "No type found");
		return -EINVAL;
	}

	err = btf_check_all_metas(env);
	if (err)
		return err;

	return btf_check_all_types(env);
}

static int btf_parse_str_sec(struct btf_verifier_env *env)
{
	const struct btf_header *hdr;
	struct btf *btf = env->btf;
	const char *start, *end;

	hdr = &btf->hdr;
	start = btf->nohdr_data + hdr->str_off;
	end = start + hdr->str_len;

	if (hdr->hdr_len < sizeof(struct btf_header) &&
	    end != btf->data + btf->data_size) {
		btf_verifier_log(env, "String section is not at the end");
		return -EINVAL;
	}

	btf->strings = start;

	if (btf->base_btf && !hdr->str_len)
		return 0;
	if (!hdr->str_len || hdr->str_len - 1 > BTF_MAX_NAME_OFFSET || end[-1]) {
		btf_verifier_log(env, "Invalid string section");
		return -EINVAL;
	}
	if (!btf->base_btf && start[0]) {
		btf_verifier_log(env, "Invalid string section");
		return -EINVAL;
	}

	return 0;
}

static int btf_parse_layout_sec(struct btf_verifier_env *env)
{
	const struct btf_header *hdr = &env->btf->hdr;
	struct btf *btf = env->btf;
	void *start, *end;

	if (hdr->hdr_len < sizeof(struct btf_header) ||
	    hdr->layout_len == 0)
		return 0;

	/* Layout section must align to 4 bytes */
	if (hdr->layout_off & (sizeof(u32) - 1)) {
		btf_verifier_log(env, "Unaligned layout_off");
		return -EINVAL;
	}
	start = btf->nohdr_data + hdr->layout_off;
	end = start + hdr->layout_len;

	if (hdr->layout_len < sizeof(struct btf_layout)) {
		btf_verifier_log(env, "Layout section is too small");
		return -EINVAL;
	}
	if (hdr->layout_len % sizeof(struct btf_layout) != 0) {
		btf_verifier_log(env, "layout_len is not multiple of %zu",
				 sizeof(struct btf_layout));
		return -EINVAL;
	}
	if (end > btf->data + btf->data_size) {
		btf_verifier_log(env, "Layout section is too big");
		return -EINVAL;
	}
	btf->layout = start;

	return 0;
}

static const size_t btf_sec_info_offset[] = {
	offsetof(struct btf_header, type_off),
	offsetof(struct btf_header, str_off),
	offsetof(struct btf_header, layout_off)
};

static int btf_sec_info_cmp(const void *a, const void *b)
{
	const struct btf_sec_info *x = a;
	const struct btf_sec_info *y = b;

	return (int)(x->off - y->off) ? : (int)(x->len - y->len);
}

static int btf_check_sec_info(struct btf_verifier_env *env,
			      u32 btf_data_size)
{
	struct btf_sec_info secs[ARRAY_SIZE(btf_sec_info_offset)];
	u32 total, expected_total, i;
	u32 nr_secs = ARRAY_SIZE(btf_sec_info_offset);
	const struct btf_header *hdr;
	const struct btf *btf;

	btf = env->btf;
	hdr = &btf->hdr;

	if (hdr->hdr_len < sizeof(struct btf_header) || hdr->layout_len == 0)
		nr_secs--;

	/* Populate the secs from hdr */
	for (i = 0; i < nr_secs; i++)
		secs[i] = *(struct btf_sec_info *)((void *)hdr +
						   btf_sec_info_offset[i]);

	sort(secs, nr_secs,
	     sizeof(struct btf_sec_info), btf_sec_info_cmp, NULL);