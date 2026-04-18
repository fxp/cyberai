// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 4/6



BTF_ID_LIST_SINGLE(bpf_arena_map_btf_ids, struct, bpf_arena)
const struct bpf_map_ops arena_map_ops = {
	.map_meta_equal = bpf_map_meta_equal,
	.map_alloc = arena_map_alloc,
	.map_free = arena_map_free,
	.map_direct_value_addr = arena_map_direct_value_addr,
	.map_mmap = arena_map_mmap,
	.map_get_unmapped_area = arena_get_unmapped_area,
	.map_get_next_key = arena_map_get_next_key,
	.map_push_elem = arena_map_push_elem,
	.map_peek_elem = arena_map_peek_elem,
	.map_pop_elem = arena_map_pop_elem,
	.map_lookup_elem = arena_map_lookup_elem,
	.map_update_elem = arena_map_update_elem,
	.map_delete_elem = arena_map_delete_elem,
	.map_check_btf = arena_map_check_btf,
	.map_mem_usage = arena_map_mem_usage,
	.map_btf_id = &bpf_arena_map_btf_ids[0],
};

static u64 clear_lo32(u64 val)
{
	return val & ~(u64)~0U;
}

/*
 * Allocate pages and vmap them into kernel vmalloc area.
 * Later the pages will be mmaped into user space vma.
 */
static long arena_alloc_pages(struct bpf_arena *arena, long uaddr, long page_cnt, int node_id,
			      bool sleepable)
{
	/* user_vm_end/start are fixed before bpf prog runs */
	long page_cnt_max = (arena->user_vm_end - arena->user_vm_start) >> PAGE_SHIFT;
	u64 kern_vm_start = bpf_arena_get_kern_vm_start(arena);
	struct mem_cgroup *new_memcg, *old_memcg;
	struct apply_range_data data;
	struct page **pages = NULL;
	long remaining, mapped = 0;
	long alloc_pages;
	unsigned long flags;
	long pgoff = 0;
	u32 uaddr32;
	int ret, i;

	if (node_id != NUMA_NO_NODE &&
	    ((unsigned int)node_id >= nr_node_ids || !node_online(node_id)))
		return 0;

	if (page_cnt > page_cnt_max)
		return 0;

	if (uaddr) {
		if (uaddr & ~PAGE_MASK)
			return 0;
		pgoff = compute_pgoff(arena, uaddr);
		if (pgoff > page_cnt_max - page_cnt)
			/* requested address will be outside of user VMA */
			return 0;
	}

	bpf_map_memcg_enter(&arena->map, &old_memcg, &new_memcg);
	/* Cap allocation size to KMALLOC_MAX_CACHE_SIZE so kmalloc_nolock() can succeed. */
	alloc_pages = min(page_cnt, KMALLOC_MAX_CACHE_SIZE / sizeof(struct page *));
	pages = kmalloc_nolock(alloc_pages * sizeof(struct page *), __GFP_ACCOUNT, NUMA_NO_NODE);
	if (!pages) {
		bpf_map_memcg_exit(old_memcg, new_memcg);
		return 0;
	}
	data.pages = pages;

	if (raw_res_spin_lock_irqsave(&arena->spinlock, flags))
		goto out_free_pages;

	if (uaddr) {
		ret = is_range_tree_set(&arena->rt, pgoff, page_cnt);
		if (ret)
			goto out_unlock_free_pages;
		ret = range_tree_clear(&arena->rt, pgoff, page_cnt);
	} else {
		ret = pgoff = range_tree_find(&arena->rt, page_cnt);
		if (pgoff >= 0)
			ret = range_tree_clear(&arena->rt, pgoff, page_cnt);
	}
	if (ret)
		goto out_unlock_free_pages;

	remaining = page_cnt;
	uaddr32 = (u32)(arena->user_vm_start + pgoff * PAGE_SIZE);

	while (remaining) {
		long this_batch = min(remaining, alloc_pages);

		/* zeroing is needed, since alloc_pages_bulk() only fills in non-zero entries */
		memset(pages, 0, this_batch * sizeof(struct page *));

		ret = bpf_map_alloc_pages(&arena->map, node_id, this_batch, pages);
		if (ret)
			goto out;

		/*
		 * Earlier checks made sure that uaddr32 + page_cnt * PAGE_SIZE - 1
		 * will not overflow 32-bit. Lower 32-bit need to represent
		 * contiguous user address range.
		 * Map these pages at kern_vm_start base.
		 * kern_vm_start + uaddr32 + page_cnt * PAGE_SIZE - 1 can overflow
		 * lower 32-bit and it's ok.
		 */
		data.i = 0;
		ret = apply_to_page_range(&init_mm,
					  kern_vm_start + uaddr32 + (mapped << PAGE_SHIFT),
					  this_batch << PAGE_SHIFT, apply_range_set_cb, &data);
		if (ret) {
			/* data.i pages were mapped, account them and free the remaining */
			mapped += data.i;
			for (i = data.i; i < this_batch; i++)
				free_pages_nolock(pages[i], 0);
			goto out;
		}

		mapped += this_batch;
		remaining -= this_batch;
	}
	flush_vmap_cache(kern_vm_start + uaddr32, mapped << PAGE_SHIFT);
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
	kfree_nolock(pages);
	bpf_map_memcg_exit(old_memcg, new_memcg);
	return clear_lo32(arena->user_vm_start) + uaddr32;
out:
	range_tree_set(&arena->rt, pgoff + mapped, page_cnt - mapped);
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
	if (mapped) {
		flush_vmap_cache(kern_vm_start + uaddr32, mapped << PAGE_SHIFT);
		arena_free_pages(arena, uaddr32, mapped, sleepable);
	}
	goto out_free_pages;
out_unlock_free_pages:
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
out_free_pages:
	kfree_nolock(pages);
	bpf_map_memcg_exit(old_memcg, new_memcg);
	return 0;
}

/*
 * If page is present in vmalloc area, unmap it from vmalloc area,
 * unmap it from all user space vma-s,
 * and free it.
 */
static void zap_pages(struct bpf_arena *arena, long uaddr, long page_cnt)
{
	struct vma_list *vml;

	guard(mutex)(&arena->lock);
	/* iterate link list under lock */
	list_for_each_entry(vml, &arena->vma_list, head)
		zap_vma_range(vml->vma, uaddr, PAGE_SIZE * page_cnt);
}