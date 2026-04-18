// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 27/55



	/* enum type either no name or a valid one */
	if (t->name_off &&
	    !btf_name_valid_identifier(env->btf, t->name_off)) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	for (i = 0; i < nr_enums; i++) {
		if (!btf_name_offset_valid(btf, enums[i].name_off)) {
			btf_verifier_log(env, "\tInvalid name_offset:%u",
					 enums[i].name_off);
			return -EINVAL;
		}

		/* enum member must have a valid name */
		if (!enums[i].name_off ||
		    !btf_name_valid_identifier(btf, enums[i].name_off)) {
			btf_verifier_log_type(env, t, "Invalid name");
			return -EINVAL;
		}

		if (env->log.level == BPF_LOG_KERNEL)
			continue;
		fmt_str = btf_type_kflag(t) ? "\t%s val=%d\n" : "\t%s val=%u\n";
		btf_verifier_log(env, fmt_str,
				 __btf_name_by_offset(btf, enums[i].name_off),
				 enums[i].val);
	}

	return meta_needed;
}

static void btf_enum_log(struct btf_verifier_env *env,
			 const struct btf_type *t)
{
	btf_verifier_log(env, "size=%u vlen=%u", t->size, btf_type_vlen(t));
}

static void btf_enum_show(const struct btf *btf, const struct btf_type *t,
			  u32 type_id, void *data, u8 bits_offset,
			  struct btf_show *show)
{
	const struct btf_enum *enums = btf_type_enum(t);
	u32 i, nr_enums = btf_type_vlen(t);
	void *safe_data;
	int v;

	safe_data = btf_show_start_type(show, t, type_id, data);
	if (!safe_data)
		return;

	v = *(int *)safe_data;

	for (i = 0; i < nr_enums; i++) {
		if (v != enums[i].val)
			continue;

		btf_show_type_value(show, "%s",
				    __btf_name_by_offset(btf,
							 enums[i].name_off));

		btf_show_end_type(show);
		return;
	}

	if (btf_type_kflag(t))
		btf_show_type_value(show, "%d", v);
	else
		btf_show_type_value(show, "%u", v);
	btf_show_end_type(show);
}

static const struct btf_kind_operations enum_ops = {
	.check_meta = btf_enum_check_meta,
	.resolve = btf_df_resolve,
	.check_member = btf_enum_check_member,
	.check_kflag_member = btf_enum_check_kflag_member,
	.log_details = btf_enum_log,
	.show = btf_enum_show,
};

static s32 btf_enum64_check_meta(struct btf_verifier_env *env,
				 const struct btf_type *t,
				 u32 meta_left)
{
	const struct btf_enum64 *enums = btf_type_enum64(t);
	struct btf *btf = env->btf;
	const char *fmt_str;
	u16 i, nr_enums;
	u32 meta_needed;

	nr_enums = btf_type_vlen(t);
	meta_needed = nr_enums * sizeof(*enums);

	if (meta_left < meta_needed) {
		btf_verifier_log_basic(env, t,
				       "meta_left:%u meta_needed:%u",
				       meta_left, meta_needed);
		return -EINVAL;
	}

	if (t->size > 8 || !is_power_of_2(t->size)) {
		btf_verifier_log_type(env, t, "Unexpected size");
		return -EINVAL;
	}

	/* enum type either no name or a valid one */
	if (t->name_off &&
	    !btf_name_valid_identifier(env->btf, t->name_off)) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	for (i = 0; i < nr_enums; i++) {
		if (!btf_name_offset_valid(btf, enums[i].name_off)) {
			btf_verifier_log(env, "\tInvalid name_offset:%u",
					 enums[i].name_off);
			return -EINVAL;
		}

		/* enum member must have a valid name */
		if (!enums[i].name_off ||
		    !btf_name_valid_identifier(btf, enums[i].name_off)) {
			btf_verifier_log_type(env, t, "Invalid name");
			return -EINVAL;
		}

		if (env->log.level == BPF_LOG_KERNEL)
			continue;

		fmt_str = btf_type_kflag(t) ? "\t%s val=%lld\n" : "\t%s val=%llu\n";
		btf_verifier_log(env, fmt_str,
				 __btf_name_by_offset(btf, enums[i].name_off),
				 btf_enum64_value(enums + i));
	}

	return meta_needed;
}

static void btf_enum64_show(const struct btf *btf, const struct btf_type *t,
			    u32 type_id, void *data, u8 bits_offset,
			    struct btf_show *show)
{
	const struct btf_enum64 *enums = btf_type_enum64(t);
	u32 i, nr_enums = btf_type_vlen(t);
	void *safe_data;
	s64 v;

	safe_data = btf_show_start_type(show, t, type_id, data);
	if (!safe_data)
		return;

	v = *(u64 *)safe_data;

	for (i = 0; i < nr_enums; i++) {
		if (v != btf_enum64_value(enums + i))
			continue;

		btf_show_type_value(show, "%s",
				    __btf_name_by_offset(btf,
							 enums[i].name_off));

		btf_show_end_type(show);
		return;
	}

	if (btf_type_kflag(t))
		btf_show_type_value(show, "%lld", v);
	else
		btf_show_type_value(show, "%llu", v);
	btf_show_end_type(show);
}

static const struct btf_kind_operations enum64_ops = {
	.check_meta = btf_enum64_check_meta,
	.resolve = btf_df_resolve,
	.check_member = btf_enum_check_member,
	.check_kflag_member = btf_enum_check_kflag_member,
	.log_details = btf_enum_log,
	.show = btf_enum64_show,
};

static s32 btf_func_proto_check_meta(struct btf_verifier_env *env,
				     const struct btf_type *t,
				     u32 meta_left)
{
	u32 meta_needed = btf_type_vlen(t) * sizeof(struct btf_param);

	if (meta_left < meta_needed) {
		btf_verifier_log_basic(env, t,
				       "meta_left:%u meta_needed:%u",
				       meta_left, meta_needed);
		return -EINVAL;
	}