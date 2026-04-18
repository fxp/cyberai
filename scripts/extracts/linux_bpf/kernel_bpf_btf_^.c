// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 30/55



	start_offset_bytes = member->offset / BITS_PER_BYTE;
	end_offset_bytes = start_offset_bytes + member_type->size;
	if (end_offset_bytes > struct_type->size) {
		btf_verifier_log_member(env, struct_type, member,
					"Member exceeds struct_size");
		return -EINVAL;
	}

	return 0;
}

static void btf_float_log(struct btf_verifier_env *env,
			  const struct btf_type *t)
{
	btf_verifier_log(env, "size=%u", t->size);
}

static const struct btf_kind_operations float_ops = {
	.check_meta = btf_float_check_meta,
	.resolve = btf_df_resolve,
	.check_member = btf_float_check_member,
	.check_kflag_member = btf_generic_check_kflag_member,
	.log_details = btf_float_log,
	.show = btf_df_show,
};

static s32 btf_decl_tag_check_meta(struct btf_verifier_env *env,
			      const struct btf_type *t,
			      u32 meta_left)
{
	const struct btf_decl_tag *tag;
	u32 meta_needed = sizeof(*tag);
	s32 component_idx;
	const char *value;

	if (meta_left < meta_needed) {
		btf_verifier_log_basic(env, t,
				       "meta_left:%u meta_needed:%u",
				       meta_left, meta_needed);
		return -EINVAL;
	}

	value = btf_name_by_offset(env->btf, t->name_off);
	if (!value || !value[0]) {
		btf_verifier_log_type(env, t, "Invalid value");
		return -EINVAL;
	}

	if (btf_type_vlen(t)) {
		btf_verifier_log_type(env, t, "vlen != 0");
		return -EINVAL;
	}

	component_idx = btf_type_decl_tag(t)->component_idx;
	if (component_idx < -1) {
		btf_verifier_log_type(env, t, "Invalid component_idx");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	return meta_needed;
}

static int btf_decl_tag_resolve(struct btf_verifier_env *env,
			   const struct resolve_vertex *v)
{
	const struct btf_type *next_type;
	const struct btf_type *t = v->t;
	u32 next_type_id = t->type;
	struct btf *btf = env->btf;
	s32 component_idx;
	u32 vlen;

	next_type = btf_type_by_id(btf, next_type_id);
	if (!next_type || !btf_type_is_decl_tag_target(next_type)) {
		btf_verifier_log_type(env, v->t, "Invalid type_id");
		return -EINVAL;
	}

	if (!env_type_is_resolve_sink(env, next_type) &&
	    !env_type_is_resolved(env, next_type_id))
		return env_stack_push(env, next_type, next_type_id);

	component_idx = btf_type_decl_tag(t)->component_idx;
	if (component_idx != -1) {
		if (btf_type_is_var(next_type) || btf_type_is_typedef(next_type)) {
			btf_verifier_log_type(env, v->t, "Invalid component_idx");
			return -EINVAL;
		}

		if (btf_type_is_struct(next_type)) {
			vlen = btf_type_vlen(next_type);
		} else {
			/* next_type should be a function */
			next_type = btf_type_by_id(btf, next_type->type);
			vlen = btf_type_vlen(next_type);
		}

		if ((u32)component_idx >= vlen) {
			btf_verifier_log_type(env, v->t, "Invalid component_idx");
			return -EINVAL;
		}
	}

	env_stack_pop_resolved(env, next_type_id, 0);

	return 0;
}

static void btf_decl_tag_log(struct btf_verifier_env *env, const struct btf_type *t)
{
	btf_verifier_log(env, "type=%u component_idx=%d", t->type,
			 btf_type_decl_tag(t)->component_idx);
}

static const struct btf_kind_operations decl_tag_ops = {
	.check_meta = btf_decl_tag_check_meta,
	.resolve = btf_decl_tag_resolve,
	.check_member = btf_df_check_member,
	.check_kflag_member = btf_df_check_kflag_member,
	.log_details = btf_decl_tag_log,
	.show = btf_df_show,
};

static int btf_func_proto_check(struct btf_verifier_env *env,
				const struct btf_type *t)
{
	const struct btf_type *ret_type;
	const struct btf_param *args;
	const struct btf *btf;
	u16 nr_args, i;
	int err;

	btf = env->btf;
	args = (const struct btf_param *)(t + 1);
	nr_args = btf_type_vlen(t);

	/* Check func return type which could be "void" (t->type == 0) */
	if (t->type) {
		u32 ret_type_id = t->type;

		ret_type = btf_type_by_id(btf, ret_type_id);
		if (!ret_type) {
			btf_verifier_log_type(env, t, "Invalid return type");
			return -EINVAL;
		}

		if (btf_type_is_resolve_source_only(ret_type)) {
			btf_verifier_log_type(env, t, "Invalid return type");
			return -EINVAL;
		}

		if (btf_type_needs_resolve(ret_type) &&
		    !env_type_is_resolved(env, ret_type_id)) {
			err = btf_resolve(env, ret_type, ret_type_id);
			if (err)
				return err;
		}

		/* Ensure the return type is a type that has a size */
		if (!btf_type_id_size(btf, &ret_type_id, NULL)) {
			btf_verifier_log_type(env, t, "Invalid return type");
			return -EINVAL;
		}
	}

	if (!nr_args)
		return 0;

	/* Last func arg type_id could be 0 if it is a vararg */
	if (!args[nr_args - 1].type) {
		if (args[nr_args - 1].name_off) {
			btf_verifier_log_type(env, t, "Invalid arg#%u",
					      nr_args);
			return -EINVAL;
		}
		nr_args--;
	}

	for (i = 0; i < nr_args; i++) {
		const struct btf_type *arg_type;
		u32 arg_type_id;

		arg_type_id = args[i].type;
		arg_type = btf_type_by_id(btf, arg_type_id);
		if (!arg_type) {
			btf_verifier_log_type(env, t, "Invalid arg#%u", i + 1);
			return -EINVAL;
		}