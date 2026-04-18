// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 6/36



	/*
	 * Even though we do not flush TLB, this will still adversely
	 * affect performance on pre-Haswell Intel EPT, where there is
	 * no EPT Access Bit to clear so that we have to tear down EPT
	 * tables instead. If we find this unacceptable, we can always
	 * add a parameter to kvm_age_hva so that it effectively doesn't
	 * do anything on clear_young.
	 *
	 * Also note that currently we never issue secondary TLB flushes
	 * from clear_young, leaving this job up to the regular system
	 * cadence. If we find this inaccurate, we might come up with a
	 * more sophisticated heuristic later.
	 */
	return kvm_age_hva_range_no_flush(mn, start, end, kvm_age_gfn);
}

static bool kvm_mmu_notifier_test_young(struct mmu_notifier *mn,
		struct mm_struct *mm, unsigned long address)
{
	trace_kvm_test_age_hva(address);

	return kvm_age_hva_range_no_flush(mn, address, address + 1,
					  kvm_test_age_gfn);
}

static void kvm_mmu_notifier_release(struct mmu_notifier *mn,
				     struct mm_struct *mm)
{
	struct kvm *kvm = mmu_notifier_to_kvm(mn);
	int idx;

	idx = srcu_read_lock(&kvm->srcu);
	kvm_flush_shadow_all(kvm);
	srcu_read_unlock(&kvm->srcu, idx);
}

static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {
	.invalidate_range_start	= kvm_mmu_notifier_invalidate_range_start,
	.invalidate_range_end	= kvm_mmu_notifier_invalidate_range_end,
	.clear_flush_young	= kvm_mmu_notifier_clear_flush_young,
	.clear_young		= kvm_mmu_notifier_clear_young,
	.test_young		= kvm_mmu_notifier_test_young,
	.release		= kvm_mmu_notifier_release,
};

static int kvm_init_mmu_notifier(struct kvm *kvm)
{
	kvm->mmu_notifier.ops = &kvm_mmu_notifier_ops;
	return mmu_notifier_register(&kvm->mmu_notifier, current->mm);
}

#ifdef CONFIG_HAVE_KVM_PM_NOTIFIER
static int kvm_pm_notifier_call(struct notifier_block *bl,
				unsigned long state,
				void *unused)
{
	struct kvm *kvm = container_of(bl, struct kvm, pm_notifier);

	return kvm_arch_pm_notifier(kvm, state);
}

static void kvm_init_pm_notifier(struct kvm *kvm)
{
	kvm->pm_notifier.notifier_call = kvm_pm_notifier_call;
	/* Suspend KVM before we suspend ftrace, RCU, etc. */
	kvm->pm_notifier.priority = INT_MAX;
	register_pm_notifier(&kvm->pm_notifier);
}

static void kvm_destroy_pm_notifier(struct kvm *kvm)
{
	unregister_pm_notifier(&kvm->pm_notifier);
}
#else /* !CONFIG_HAVE_KVM_PM_NOTIFIER */
static void kvm_init_pm_notifier(struct kvm *kvm)
{
}

static void kvm_destroy_pm_notifier(struct kvm *kvm)
{
}
#endif /* CONFIG_HAVE_KVM_PM_NOTIFIER */

static void kvm_destroy_dirty_bitmap(struct kvm_memory_slot *memslot)
{
	if (!memslot->dirty_bitmap)
		return;

	vfree(memslot->dirty_bitmap);
	memslot->dirty_bitmap = NULL;
}

/* This does not remove the slot from struct kvm_memslots data structures */
static void kvm_free_memslot(struct kvm *kvm, struct kvm_memory_slot *slot)
{
	if (slot->flags & KVM_MEM_GUEST_MEMFD)
		kvm_gmem_unbind(slot);

	kvm_destroy_dirty_bitmap(slot);

	kvm_arch_free_memslot(kvm, slot);

	kfree(slot);
}

static void kvm_free_memslots(struct kvm *kvm, struct kvm_memslots *slots)
{
	struct hlist_node *idnode;
	struct kvm_memory_slot *memslot;
	int bkt;

	/*
	 * The same memslot objects live in both active and inactive sets,
	 * arbitrarily free using index '1' so the second invocation of this
	 * function isn't operating over a structure with dangling pointers
	 * (even though this function isn't actually touching them).
	 */
	if (!slots->node_idx)
		return;

	hash_for_each_safe(slots->id_hash, bkt, idnode, memslot, id_node[1])
		kvm_free_memslot(kvm, memslot);
}

static umode_t kvm_stats_debugfs_mode(const struct kvm_stats_desc *desc)
{
	switch (desc->flags & KVM_STATS_TYPE_MASK) {
	case KVM_STATS_TYPE_INSTANT:
		return 0444;
	case KVM_STATS_TYPE_CUMULATIVE:
	case KVM_STATS_TYPE_PEAK:
	default:
		return 0644;
	}
}


static void kvm_destroy_vm_debugfs(struct kvm *kvm)
{
	int i;
	int kvm_debugfs_num_entries = kvm_vm_stats_header.num_desc +
				      kvm_vcpu_stats_header.num_desc;

	if (IS_ERR(kvm->debugfs_dentry))
		return;

	debugfs_remove_recursive(kvm->debugfs_dentry);

	if (kvm->debugfs_stat_data) {
		for (i = 0; i < kvm_debugfs_num_entries; i++)
			kfree(kvm->debugfs_stat_data[i]);
		kfree(kvm->debugfs_stat_data);
	}
}

static int kvm_create_vm_debugfs(struct kvm *kvm, const char *fdname)
{
	static DEFINE_MUTEX(kvm_debugfs_lock);
	struct dentry *dent;
	char dir_name[ITOA_MAX_LEN * 2];
	struct kvm_stat_data *stat_data;
	const struct kvm_stats_desc *pdesc;
	int i, ret = -ENOMEM;
	int kvm_debugfs_num_entries = kvm_vm_stats_header.num_desc +
				      kvm_vcpu_stats_header.num_desc;

	if (!debugfs_initialized())
		return 0;