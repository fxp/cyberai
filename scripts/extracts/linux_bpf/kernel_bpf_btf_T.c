// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 52/55



	tab->cnt += add_cnt;

	sort(tab->dtors, tab->cnt, sizeof(tab->dtors[0]), btf_id_cmp_func, NULL);

end:
	if (ret)
		btf_free_dtor_kfunc_tab(btf);
	btf_put(btf);
	return ret;
}
EXPORT_SYMBOL_GPL(register_btf_id_dtor_kfuncs);

#define MAX_TYPES_ARE_COMPAT_DEPTH 2

/* Check local and target types for compatibility. This check is used for
 * type-based CO-RE relocations and follow slightly different rules than
 * field-based relocations. This function assumes that root types were already
 * checked for name match. Beyond that initial root-level name check, names
 * are completely ignored. Compatibility rules are as follows:
 *   - any two STRUCTs/UNIONs/FWDs/ENUMs/INTs/ENUM64s are considered compatible, but
 *     kind should match for local and target types (i.e., STRUCT is not
 *     compatible with UNION);
 *   - for ENUMs/ENUM64s, the size is ignored;
 *   - for INT, size and signedness are ignored;
 *   - for ARRAY, dimensionality is ignored, element types are checked for
 *     compatibility recursively;
 *   - CONST/VOLATILE/RESTRICT modifiers are ignored;
 *   - TYPEDEFs/PTRs are compatible if types they pointing to are compatible;
 *   - FUNC_PROTOs are compatible if they have compatible signature: same
 *     number of input args and compatible return and argument types.
 * These rules are not set in stone and probably will be adjusted as we get
 * more experience with using BPF CO-RE relocations.
 */
int bpf_core_types_are_compat(const struct btf *local_btf, __u32 local_id,
			      const struct btf *targ_btf, __u32 targ_id)
{
	return __bpf_core_types_are_compat(local_btf, local_id, targ_btf, targ_id,
					   MAX_TYPES_ARE_COMPAT_DEPTH);
}

#define MAX_TYPES_MATCH_DEPTH 2

int bpf_core_types_match(const struct btf *local_btf, u32 local_id,
			 const struct btf *targ_btf, u32 targ_id)
{
	return __bpf_core_types_match(local_btf, local_id, targ_btf, targ_id, false,
				      MAX_TYPES_MATCH_DEPTH);
}

static bool bpf_core_is_flavor_sep(const char *s)
{
	/* check X___Y name pattern, where X and Y are not underscores */
	return s[0] != '_' &&				      /* X */
	       s[1] == '_' && s[2] == '_' && s[3] == '_' &&   /* ___ */
	       s[4] != '_';				      /* Y */
}

size_t bpf_core_essential_name_len(const char *name)
{
	size_t n = strlen(name);
	int i;

	for (i = n - 5; i >= 0; i--) {
		if (bpf_core_is_flavor_sep(name + i))
			return i + 1;
	}
	return n;
}

static void bpf_free_cands(struct bpf_cand_cache *cands)
{
	if (!cands->cnt)
		/* empty candidate array was allocated on stack */
		return;
	kfree(cands);
}

static void bpf_free_cands_from_cache(struct bpf_cand_cache *cands)
{
	kfree(cands->name);
	kfree(cands);
}

#define VMLINUX_CAND_CACHE_SIZE 31
static struct bpf_cand_cache *vmlinux_cand_cache[VMLINUX_CAND_CACHE_SIZE];

#define MODULE_CAND_CACHE_SIZE 31
static struct bpf_cand_cache *module_cand_cache[MODULE_CAND_CACHE_SIZE];

static void __print_cand_cache(struct bpf_verifier_log *log,
			       struct bpf_cand_cache **cache,
			       int cache_size)
{
	struct bpf_cand_cache *cc;
	int i, j;

	for (i = 0; i < cache_size; i++) {
		cc = cache[i];
		if (!cc)
			continue;
		bpf_log(log, "[%d]%s(", i, cc->name);
		for (j = 0; j < cc->cnt; j++) {
			bpf_log(log, "%d", cc->cands[j].id);
			if (j < cc->cnt - 1)
				bpf_log(log, " ");
		}
		bpf_log(log, "), ");
	}
}

static void print_cand_cache(struct bpf_verifier_log *log)
{
	mutex_lock(&cand_cache_mutex);
	bpf_log(log, "vmlinux_cand_cache:");
	__print_cand_cache(log, vmlinux_cand_cache, VMLINUX_CAND_CACHE_SIZE);
	bpf_log(log, "\nmodule_cand_cache:");
	__print_cand_cache(log, module_cand_cache, MODULE_CAND_CACHE_SIZE);
	bpf_log(log, "\n");
	mutex_unlock(&cand_cache_mutex);
}

static u32 hash_cands(struct bpf_cand_cache *cands)
{
	return jhash(cands->name, cands->name_len, 0);
}

static struct bpf_cand_cache *check_cand_cache(struct bpf_cand_cache *cands,
					       struct bpf_cand_cache **cache,
					       int cache_size)
{
	struct bpf_cand_cache *cc = cache[hash_cands(cands) % cache_size];

	if (cc && cc->name_len == cands->name_len &&
	    !strncmp(cc->name, cands->name, cands->name_len))
		return cc;
	return NULL;
}

static size_t sizeof_cands(int cnt)
{
	return offsetof(struct bpf_cand_cache, cands[cnt]);
}

static struct bpf_cand_cache *populate_cand_cache(struct bpf_cand_cache *cands,
						  struct bpf_cand_cache **cache,
						  int cache_size)
{
	struct bpf_cand_cache **cc = &cache[hash_cands(cands) % cache_size], *new_cands;