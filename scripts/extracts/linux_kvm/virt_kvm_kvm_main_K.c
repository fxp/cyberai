// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 11/36



	kvm_arch_commit_memory_region(kvm, old, new, change);

	switch (change) {
	case KVM_MR_CREATE:
		/* Nothing more to do. */
		break;
	case KVM_MR_DELETE:
		/* Free the old memslot and all its metadata. */
		kvm_free_memslot(kvm, old);
		break;
	case KVM_MR_MOVE:
		/*
		 * Moving a guest_memfd memslot isn't supported, and will never
		 * be supported.
		 */
		WARN_ON_ONCE(old->flags & KVM_MEM_GUEST_MEMFD);
		fallthrough;
	case KVM_MR_FLAGS_ONLY:
		/*
		 * Free the dirty bitmap as needed; the below check encompasses
		 * both the flags and whether a ring buffer is being used)
		 */
		if (old->dirty_bitmap && !new->dirty_bitmap)
			kvm_destroy_dirty_bitmap(old);

		/*
		 * Unbind the guest_memfd instance as needed; the @new slot has
		 * already created its own binding.  TODO: Drop the WARN when
		 * dirty logging guest_memfd memslots is supported.  Until then,
		 * flags-only changes on guest_memfd slots should be impossible.
		 */
		if (WARN_ON_ONCE(old->flags & KVM_MEM_GUEST_MEMFD))
			kvm_gmem_unbind(old);

		/*
		 * The final quirk.  Free the detached, old slot, but only its
		 * memory, not any metadata.  Metadata, including arch specific
		 * data, may be reused by @new.
		 */
		kfree(old);
		break;
	default:
		BUG();
	}
}

/*
 * Activate @new, which must be installed in the inactive slots by the caller,
 * by swapping the active slots and then propagating @new to @old once @old is
 * unreachable and can be safely modified.
 *
 * With NULL @old this simply adds @new to @active (while swapping the sets).
 * With NULL @new this simply removes @old from @active and frees it
 * (while also swapping the sets).
 */
static void kvm_activate_memslot(struct kvm *kvm,
				 struct kvm_memory_slot *old,
				 struct kvm_memory_slot *new)
{
	int as_id = kvm_memslots_get_as_id(old, new);

	kvm_swap_active_memslots(kvm, as_id);

	/* Propagate the new memslot to the now inactive memslots. */
	kvm_replace_memslot(kvm, old, new);
}

static void kvm_copy_memslot(struct kvm_memory_slot *dest,
			     const struct kvm_memory_slot *src)
{
	dest->base_gfn = src->base_gfn;
	dest->npages = src->npages;
	dest->dirty_bitmap = src->dirty_bitmap;
	dest->arch = src->arch;
	dest->userspace_addr = src->userspace_addr;
	dest->flags = src->flags;
	dest->id = src->id;
	dest->as_id = src->as_id;
}

static void kvm_invalidate_memslot(struct kvm *kvm,
				   struct kvm_memory_slot *old,
				   struct kvm_memory_slot *invalid_slot)
{
	/*
	 * Mark the current slot INVALID.  As with all memslot modifications,
	 * this must be done on an unreachable slot to avoid modifying the
	 * current slot in the active tree.
	 */
	kvm_copy_memslot(invalid_slot, old);
	invalid_slot->flags |= KVM_MEMSLOT_INVALID;
	kvm_replace_memslot(kvm, old, invalid_slot);

	/*
	 * Activate the slot that is now marked INVALID, but don't propagate
	 * the slot to the now inactive slots. The slot is either going to be
	 * deleted or recreated as a new slot.
	 */
	kvm_swap_active_memslots(kvm, old->as_id);

	/*
	 * From this point no new shadow pages pointing to a deleted, or moved,
	 * memslot will be created.  Validation of sp->gfn happens in:
	 *	- gfn_to_hva (kvm_read_guest, gfn_to_pfn)
	 *	- kvm_is_visible_gfn (mmu_check_root)
	 */
	kvm_arch_flush_shadow_memslot(kvm, old);
	kvm_arch_guest_memory_reclaimed(kvm);

	/* Was released by kvm_swap_active_memslots(), reacquire. */
	mutex_lock(&kvm->slots_arch_lock);

	/*
	 * Copy the arch-specific field of the newly-installed slot back to the
	 * old slot as the arch data could have changed between releasing
	 * slots_arch_lock in kvm_swap_active_memslots() and re-acquiring the lock
	 * above.  Writers are required to retrieve memslots *after* acquiring
	 * slots_arch_lock, thus the active slot's data is guaranteed to be fresh.
	 */
	old->arch = invalid_slot->arch;
}

static void kvm_create_memslot(struct kvm *kvm,
			       struct kvm_memory_slot *new)
{
	/* Add the new memslot to the inactive set and activate. */
	kvm_replace_memslot(kvm, NULL, new);
	kvm_activate_memslot(kvm, NULL, new);
}

static void kvm_delete_memslot(struct kvm *kvm,
			       struct kvm_memory_slot *old,
			       struct kvm_memory_slot *invalid_slot)
{
	/*
	 * Remove the old memslot (in the inactive memslots) by passing NULL as
	 * the "new" slot, and for the invalid version in the active slots.
	 */
	kvm_replace_memslot(kvm, old, NULL);
	kvm_activate_memslot(kvm, invalid_slot, NULL);
}

static void kvm_move_memslot(struct kvm *kvm,
			     struct kvm_memory_slot *old,
			     struct kvm_memory_slot *new,
			     struct kvm_memory_slot *invalid_slot)
{
	/*
	 * Replace the old memslot in the inactive slots, and then swap slots
	 * and replace the current INVALID with the new as well.
	 */
	kvm_replace_memslot(kvm, old, new);
	kvm_activate_memslot(kvm, invalid_slot, new);
}