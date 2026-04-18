// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 10/36



/*
 * Flags that do not access any of the extra space of struct
 * kvm_userspace_memory_region2.  KVM_SET_USER_MEMORY_REGION_V1_FLAGS
 * only allows these.
 */
#define KVM_SET_USER_MEMORY_REGION_V1_FLAGS \
	(KVM_MEM_LOG_DIRTY_PAGES | KVM_MEM_READONLY)

static int check_memory_region_flags(struct kvm *kvm,
				     const struct kvm_userspace_memory_region2 *mem)
{
	u32 valid_flags = KVM_MEM_LOG_DIRTY_PAGES;

	if (IS_ENABLED(CONFIG_KVM_GUEST_MEMFD))
		valid_flags |= KVM_MEM_GUEST_MEMFD;

	/* Dirty logging private memory is not currently supported. */
	if (mem->flags & KVM_MEM_GUEST_MEMFD)
		valid_flags &= ~KVM_MEM_LOG_DIRTY_PAGES;

	/*
	 * GUEST_MEMFD is incompatible with read-only memslots, as writes to
	 * read-only memslots have emulated MMIO, not page fault, semantics,
	 * and KVM doesn't allow emulated MMIO for private memory.
	 */
	if (kvm_arch_has_readonly_mem(kvm) &&
	    !(mem->flags & KVM_MEM_GUEST_MEMFD))
		valid_flags |= KVM_MEM_READONLY;

	if (mem->flags & ~valid_flags)
		return -EINVAL;

	return 0;
}

static void kvm_swap_active_memslots(struct kvm *kvm, int as_id)
{
	struct kvm_memslots *slots = kvm_get_inactive_memslots(kvm, as_id);

	/* Grab the generation from the activate memslots. */
	u64 gen = __kvm_memslots(kvm, as_id)->generation;

	WARN_ON(gen & KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS);
	slots->generation = gen | KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS;

	/*
	 * Do not store the new memslots while there are invalidations in
	 * progress, otherwise the locking in invalidate_range_start and
	 * invalidate_range_end will be unbalanced.
	 */
	spin_lock(&kvm->mn_invalidate_lock);
	prepare_to_rcuwait(&kvm->mn_memslots_update_rcuwait);
	while (kvm->mn_active_invalidate_count) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		spin_unlock(&kvm->mn_invalidate_lock);
		schedule();
		spin_lock(&kvm->mn_invalidate_lock);
	}
	finish_rcuwait(&kvm->mn_memslots_update_rcuwait);
	rcu_assign_pointer(kvm->memslots[as_id], slots);
	spin_unlock(&kvm->mn_invalidate_lock);

	/*
	 * Acquired in kvm_set_memslot. Must be released before synchronize
	 * SRCU below in order to avoid deadlock with another thread
	 * acquiring the slots_arch_lock in an srcu critical section.
	 */
	mutex_unlock(&kvm->slots_arch_lock);

	synchronize_srcu_expedited(&kvm->srcu);

	/*
	 * Increment the new memslot generation a second time, dropping the
	 * update in-progress flag and incrementing the generation based on
	 * the number of address spaces.  This provides a unique and easily
	 * identifiable generation number while the memslots are in flux.
	 */
	gen = slots->generation & ~KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS;

	/*
	 * Generations must be unique even across address spaces.  We do not need
	 * a global counter for that, instead the generation space is evenly split
	 * across address spaces.  For example, with two address spaces, address
	 * space 0 will use generations 0, 2, 4, ... while address space 1 will
	 * use generations 1, 3, 5, ...
	 */
	gen += kvm_arch_nr_memslot_as_ids(kvm);

	kvm_arch_memslots_updated(kvm, gen);

	slots->generation = gen;
}

static int kvm_prepare_memory_region(struct kvm *kvm,
				     const struct kvm_memory_slot *old,
				     struct kvm_memory_slot *new,
				     enum kvm_mr_change change)
{
	int r;

	/*
	 * If dirty logging is disabled, nullify the bitmap; the old bitmap
	 * will be freed on "commit".  If logging is enabled in both old and
	 * new, reuse the existing bitmap.  If logging is enabled only in the
	 * new and KVM isn't using a ring buffer, allocate and initialize a
	 * new bitmap.
	 */
	if (change != KVM_MR_DELETE) {
		if (!(new->flags & KVM_MEM_LOG_DIRTY_PAGES))
			new->dirty_bitmap = NULL;
		else if (old && old->dirty_bitmap)
			new->dirty_bitmap = old->dirty_bitmap;
		else if (kvm_use_dirty_bitmap(kvm)) {
			r = kvm_alloc_dirty_bitmap(new);
			if (r)
				return r;

			if (kvm_dirty_log_manual_protect_and_init_set(kvm))
				bitmap_set(new->dirty_bitmap, 0, new->npages);
		}
	}

	r = kvm_arch_prepare_memory_region(kvm, old, new, change);

	/* Free the bitmap on failure if it was allocated above. */
	if (r && new && new->dirty_bitmap && (!old || !old->dirty_bitmap))
		kvm_destroy_dirty_bitmap(new);

	return r;
}

static void kvm_commit_memory_region(struct kvm *kvm,
				     struct kvm_memory_slot *old,
				     const struct kvm_memory_slot *new,
				     enum kvm_mr_change change)
{
	int old_flags = old ? old->flags : 0;
	int new_flags = new ? new->flags : 0;
	/*
	 * Update the total number of memslot pages before calling the arch
	 * hook so that architectures can consume the result directly.
	 */
	if (change == KVM_MR_DELETE)
		kvm->nr_memslot_pages -= old->npages;
	else if (change == KVM_MR_CREATE)
		kvm->nr_memslot_pages += new->npages;

	if ((old_flags ^ new_flags) & KVM_MEM_LOG_DIRTY_PAGES) {
		int change = (new_flags & KVM_MEM_LOG_DIRTY_PAGES) ? 1 : -1;
		atomic_set(&kvm->nr_memslots_dirty_logging,
			   atomic_read(&kvm->nr_memslots_dirty_logging) + change);
	}