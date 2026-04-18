// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 42/55



			/* check type tag */
			t = btf_type_by_id(btf, mtype->type);
			if (btf_type_is_type_tag(t) && !btf_type_kflag(t)) {
				tag_value = __btf_name_by_offset(btf, t->name_off);
				/* check __user tag */
				if (strcmp(tag_value, "user") == 0)
					tmp_flag = MEM_USER;
				/* check __percpu tag */
				if (strcmp(tag_value, "percpu") == 0)
					tmp_flag = MEM_PERCPU;
				/* check __rcu tag */
				if (strcmp(tag_value, "rcu") == 0)
					tmp_flag = MEM_RCU;
			}

			stype = btf_type_skip_modifiers(btf, mtype->type, &id);
			if (btf_type_is_struct(stype)) {
				*next_btf_id = id;
				*flag |= tmp_flag;
				if (field_name)
					*field_name = mname;
				return WALK_PTR;
			}

			return WALK_PTR_UNTRUSTED;
		}

		/* Allow more flexible access within an int as long as
		 * it is within mtrue_end.
		 * Since mtrue_end could be the end of an array,
		 * that also allows using an array of int as a scratch
		 * space. e.g. skb->cb[].
		 */
		if (off + size > mtrue_end && !(*flag & PTR_UNTRUSTED)) {
			bpf_log(log,
				"access beyond the end of member %s (mend:%u) in struct %s with off %u size %u\n",
				mname, mtrue_end, tname, off, size);
			return -EACCES;
		}

		return WALK_SCALAR;
	}
	bpf_log(log, "struct %s doesn't have field at offset %d\n", tname, off);
	return -EINVAL;
}

int btf_struct_access(struct bpf_verifier_log *log,
		      const struct bpf_reg_state *reg,
		      int off, int size, enum bpf_access_type atype __maybe_unused,
		      u32 *next_btf_id, enum bpf_type_flag *flag,
		      const char **field_name)
{
	const struct btf *btf = reg->btf;
	enum bpf_type_flag tmp_flag = 0;
	const struct btf_type *t;
	u32 id = reg->btf_id;
	int err;

	while (type_is_alloc(reg->type)) {
		struct btf_struct_meta *meta;
		struct btf_record *rec;
		int i;

		meta = btf_find_struct_meta(btf, id);
		if (!meta)
			break;
		rec = meta->record;
		for (i = 0; i < rec->cnt; i++) {
			struct btf_field *field = &rec->fields[i];
			u32 offset = field->offset;
			if (off < offset + field->size && offset < off + size) {
				bpf_log(log,
					"direct access to %s is disallowed\n",
					btf_field_type_name(field->type));
				return -EACCES;
			}
		}
		break;
	}

	t = btf_type_by_id(btf, id);
	do {
		err = btf_struct_walk(log, btf, t, off, size, &id, &tmp_flag, field_name);

		switch (err) {
		case WALK_PTR:
			/* For local types, the destination register cannot
			 * become a pointer again.
			 */
			if (type_is_alloc(reg->type))
				return SCALAR_VALUE;
			/* If we found the pointer or scalar on t+off,
			 * we're done.
			 */
			*next_btf_id = id;
			*flag = tmp_flag;
			return PTR_TO_BTF_ID;
		case WALK_PTR_UNTRUSTED:
			*flag = MEM_RDONLY | PTR_UNTRUSTED;
			return PTR_TO_MEM;
		case WALK_SCALAR:
			return SCALAR_VALUE;
		case WALK_STRUCT:
			/* We found nested struct, so continue the search
			 * by diving in it. At this point the offset is
			 * aligned with the new type, so set it to 0.
			 */
			t = btf_type_by_id(btf, id);
			off = 0;
			break;
		default:
			/* It's either error or unknown return value..
			 * scream and leave.
			 */
			if (WARN_ONCE(err > 0, "unknown btf_struct_walk return value"))
				return -EINVAL;
			return err;
		}
	} while (t);

	return -EINVAL;
}

/* Check that two BTF types, each specified as an BTF object + id, are exactly
 * the same. Trivial ID check is not enough due to module BTFs, because we can
 * end up with two different module BTFs, but IDs point to the common type in
 * vmlinux BTF.
 */
bool btf_types_are_same(const struct btf *btf1, u32 id1,
			const struct btf *btf2, u32 id2)
{
	if (id1 != id2)
		return false;
	if (btf1 == btf2)
		return true;
	return btf_type_by_id(btf1, id1) == btf_type_by_id(btf2, id2);
}

bool btf_struct_ids_match(struct bpf_verifier_log *log,
			  const struct btf *btf, u32 id, int off,
			  const struct btf *need_btf, u32 need_type_id,
			  bool strict)
{
	const struct btf_type *type;
	enum bpf_type_flag flag = 0;
	int err;

	/* Are we already done? */
	if (off == 0 && btf_types_are_same(btf, id, need_btf, need_type_id))
		return true;
	/* In case of strict type match, we do not walk struct, the top level
	 * type match must succeed. When strict is true, off should have already
	 * been 0.
	 */
	if (strict)
		return false;
again:
	type = btf_type_by_id(btf, id);
	if (!type)
		return false;
	err = btf_struct_walk(log, btf, type, off, 1, &id, &flag, NULL);
	if (err != WALK_STRUCT)
		return false;

	/* We found nested struct object. If it matches
	 * the requested ID, we're done. Otherwise let's
	 * continue the search with offset 0 in the new
	 * type.
	 */
	if (!btf_types_are_same(btf, id, need_btf, need_type_id)) {
		off = 0;
		goto again;
	}

	return true;
}

static int __get_type_size(struct btf *btf, u32 btf_id,
			   const struct btf_type **ret_type)
{
	const struct btf_type *t;