// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 44/55



		/* global functions are validated with scalars and pointers
		 * to context only. And only global functions can be replaced.
		 * Hence type check only those types.
		 */
		if (btf_type_is_int(t1) || btf_is_any_enum(t1))
			continue;
		if (!btf_type_is_ptr(t1)) {
			bpf_log(log,
				"arg%d in %s() has unrecognized type\n",
				i, fn1);
			return -EINVAL;
		}
		t1 = btf_type_skip_modifiers(btf1, t1->type, NULL);
		t2 = btf_type_skip_modifiers(btf2, t2->type, NULL);
		if (!btf_type_is_struct(t1)) {
			bpf_log(log,
				"arg%d in %s() is not a pointer to context\n",
				i, fn1);
			return -EINVAL;
		}
		if (!btf_type_is_struct(t2)) {
			bpf_log(log,
				"arg%d in %s() is not a pointer to context\n",
				i, fn2);
			return -EINVAL;
		}
		/* This is an optional check to make program writing easier.
		 * Compare names of structs and report an error to the user.
		 * btf_prepare_func_args() already checked that t2 struct
		 * is a context type. btf_prepare_func_args() will check
		 * later that t1 struct is a context type as well.
		 */
		s1 = btf_name_by_offset(btf1, t1->name_off);
		s2 = btf_name_by_offset(btf2, t2->name_off);
		if (strcmp(s1, s2)) {
			bpf_log(log,
				"arg%d %s(struct %s *) doesn't match %s(struct %s *)\n",
				i, fn1, s1, fn2, s2);
			return -EINVAL;
		}
	}
	return 0;
}

/* Compare BTFs of given program with BTF of target program */
int btf_check_type_match(struct bpf_verifier_log *log, const struct bpf_prog *prog,
			 struct btf *btf2, const struct btf_type *t2)
{
	struct btf *btf1 = prog->aux->btf;
	const struct btf_type *t1;
	u32 btf_id = 0;

	if (!prog->aux->func_info) {
		bpf_log(log, "Program extension requires BTF\n");
		return -EINVAL;
	}

	btf_id = prog->aux->func_info[0].type_id;
	if (!btf_id)
		return -EFAULT;

	t1 = btf_type_by_id(btf1, btf_id);
	if (!t1 || !btf_type_is_func(t1))
		return -EFAULT;

	return btf_check_func_type_match(log, btf1, t1, btf2, t2);
}

static bool btf_is_dynptr_ptr(const struct btf *btf, const struct btf_type *t)
{
	const char *name;

	t = btf_type_by_id(btf, t->type); /* skip PTR */

	while (btf_type_is_modifier(t))
		t = btf_type_by_id(btf, t->type);

	/* allow either struct or struct forward declaration */
	if (btf_type_is_struct(t) ||
	    (btf_type_is_fwd(t) && btf_type_kflag(t) == 0)) {
		name = btf_str_by_offset(btf, t->name_off);
		return name && strcmp(name, "bpf_dynptr") == 0;
	}

	return false;
}

struct bpf_cand_cache {
	const char *name;
	u32 name_len;
	u16 kind;
	u16 cnt;
	struct {
		const struct btf *btf;
		u32 id;
	} cands[];
};

static DEFINE_MUTEX(cand_cache_mutex);

static struct bpf_cand_cache *
bpf_core_find_cands(struct bpf_core_ctx *ctx, u32 local_type_id);

static int btf_get_ptr_to_btf_id(struct bpf_verifier_log *log, int arg_idx,
				 const struct btf *btf, const struct btf_type *t)
{
	struct bpf_cand_cache *cc;
	struct bpf_core_ctx ctx = {
		.btf = btf,
		.log = log,
	};
	u32 kern_type_id, type_id;
	int err = 0;

	/* skip PTR and modifiers */
	type_id = t->type;
	t = btf_type_by_id(btf, t->type);
	while (btf_type_is_modifier(t)) {
		type_id = t->type;
		t = btf_type_by_id(btf, t->type);
	}

	mutex_lock(&cand_cache_mutex);
	cc = bpf_core_find_cands(&ctx, type_id);
	if (IS_ERR(cc)) {
		err = PTR_ERR(cc);
		bpf_log(log, "arg#%d reference type('%s %s') candidate matching error: %d\n",
			arg_idx, btf_type_str(t), __btf_name_by_offset(btf, t->name_off),
			err);
		goto cand_cache_unlock;
	}
	if (cc->cnt != 1) {
		bpf_log(log, "arg#%d reference type('%s %s') %s\n",
			arg_idx, btf_type_str(t), __btf_name_by_offset(btf, t->name_off),
			cc->cnt == 0 ? "has no matches" : "is ambiguous");
		err = cc->cnt == 0 ? -ENOENT : -ESRCH;
		goto cand_cache_unlock;
	}
	if (btf_is_module(cc->cands[0].btf)) {
		bpf_log(log, "arg#%d reference type('%s %s') points to kernel module type (unsupported)\n",
			arg_idx, btf_type_str(t), __btf_name_by_offset(btf, t->name_off));
		err = -EOPNOTSUPP;
		goto cand_cache_unlock;
	}
	kern_type_id = cc->cands[0].id;

cand_cache_unlock:
	mutex_unlock(&cand_cache_mutex);
	if (err)
		return err;

	return kern_type_id;
}

enum btf_arg_tag {
	ARG_TAG_CTX	  = BIT_ULL(0),
	ARG_TAG_NONNULL   = BIT_ULL(1),
	ARG_TAG_TRUSTED   = BIT_ULL(2),
	ARG_TAG_UNTRUSTED = BIT_ULL(3),
	ARG_TAG_NULLABLE  = BIT_ULL(4),
	ARG_TAG_ARENA	  = BIT_ULL(5),
};