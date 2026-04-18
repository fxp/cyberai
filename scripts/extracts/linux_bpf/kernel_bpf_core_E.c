// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 5/20



	return offset >= ksym->fp_start && offset < ksym->fp_end;
}

const struct exception_table_entry *search_bpf_extables(unsigned long addr)
{
	const struct exception_table_entry *e = NULL;
	struct bpf_prog *prog;

	rcu_read_lock();
	prog = bpf_prog_ksym_find(addr);
	if (!prog)
		goto out;
	if (!prog->aux->num_exentries)
		goto out;

	e = search_extable(prog->aux->extable, prog->aux->num_exentries, addr);
out:
	rcu_read_unlock();
	return e;
}

int bpf_get_kallsym(unsigned int symnum, unsigned long *value, char *type,
		    char *sym)
{
	struct bpf_ksym *ksym;
	unsigned int it = 0;
	int ret = -ERANGE;

	if (!bpf_jit_kallsyms_enabled())
		return ret;

	rcu_read_lock();
	list_for_each_entry_rcu(ksym, &bpf_kallsyms, lnode) {
		if (it++ != symnum)
			continue;

		strscpy(sym, ksym->name, KSYM_NAME_LEN);

		*value = ksym->start;
		*type  = BPF_SYM_ELF_TYPE;

		ret = 0;
		break;
	}
	rcu_read_unlock();

	return ret;
}

int bpf_jit_add_poke_descriptor(struct bpf_prog *prog,
				struct bpf_jit_poke_descriptor *poke)
{
	struct bpf_jit_poke_descriptor *tab = prog->aux->poke_tab;
	static const u32 poke_tab_max = 1024;
	u32 slot = prog->aux->size_poke_tab;
	u32 size = slot + 1;

	if (size > poke_tab_max)
		return -ENOSPC;
	if (poke->tailcall_target || poke->tailcall_target_stable ||
	    poke->tailcall_bypass || poke->adj_off || poke->bypass_addr)
		return -EINVAL;

	switch (poke->reason) {
	case BPF_POKE_REASON_TAIL_CALL:
		if (!poke->tail_call.map)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	tab = krealloc_array(tab, size, sizeof(*poke), GFP_KERNEL);
	if (!tab)
		return -ENOMEM;

	memcpy(&tab[slot], poke, sizeof(*poke));
	prog->aux->size_poke_tab = size;
	prog->aux->poke_tab = tab;

	return slot;
}

/*
 * BPF program pack allocator.
 *
 * Most BPF programs are pretty small. Allocating a hole page for each
 * program is sometime a waste. Many small bpf program also adds pressure
 * to instruction TLB. To solve this issue, we introduce a BPF program pack
 * allocator. The prog_pack allocator uses HPAGE_PMD_SIZE page (2MB on x86)
 * to host BPF programs.
 */
#define BPF_PROG_CHUNK_SHIFT	6
#define BPF_PROG_CHUNK_SIZE	(1 << BPF_PROG_CHUNK_SHIFT)
#define BPF_PROG_CHUNK_MASK	(~(BPF_PROG_CHUNK_SIZE - 1))

struct bpf_prog_pack {
	struct list_head list;
	void *ptr;
	unsigned long bitmap[];
};

void bpf_jit_fill_hole_with_zero(void *area, unsigned int size)
{
	memset(area, 0, size);
}

#define BPF_PROG_SIZE_TO_NBITS(size)	(round_up(size, BPF_PROG_CHUNK_SIZE) / BPF_PROG_CHUNK_SIZE)

static DEFINE_MUTEX(pack_mutex);
static LIST_HEAD(pack_list);

/* PMD_SIZE is not available in some special config, e.g. ARCH=arm with
 * CONFIG_MMU=n. Use PAGE_SIZE in these cases.
 */
#ifdef PMD_SIZE
/* PMD_SIZE is really big for some archs. It doesn't make sense to
 * reserve too much memory in one allocation. Hardcode BPF_PROG_PACK_SIZE to
 * 2MiB * num_possible_nodes(). On most architectures PMD_SIZE will be
 * greater than or equal to 2MB.
 */
#define BPF_PROG_PACK_SIZE (SZ_2M * num_possible_nodes())
#else
#define BPF_PROG_PACK_SIZE PAGE_SIZE
#endif

#define BPF_PROG_CHUNK_COUNT (BPF_PROG_PACK_SIZE / BPF_PROG_CHUNK_SIZE)

static struct bpf_prog_pack *alloc_new_pack(bpf_jit_fill_hole_t bpf_fill_ill_insns)
{
	struct bpf_prog_pack *pack;
	int err;

	pack = kzalloc_flex(*pack, bitmap, BITS_TO_LONGS(BPF_PROG_CHUNK_COUNT));
	if (!pack)
		return NULL;
	pack->ptr = bpf_jit_alloc_exec(BPF_PROG_PACK_SIZE);
	if (!pack->ptr)
		goto out;
	bpf_fill_ill_insns(pack->ptr, BPF_PROG_PACK_SIZE);
	bitmap_zero(pack->bitmap, BPF_PROG_PACK_SIZE / BPF_PROG_CHUNK_SIZE);

	set_vm_flush_reset_perms(pack->ptr);
	err = set_memory_rox((unsigned long)pack->ptr,
			     BPF_PROG_PACK_SIZE / PAGE_SIZE);
	if (err)
		goto out;
	list_add_tail(&pack->list, &pack_list);
	return pack;

out:
	bpf_jit_free_exec(pack->ptr);
	kfree(pack);
	return NULL;
}

void *bpf_prog_pack_alloc(u32 size, bpf_jit_fill_hole_t bpf_fill_ill_insns)
{
	unsigned int nbits = BPF_PROG_SIZE_TO_NBITS(size);
	struct bpf_prog_pack *pack;
	unsigned long pos;
	void *ptr = NULL;

	mutex_lock(&pack_mutex);
	if (size > BPF_PROG_PACK_SIZE) {
		size = round_up(size, PAGE_SIZE);
		ptr = bpf_jit_alloc_exec(size);
		if (ptr) {
			int err;

			bpf_fill_ill_insns(ptr, size);
			set_vm_flush_reset_perms(ptr);
			err = set_memory_rox((unsigned long)ptr,
					     size / PAGE_SIZE);
			if (err) {
				bpf_jit_free_exec(ptr);
				ptr = NULL;
			}
		}
		goto out;
	}
	list_for_each_entry(pack, &pack_list, list) {
		pos = bitmap_find_next_zero_area(pack->bitmap, BPF_PROG_CHUNK_COUNT, 0,
						 nbits, 0);
		if (pos < BPF_PROG_CHUNK_COUNT)
			goto found_free_area;
	}

	pack = alloc_new_pack(bpf_fill_ill_insns);
	if (!pack)
		goto out;

	pos = 0;

found_free_area:
	bitmap_set(pack->bitmap, pos, nbits);
	ptr = (void *)(pack->ptr) + (pos << BPF_PROG_CHUNK_SHIFT);

out:
	mutex_unlock(&pack_mutex);
	return ptr;
}