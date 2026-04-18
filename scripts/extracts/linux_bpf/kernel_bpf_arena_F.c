// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 6/6



	/* Iterate the list again without holding spinlock to do the tlb flush and zap_pages */
	llist_for_each_safe(pos, t, list) {
		s = llist_entry(pos, struct arena_free_span, node);
		page_cnt = s->page_cnt;
		full_uaddr = clear_lo32(user_vm_start) + s->uaddr;
		kaddr = arena_vm_start + s->uaddr;

		/* ensure no stale TLB entries */
		flush_tlb_kernel_range(kaddr, kaddr + (page_cnt * PAGE_SIZE));

		/* remove pages from user vmas */
		zap_pages(arena, full_uaddr, page_cnt);

		kfree_nolock(s);
	}

	/* free all pages collected by apply_to_existing_page_range() in the first loop */
	llist_for_each_safe(pos, t, __llist_del_all(&free_pages)) {
		page = llist_entry(pos, struct page, pcp_llist);
		__free_page(page);
	}

	bpf_map_memcg_exit(old_memcg, new_memcg);
}

static void arena_free_irq(struct irq_work *iw)
{
	struct bpf_arena *arena = container_of(iw, struct bpf_arena, free_irq);

	schedule_work(&arena->free_work);
}

__bpf_kfunc_start_defs();

__bpf_kfunc void *bpf_arena_alloc_pages(void *p__map, void *addr__ign, u32 page_cnt,
					int node_id, u64 flags)
{
	struct bpf_map *map = p__map;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if (map->map_type != BPF_MAP_TYPE_ARENA || flags || !page_cnt)
		return NULL;

	return (void *)arena_alloc_pages(arena, (long)addr__ign, page_cnt, node_id, true);
}

void *bpf_arena_alloc_pages_non_sleepable(void *p__map, void *addr__ign, u32 page_cnt,
					  int node_id, u64 flags)
{
	struct bpf_map *map = p__map;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if (map->map_type != BPF_MAP_TYPE_ARENA || flags || !page_cnt)
		return NULL;

	return (void *)arena_alloc_pages(arena, (long)addr__ign, page_cnt, node_id, false);
}
__bpf_kfunc void bpf_arena_free_pages(void *p__map, void *ptr__ign, u32 page_cnt)
{
	struct bpf_map *map = p__map;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if (map->map_type != BPF_MAP_TYPE_ARENA || !page_cnt || !ptr__ign)
		return;
	arena_free_pages(arena, (long)ptr__ign, page_cnt, true);
}

void bpf_arena_free_pages_non_sleepable(void *p__map, void *ptr__ign, u32 page_cnt)
{
	struct bpf_map *map = p__map;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if (map->map_type != BPF_MAP_TYPE_ARENA || !page_cnt || !ptr__ign)
		return;
	arena_free_pages(arena, (long)ptr__ign, page_cnt, false);
}

__bpf_kfunc int bpf_arena_reserve_pages(void *p__map, void *ptr__ign, u32 page_cnt)
{
	struct bpf_map *map = p__map;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if (map->map_type != BPF_MAP_TYPE_ARENA)
		return -EINVAL;

	if (!page_cnt)
		return 0;

	return arena_reserve_pages(arena, (long)ptr__ign, page_cnt);
}
__bpf_kfunc_end_defs();

BTF_KFUNCS_START(arena_kfuncs)
BTF_ID_FLAGS(func, bpf_arena_alloc_pages, KF_ARENA_RET | KF_ARENA_ARG2)
BTF_ID_FLAGS(func, bpf_arena_free_pages, KF_ARENA_ARG2)
BTF_ID_FLAGS(func, bpf_arena_reserve_pages, KF_ARENA_ARG2)
BTF_KFUNCS_END(arena_kfuncs)

static const struct btf_kfunc_id_set common_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &arena_kfuncs,
};

static int __init kfunc_init(void)
{
	return register_btf_kfunc_id_set(BPF_PROG_TYPE_UNSPEC, &common_kfunc_set);
}
late_initcall(kfunc_init);

void bpf_prog_report_arena_violation(bool write, unsigned long addr, unsigned long fault_ip)
{
	struct bpf_stream_stage ss;
	struct bpf_prog *prog;
	u64 user_vm_start;

	/*
	 * The RCU read lock is held to safely traverse the latch tree, but we
	 * don't need its protection when accessing the prog, since it will not
	 * disappear while we are handling the fault.
	 */
	rcu_read_lock();
	prog = bpf_prog_ksym_find(fault_ip);
	rcu_read_unlock();
	if (!prog)
		return;

	/* Use main prog for stream access */
	prog = prog->aux->main_prog_aux->prog;

	user_vm_start = bpf_arena_get_user_vm_start(prog->aux->arena);
	addr += clear_lo32(user_vm_start);

	bpf_stream_stage(ss, prog, BPF_STDERR, ({
		bpf_stream_printk(ss, "ERROR: Arena %s access at unmapped address 0x%lx\n",
				  write ? "WRITE" : "READ", addr);
		bpf_stream_dump_stack(ss);
	}));
}
