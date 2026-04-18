// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 12/36



static void kvm_update_flags_memslot(struct kvm *kvm,
				     struct kvm_memory_slot *old,
				     struct kvm_memory_slot *new)
{
	/*
	 * Similar to the MOVE case, but the slot doesn't need to be zapped as
	 * an intermediate step. Instead, the old memslot is simply replaced
	 * with a new, updated copy in both memslot sets.
	 */
	kvm_replace_memslot(kvm, old, new);
	kvm_activate_memslot(kvm, old, new);
}

static int kvm_set_memslot(struct kvm *kvm,
			   struct kvm_memory_slot *old,
			   struct kvm_memory_slot *new,
			   enum kvm_mr_change change)
{
	struct kvm_memory_slot *invalid_slot;
	int r;

	/*
	 * Released in kvm_swap_active_memslots().
	 *
	 * Must be held from before the current memslots are copied until after
	 * the new memslots are installed with rcu_assign_pointer, then
	 * released before the synchronize srcu in kvm_swap_active_memslots().
	 *
	 * When modifying memslots outside of the slots_lock, must be held
	 * before reading the pointer to the current memslots until after all
	 * changes to those memslots are complete.
	 *
	 * These rules ensure that installing new memslots does not lose
	 * changes made to the previous memslots.
	 */
	mutex_lock(&kvm->slots_arch_lock);

	/*
	 * Invalidate the old slot if it's being deleted or moved.  This is
	 * done prior to actually deleting/moving the memslot to allow vCPUs to
	 * continue running by ensuring there are no mappings or shadow pages
	 * for the memslot when it is deleted/moved.  Without pre-invalidation
	 * (and without a lock), a window would exist between effecting the
	 * delete/move and committing the changes in arch code where KVM or a
	 * guest could access a non-existent memslot.
	 *
	 * Modifications are done on a temporary, unreachable slot.  The old
	 * slot needs to be preserved in case a later step fails and the
	 * invalidation needs to be reverted.
	 */
	if (change == KVM_MR_DELETE || change == KVM_MR_MOVE) {
		invalid_slot = kzalloc_obj(*invalid_slot, GFP_KERNEL_ACCOUNT);
		if (!invalid_slot) {
			mutex_unlock(&kvm->slots_arch_lock);
			return -ENOMEM;
		}
		kvm_invalidate_memslot(kvm, old, invalid_slot);
	}

	r = kvm_prepare_memory_region(kvm, old, new, change);
	if (r) {
		/*
		 * For DELETE/MOVE, revert the above INVALID change.  No
		 * modifications required since the original slot was preserved
		 * in the inactive slots.  Changing the active memslots also
		 * release slots_arch_lock.
		 */
		if (change == KVM_MR_DELETE || change == KVM_MR_MOVE) {
			kvm_activate_memslot(kvm, invalid_slot, old);
			kfree(invalid_slot);
		} else {
			mutex_unlock(&kvm->slots_arch_lock);
		}
		return r;
	}

	/*
	 * For DELETE and MOVE, the working slot is now active as the INVALID
	 * version of the old slot.  MOVE is particularly special as it reuses
	 * the old slot and returns a copy of the old slot (in working_slot).
	 * For CREATE, there is no old slot.  For DELETE and FLAGS_ONLY, the
	 * old slot is detached but otherwise preserved.
	 */
	if (change == KVM_MR_CREATE)
		kvm_create_memslot(kvm, new);
	else if (change == KVM_MR_DELETE)
		kvm_delete_memslot(kvm, old, invalid_slot);
	else if (change == KVM_MR_MOVE)
		kvm_move_memslot(kvm, old, new, invalid_slot);
	else if (change == KVM_MR_FLAGS_ONLY)
		kvm_update_flags_memslot(kvm, old, new);
	else
		BUG();

	/* Free the temporary INVALID slot used for DELETE and MOVE. */
	if (change == KVM_MR_DELETE || change == KVM_MR_MOVE)
		kfree(invalid_slot);

	/*
	 * No need to refresh new->arch, changes after dropping slots_arch_lock
	 * will directly hit the final, active memslot.  Architectures are
	 * responsible for knowing that new->arch may be stale.
	 */
	kvm_commit_memory_region(kvm, old, new, change);

	return 0;
}

static bool kvm_check_memslot_overlap(struct kvm_memslots *slots, int id,
				      gfn_t start, gfn_t end)
{
	struct kvm_memslot_iter iter;

	kvm_for_each_memslot_in_gfn_range(&iter, slots, start, end) {
		if (iter.slot->id != id)
			return true;
	}

	return false;
}

static int kvm_set_memory_region(struct kvm *kvm,
				 const struct kvm_userspace_memory_region2 *mem)
{
	struct kvm_memory_slot *old, *new;
	struct kvm_memslots *slots;
	enum kvm_mr_change change;
	unsigned long npages;
	gfn_t base_gfn;
	int as_id, id;
	int r;

	lockdep_assert_held(&kvm->slots_lock);

	r = check_memory_region_flags(kvm, mem);
	if (r)
		return r;

	as_id = mem->slot >> 16;
	id = (u16)mem->slot;