// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 5/36



	if (likely(kvm->mmu_invalidate_range_start == INVALID_GPA)) {
		kvm->mmu_invalidate_range_start = start;
		kvm->mmu_invalidate_range_end = end;
	} else {
		/*
		 * Fully tracking multiple concurrent ranges has diminishing
		 * returns. Keep things simple and just find the minimal range
		 * which includes the current and new ranges. As there won't be
		 * enough information to subtract a range after its invalidate
		 * completes, any ranges invalidated concurrently will
		 * accumulate and persist until all outstanding invalidates
		 * complete.
		 */
		kvm->mmu_invalidate_range_start =
			min(kvm->mmu_invalidate_range_start, start);
		kvm->mmu_invalidate_range_end =
			max(kvm->mmu_invalidate_range_end, end);
	}
}

bool kvm_mmu_unmap_gfn_range(struct kvm *kvm, struct kvm_gfn_range *range)
{
	kvm_mmu_invalidate_range_add(kvm, range->start, range->end);
	return kvm_unmap_gfn_range(kvm, range);
}

static int kvm_mmu_notifier_invalidate_range_start(struct mmu_notifier *mn,
					const struct mmu_notifier_range *range)
{
	struct kvm *kvm = mmu_notifier_to_kvm(mn);
	const struct kvm_mmu_notifier_range hva_range = {
		.start		= range->start,
		.end		= range->end,
		.handler	= kvm_mmu_unmap_gfn_range,
		.on_lock	= kvm_mmu_invalidate_begin,
		.flush_on_ret	= true,
		.may_block	= mmu_notifier_range_blockable(range),
	};

	trace_kvm_unmap_hva_range(range->start, range->end);

	/*
	 * Prevent memslot modification between range_start() and range_end()
	 * so that conditionally locking provides the same result in both
	 * functions.  Without that guarantee, the mmu_invalidate_in_progress
	 * adjustments will be imbalanced.
	 *
	 * Pairs with the decrement in range_end().
	 */
	spin_lock(&kvm->mn_invalidate_lock);
	kvm->mn_active_invalidate_count++;
	spin_unlock(&kvm->mn_invalidate_lock);

	/*
	 * Invalidate pfn caches _before_ invalidating the secondary MMUs, i.e.
	 * before acquiring mmu_lock, to avoid holding mmu_lock while acquiring
	 * each cache's lock.  There are relatively few caches in existence at
	 * any given time, and the caches themselves can check for hva overlap,
	 * i.e. don't need to rely on memslot overlap checks for performance.
	 * Because this runs without holding mmu_lock, the pfn caches must use
	 * mn_active_invalidate_count (see above) instead of
	 * mmu_invalidate_in_progress.
	 */
	gfn_to_pfn_cache_invalidate_start(kvm, range->start, range->end);

	/*
	 * If one or more memslots were found and thus zapped, notify arch code
	 * that guest memory has been reclaimed.  This needs to be done *after*
	 * dropping mmu_lock, as x86's reclaim path is slooooow.
	 */
	if (kvm_handle_hva_range(kvm, &hva_range).found_memslot)
		kvm_arch_guest_memory_reclaimed(kvm);

	return 0;
}

void kvm_mmu_invalidate_end(struct kvm *kvm)
{
	lockdep_assert_held_write(&kvm->mmu_lock);

	/*
	 * This sequence increase will notify the kvm page fault that
	 * the page that is going to be mapped in the spte could have
	 * been freed.
	 */
	kvm->mmu_invalidate_seq++;
	smp_wmb();
	/*
	 * The above sequence increase must be visible before the
	 * below count decrease, which is ensured by the smp_wmb above
	 * in conjunction with the smp_rmb in mmu_invalidate_retry().
	 */
	kvm->mmu_invalidate_in_progress--;
	KVM_BUG_ON(kvm->mmu_invalidate_in_progress < 0, kvm);

	/*
	 * Assert that at least one range was added between start() and end().
	 * Not adding a range isn't fatal, but it is a KVM bug.
	 */
	WARN_ON_ONCE(kvm->mmu_invalidate_range_start == INVALID_GPA);
}

static void kvm_mmu_notifier_invalidate_range_end(struct mmu_notifier *mn,
					const struct mmu_notifier_range *range)
{
	struct kvm *kvm = mmu_notifier_to_kvm(mn);
	const struct kvm_mmu_notifier_range hva_range = {
		.start		= range->start,
		.end		= range->end,
		.handler	= (void *)kvm_null_fn,
		.on_lock	= kvm_mmu_invalidate_end,
		.flush_on_ret	= false,
		.may_block	= mmu_notifier_range_blockable(range),
	};
	bool wake;

	kvm_handle_hva_range(kvm, &hva_range);

	/* Pairs with the increment in range_start(). */
	spin_lock(&kvm->mn_invalidate_lock);
	if (!WARN_ON_ONCE(!kvm->mn_active_invalidate_count))
		--kvm->mn_active_invalidate_count;
	wake = !kvm->mn_active_invalidate_count;
	spin_unlock(&kvm->mn_invalidate_lock);

	/*
	 * There can only be one waiter, since the wait happens under
	 * slots_lock.
	 */
	if (wake)
		rcuwait_wake_up(&kvm->mn_memslots_update_rcuwait);
}

static bool kvm_mmu_notifier_clear_flush_young(struct mmu_notifier *mn,
		struct mm_struct *mm, unsigned long start, unsigned long end)
{
	trace_kvm_age_hva(start, end);

	return kvm_age_hva_range(mn, start, end, kvm_age_gfn,
				 !IS_ENABLED(CONFIG_KVM_ELIDE_TLB_FLUSH_IF_YOUNG));
}

static bool kvm_mmu_notifier_clear_young(struct mmu_notifier *mn,
		struct mm_struct *mm, unsigned long start, unsigned long end)
{
	trace_kvm_age_hva(start, end);