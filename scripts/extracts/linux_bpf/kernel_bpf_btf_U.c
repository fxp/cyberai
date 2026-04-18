// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 53/55



	if (*cc) {
		bpf_free_cands_from_cache(*cc);
		*cc = NULL;
	}
	new_cands = kmemdup(cands, sizeof_cands(cands->cnt), GFP_KERNEL_ACCOUNT);
	if (!new_cands) {
		bpf_free_cands(cands);
		return ERR_PTR(-ENOMEM);
	}
	/* strdup the name, since it will stay in cache.
	 * the cands->name points to strings in prog's BTF and the prog can be unloaded.
	 */
	new_cands->name = kmemdup_nul(cands->name, cands->name_len, GFP_KERNEL_ACCOUNT);
	bpf_free_cands(cands);
	if (!new_cands->name) {
		kfree(new_cands);
		return ERR_PTR(-ENOMEM);
	}
	*cc = new_cands;
	return new_cands;
}

#ifdef CONFIG_DEBUG_INFO_BTF_MODULES
static void __purge_cand_cache(struct btf *btf, struct bpf_cand_cache **cache,
			       int cache_size)
{
	struct bpf_cand_cache *cc;
	int i, j;

	for (i = 0; i < cache_size; i++) {
		cc = cache[i];
		if (!cc)
			continue;
		if (!btf) {
			/* when new module is loaded purge all of module_cand_cache,
			 * since new module might have candidates with the name
			 * that matches cached cands.
			 */
			bpf_free_cands_from_cache(cc);
			cache[i] = NULL;
			continue;
		}
		/* when module is unloaded purge cache entries
		 * that match module's btf
		 */
		for (j = 0; j < cc->cnt; j++)
			if (cc->cands[j].btf == btf) {
				bpf_free_cands_from_cache(cc);
				cache[i] = NULL;
				break;
			}
	}

}

static void purge_cand_cache(struct btf *btf)
{
	mutex_lock(&cand_cache_mutex);
	__purge_cand_cache(btf, module_cand_cache, MODULE_CAND_CACHE_SIZE);
	mutex_unlock(&cand_cache_mutex);
}
#endif

static struct bpf_cand_cache *
bpf_core_add_cands(struct bpf_cand_cache *cands, const struct btf *targ_btf,
		   int targ_start_id)
{
	struct bpf_cand_cache *new_cands;
	const struct btf_type *t;
	const char *targ_name;
	size_t targ_essent_len;
	int n, i;

	n = btf_nr_types(targ_btf);
	for (i = targ_start_id; i < n; i++) {
		t = btf_type_by_id(targ_btf, i);
		if (btf_kind(t) != cands->kind)
			continue;

		targ_name = btf_name_by_offset(targ_btf, t->name_off);
		if (!targ_name)
			continue;

		/* the resched point is before strncmp to make sure that search
		 * for non-existing name will have a chance to schedule().
		 */
		cond_resched();

		if (strncmp(cands->name, targ_name, cands->name_len) != 0)
			continue;

		targ_essent_len = bpf_core_essential_name_len(targ_name);
		if (targ_essent_len != cands->name_len)
			continue;

		/* most of the time there is only one candidate for a given kind+name pair */
		new_cands = kmalloc(sizeof_cands(cands->cnt + 1), GFP_KERNEL_ACCOUNT);
		if (!new_cands) {
			bpf_free_cands(cands);
			return ERR_PTR(-ENOMEM);
		}

		memcpy(new_cands, cands, sizeof_cands(cands->cnt));
		bpf_free_cands(cands);
		cands = new_cands;
		cands->cands[cands->cnt].btf = targ_btf;
		cands->cands[cands->cnt].id = i;
		cands->cnt++;
	}
	return cands;
}

static struct bpf_cand_cache *
bpf_core_find_cands(struct bpf_core_ctx *ctx, u32 local_type_id)
{
	struct bpf_cand_cache *cands, *cc, local_cand = {};
	const struct btf *local_btf = ctx->btf;
	const struct btf_type *local_type;
	const struct btf *main_btf;
	size_t local_essent_len;
	struct btf *mod_btf;
	const char *name;
	int id;

	main_btf = bpf_get_btf_vmlinux();
	if (IS_ERR(main_btf))
		return ERR_CAST(main_btf);
	if (!main_btf)
		return ERR_PTR(-EINVAL);

	local_type = btf_type_by_id(local_btf, local_type_id);
	if (!local_type)
		return ERR_PTR(-EINVAL);

	name = btf_name_by_offset(local_btf, local_type->name_off);
	if (str_is_empty(name))
		return ERR_PTR(-EINVAL);
	local_essent_len = bpf_core_essential_name_len(name);

	cands = &local_cand;
	cands->name = name;
	cands->kind = btf_kind(local_type);
	cands->name_len = local_essent_len;

	cc = check_cand_cache(cands, vmlinux_cand_cache, VMLINUX_CAND_CACHE_SIZE);
	/* cands is a pointer to stack here */
	if (cc) {
		if (cc->cnt)
			return cc;
		goto check_modules;
	}

	/* Attempt to find target candidates in vmlinux BTF first */
	cands = bpf_core_add_cands(cands, main_btf, btf_named_start_id(main_btf, true));
	if (IS_ERR(cands))
		return ERR_CAST(cands);

	/* cands is a pointer to kmalloced memory here if cands->cnt > 0 */

	/* populate cache even when cands->cnt == 0 */
	cc = populate_cand_cache(cands, vmlinux_cand_cache, VMLINUX_CAND_CACHE_SIZE);
	if (IS_ERR(cc))
		return ERR_CAST(cc);

	/* if vmlinux BTF has any candidate, don't go for module BTFs */
	if (cc->cnt)
		return cc;

check_modules:
	/* cands is a pointer to stack here and cands->cnt == 0 */
	cc = check_cand_cache(cands, module_cand_cache, MODULE_CAND_CACHE_SIZE);
	if (cc)
		/* if cache has it return it even if cc->cnt == 0 */
		return cc;