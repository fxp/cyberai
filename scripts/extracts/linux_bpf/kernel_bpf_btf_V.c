// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 54/55



	/* If candidate is not found in vmlinux's BTF then search in module's BTFs */
	spin_lock_bh(&btf_idr_lock);
	idr_for_each_entry(&btf_idr, mod_btf, id) {
		if (!btf_is_module(mod_btf))
			continue;
		/* linear search could be slow hence unlock/lock
		 * the IDR to avoiding holding it for too long
		 */
		btf_get(mod_btf);
		spin_unlock_bh(&btf_idr_lock);
		cands = bpf_core_add_cands(cands, mod_btf, btf_named_start_id(mod_btf, true));
		btf_put(mod_btf);
		if (IS_ERR(cands))
			return ERR_CAST(cands);
		spin_lock_bh(&btf_idr_lock);
	}
	spin_unlock_bh(&btf_idr_lock);
	/* cands is a pointer to kmalloced memory here if cands->cnt > 0
	 * or pointer to stack if cands->cnd == 0.
	 * Copy it into the cache even when cands->cnt == 0 and
	 * return the result.
	 */
	return populate_cand_cache(cands, module_cand_cache, MODULE_CAND_CACHE_SIZE);
}

int bpf_core_apply(struct bpf_core_ctx *ctx, const struct bpf_core_relo *relo,
		   int relo_idx, void *insn)
{
	bool need_cands = relo->kind != BPF_CORE_TYPE_ID_LOCAL;
	struct bpf_core_cand_list cands = {};
	struct bpf_core_relo_res targ_res;
	struct bpf_core_spec *specs;
	const struct btf_type *type;
	int err;

	/* ~4k of temp memory necessary to convert LLVM spec like "0:1:0:5"
	 * into arrays of btf_ids of struct fields and array indices.
	 */
	specs = kzalloc_objs(*specs, 3, GFP_KERNEL_ACCOUNT);
	if (!specs)
		return -ENOMEM;

	type = btf_type_by_id(ctx->btf, relo->type_id);
	if (!type) {
		bpf_log(ctx->log, "relo #%u: bad type id %u\n",
			relo_idx, relo->type_id);
		kfree(specs);
		return -EINVAL;
	}

	if (need_cands) {
		struct bpf_cand_cache *cc;
		int i;

		mutex_lock(&cand_cache_mutex);
		cc = bpf_core_find_cands(ctx, relo->type_id);
		if (IS_ERR(cc)) {
			bpf_log(ctx->log, "target candidate search failed for %d\n",
				relo->type_id);
			err = PTR_ERR(cc);
			goto out;
		}
		if (cc->cnt) {
			cands.cands = kzalloc_objs(*cands.cands, cc->cnt,
						   GFP_KERNEL_ACCOUNT);
			if (!cands.cands) {
				err = -ENOMEM;
				goto out;
			}
		}
		for (i = 0; i < cc->cnt; i++) {
			bpf_log(ctx->log,
				"CO-RE relocating %s %s: found target candidate [%d]\n",
				btf_kind_str[cc->kind], cc->name, cc->cands[i].id);
			cands.cands[i].btf = cc->cands[i].btf;
			cands.cands[i].id = cc->cands[i].id;
		}
		cands.len = cc->cnt;
		/* cand_cache_mutex needs to span the cache lookup and
		 * copy of btf pointer into bpf_core_cand_list,
		 * since module can be unloaded while bpf_core_calc_relo_insn
		 * is working with module's btf.
		 */
	}

	err = bpf_core_calc_relo_insn((void *)ctx->log, relo, relo_idx, ctx->btf, &cands, specs,
				      &targ_res);
	if (err)
		goto out;

	err = bpf_core_patch_insn((void *)ctx->log, insn, relo->insn_off / 8, relo, relo_idx,
				  &targ_res);

out:
	kfree(specs);
	if (need_cands) {
		kfree(cands.cands);
		mutex_unlock(&cand_cache_mutex);
		if (ctx->log->level & BPF_LOG_LEVEL2)
			print_cand_cache(ctx->log);
	}
	return err;
}

bool btf_nested_type_is_trusted(struct bpf_verifier_log *log,
				const struct bpf_reg_state *reg,
				const char *field_name, u32 btf_id, const char *suffix)
{
	struct btf *btf = reg->btf;
	const struct btf_type *walk_type, *safe_type;
	const char *tname;
	char safe_tname[64];
	long ret, safe_id;
	const struct btf_member *member;
	u32 i;

	walk_type = btf_type_by_id(btf, reg->btf_id);
	if (!walk_type)
		return false;

	tname = btf_name_by_offset(btf, walk_type->name_off);

	ret = snprintf(safe_tname, sizeof(safe_tname), "%s%s", tname, suffix);
	if (ret >= sizeof(safe_tname))
		return false;

	safe_id = btf_find_by_name_kind(btf, safe_tname, BTF_INFO_KIND(walk_type->info));
	if (safe_id < 0)
		return false;

	safe_type = btf_type_by_id(btf, safe_id);
	if (!safe_type)
		return false;

	for_each_member(i, safe_type, member) {
		const char *m_name = __btf_name_by_offset(btf, member->name_off);
		const struct btf_type *mtype = btf_type_by_id(btf, member->type);
		u32 id;

		if (!btf_type_is_ptr(mtype))
			continue;

		btf_type_skip_modifiers(btf, mtype->type, &id);
		/* If we match on both type and name, the field is considered trusted. */
		if (btf_id == id && !strcmp(field_name, m_name))
			return true;
	}

	return false;
}

bool btf_type_ids_nocast_alias(struct bpf_verifier_log *log,
			       const struct btf *reg_btf, u32 reg_id,
			       const struct btf *arg_btf, u32 arg_id)
{
	const char *reg_name, *arg_name, *search_needle;
	const struct btf_type *reg_type, *arg_type;
	int reg_len, arg_len, cmp_len;
	size_t pattern_len = sizeof(NOCAST_ALIAS_SUFFIX) - sizeof(char);

	reg_type = btf_type_by_id(reg_btf, reg_id);
	if (!reg_type)
		return false;

	arg_type = btf_type_by_id(arg_btf, arg_id);
	if (!arg_type)
		return false;

	reg_name = btf_name_by_offset(reg_btf, reg_type->name_off);
	arg_name = btf_name_by_offset(arg_btf, arg_type->name_off);

	reg_len = strlen(reg_name);
	arg_len = strlen(arg_name);