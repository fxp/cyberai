// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 29/55



	if (!t->size) {
		btf_verifier_log_type(env, t, "size == 0");
		return -EINVAL;
	}

	if (btf_type_kflag(t)) {
		btf_verifier_log_type(env, t, "Invalid btf_info kind_flag");
		return -EINVAL;
	}

	if (!t->name_off ||
	    !btf_name_valid_section(env->btf, t->name_off)) {
		btf_verifier_log_type(env, t, "Invalid name");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	for_each_vsi(i, t, vsi) {
		/* A var cannot be in type void */
		if (!vsi->type || !BTF_TYPE_ID_VALID(vsi->type)) {
			btf_verifier_log_vsi(env, t, vsi,
					     "Invalid type_id");
			return -EINVAL;
		}

		if (vsi->offset < last_vsi_end_off || vsi->offset >= t->size) {
			btf_verifier_log_vsi(env, t, vsi,
					     "Invalid offset");
			return -EINVAL;
		}

		if (!vsi->size || vsi->size > t->size) {
			btf_verifier_log_vsi(env, t, vsi,
					     "Invalid size");
			return -EINVAL;
		}

		last_vsi_end_off = vsi->offset + vsi->size;
		if (last_vsi_end_off > t->size) {
			btf_verifier_log_vsi(env, t, vsi,
					     "Invalid offset+size");
			return -EINVAL;
		}

		btf_verifier_log_vsi(env, t, vsi, NULL);
		sum += vsi->size;
	}

	if (t->size < sum) {
		btf_verifier_log_type(env, t, "Invalid btf_info size");
		return -EINVAL;
	}

	return meta_needed;
}

static int btf_datasec_resolve(struct btf_verifier_env *env,
			       const struct resolve_vertex *v)
{
	const struct btf_var_secinfo *vsi;
	struct btf *btf = env->btf;
	u16 i;

	env->resolve_mode = RESOLVE_TBD;
	for_each_vsi_from(i, v->next_member, v->t, vsi) {
		u32 var_type_id = vsi->type, type_id, type_size = 0;
		const struct btf_type *var_type = btf_type_by_id(env->btf,
								 var_type_id);
		if (!var_type || !btf_type_is_var(var_type)) {
			btf_verifier_log_vsi(env, v->t, vsi,
					     "Not a VAR kind member");
			return -EINVAL;
		}

		if (!env_type_is_resolve_sink(env, var_type) &&
		    !env_type_is_resolved(env, var_type_id)) {
			env_stack_set_next_member(env, i + 1);
			return env_stack_push(env, var_type, var_type_id);
		}

		type_id = var_type->type;
		if (!btf_type_id_size(btf, &type_id, &type_size)) {
			btf_verifier_log_vsi(env, v->t, vsi, "Invalid type");
			return -EINVAL;
		}

		if (vsi->size < type_size) {
			btf_verifier_log_vsi(env, v->t, vsi, "Invalid size");
			return -EINVAL;
		}
	}

	env_stack_pop_resolved(env, 0, 0);
	return 0;
}

static void btf_datasec_log(struct btf_verifier_env *env,
			    const struct btf_type *t)
{
	btf_verifier_log(env, "size=%u vlen=%u", t->size, btf_type_vlen(t));
}

static void btf_datasec_show(const struct btf *btf,
			     const struct btf_type *t, u32 type_id,
			     void *data, u8 bits_offset,
			     struct btf_show *show)
{
	const struct btf_var_secinfo *vsi;
	const struct btf_type *var;
	u32 i;

	if (!btf_show_start_type(show, t, type_id, data))
		return;

	btf_show_type_value(show, "section (\"%s\") = {",
			    __btf_name_by_offset(btf, t->name_off));
	for_each_vsi(i, t, vsi) {
		var = btf_type_by_id(btf, vsi->type);
		if (i)
			btf_show(show, ",");
		btf_type_ops(var)->show(btf, var, vsi->type,
					data + vsi->offset, bits_offset, show);
	}
	btf_show_end_type(show);
}

static const struct btf_kind_operations datasec_ops = {
	.check_meta		= btf_datasec_check_meta,
	.resolve		= btf_datasec_resolve,
	.check_member		= btf_df_check_member,
	.check_kflag_member	= btf_df_check_kflag_member,
	.log_details		= btf_datasec_log,
	.show			= btf_datasec_show,
};

static s32 btf_float_check_meta(struct btf_verifier_env *env,
				const struct btf_type *t,
				u32 meta_left)
{
	if (btf_type_vlen(t)) {
		btf_verifier_log_type(env, t, "vlen != 0");
		return -EINVAL;
	}

	if (btf_type_kflag(t)) {
		btf_verifier_log_type(env, t, "Invalid btf_info kind_flag");
		return -EINVAL;
	}

	if (t->size != 2 && t->size != 4 && t->size != 8 && t->size != 12 &&
	    t->size != 16) {
		btf_verifier_log_type(env, t, "Invalid type_size");
		return -EINVAL;
	}

	btf_verifier_log_type(env, t, NULL);

	return 0;
}

static int btf_float_check_member(struct btf_verifier_env *env,
				  const struct btf_type *struct_type,
				  const struct btf_member *member,
				  const struct btf_type *member_type)
{
	u64 start_offset_bytes;
	u64 end_offset_bytes;
	u64 misalign_bits;
	u64 align_bytes;
	u64 align_bits;

	/* Different architectures have different alignment requirements, so
	 * here we check only for the reasonable minimum. This way we ensure
	 * that types after CO-RE can pass the kernel BTF verifier.
	 */
	align_bytes = min_t(u64, sizeof(void *), member_type->size);
	align_bits = align_bytes * BITS_PER_BYTE;
	div64_u64_rem(member->offset, align_bits, &misalign_bits);
	if (misalign_bits) {
		btf_verifier_log_member(env, struct_type, member,
					"Member is not properly aligned");
		return -EINVAL;
	}