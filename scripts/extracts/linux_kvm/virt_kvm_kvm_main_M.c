// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 13/36



	/* General sanity checks */
	if ((mem->memory_size & (PAGE_SIZE - 1)) ||
	    (mem->memory_size != (unsigned long)mem->memory_size))
		return -EINVAL;
	if (mem->guest_phys_addr & (PAGE_SIZE - 1))
		return -EINVAL;
	/* We can read the guest memory with __xxx_user() later on. */
	if ((mem->userspace_addr & (PAGE_SIZE - 1)) ||
	    (mem->userspace_addr != untagged_addr(mem->userspace_addr)) ||
	     !access_ok((void __user *)(unsigned long)mem->userspace_addr,
			mem->memory_size))
		return -EINVAL;
	if (mem->flags & KVM_MEM_GUEST_MEMFD &&
	    (mem->guest_memfd_offset & (PAGE_SIZE - 1) ||
	     mem->guest_memfd_offset + mem->memory_size < mem->guest_memfd_offset))
		return -EINVAL;
	if (as_id >= kvm_arch_nr_memslot_as_ids(kvm) || id >= KVM_MEM_SLOTS_NUM)
		return -EINVAL;
	if (mem->guest_phys_addr + mem->memory_size < mem->guest_phys_addr)
		return -EINVAL;

	/*
	 * The size of userspace-defined memory regions is restricted in order
	 * to play nice with dirty bitmap operations, which are indexed with an
	 * "unsigned int".  KVM's internal memory regions don't support dirty
	 * logging, and so are exempt.
	 */
	if (id < KVM_USER_MEM_SLOTS &&
	    (mem->memory_size >> PAGE_SHIFT) > KVM_MEM_MAX_NR_PAGES)
		return -EINVAL;

	slots = __kvm_memslots(kvm, as_id);

	/*
	 * Note, the old memslot (and the pointer itself!) may be invalidated
	 * and/or destroyed by kvm_set_memslot().
	 */
	old = id_to_memslot(slots, id);

	if (!mem->memory_size) {
		if (!old || !old->npages)
			return -EINVAL;

		if (WARN_ON_ONCE(kvm->nr_memslot_pages < old->npages))
			return -EIO;

		return kvm_set_memslot(kvm, old, NULL, KVM_MR_DELETE);
	}

	base_gfn = (mem->guest_phys_addr >> PAGE_SHIFT);
	npages = (mem->memory_size >> PAGE_SHIFT);

	if (!old || !old->npages) {
		change = KVM_MR_CREATE;

		/*
		 * To simplify KVM internals, the total number of pages across
		 * all memslots must fit in an unsigned long.
		 */
		if ((kvm->nr_memslot_pages + npages) < kvm->nr_memslot_pages)
			return -EINVAL;
	} else { /* Modify an existing slot. */
		/* Private memslots are immutable, they can only be deleted. */
		if (mem->flags & KVM_MEM_GUEST_MEMFD)
			return -EINVAL;
		if ((mem->userspace_addr != old->userspace_addr) ||
		    (npages != old->npages) ||
		    ((mem->flags ^ old->flags) & (KVM_MEM_READONLY | KVM_MEM_GUEST_MEMFD)))
			return -EINVAL;

		if (base_gfn != old->base_gfn)
			change = KVM_MR_MOVE;
		else if (mem->flags != old->flags)
			change = KVM_MR_FLAGS_ONLY;
		else /* Nothing to change. */
			return 0;
	}

	if ((change == KVM_MR_CREATE || change == KVM_MR_MOVE) &&
	    kvm_check_memslot_overlap(slots, id, base_gfn, base_gfn + npages))
		return -EEXIST;

	/* Allocate a slot that will persist in the memslot. */
	new = kzalloc_obj(*new, GFP_KERNEL_ACCOUNT);
	if (!new)
		return -ENOMEM;

	new->as_id = as_id;
	new->id = id;
	new->base_gfn = base_gfn;
	new->npages = npages;
	new->flags = mem->flags;
	new->userspace_addr = mem->userspace_addr;
	if (mem->flags & KVM_MEM_GUEST_MEMFD) {
		r = kvm_gmem_bind(kvm, new, mem->guest_memfd, mem->guest_memfd_offset);
		if (r)
			goto out;
	}

	r = kvm_set_memslot(kvm, old, new, change);
	if (r)
		goto out_unbind;

	return 0;

out_unbind:
	if (mem->flags & KVM_MEM_GUEST_MEMFD)
		kvm_gmem_unbind(new);
out:
	kfree(new);
	return r;
}

int kvm_set_internal_memslot(struct kvm *kvm,
			     const struct kvm_userspace_memory_region2 *mem)
{
	if (WARN_ON_ONCE(mem->slot < KVM_USER_MEM_SLOTS))
		return -EINVAL;

	if (WARN_ON_ONCE(mem->flags))
		return -EINVAL;

	return kvm_set_memory_region(kvm, mem);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_set_internal_memslot);

static int kvm_vm_ioctl_set_memory_region(struct kvm *kvm,
					  struct kvm_userspace_memory_region2 *mem)
{
	if ((u16)mem->slot >= KVM_USER_MEM_SLOTS)
		return -EINVAL;

	guard(mutex)(&kvm->slots_lock);
	return kvm_set_memory_region(kvm, mem);
}

#ifndef CONFIG_KVM_GENERIC_DIRTYLOG_READ_PROTECT
/**
 * kvm_get_dirty_log - get a snapshot of dirty pages
 * @kvm:	pointer to kvm instance
 * @log:	slot id and address to which we copy the log
 * @is_dirty:	set to '1' if any dirty pages were found
 * @memslot:	set to the associated memslot, always valid on success
 */
int kvm_get_dirty_log(struct kvm *kvm, struct kvm_dirty_log *log,
		      int *is_dirty, struct kvm_memory_slot **memslot)
{
	struct kvm_memslots *slots;
	int i, as_id, id;
	unsigned long n;
	unsigned long any = 0;

	/* Dirty ring tracking may be exclusive to dirty log tracking */
	if (!kvm_use_dirty_bitmap(kvm))
		return -ENXIO;

	*memslot = NULL;
	*is_dirty = 0;

	as_id = log->slot >> 16;
	id = (u16)log->slot;
	if (as_id >= kvm_arch_nr_memslot_as_ids(kvm) || id >= KVM_USER_MEM_SLOTS)
		return -EINVAL;

	slots = __kvm_memslots(kvm, as_id);
	*memslot = id_to_memslot(slots, id);
	if (!(*memslot) || !(*memslot)->dirty_bitmap)
		return -ENOENT;

	kvm_arch_sync_dirty_log(kvm, *memslot);

	n = kvm_dirty_bitmap_bytes(*memslot);