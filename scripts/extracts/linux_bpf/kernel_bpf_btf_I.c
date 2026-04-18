// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 41/55



	vlen = btf_type_vlen(t);
	if (BTF_INFO_KIND(t->info) == BTF_KIND_UNION && vlen != 1 && !(*flag & PTR_UNTRUSTED))
		/*
		 * walking unions yields untrusted pointers
		 * with exception of __bpf_md_ptr and other
		 * unions with a single member
		 */
		*flag |= PTR_UNTRUSTED;

	if (off + size > t->size) {
		/* If the last element is a variable size array, we may
		 * need to relax the rule.
		 */
		struct btf_array *array_elem;

		if (vlen == 0)
			goto error;

		member = btf_type_member(t) + vlen - 1;
		mtype = btf_type_skip_modifiers(btf, member->type,
						NULL);
		if (!btf_type_is_array(mtype))
			goto error;

		array_elem = (struct btf_array *)(mtype + 1);
		if (array_elem->nelems != 0)
			goto error;

		moff = __btf_member_bit_offset(t, member) / 8;
		if (off < moff)
			goto error;

		/* allow structure and integer */
		t = btf_type_skip_modifiers(btf, array_elem->type,
					    NULL);

		if (btf_type_is_int(t))
			return WALK_SCALAR;

		if (!btf_type_is_struct(t))
			goto error;

		off = (off - moff) % t->size;
		goto again;

error:
		bpf_log(log, "access beyond struct %s at off %u size %u\n",
			tname, off, size);
		return -EACCES;
	}

	for_each_member(i, t, member) {
		/* offset of the field in bytes */
		moff = __btf_member_bit_offset(t, member) / 8;
		if (off + size <= moff)
			/* won't find anything, field is already too far */
			break;

		if (__btf_member_bitfield_size(t, member)) {
			u32 end_bit = __btf_member_bit_offset(t, member) +
				__btf_member_bitfield_size(t, member);

			/* off <= moff instead of off == moff because clang
			 * does not generate a BTF member for anonymous
			 * bitfield like the ":16" here:
			 * struct {
			 *	int :16;
			 *	int x:8;
			 * };
			 */
			if (off <= moff &&
			    BITS_ROUNDUP_BYTES(end_bit) <= off + size)
				return WALK_SCALAR;

			/* off may be accessing a following member
			 *
			 * or
			 *
			 * Doing partial access at either end of this
			 * bitfield.  Continue on this case also to
			 * treat it as not accessing this bitfield
			 * and eventually error out as field not
			 * found to keep it simple.
			 * It could be relaxed if there was a legit
			 * partial access case later.
			 */
			continue;
		}

		/* In case of "off" is pointing to holes of a struct */
		if (off < moff)
			break;

		/* type of the field */
		mid = member->type;
		mtype = btf_type_by_id(btf, member->type);
		mname = __btf_name_by_offset(btf, member->name_off);

		mtype = __btf_resolve_size(btf, mtype, &msize,
					   &elem_type, &elem_id, &total_nelems,
					   &mid);
		if (IS_ERR(mtype)) {
			bpf_log(log, "field %s doesn't have size\n", mname);
			return -EFAULT;
		}

		mtrue_end = moff + msize;
		if (off >= mtrue_end)
			/* no overlap with member, keep iterating */
			continue;

		if (btf_type_is_array(mtype)) {
			u32 elem_idx;

			/* __btf_resolve_size() above helps to
			 * linearize a multi-dimensional array.
			 *
			 * The logic here is treating an array
			 * in a struct as the following way:
			 *
			 * struct outer {
			 *	struct inner array[2][2];
			 * };
			 *
			 * looks like:
			 *
			 * struct outer {
			 *	struct inner array_elem0;
			 *	struct inner array_elem1;
			 *	struct inner array_elem2;
			 *	struct inner array_elem3;
			 * };
			 *
			 * When accessing outer->array[1][0], it moves
			 * moff to "array_elem2", set mtype to
			 * "struct inner", and msize also becomes
			 * sizeof(struct inner).  Then most of the
			 * remaining logic will fall through without
			 * caring the current member is an array or
			 * not.
			 *
			 * Unlike mtype/msize/moff, mtrue_end does not
			 * change.  The naming difference ("_true") tells
			 * that it is not always corresponding to
			 * the current mtype/msize/moff.
			 * It is the true end of the current
			 * member (i.e. array in this case).  That
			 * will allow an int array to be accessed like
			 * a scratch space,
			 * i.e. allow access beyond the size of
			 *      the array's element as long as it is
			 *      within the mtrue_end boundary.
			 */

			/* skip empty array */
			if (moff == mtrue_end)
				continue;

			msize /= total_nelems;
			elem_idx = (off - moff) / msize;
			moff += elem_idx * msize;
			mtype = elem_type;
			mid = elem_id;
		}

		/* the 'off' we're looking for is either equal to start
		 * of this field or inside of this struct
		 */
		if (btf_type_is_struct(mtype)) {
			/* our field must be inside that union or struct */
			t = mtype;

			/* return if the offset matches the member offset */
			if (off == moff) {
				*next_btf_id = mid;
				return WALK_STRUCT;
			}

			/* adjust offset we're looking for */
			off -= moff;
			goto again;
		}

		if (btf_type_is_ptr(mtype)) {
			const struct btf_type *stype, *t;
			enum bpf_type_flag tmp_flag = 0;
			u32 id;

			if (msize != size || off != moff) {
				bpf_log(log,
					"cannot access ptr member %s with moff %u in struct %s with off %u size %u\n",
					mname, moff, tname, off, size);
				return -EACCES;
			}