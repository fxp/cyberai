// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 16/36



static bool kvm_pre_set_memory_attributes(struct kvm *kvm,
					  struct kvm_gfn_range *range)
{
	/*
	 * Unconditionally add the range to the invalidation set, regardless of
	 * whether or not the arch callback actually needs to zap SPTEs.  E.g.
	 * if KVM supports RWX attributes in the future and the attributes are
	 * going from R=>RW, zapping isn't strictly necessary.  Unconditionally
	 * adding the range allows KVM to require that MMU invalidations add at
	 * least one range between begin() and end(), e.g. allows KVM to detect
	 * bugs where the add() is missed.  Relaxing the rule *might* be safe,
	 * but it's not obvious that allowing new mappings while the attributes
	 * are in flux is desirable or worth the complexity.
	 */
	kvm_mmu_invalidate_range_add(kvm, range->start, range->end);

	return kvm_arch_pre_set_memory_attributes(kvm, range);
}

/* Set @attributes for the gfn range [@start, @end). */
static int kvm_vm_set_mem_attributes(struct kvm *kvm, gfn_t start, gfn_t end,
				     unsigned long attributes)
{
	struct kvm_mmu_notifier_range pre_set_range = {
		.start = start,
		.end = end,
		.arg.attributes = attributes,
		.handler = kvm_pre_set_memory_attributes,
		.on_lock = kvm_mmu_invalidate_begin,
		.flush_on_ret = true,
		.may_block = true,
	};
	struct kvm_mmu_notifier_range post_set_range = {
		.start = start,
		.end = end,
		.arg.attributes = attributes,
		.handler = kvm_arch_post_set_memory_attributes,
		.on_lock = kvm_mmu_invalidate_end,
		.may_block = true,
	};
	unsigned long i;
	void *entry;
	int r = 0;

	entry = attributes ? xa_mk_value(attributes) : NULL;

	trace_kvm_vm_set_mem_attributes(start, end, attributes);

	mutex_lock(&kvm->slots_lock);

	/* Nothing to do if the entire range has the desired attributes. */
	if (kvm_range_has_memory_attributes(kvm, start, end, ~0, attributes))
		goto out_unlock;

	/*
	 * Reserve memory ahead of time to avoid having to deal with failures
	 * partway through setting the new attributes.
	 */
	for (i = start; i < end; i++) {
		r = xa_reserve(&kvm->mem_attr_array, i, GFP_KERNEL_ACCOUNT);
		if (r)
			goto out_unlock;

		cond_resched();
	}

	kvm_handle_gfn_range(kvm, &pre_set_range);

	for (i = start; i < end; i++) {
		r = xa_err(xa_store(&kvm->mem_attr_array, i, entry,
				    GFP_KERNEL_ACCOUNT));
		KVM_BUG_ON(r, kvm);
		cond_resched();
	}

	kvm_handle_gfn_range(kvm, &post_set_range);

out_unlock:
	mutex_unlock(&kvm->slots_lock);

	return r;
}
static int kvm_vm_ioctl_set_mem_attributes(struct kvm *kvm,
					   struct kvm_memory_attributes *attrs)
{
	gfn_t start, end;

	/* flags is currently not used. */
	if (attrs->flags)
		return -EINVAL;
	if (attrs->attributes & ~kvm_supported_mem_attributes(kvm))
		return -EINVAL;
	if (attrs->size == 0 || attrs->address + attrs->size < attrs->address)
		return -EINVAL;
	if (!PAGE_ALIGNED(attrs->address) || !PAGE_ALIGNED(attrs->size))
		return -EINVAL;

	start = attrs->address >> PAGE_SHIFT;
	end = (attrs->address + attrs->size) >> PAGE_SHIFT;

	/*
	 * xarray tracks data using "unsigned long", and as a result so does
	 * KVM.  For simplicity, supports generic attributes only on 64-bit
	 * architectures.
	 */
	BUILD_BUG_ON(sizeof(attrs->attributes) != sizeof(unsigned long));

	return kvm_vm_set_mem_attributes(kvm, start, end, attrs->attributes);
}
#endif /* CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES */

struct kvm_memory_slot *gfn_to_memslot(struct kvm *kvm, gfn_t gfn)
{
	return __gfn_to_memslot(kvm_memslots(kvm), gfn);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(gfn_to_memslot);

struct kvm_memory_slot *kvm_vcpu_gfn_to_memslot(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	struct kvm_memslots *slots = kvm_vcpu_memslots(vcpu);
	u64 gen = slots->generation;
	struct kvm_memory_slot *slot;

	/*
	 * This also protects against using a memslot from a different address space,
	 * since different address spaces have different generation numbers.
	 */
	if (unlikely(gen != vcpu->last_used_slot_gen)) {
		vcpu->last_used_slot = NULL;
		vcpu->last_used_slot_gen = gen;
	}

	slot = try_get_memslot(vcpu->last_used_slot, gfn);
	if (slot)
		return slot;

	/*
	 * Fall back to searching all memslots. We purposely use
	 * search_memslots() instead of __gfn_to_memslot() to avoid
	 * thrashing the VM-wide last_used_slot in kvm_memslots.
	 */
	slot = search_memslots(slots, gfn, false);
	if (slot) {
		vcpu->last_used_slot = slot;
		return slot;
	}

	return NULL;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_gfn_to_memslot);

bool kvm_is_visible_gfn(struct kvm *kvm, gfn_t gfn)
{
	struct kvm_memory_slot *memslot = gfn_to_memslot(kvm, gfn);

	return kvm_is_visible_memslot(memslot);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_is_visible_gfn);

bool kvm_vcpu_is_visible_gfn(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	struct kvm_memory_slot *memslot = kvm_vcpu_gfn_to_memslot(vcpu, gfn);

	return kvm_is_visible_memslot(memslot);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_is_visible_gfn);