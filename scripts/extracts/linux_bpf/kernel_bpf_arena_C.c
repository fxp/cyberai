// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 3/6



static vm_fault_t arena_vm_fault(struct vm_fault *vmf)
{
	struct bpf_map *map = vmf->vma->vm_file->private_data;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);
	struct mem_cgroup *new_memcg, *old_memcg;
	struct page *page;
	long kbase, kaddr;
	unsigned long flags;
	int ret;

	kbase = bpf_arena_get_kern_vm_start(arena);
	kaddr = kbase + (u32)(vmf->address);

	if (raw_res_spin_lock_irqsave(&arena->spinlock, flags))
		/* Make a reasonable effort to address impossible case */
		return VM_FAULT_RETRY;

	page = vmalloc_to_page((void *)kaddr);
	if (page)
		/* already have a page vmap-ed */
		goto out;

	bpf_map_memcg_enter(&arena->map, &old_memcg, &new_memcg);

	if (arena->map.map_flags & BPF_F_SEGV_ON_FAULT)
		/* User space requested to segfault when page is not allocated by bpf prog */
		goto out_unlock_sigsegv;

	ret = range_tree_clear(&arena->rt, vmf->pgoff, 1);
	if (ret)
		goto out_unlock_sigsegv;

	struct apply_range_data data = { .pages = &page, .i = 0 };
	/* Account into memcg of the process that created bpf_arena */
	ret = bpf_map_alloc_pages(map, NUMA_NO_NODE, 1, &page);
	if (ret) {
		range_tree_set(&arena->rt, vmf->pgoff, 1);
		goto out_unlock_sigsegv;
	}

	ret = apply_to_page_range(&init_mm, kaddr, PAGE_SIZE, apply_range_set_cb, &data);
	if (ret) {
		range_tree_set(&arena->rt, vmf->pgoff, 1);
		free_pages_nolock(page, 0);
		goto out_unlock_sigsegv;
	}
	flush_vmap_cache(kaddr, PAGE_SIZE);
	bpf_map_memcg_exit(old_memcg, new_memcg);
out:
	page_ref_add(page, 1);
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
	vmf->page = page;
	return 0;
out_unlock_sigsegv:
	bpf_map_memcg_exit(old_memcg, new_memcg);
	raw_res_spin_unlock_irqrestore(&arena->spinlock, flags);
	return VM_FAULT_SIGSEGV;
}

static const struct vm_operations_struct arena_vm_ops = {
	.open		= arena_vm_open,
	.may_split	= arena_vm_may_split,
	.mremap		= arena_vm_mremap,
	.close		= arena_vm_close,
	.fault          = arena_vm_fault,
};

static unsigned long arena_get_unmapped_area(struct file *filp, unsigned long addr,
					     unsigned long len, unsigned long pgoff,
					     unsigned long flags)
{
	struct bpf_map *map = filp->private_data;
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);
	long ret;

	if (pgoff)
		return -EINVAL;
	if (len > SZ_4G)
		return -E2BIG;

	/* if user_vm_start was specified at arena creation time */
	if (arena->user_vm_start) {
		if (len > arena->user_vm_end - arena->user_vm_start)
			return -E2BIG;
		if (len != arena->user_vm_end - arena->user_vm_start)
			return -EINVAL;
		if (addr != arena->user_vm_start)
			return -EINVAL;
	}

	ret = mm_get_unmapped_area(filp, addr, len * 2, 0, flags);
	if (IS_ERR_VALUE(ret))
		return ret;
	if ((ret >> 32) == ((ret + len - 1) >> 32))
		return ret;
	if (WARN_ON_ONCE(arena->user_vm_start))
		/* checks at map creation time should prevent this */
		return -EFAULT;
	return round_up(ret, SZ_4G);
}

static int arena_map_mmap(struct bpf_map *map, struct vm_area_struct *vma)
{
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	guard(mutex)(&arena->lock);
	if (arena->user_vm_start && arena->user_vm_start != vma->vm_start)
		/*
		 * If map_extra was not specified at arena creation time then
		 * 1st user process can do mmap(NULL, ...) to pick user_vm_start
		 * 2nd user process must pass the same addr to mmap(addr, MAP_FIXED..);
		 *   or
		 * specify addr in map_extra and
		 * use the same addr later with mmap(addr, MAP_FIXED..);
		 */
		return -EBUSY;

	if (arena->user_vm_end && arena->user_vm_end != vma->vm_end)
		/* all user processes must have the same size of mmap-ed region */
		return -EBUSY;

	/* Earlier checks should prevent this */
	if (WARN_ON_ONCE(vma->vm_end - vma->vm_start > SZ_4G || vma->vm_pgoff))
		return -EFAULT;

	if (remember_vma(arena, vma))
		return -ENOMEM;

	arena->user_vm_start = vma->vm_start;
	arena->user_vm_end = vma->vm_end;
	/*
	 * bpf_map_mmap() checks that it's being mmaped as VM_SHARED and
	 * clears VM_MAYEXEC. Set VM_DONTEXPAND to avoid potential change
	 * of user_vm_start. Set VM_DONTCOPY to prevent arena VMA from
	 * being copied into the child process on fork.
	 */
	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTCOPY);
	vma->vm_ops = &arena_vm_ops;
	return 0;
}

static int arena_map_direct_value_addr(const struct bpf_map *map, u64 *imm, u32 off)
{
	struct bpf_arena *arena = container_of(map, struct bpf_arena, map);

	if ((u64)off > arena->user_vm_end - arena->user_vm_start)
		return -ERANGE;
	*imm = (unsigned long)arena->user_vm_start;
	return 0;
}