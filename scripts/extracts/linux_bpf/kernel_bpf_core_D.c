// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 4/20



	BUILD_BUG_ON(sizeof("bpf_prog_") +
		     sizeof(prog->tag) * 2 +
		     /* name has been null terminated.
		      * We should need +1 for the '_' preceding
		      * the name.  However, the null character
		      * is double counted between the name and the
		      * sizeof("bpf_prog_") above, so we omit
		      * the +1 here.
		      */
		     sizeof(prog->aux->name) > KSYM_NAME_LEN);

	sym += snprintf(sym, KSYM_NAME_LEN, "bpf_prog_");
	sym  = bin2hex(sym, prog->tag, sizeof(prog->tag));

	/* prog->aux->name will be ignored if full btf name is available */
	if (prog->aux->func_info_cnt && prog->aux->func_idx < prog->aux->func_info_cnt) {
		type = btf_type_by_id(prog->aux->btf,
				      prog->aux->func_info[prog->aux->func_idx].type_id);
		func_name = btf_name_by_offset(prog->aux->btf, type->name_off);
		snprintf(sym, (size_t)(end - sym), "_%s", func_name);
		return;
	}

	if (prog->aux->name[0])
		snprintf(sym, (size_t)(end - sym), "_%s", prog->aux->name);
	else
		*sym = 0;
}

static unsigned long bpf_get_ksym_start(struct latch_tree_node *n)
{
	return container_of(n, struct bpf_ksym, tnode)->start;
}

static __always_inline bool bpf_tree_less(struct latch_tree_node *a,
					  struct latch_tree_node *b)
{
	return bpf_get_ksym_start(a) < bpf_get_ksym_start(b);
}

static __always_inline int bpf_tree_comp(void *key, struct latch_tree_node *n)
{
	unsigned long val = (unsigned long)key;
	const struct bpf_ksym *ksym;

	ksym = container_of(n, struct bpf_ksym, tnode);

	if (val < ksym->start)
		return -1;
	/* Ensure that we detect return addresses as part of the program, when
	 * the final instruction is a call for a program part of the stack
	 * trace. Therefore, do val > ksym->end instead of val >= ksym->end.
	 */
	if (val > ksym->end)
		return  1;

	return 0;
}

static const struct latch_tree_ops bpf_tree_ops = {
	.less	= bpf_tree_less,
	.comp	= bpf_tree_comp,
};

static DEFINE_SPINLOCK(bpf_lock);
static LIST_HEAD(bpf_kallsyms);
static struct latch_tree_root bpf_tree __cacheline_aligned;

void bpf_ksym_add(struct bpf_ksym *ksym)
{
	spin_lock_bh(&bpf_lock);
	WARN_ON_ONCE(!list_empty(&ksym->lnode));
	list_add_tail_rcu(&ksym->lnode, &bpf_kallsyms);
	latch_tree_insert(&ksym->tnode, &bpf_tree, &bpf_tree_ops);
	spin_unlock_bh(&bpf_lock);
}

static void __bpf_ksym_del(struct bpf_ksym *ksym)
{
	if (list_empty(&ksym->lnode))
		return;

	latch_tree_erase(&ksym->tnode, &bpf_tree, &bpf_tree_ops);
	list_del_rcu(&ksym->lnode);
}

void bpf_ksym_del(struct bpf_ksym *ksym)
{
	spin_lock_bh(&bpf_lock);
	__bpf_ksym_del(ksym);
	spin_unlock_bh(&bpf_lock);
}

static bool bpf_prog_kallsyms_candidate(const struct bpf_prog *fp)
{
	return fp->jited && !bpf_prog_was_classic(fp);
}

void bpf_prog_kallsyms_add(struct bpf_prog *fp)
{
	if (!bpf_prog_kallsyms_candidate(fp) ||
	    !bpf_token_capable(fp->aux->token, CAP_BPF))
		return;

	bpf_prog_ksym_set_addr(fp);
	bpf_prog_ksym_set_name(fp);
	fp->aux->ksym.prog = true;

	bpf_ksym_add(&fp->aux->ksym);

#ifdef CONFIG_FINEIBT
	/*
	 * When FineIBT, code in the __cfi_foo() symbols can get executed
	 * and hence unwinder needs help.
	 */
	if (cfi_mode != CFI_FINEIBT)
		return;

	snprintf(fp->aux->ksym_prefix.name, KSYM_NAME_LEN,
		 "__cfi_%s", fp->aux->ksym.name);

	fp->aux->ksym_prefix.start = (unsigned long) fp->bpf_func - 16;
	fp->aux->ksym_prefix.end   = (unsigned long) fp->bpf_func;

	bpf_ksym_add(&fp->aux->ksym_prefix);
#endif
}

void bpf_prog_kallsyms_del(struct bpf_prog *fp)
{
	if (!bpf_prog_kallsyms_candidate(fp))
		return;

	bpf_ksym_del(&fp->aux->ksym);
#ifdef CONFIG_FINEIBT
	if (cfi_mode != CFI_FINEIBT)
		return;
	bpf_ksym_del(&fp->aux->ksym_prefix);
#endif
}

static struct bpf_ksym *bpf_ksym_find(unsigned long addr)
{
	struct latch_tree_node *n;

	n = latch_tree_find((void *)addr, &bpf_tree, &bpf_tree_ops);
	return n ? container_of(n, struct bpf_ksym, tnode) : NULL;
}

int bpf_address_lookup(unsigned long addr, unsigned long *size,
		       unsigned long *off, char *sym)
{
	struct bpf_ksym *ksym;
	int ret = 0;

	rcu_read_lock();
	ksym = bpf_ksym_find(addr);
	if (ksym) {
		unsigned long symbol_start = ksym->start;
		unsigned long symbol_end = ksym->end;

		ret = strscpy(sym, ksym->name, KSYM_NAME_LEN);

		if (size)
			*size = symbol_end - symbol_start;
		if (off)
			*off  = addr - symbol_start;
	}
	rcu_read_unlock();

	return ret;
}

bool is_bpf_text_address(unsigned long addr)
{
	bool ret;

	rcu_read_lock();
	ret = bpf_ksym_find(addr) != NULL;
	rcu_read_unlock();

	return ret;
}

struct bpf_prog *bpf_prog_ksym_find(unsigned long addr)
{
	struct bpf_ksym *ksym;

	WARN_ON_ONCE(!rcu_read_lock_held());
	ksym = bpf_ksym_find(addr);

	return ksym && ksym->prog ?
	       container_of(ksym, struct bpf_prog_aux, ksym)->prog :
	       NULL;
}

bool bpf_has_frame_pointer(unsigned long ip)
{
	struct bpf_ksym *ksym;
	unsigned long offset;

	guard(rcu)();

	ksym = bpf_ksym_find(ip);
	if (!ksym || !ksym->fp_start || !ksym->fp_end)
		return false;

	offset = ip - ksym->start;