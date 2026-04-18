// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 9/36



out_unlock:
	kvm_for_each_vcpu(j, vcpu, kvm) {
		if (i == j)
			break;
		mutex_unlock(&vcpu->mutex);
	}
	return -EINTR;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_trylock_all_vcpus);

int kvm_lock_all_vcpus(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i, j;
	int r;

	lockdep_assert_held(&kvm->lock);

	kvm_for_each_vcpu(i, vcpu, kvm) {
		r = mutex_lock_killable_nest_lock(&vcpu->mutex, &kvm->lock);
		if (r)
			goto out_unlock;
	}
	return 0;

out_unlock:
	kvm_for_each_vcpu(j, vcpu, kvm) {
		if (i == j)
			break;
		mutex_unlock(&vcpu->mutex);
	}
	return r;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_lock_all_vcpus);

void kvm_unlock_all_vcpus(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;

	lockdep_assert_held(&kvm->lock);

	kvm_for_each_vcpu(i, vcpu, kvm)
		mutex_unlock(&vcpu->mutex);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_unlock_all_vcpus);

/*
 * Allocation size is twice as large as the actual dirty bitmap size.
 * See kvm_vm_ioctl_get_dirty_log() why this is needed.
 */
static int kvm_alloc_dirty_bitmap(struct kvm_memory_slot *memslot)
{
	unsigned long dirty_bytes = kvm_dirty_bitmap_bytes(memslot);

	memslot->dirty_bitmap = __vcalloc(2, dirty_bytes, GFP_KERNEL_ACCOUNT);
	if (!memslot->dirty_bitmap)
		return -ENOMEM;

	return 0;
}

static struct kvm_memslots *kvm_get_inactive_memslots(struct kvm *kvm, int as_id)
{
	struct kvm_memslots *active = __kvm_memslots(kvm, as_id);
	int node_idx_inactive = active->node_idx ^ 1;

	return &kvm->__memslots[as_id][node_idx_inactive];
}

/*
 * Helper to get the address space ID when one of memslot pointers may be NULL.
 * This also serves as a sanity that at least one of the pointers is non-NULL,
 * and that their address space IDs don't diverge.
 */
static int kvm_memslots_get_as_id(struct kvm_memory_slot *a,
				  struct kvm_memory_slot *b)
{
	if (WARN_ON_ONCE(!a && !b))
		return 0;

	if (!a)
		return b->as_id;
	if (!b)
		return a->as_id;

	WARN_ON_ONCE(a->as_id != b->as_id);
	return a->as_id;
}

static void kvm_insert_gfn_node(struct kvm_memslots *slots,
				struct kvm_memory_slot *slot)
{
	struct rb_root *gfn_tree = &slots->gfn_tree;
	struct rb_node **node, *parent;
	int idx = slots->node_idx;

	parent = NULL;
	for (node = &gfn_tree->rb_node; *node; ) {
		struct kvm_memory_slot *tmp;

		tmp = container_of(*node, struct kvm_memory_slot, gfn_node[idx]);
		parent = *node;
		if (slot->base_gfn < tmp->base_gfn)
			node = &(*node)->rb_left;
		else if (slot->base_gfn > tmp->base_gfn)
			node = &(*node)->rb_right;
		else
			BUG();
	}

	rb_link_node(&slot->gfn_node[idx], parent, node);
	rb_insert_color(&slot->gfn_node[idx], gfn_tree);
}

static void kvm_erase_gfn_node(struct kvm_memslots *slots,
			       struct kvm_memory_slot *slot)
{
	rb_erase(&slot->gfn_node[slots->node_idx], &slots->gfn_tree);
}

static void kvm_replace_gfn_node(struct kvm_memslots *slots,
				 struct kvm_memory_slot *old,
				 struct kvm_memory_slot *new)
{
	int idx = slots->node_idx;

	WARN_ON_ONCE(old->base_gfn != new->base_gfn);

	rb_replace_node(&old->gfn_node[idx], &new->gfn_node[idx],
			&slots->gfn_tree);
}

/*
 * Replace @old with @new in the inactive memslots.
 *
 * With NULL @old this simply adds @new.
 * With NULL @new this simply removes @old.
 *
 * If @new is non-NULL its hva_node[slots_idx] range has to be set
 * appropriately.
 */
static void kvm_replace_memslot(struct kvm *kvm,
				struct kvm_memory_slot *old,
				struct kvm_memory_slot *new)
{
	int as_id = kvm_memslots_get_as_id(old, new);
	struct kvm_memslots *slots = kvm_get_inactive_memslots(kvm, as_id);
	int idx = slots->node_idx;

	if (old) {
		hash_del(&old->id_node[idx]);
		interval_tree_remove(&old->hva_node[idx], &slots->hva_tree);

		if ((long)old == atomic_long_read(&slots->last_used_slot))
			atomic_long_set(&slots->last_used_slot, (long)new);

		if (!new) {
			kvm_erase_gfn_node(slots, old);
			return;
		}
	}

	/*
	 * Initialize @new's hva range.  Do this even when replacing an @old
	 * slot, kvm_copy_memslot() deliberately does not touch node data.
	 */
	new->hva_node[idx].start = new->userspace_addr;
	new->hva_node[idx].last = new->userspace_addr +
				  (new->npages << PAGE_SHIFT) - 1;

	/*
	 * (Re)Add the new memslot.  There is no O(1) interval_tree_replace(),
	 * hva_node needs to be swapped with remove+insert even though hva can't
	 * change when replacing an existing slot.
	 */
	hash_add(slots->id_hash, &new->id_node[idx], new->id);
	interval_tree_insert(&new->hva_node[idx], &slots->hva_tree);

	/*
	 * If the memslot gfn is unchanged, rb_replace_node() can be used to
	 * switch the node in the gfn tree instead of removing the old and
	 * inserting the new as two separate operations. Replacement is a
	 * single O(1) operation versus two O(log(n)) operations for
	 * remove+insert.
	 */
	if (old && old->base_gfn == new->base_gfn) {
		kvm_replace_gfn_node(slots, old, new);
	} else {
		if (old)
			kvm_erase_gfn_node(slots, old);
		kvm_insert_gfn_node(slots, new);
	}
}