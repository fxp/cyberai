// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 5/6



static void arena_free_pages(struct bpf_arena *arena, long uaddr, long page_cnt, bool sleepable)
{
	struct mem_cgroup *new_memcg, *old_memcg;
	u64 full_uaddr, uaddr_end;
	long kaddr, pgoff;
	struct page *page;
	struct llist_head free_pages;
	struct llist_node *pos, *t;
	struct arena_free_span *s;
	unsigned long flags;
	int ret = 0;

	/* only aligned lower 32-bit are relevant */
	uaddr = (u32)uaddr;
	uaddr &= PAGE_MASK;
	kaddr = bpf_arena_get_kern_vm_start(arena) + uaddr;
	full_uaddr = clear_lo32(arena->user_vm_start) + uaddr;
	uaddr_end = min(arena->user_vm_end, full_uaddr + (page_cnt << PAGE_SHIFT));
	if (full_uaddr >= uaddr_end)
		return;

	page_cnt = (uaddr_end - full_uaddr) >> PAGE_SHIFT;
	pgoff = compute_pgoff(arena, uaddr);
	bpf_map_memcg_enter(&arena->map, &old_memcg, &new_memcg);

	if (!sleepable)
		goto defer;

	ret = raw_res_spin_lock_irqsave(&arena->spinlock, flags);

	/* Can't proceed without holding the spinlock so defer the free */
	if (ret)
		goto defer;

	range_tree_set(&arena->rt, pgoff, page_cnt);

	init_llist_head(&free_pages);
	/* clear ptes and collect struct pages */
	apply_to_existing_page_range(&init_mm, kaddr, page_cnt << PAGE_SHIFT,
				     apply_range_clear_cb, &free_pages);

	/* drop the lock to do the tlb flush and zap pages */
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);

	/* ensure no stale TLB entries */
	flush_tlb_kernel_range(kaddr, kaddr + (page_cnt * PAGE_SIZE));

	if (page_cnt > 1)
		/* bulk zap if multiple pages being freed */
		zap_pages(arena, full_uaddr, page_cnt);

	llist_for_each_safe(pos, t, __llist_del_all(&free_pages)) {
		page = llist_entry(pos, struct page, pcp_llist);
		if (page_cnt == 1 && page_mapped(page)) /* mapped by some user process */
			/* Optimization for the common case of page_cnt==1:
			 * If page wasn't mapped into some user vma there
			 * is no need to call zap_pages which is slow. When
			 * page_cnt is big it's faster to do the batched zap.
			 */
			zap_pages(arena, full_uaddr, 1);
		__free_page(page);
	}
	bpf_map_memcg_exit(old_memcg, new_memcg);

	return;

defer:
	s = kmalloc_nolock(sizeof(struct arena_free_span), __GFP_ACCOUNT, -1);
	bpf_map_memcg_exit(old_memcg, new_memcg);
	if (!s)
		/*
		 * If allocation fails in non-sleepable context, pages are intentionally left
		 * inaccessible (leaked) until the arena is destroyed. Cleanup or retries are not
		 * possible here, so we intentionally omit them for safety.
		 */
		return;

	s->page_cnt = page_cnt;
	s->uaddr = uaddr;
	llist_add(&s->node, &arena->free_spans);
	irq_work_queue(&arena->free_irq);
}

/*
 * Reserve an arena virtual address range without populating it. This call stops
 * bpf_arena_alloc_pages from adding pages to this range.
 */
static int arena_reserve_pages(struct bpf_arena *arena, long uaddr, u32 page_cnt)
{
	long page_cnt_max = (arena->user_vm_end - arena->user_vm_start) >> PAGE_SHIFT;
	struct mem_cgroup *new_memcg, *old_memcg;
	unsigned long flags;
	long pgoff;
	int ret;

	if (uaddr & ~PAGE_MASK)
		return 0;

	pgoff = compute_pgoff(arena, uaddr);
	if (pgoff + page_cnt > page_cnt_max)
		return -EINVAL;

	if (raw_res_spin_lock_irqsave(&arena->spinlock, flags))
		return -EBUSY;

	/* Cannot guard already allocated pages. */
	ret = is_range_tree_set(&arena->rt, pgoff, page_cnt);
	if (ret) {
		ret = -EBUSY;
		goto out;
	}

	/* "Allocate" the region to prevent it from being allocated. */
	bpf_map_memcg_enter(&arena->map, &old_memcg, &new_memcg);
	ret = range_tree_clear(&arena->rt, pgoff, page_cnt);
	bpf_map_memcg_exit(old_memcg, new_memcg);
out:
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
	return ret;
}

static void arena_free_worker(struct work_struct *work)
{
	struct bpf_arena *arena = container_of(work, struct bpf_arena, free_work);
	struct mem_cgroup *new_memcg, *old_memcg;
	struct llist_node *list, *pos, *t;
	struct arena_free_span *s;
	u64 arena_vm_start, user_vm_start;
	struct llist_head free_pages;
	struct page *page;
	unsigned long full_uaddr;
	long kaddr, page_cnt, pgoff;
	unsigned long flags;

	if (raw_res_spin_lock_irqsave(&arena->spinlock, flags)) {
		schedule_work(work);
		return;
	}

	bpf_map_memcg_enter(&arena->map, &old_memcg, &new_memcg);

	init_llist_head(&free_pages);
	arena_vm_start = bpf_arena_get_kern_vm_start(arena);
	user_vm_start = bpf_arena_get_user_vm_start(arena);

	list = llist_del_all(&arena->free_spans);
	llist_for_each(pos, list) {
		s = llist_entry(pos, struct arena_free_span, node);
		page_cnt = s->page_cnt;
		kaddr = arena_vm_start + s->uaddr;
		pgoff = compute_pgoff(arena, s->uaddr);

		/* clear ptes and collect pages in free_pages llist */
		apply_to_existing_page_range(&init_mm, kaddr, page_cnt << PAGE_SHIFT,
					     apply_range_clear_cb, &free_pages);

		range_tree_set(&arena->rt, pgoff, page_cnt);
	}
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);