// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 28/55



	if (t->name_off) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	if (btf_type_kflag(t)) {
		btf_verifier_log_type(env, t, "Invalid btf_info kind_flag");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	return meta_needed;
}

static void btf_func_proto_log(struct btf_verifier_env *env,
			       const struct btf_type *t)
{
	const struct btf_param *args = (const struct btf_param *)(t + 1);
	u16 nr_args = btf_type_vlen(t), i;

	btf_verifier_log(env, "return=%u args=(", t->type);
	if (!nr_args) {
		btf_verifier_log(env, "void");
		goto done;
	}

	if (nr_args == 1 && !args[0].type) {
		/* Only one vararg */
		btf_verifier_log(env, "vararg");
		goto done;
	}

	btf_verifier_log(env, "%u %s", args[0].type,
			 __btf_name_by_offset(env->btf,
					      args[0].name_off));
	for (i = 1; i < nr_args - 1; i++)
		btf_verifier_log(env, ", %u %s", args[i].type,
				 __btf_name_by_offset(env->btf,
						      args[i].name_off));

	if (nr_args > 1) {
		const struct btf_param *last_arg = &args[nr_args - 1];

		if (last_arg->type)
			btf_verifier_log(env, ", %u %s", last_arg->type,
					 __btf_name_by_offset(env->btf,
							      last_arg->name_off));
		else
			btf_verifier_log(env, ", vararg");
	}

done:
	btf_verifier_log(env, ")");
}

static const struct btf_kind_operations func_proto_ops = {
	.check_meta = btf_func_proto_check_meta,
	.resolve = btf_df_resolve,
	/*
	 * BTF_KIND_FUNC_PROTO cannot be directly referred by
	 * a struct's member.
	 *
	 * It should be a function pointer instead.
	 * (i.e. struct's member -> BTF_KIND_PTR -> BTF_KIND_FUNC_PROTO)
	 *
	 * Hence, there is no btf_func_check_member().
	 */
	.check_member = btf_df_check_member,
	.check_kflag_member = btf_df_check_kflag_member,
	.log_details = btf_func_proto_log,
	.show = btf_df_show,
};

static s32 btf_func_check_meta(struct btf_verifier_env *env,
			       const struct btf_type *t,
			       u32 meta_left)
{
	if (!t->name_off ||
	    !btf_name_valid_identifier(env->btf, t->name_off)) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	if (btf_type_vlen(t) > BTF_FUNC_GLOBAL) {
		btf_verifier_log_type(env, t, "Invalid func linkage");
		return -EINVAL;
	}

	if (btf_type_kflag(t)) {
		btf_verifier_log_type(env, t, "Invalid btf_info kind_flag");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	return 0;
}

static int btf_func_resolve(struct btf_verifier_env *env,
			    const struct resolve_vertex *v)
{
	const struct btf_type *t = v->t;
	u32 next_type_id = t->type;
	int err;

	err = btf_func_check(env, t);
	if (err)
		return err;

	env_stack_pop_resolved(env, next_type_id, 0);
	return 0;
}

static const struct btf_kind_operations func_ops = {
	.check_meta = btf_func_check_meta,
	.resolve = btf_func_resolve,
	.check_member = btf_df_check_member,
	.check_kflag_member = btf_df_check_kflag_member,
	.log_details = btf_ref_type_log,
	.show = btf_df_show,
};

static s32 btf_var_check_meta(struct btf_verifier_env *env,
			      const struct btf_type *t,
			      u32 meta_left)
{
	const struct btf_var *var;
	u32 meta_needed = sizeof(*var);

	if (meta_left < meta_needed) {
		btf_verifier_log_basic(env, t,
				       "meta_left:%u meta_needed:%u",
				       meta_left, meta_needed);
		return -EINVAL;
	}

	if (btf_type_vlen(t)) {
		btf_verifier_log_type(env, t, "vlen != 0");
		return -EINVAL;
	}

	if (btf_type_kflag(t)) {
		btf_verifier_log_type(env, t, "Invalid btf_info kind_flag");
		return -EINVAL;
	}

	if (!t->name_off ||
	    !btf_name_valid_identifier(env->btf, t->name_off)) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	/* A var cannot be in type void */
	if (!t->type || !BTF_TYPE_ID_VALID(t->type)) {
		btf_verifier_log_type(env, t, "Invalid type_id");
		return -EINVAL;
	}

	var = btf_type_var(t);
	if (var->linkage != BTF_VAR_STATIC &&
	    var->linkage != BTF_VAR_GLOBAL_ALLOCATED) {
		btf_verifier_log_type(env, t, "Linkage not supported");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	return meta_needed;
}

static void btf_var_log(struct btf_verifier_env *env, const struct btf_type *t)
{
	const struct btf_var *var = btf_type_var(t);

	btf_verifier_log(env, "type_id=%u linkage=%u", t->type, var->linkage);
}

static const struct btf_kind_operations var_ops = {
	.check_meta		= btf_var_check_meta,
	.resolve		= btf_var_resolve,
	.check_member		= btf_df_check_member,
	.check_kflag_member	= btf_df_check_kflag_member,
	.log_details		= btf_var_log,
	.show			= btf_var_show,
};

static s32 btf_datasec_check_meta(struct btf_verifier_env *env,
				  const struct btf_type *t,
				  u32 meta_left)
{
	const struct btf_var_secinfo *vsi;
	u64 last_vsi_end_off = 0, sum = 0;
	u32 i, meta_needed;

	meta_needed = btf_type_vlen(t) * sizeof(*vsi);
	if (meta_left < meta_needed) {
		btf_verifier_log_basic(env, t,
				       "meta_left:%u meta_needed:%u",
				       meta_left, meta_needed);
		return -EINVAL;
	}