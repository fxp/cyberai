// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 81/86



	nr_slots = atomic_read(&kvm->nr_memslots_dirty_logging);
	if ((enable && nr_slots == 1) || !nr_slots)
		kvm_make_all_cpus_request(kvm, KVM_REQ_UPDATE_CPU_DIRTY_LOGGING);
}

static void kvm_mmu_slot_apply_flags(struct kvm *kvm,
				     struct kvm_memory_slot *old,
				     const struct kvm_memory_slot *new,
				     enum kvm_mr_change change)
{
	u32 old_flags = old ? old->flags : 0;
	u32 new_flags = new ? new->flags : 0;
	bool log_dirty_pages = new_flags & KVM_MEM_LOG_DIRTY_PAGES;

	/*
	 * Update CPU dirty logging if dirty logging is being toggled.  This
	 * applies to all operations.
	 */
	if ((old_flags ^ new_flags) & KVM_MEM_LOG_DIRTY_PAGES)
		kvm_mmu_update_cpu_dirty_logging(kvm, log_dirty_pages);

	/*
	 * Nothing more to do for RO slots (which can't be dirtied and can't be
	 * made writable) or CREATE/MOVE/DELETE of a slot.
	 *
	 * For a memslot with dirty logging disabled:
	 * CREATE:      No dirty mappings will already exist.
	 * MOVE/DELETE: The old mappings will already have been cleaned up by
	 *		kvm_arch_flush_shadow_memslot()
	 *
	 * For a memslot with dirty logging enabled:
	 * CREATE:      No shadow pages exist, thus nothing to write-protect
	 *		and no dirty bits to clear.
	 * MOVE/DELETE: The old mappings will already have been cleaned up by
	 *		kvm_arch_flush_shadow_memslot().
	 */
	if ((change != KVM_MR_FLAGS_ONLY) || (new_flags & KVM_MEM_READONLY))
		return;

	/*
	 * READONLY and non-flags changes were filtered out above, and the only
	 * other flag is LOG_DIRTY_PAGES, i.e. something is wrong if dirty
	 * logging isn't being toggled on or off.
	 */
	if (WARN_ON_ONCE(!((old_flags ^ new_flags) & KVM_MEM_LOG_DIRTY_PAGES)))
		return;

	if (!log_dirty_pages) {
		/*
		 * Recover huge page mappings in the slot now that dirty logging
		 * is disabled, i.e. now that KVM does not have to track guest
		 * writes at 4KiB granularity.
		 *
		 * Dirty logging might be disabled by userspace if an ongoing VM
		 * live migration is cancelled and the VM must continue running
		 * on the source.
		 */
		kvm_mmu_recover_huge_pages(kvm, new);
	} else {
		/*
		 * Initially-all-set does not require write protecting any page,
		 * because they're all assumed to be dirty.
		 */
		if (kvm_dirty_log_manual_protect_and_init_set(kvm))
			return;

		if (READ_ONCE(eager_page_split))
			kvm_mmu_slot_try_split_huge_pages(kvm, new, PG_LEVEL_4K);

		if (kvm->arch.cpu_dirty_log_size) {
			kvm_mmu_slot_leaf_clear_dirty(kvm, new);
			kvm_mmu_slot_remove_write_access(kvm, new, PG_LEVEL_2M);
		} else {
			kvm_mmu_slot_remove_write_access(kvm, new, PG_LEVEL_4K);
		}

		/*
		 * Unconditionally flush the TLBs after enabling dirty logging.
		 * A flush is almost always going to be necessary (see below),
		 * and unconditionally flushing allows the helpers to omit
		 * the subtly complex checks when removing write access.
		 *
		 * Do the flush outside of mmu_lock to reduce the amount of
		 * time mmu_lock is held.  Flushing after dropping mmu_lock is
		 * safe as KVM only needs to guarantee the slot is fully
		 * write-protected before returning to userspace, i.e. before
		 * userspace can consume the dirty status.
		 *
		 * Flushing outside of mmu_lock requires KVM to be careful when
		 * making decisions based on writable status of an SPTE, e.g. a
		 * !writable SPTE doesn't guarantee a CPU can't perform writes.
		 *
		 * Specifically, KVM also write-protects guest page tables to
		 * monitor changes when using shadow paging, and must guarantee
		 * no CPUs can write to those page before mmu_lock is dropped.
		 * Because CPUs may have stale TLB entries at this point, a
		 * !writable SPTE doesn't guarantee CPUs can't perform writes.
		 *
		 * KVM also allows making SPTES writable outside of mmu_lock,
		 * e.g. to allow dirty logging without taking mmu_lock.
		 *
		 * To handle these scenarios, KVM uses a separate software-only
		 * bit (MMU-writable) to track if a SPTE is !writable due to
		 * a guest page table being write-protected (KVM clears the
		 * MMU-writable flag when write-protecting for shadow paging).
		 *
		 * The use of MMU-writable is also the primary motivation for
		 * the unconditional flush.  Because KVM must guarantee that a
		 * CPU doesn't contain stale, writable TLB entries for a
		 * !MMU-writable SPTE, KVM must flush if it encounters any
		 * MMU-writable SPTE regardless of whether the actual hardware
		 * writable bit was set.  I.e. KVM is almost guaranteed to need
		 * to flush, while unconditionally flushing allows the "remove
		 * write access" helpers to ignore MMU-writable entirely.
		 *
		 * See is_writable_pte() for more details (the case involving
		 * access-tracked SPTEs is particularly relevant).
		 */
		kvm_flush_remote_tlbs_memslot(kvm, new);
	}
}

void kvm_arch_commit_memory_region(struct kvm *kvm,
				struct kvm_memory_slot *old,
				const struct kvm_memory_slot *new,
				enum kvm_mr_change change)
{
	if (change == KVM_MR_DELETE)
		kvm_page_track_delete_slot(kvm, old);