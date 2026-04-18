// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 2/6



	if (attr->key_size || attr->value_size || attr->max_entries == 0 ||
	    /* BPF_F_MMAPABLE must be set */
	    !(attr->map_flags & BPF_F_MMAPABLE) ||
	    /* No unsupported flags present */
	    (attr->map_flags & ~(BPF_F_SEGV_ON_FAULT | BPF_F_MMAPABLE | BPF_F_NO_USER_CONV)))
		return ERR_PTR(-EINVAL);

	if (attr->map_extra & ~PAGE_MASK)
		/* If non-zero the map_extra is an expected user VMA start address */
		return ERR_PTR(-EINVAL);

	vm_range = (u64)attr->max_entries * PAGE_SIZE;
	if (vm_range > SZ_4G)
		return ERR_PTR(-E2BIG);

	if ((attr->map_extra >> 32) != ((attr->map_extra + vm_range - 1) >> 32))
		/* user vma must not cross 32-bit boundary */
		return ERR_PTR(-ERANGE);

	kern_vm = get_vm_area(KERN_VM_SZ, VM_SPARSE | VM_USERMAP);
	if (!kern_vm)
		return ERR_PTR(-ENOMEM);

	arena = bpf_map_area_alloc(sizeof(*arena), numa_node);
	if (!arena)
		goto err;

	arena->kern_vm = kern_vm;
	arena->user_vm_start = attr->map_extra;
	if (arena->user_vm_start)
		arena->user_vm_end = arena->user_vm_start + vm_range;

	INIT_LIST_HEAD(&arena->vma_list);
	init_llist_head(&arena->free_spans);
	init_irq_work(&arena->free_irq, arena_free_irq);
	INIT_WORK(&arena->free_work, arena_free_worker);
	bpf_map_init_from_attr(&arena->map, attr);
	range_tree_init(&arena->rt);
	err = range_tree_set(&arena->rt, 0, attr->max_entries);
	if (err) {
		bpf_map_area_free(arena);
		goto err;
	}
	mutex_init(&arena->lock);
	raw_res_spin_lock_init(&arena->spinlock);
	err = populate_pgtable_except_pte(arena);
	if (err) {
		range_tree_destroy(&arena->rt);
		bpf_map_area_free(arena);
		goto err;
	}

	return &arena->map;
err:
	free_vm_area(kern_vm);
	return ERR_PTR(err);
}

static int existing_page_cb(pte_t *ptep, unsigned long addr, void *data)
{
	struct page *page;
	pte_t pte;

	pte = ptep_get(ptep);
	if (!pte_present(pte)) /* sanity check */
		return 0;
	page = pte_page(pte);
	/*
	 * We do not update pte here:
	 * 1. Nobody should be accessing bpf_arena's range outside of a kernel bug
	 * 2. TLB flushing is batched or deferred. Even if we clear pte,
	 * the TLB entries can stick around and continue to permit access to
	 * the freed page. So it all relies on 1.
	 */
	__free_page(page);
	return 0;
}

static void arena_map_free(struct bpf_map *map)
{
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	/*
	 * Check that user vma-s are not around when bpf map is freed.
	 * mmap() holds vm_file which holds bpf_map refcnt.
	 * munmap() must have happened on vma followed by arena_vm_close()
	 * which would clear arena->vma_list.
	 */
	if (WARN_ON_ONCE(!list_empty(&arena->vma_list)))
		return;

	/* Ensure no pending deferred frees */
	irq_work_sync(&arena->free_irq);
	flush_work(&arena->free_work);

	/*
	 * free_vm_area() calls remove_vm_area() that calls free_unmap_vmap_area().
	 * It unmaps everything from vmalloc area and clears pgtables.
	 * Call apply_to_existing_page_range() first to find populated ptes and
	 * free those pages.
	 */
	apply_to_existing_page_range(&init_mm, bpf_arena_get_kern_vm_start(arena),
				     KERN_VM_SZ - GUARD_SZ, existing_page_cb, NULL);
	free_vm_area(arena->kern_vm);
	range_tree_destroy(&arena->rt);
	bpf_map_area_free(arena);
}

static void *arena_map_lookup_elem(struct bpf_map *map, void *key)
{
	return ERR_PTR(-EINVAL);
}

static long arena_map_update_elem(struct bpf_map *map, void *key,
				  void *value, u64 flags)
{
	return -EOPNOTSUPP;
}

static int arena_map_check_btf(struct bpf_map *map, const struct btf *btf,
			       const struct btf_type *key_type, const struct btf_type *value_type)
{
	return 0;
}

static u64 arena_map_mem_usage(const struct bpf_map *map)
{
	return 0;
}

struct vma_list {
	struct vm_area_struct *vma;
	struct list_head head;
	refcount_t mmap_count;
};

static int remember_vma(struct bpf_arena *arena, struct vm_area_struct *vma)
{
	struct vma_list *vml;

	vml = kmalloc_obj(*vml);
	if (!vml)
		return -ENOMEM;
	refcount_set(&vml->mmap_count, 1);
	vma->vm_private_data = vml;
	vml->vma = vma;
	list_add(&vml->head, &arena->vma_list);
	return 0;
}

static void arena_vm_open(struct vm_area_struct *vma)
{
	struct vma_list *vml = vma->vm_private_data;

	refcount_inc(&vml->mmap_count);
}

static int arena_vm_may_split(struct vm_area_struct *vma, unsigned long addr)
{
	return -EINVAL;
}

static int arena_vm_mremap(struct vm_area_struct *vma)
{
	return -EINVAL;
}

static void arena_vm_close(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);
	struct vma_list *vml = vma->vm_private_data;

	if (!refcount_dec_and_test(&vml->mmap_count))
		return;
	guard(mutex)(&arena->lock);
	/* update link list under lock */
	list_del(&vml->head);
	vma->vm_private_data = NULL;
	kfree(vml);
}