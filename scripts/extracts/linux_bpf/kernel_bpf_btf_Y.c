// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 25/55



		switch (info_arr[i].type) {
		case BPF_SPIN_LOCK:
			WARN_ON_ONCE(rec->spin_lock_off >= 0);
			/* Cache offset for faster lookup at runtime */
			rec->spin_lock_off = rec->fields[i].offset;
			break;
		case BPF_RES_SPIN_LOCK:
			WARN_ON_ONCE(rec->spin_lock_off >= 0);
			/* Cache offset for faster lookup at runtime */
			rec->res_spin_lock_off = rec->fields[i].offset;
			break;
		case BPF_TIMER:
			WARN_ON_ONCE(rec->timer_off >= 0);
			/* Cache offset for faster lookup at runtime */
			rec->timer_off = rec->fields[i].offset;
			break;
		case BPF_WORKQUEUE:
			WARN_ON_ONCE(rec->wq_off >= 0);
			/* Cache offset for faster lookup at runtime */
			rec->wq_off = rec->fields[i].offset;
			break;
		case BPF_TASK_WORK:
			WARN_ON_ONCE(rec->task_work_off >= 0);
			rec->task_work_off = rec->fields[i].offset;
			break;
		case BPF_REFCOUNT:
			WARN_ON_ONCE(rec->refcount_off >= 0);
			/* Cache offset for faster lookup at runtime */
			rec->refcount_off = rec->fields[i].offset;
			break;
		case BPF_KPTR_UNREF:
		case BPF_KPTR_REF:
		case BPF_KPTR_PERCPU:
		case BPF_UPTR:
			ret = btf_parse_kptr(btf, &rec->fields[i], &info_arr[i]);
			if (ret < 0)
				goto end;
			break;
		case BPF_LIST_HEAD:
			ret = btf_parse_list_head(btf, &rec->fields[i], &info_arr[i]);
			if (ret < 0)
				goto end;
			break;
		case BPF_RB_ROOT:
			ret = btf_parse_rb_root(btf, &rec->fields[i], &info_arr[i]);
			if (ret < 0)
				goto end;
			break;
		case BPF_LIST_NODE:
		case BPF_RB_NODE:
			break;
		default:
			ret = -EFAULT;
			goto end;
		}
		rec->cnt++;
	}

	if (rec->spin_lock_off >= 0 && rec->res_spin_lock_off >= 0) {
		ret = -EINVAL;
		goto end;
	}

	/* bpf_{list_head, rb_node} require bpf_spin_lock */
	if ((btf_record_has_field(rec, BPF_LIST_HEAD) ||
	     btf_record_has_field(rec, BPF_RB_ROOT)) &&
		 (rec->spin_lock_off < 0 && rec->res_spin_lock_off < 0)) {
		ret = -EINVAL;
		goto end;
	}

	if (rec->refcount_off < 0 &&
	    btf_record_has_field(rec, BPF_LIST_NODE) &&
	    btf_record_has_field(rec, BPF_RB_NODE)) {
		ret = -EINVAL;
		goto end;
	}

	sort_r(rec->fields, rec->cnt, sizeof(struct btf_field), btf_field_cmp,
	       NULL, rec);

	return rec;
end:
	btf_record_free(rec);
	return ERR_PTR(ret);
}

int btf_check_and_fixup_fields(const struct btf *btf, struct btf_record *rec)
{
	int i;

	/* There are three types that signify ownership of some other type:
	 *  kptr_ref, bpf_list_head, bpf_rb_root.
	 * kptr_ref only supports storing kernel types, which can't store
	 * references to program allocated local types.
	 *
	 * Hence we only need to ensure that bpf_{list_head,rb_root} ownership
	 * does not form cycles.
	 */
	if (IS_ERR_OR_NULL(rec) || !(rec->field_mask & (BPF_GRAPH_ROOT | BPF_UPTR)))
		return 0;
	for (i = 0; i < rec->cnt; i++) {
		struct btf_struct_meta *meta;
		const struct btf_type *t;
		u32 btf_id;

		if (rec->fields[i].type == BPF_UPTR) {
			/* The uptr only supports pinning one page and cannot
			 * point to a kernel struct
			 */
			if (btf_is_kernel(rec->fields[i].kptr.btf))
				return -EINVAL;
			t = btf_type_by_id(rec->fields[i].kptr.btf,
					   rec->fields[i].kptr.btf_id);
			if (!t->size)
				return -EINVAL;
			if (t->size > PAGE_SIZE)
				return -E2BIG;
			continue;
		}

		if (!(rec->fields[i].type & BPF_GRAPH_ROOT))
			continue;
		btf_id = rec->fields[i].graph_root.value_btf_id;
		meta = btf_find_struct_meta(btf, btf_id);
		if (!meta)
			return -EFAULT;
		rec->fields[i].graph_root.value_rec = meta->record;

		/* We need to set value_rec for all root types, but no need
		 * to check ownership cycle for a type unless it's also a
		 * node type.
		 */
		if (!(rec->field_mask & BPF_GRAPH_NODE))
			continue;

		/* We need to ensure ownership acyclicity among all types. The
		 * proper way to do it would be to topologically sort all BTF
		 * IDs based on the ownership edges, since there can be multiple
		 * bpf_{list_head,rb_node} in a type. Instead, we use the
		 * following resaoning:
		 *
		 * - A type can only be owned by another type in user BTF if it
		 *   has a bpf_{list,rb}_node. Let's call these node types.
		 * - A type can only _own_ another type in user BTF if it has a
		 *   bpf_{list_head,rb_root}. Let's call these root types.
		 *
		 * We ensure that if a type is both a root and node, its
		 * element types cannot be root types.
		 *
		 * To ensure acyclicity:
		 *
		 * When A is an root type but not a node, its ownership
		 * chain can be:
		 *	A -> B -> C
		 * Where:
		 * - A is an root, e.g. has bpf_rb_root.
		 * - B is both a root and node, e.g. has bpf_rb_node and
		 *   bpf_list_head.
		 * - C is only an root, e.g. has bpf_list_node
		 *
		 * When A is both a root and node, some other type already
		 * owns it in the BTF domain, hence it can not own
		 * another root type through any of the ownership edges.
		 *	A -> B
		 * Where:
		 * - A is both an root and node.
		 * - B is only an node.
		 */
		if (meta->record->field_mask & BPF_GRAPH_ROOT)
			return -ELOOP;
	}
	return 0;
}