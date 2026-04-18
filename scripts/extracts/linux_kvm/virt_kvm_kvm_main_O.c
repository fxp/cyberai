// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 15/36



	slots = __kvm_memslots(kvm, as_id);
	memslot = id_to_memslot(slots, id);
	if (!memslot || !memslot->dirty_bitmap)
		return -ENOENT;

	dirty_bitmap = memslot->dirty_bitmap;

	n = ALIGN(log->num_pages, BITS_PER_LONG) / 8;

	if (log->first_page > memslot->npages ||
	    log->num_pages > memslot->npages - log->first_page ||
	    (log->num_pages < memslot->npages - log->first_page && (log->num_pages & 63)))
	    return -EINVAL;

	kvm_arch_sync_dirty_log(kvm, memslot);

	flush = false;
	dirty_bitmap_buffer = kvm_second_dirty_bitmap(memslot);
	if (copy_from_user(dirty_bitmap_buffer, log->dirty_bitmap, n))
		return -EFAULT;

	KVM_MMU_LOCK(kvm);
	for (offset = log->first_page, i = offset / BITS_PER_LONG,
		 n = DIV_ROUND_UP(log->num_pages, BITS_PER_LONG); n--;
	     i++, offset += BITS_PER_LONG) {
		unsigned long mask = *dirty_bitmap_buffer++;
		atomic_long_t *p = (atomic_long_t *) &dirty_bitmap[i];
		if (!mask)
			continue;

		mask &= atomic_long_fetch_andnot(mask, p);

		/*
		 * mask contains the bits that really have been cleared.  This
		 * never includes any bits beyond the length of the memslot (if
		 * the length is not aligned to 64 pages), therefore it is not
		 * a problem if userspace sets them in log->dirty_bitmap.
		*/
		if (mask) {
			flush = true;
			kvm_arch_mmu_enable_log_dirty_pt_masked(kvm, memslot,
								offset, mask);
		}
	}
	KVM_MMU_UNLOCK(kvm);

	if (flush)
		kvm_flush_remote_tlbs_memslot(kvm, memslot);

	return 0;
}

static int kvm_vm_ioctl_clear_dirty_log(struct kvm *kvm,
					struct kvm_clear_dirty_log *log)
{
	int r;

	mutex_lock(&kvm->slots_lock);

	r = kvm_clear_dirty_log_protect(kvm, log);

	mutex_unlock(&kvm->slots_lock);
	return r;
}
#endif /* CONFIG_KVM_GENERIC_DIRTYLOG_READ_PROTECT */

#ifdef CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES
static u64 kvm_supported_mem_attributes(struct kvm *kvm)
{
	if (!kvm || kvm_arch_has_private_mem(kvm))
		return KVM_MEMORY_ATTRIBUTE_PRIVATE;

	return 0;
}

/*
 * Returns true if _all_ gfns in the range [@start, @end) have attributes
 * such that the bits in @mask match @attrs.
 */
bool kvm_range_has_memory_attributes(struct kvm *kvm, gfn_t start, gfn_t end,
				     unsigned long mask, unsigned long attrs)
{
	XA_STATE(xas, &kvm->mem_attr_array, start);
	unsigned long index;
	void *entry;

	mask &= kvm_supported_mem_attributes(kvm);
	if (attrs & ~mask)
		return false;

	if (end == start + 1)
		return (kvm_get_memory_attributes(kvm, start) & mask) == attrs;

	guard(rcu)();
	if (!attrs)
		return !xas_find(&xas, end - 1);

	for (index = start; index < end; index++) {
		do {
			entry = xas_next(&xas);
		} while (xas_retry(&xas, entry));

		if (xas.xa_index != index ||
		    (xa_to_value(entry) & mask) != attrs)
			return false;
	}

	return true;
}

static __always_inline void kvm_handle_gfn_range(struct kvm *kvm,
						 struct kvm_mmu_notifier_range *range)
{
	struct kvm_gfn_range gfn_range;
	struct kvm_memory_slot *slot;
	struct kvm_memslots *slots;
	struct kvm_memslot_iter iter;
	bool found_memslot = false;
	bool ret = false;
	int i;

	gfn_range.arg = range->arg;
	gfn_range.may_block = range->may_block;

	/*
	 * If/when KVM supports more attributes beyond private .vs shared, this
	 * _could_ set KVM_FILTER_{SHARED,PRIVATE} appropriately if the entire target
	 * range already has the desired private vs. shared state (it's unclear
	 * if that is a net win).  For now, KVM reaches this point if and only
	 * if the private flag is being toggled, i.e. all mappings are in play.
	 */

	for (i = 0; i < kvm_arch_nr_memslot_as_ids(kvm); i++) {
		slots = __kvm_memslots(kvm, i);

		kvm_for_each_memslot_in_gfn_range(&iter, slots, range->start, range->end) {
			slot = iter.slot;
			gfn_range.slot = slot;

			gfn_range.start = max(range->start, slot->base_gfn);
			gfn_range.end = min(range->end, slot->base_gfn + slot->npages);
			if (gfn_range.start >= gfn_range.end)
				continue;

			if (!found_memslot) {
				found_memslot = true;
				KVM_MMU_LOCK(kvm);
				if (!IS_KVM_NULL_FN(range->on_lock))
					range->on_lock(kvm);
			}

			ret |= range->handler(kvm, &gfn_range);
		}
	}

	if (range->flush_on_ret && ret)
		kvm_flush_remote_tlbs(kvm);

	if (found_memslot)
		KVM_MMU_UNLOCK(kvm);
}