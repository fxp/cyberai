// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 80/86



void kvm_arch_destroy_vm(struct kvm *kvm)
{
	if (current->mm == kvm->mm) {
		/*
		 * Free memory regions allocated on behalf of userspace,
		 * unless the memory map has changed due to process exit
		 * or fd copying.
		 */
		mutex_lock(&kvm->slots_lock);
		__x86_set_memory_region(kvm, APIC_ACCESS_PAGE_PRIVATE_MEMSLOT,
					0, 0);
		__x86_set_memory_region(kvm, IDENTITY_PAGETABLE_PRIVATE_MEMSLOT,
					0, 0);
		__x86_set_memory_region(kvm, TSS_PRIVATE_MEMSLOT, 0, 0);
		mutex_unlock(&kvm->slots_lock);
	}
	if (kvm->arch.created_mediated_pmu)
		perf_release_mediated_pmu();
	kvm_destroy_vcpus(kvm);
	kvm_free_msr_filter(srcu_dereference_check(kvm->arch.msr_filter, &kvm->srcu, 1));
#ifdef CONFIG_KVM_IOAPIC
	kvm_pic_destroy(kvm);
	kvm_ioapic_destroy(kvm);
#endif
	kvfree(rcu_dereference_check(kvm->arch.apic_map, 1));
	kfree(srcu_dereference_check(kvm->arch.pmu_event_filter, &kvm->srcu, 1));
	kvm_mmu_uninit_vm(kvm);
	kvm_page_track_cleanup(kvm);
	kvm_xen_destroy_vm(kvm);
	kvm_hv_destroy_vm(kvm);
	kvm_x86_call(vm_destroy)(kvm);
}

static void memslot_rmap_free(struct kvm_memory_slot *slot)
{
	int i;

	for (i = 0; i < KVM_NR_PAGE_SIZES; ++i) {
		vfree(slot->arch.rmap[i]);
		slot->arch.rmap[i] = NULL;
	}
}

void kvm_arch_free_memslot(struct kvm *kvm, struct kvm_memory_slot *slot)
{
	int i;

	memslot_rmap_free(slot);

	for (i = 1; i < KVM_NR_PAGE_SIZES; ++i) {
		vfree(slot->arch.lpage_info[i - 1]);
		slot->arch.lpage_info[i - 1] = NULL;
	}

	kvm_page_track_free_memslot(slot);
}

int memslot_rmap_alloc(struct kvm_memory_slot *slot, unsigned long npages)
{
	const int sz = sizeof(*slot->arch.rmap[0]);
	int i;

	for (i = 0; i < KVM_NR_PAGE_SIZES; ++i) {
		int level = i + 1;
		int lpages = __kvm_mmu_slot_lpages(slot, npages, level);

		if (slot->arch.rmap[i])
			continue;

		slot->arch.rmap[i] = __vcalloc(lpages, sz, GFP_KERNEL_ACCOUNT);
		if (!slot->arch.rmap[i]) {
			memslot_rmap_free(slot);
			return -ENOMEM;
		}
	}

	return 0;
}

static int kvm_alloc_memslot_metadata(struct kvm *kvm,
				      struct kvm_memory_slot *slot)
{
	unsigned long npages = slot->npages;
	int i, r;

	/*
	 * Clear out the previous array pointers for the KVM_MR_MOVE case.  The
	 * old arrays will be freed by kvm_set_memory_region() if installing
	 * the new memslot is successful.
	 */
	memset(&slot->arch, 0, sizeof(slot->arch));

	if (kvm_memslots_have_rmaps(kvm)) {
		r = memslot_rmap_alloc(slot, npages);
		if (r)
			return r;
	}

	for (i = 1; i < KVM_NR_PAGE_SIZES; ++i) {
		struct kvm_lpage_info *linfo;
		unsigned long ugfn;
		int lpages;
		int level = i + 1;

		lpages = __kvm_mmu_slot_lpages(slot, npages, level);

		linfo = __vcalloc(lpages, sizeof(*linfo), GFP_KERNEL_ACCOUNT);
		if (!linfo)
			goto out_free;

		slot->arch.lpage_info[i - 1] = linfo;

		if (slot->base_gfn & (KVM_PAGES_PER_HPAGE(level) - 1))
			linfo[0].disallow_lpage = 1;
		if ((slot->base_gfn + npages) & (KVM_PAGES_PER_HPAGE(level) - 1))
			linfo[lpages - 1].disallow_lpage = 1;
		ugfn = slot->userspace_addr >> PAGE_SHIFT;
		/*
		 * If the gfn and userspace address are not aligned wrt each
		 * other, disable large page support for this slot.
		 */
		if ((slot->base_gfn ^ ugfn) & (KVM_PAGES_PER_HPAGE(level) - 1)) {
			unsigned long j;

			for (j = 0; j < lpages; ++j)
				linfo[j].disallow_lpage = 1;
		}
	}

#ifdef CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES
	kvm_mmu_init_memslot_memory_attributes(kvm, slot);
#endif

	if (kvm_page_track_create_memslot(kvm, slot, npages))
		goto out_free;

	return 0;

out_free:
	memslot_rmap_free(slot);

	for (i = 1; i < KVM_NR_PAGE_SIZES; ++i) {
		vfree(slot->arch.lpage_info[i - 1]);
		slot->arch.lpage_info[i - 1] = NULL;
	}
	return -ENOMEM;
}

void kvm_arch_memslots_updated(struct kvm *kvm, u64 gen)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;

	/*
	 * memslots->generation has been incremented.
	 * mmio generation may have reached its maximum value.
	 */
	kvm_mmu_invalidate_mmio_sptes(kvm, gen);

	/* Force re-initialization of steal_time cache */
	kvm_for_each_vcpu(i, vcpu, kvm)
		kvm_vcpu_kick(vcpu);
}

int kvm_arch_prepare_memory_region(struct kvm *kvm,
				   const struct kvm_memory_slot *old,
				   struct kvm_memory_slot *new,
				   enum kvm_mr_change change)
{
	/*
	 * KVM doesn't support moving memslots when there are external page
	 * trackers attached to the VM, i.e. if KVMGT is in use.
	 */
	if (change == KVM_MR_MOVE && kvm_page_track_has_external_user(kvm))
		return -EINVAL;

	if (change == KVM_MR_CREATE || change == KVM_MR_MOVE) {
		if ((new->base_gfn + new->npages - 1) > kvm_mmu_max_gfn())
			return -EINVAL;

		if (kvm_is_gfn_alias(kvm, new->base_gfn + new->npages - 1))
			return -EINVAL;

		return kvm_alloc_memslot_metadata(kvm, new);
	}

	if (change == KVM_MR_FLAGS_ONLY)
		memcpy(&new->arch, &old->arch, sizeof(old->arch));
	else if (WARN_ON_ONCE(change != KVM_MR_DELETE))
		return -EIO;

	return 0;
}


static void kvm_mmu_update_cpu_dirty_logging(struct kvm *kvm, bool enable)
{
	int nr_slots;

	if (!kvm->arch.cpu_dirty_log_size)
		return;