// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 31/55



		if (btf_type_is_resolve_source_only(arg_type)) {
			btf_verifier_log_type(env, t, "Invalid arg#%u", i + 1);
			return -EINVAL;
		}

		if (args[i].name_off &&
		    (!btf_name_offset_valid(btf, args[i].name_off) ||
		     !btf_name_valid_identifier(btf, args[i].name_off))) {
			btf_verifier_log_type(env, t,
					      "Invalid arg#%u", i + 1);
			return -EINVAL;
		}

		if (btf_type_needs_resolve(arg_type) &&
		    !env_type_is_resolved(env, arg_type_id)) {
			err = btf_resolve(env, arg_type, arg_type_id);
			if (err)
				return err;
		}

		if (!btf_type_id_size(btf, &arg_type_id, NULL)) {
			btf_verifier_log_type(env, t, "Invalid arg#%u", i + 1);
			return -EINVAL;
		}
	}

	return 0;
}

static int btf_func_check(struct btf_verifier_env *env,
			  const struct btf_type *t)
{
	const struct btf_type *proto_type;
	const struct btf_param *args;
	const struct btf *btf;
	u16 nr_args, i;

	btf = env->btf;
	proto_type = btf_type_by_id(btf, t->type);

	if (!proto_type || !btf_type_is_func_proto(proto_type)) {
		btf_verifier_log_type(env, t, "Invalid type_id");
		return -EINVAL;
	}

	args = (const struct btf_param *)(proto_type + 1);
	nr_args = btf_type_vlen(proto_type);
	for (i = 0; i < nr_args; i++) {
		if (!args[i].name_off && args[i].type) {
			btf_verifier_log_type(env, t, "Invalid arg#%u", i + 1);
			return -EINVAL;
		}
	}

	return 0;
}

static const struct btf_kind_operations * const kind_ops[NR_BTF_KINDS] = {
	[BTF_KIND_INT] = &int_ops,
	[BTF_KIND_PTR] = &ptr_ops,
	[BTF_KIND_ARRAY] = &array_ops,
	[BTF_KIND_STRUCT] = &struct_ops,
	[BTF_KIND_UNION] = &struct_ops,
	[BTF_KIND_ENUM] = &enum_ops,
	[BTF_KIND_FWD] = &fwd_ops,
	[BTF_KIND_TYPEDEF] = &modifier_ops,
	[BTF_KIND_VOLATILE] = &modifier_ops,
	[BTF_KIND_CONST] = &modifier_ops,
	[BTF_KIND_RESTRICT] = &modifier_ops,
	[BTF_KIND_FUNC] = &func_ops,
	[BTF_KIND_FUNC_PROTO] = &func_proto_ops,
	[BTF_KIND_VAR] = &var_ops,
	[BTF_KIND_DATASEC] = &datasec_ops,
	[BTF_KIND_FLOAT] = &float_ops,
	[BTF_KIND_DECL_TAG] = &decl_tag_ops,
	[BTF_KIND_TYPE_TAG] = &modifier_ops,
	[BTF_KIND_ENUM64] = &enum64_ops,
};

static s32 btf_check_meta(struct btf_verifier_env *env,
			  const struct btf_type *t,
			  u32 meta_left)
{
	u32 saved_meta_left = meta_left;
	s32 var_meta_size;

	if (meta_left < sizeof(*t)) {
		btf_verifier_log(env, "[%u] meta_left:%u meta_needed:%zu",
				 env->log_type_id, meta_left, sizeof(*t));
		return -EINVAL;
	}
	meta_left -= sizeof(*t);

	if (t->info & ~BTF_INFO_MASK) {
		btf_verifier_log(env, "[%u] Invalid btf_info:%x",
				 env->log_type_id, t->info);
		return -EINVAL;
	}

	if (BTF_INFO_KIND(t->info) > BTF_KIND_MAX ||
	    BTF_INFO_KIND(t->info) == BTF_KIND_UNKN) {
		btf_verifier_log(env, "[%u] Invalid kind:%u",
				 env->log_type_id, BTF_INFO_KIND(t->info));
		return -EINVAL;
	}

	if (!btf_name_offset_valid(env->btf, t->name_off)) {
		btf_verifier_log(env, "[%u] Invalid name_offset:%u",
				 env->log_type_id, t->name_off);
		return -EINVAL;
	}

	var_meta_size = btf_type_ops(t)->check_meta(env, t, meta_left);
	if (var_meta_size < 0)
		return var_meta_size;

	meta_left -= var_meta_size;

	return saved_meta_left - meta_left;
}

static int btf_check_all_metas(struct btf_verifier_env *env)
{
	struct btf *btf = env->btf;
	struct btf_header *hdr;
	void *cur, *end;

	hdr = &btf->hdr;
	cur = btf->nohdr_data + hdr->type_off;
	end = cur + hdr->type_len;

	env->log_type_id = btf->base_btf ? btf->start_id : 1;
	while (cur < end) {
		struct btf_type *t = cur;
		s32 meta_size;

		meta_size = btf_check_meta(env, t, end - cur);
		if (meta_size < 0)
			return meta_size;

		btf_add_type(env, t);
		cur += meta_size;
		env->log_type_id++;
	}

	return 0;
}

static bool btf_resolve_valid(struct btf_verifier_env *env,
			      const struct btf_type *t,
			      u32 type_id)
{
	struct btf *btf = env->btf;

	if (!env_type_is_resolved(env, type_id))
		return false;

	if (btf_type_is_struct(t) || btf_type_is_datasec(t))
		return !btf_resolved_type_id(btf, type_id) &&
		       !btf_resolved_type_size(btf, type_id);

	if (btf_type_is_decl_tag(t) || btf_type_is_func(t))
		return btf_resolved_type_id(btf, type_id) &&
		       !btf_resolved_type_size(btf, type_id);

	if (btf_type_is_modifier(t) || btf_type_is_ptr(t) ||
	    btf_type_is_var(t)) {
		t = btf_type_id_resolve(btf, &type_id);
		return t &&
		       !btf_type_is_modifier(t) &&
		       !btf_type_is_var(t) &&
		       !btf_type_is_datasec(t);
	}

	if (btf_type_is_array(t)) {
		const struct btf_array *array = btf_type_array(t);
		const struct btf_type *elem_type;
		u32 elem_type_id = array->type;
		u32 elem_size;

		elem_type = btf_type_id_size(btf, &elem_type_id, &elem_size);
		return elem_type && !btf_type_is_modifier(elem_type) &&
			(array->nelems * elem_size ==
			 btf_resolved_type_size(btf, type_id));
	}

	return false;
}

static int btf_resolve(struct btf_verifier_env *env,
		       const struct btf_type *t, u32 type_id)
{
	u32 save_log_type_id = env->log_type_id;
	const struct resolve_vertex *v;
	int err = 0;