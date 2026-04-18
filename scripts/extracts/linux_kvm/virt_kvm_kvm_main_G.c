// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 7/36



	snprintf(dir_name, sizeof(dir_name), "%d-%s", task_pid_nr(current), fdname);
	mutex_lock(&kvm_debugfs_lock);
	dent = debugfs_lookup(dir_name, kvm_debugfs_dir);
	if (dent) {
		pr_warn_ratelimited("KVM: debugfs: duplicate directory %s\n", dir_name);
		dput(dent);
		mutex_unlock(&kvm_debugfs_lock);
		return 0;
	}
	dent = debugfs_create_dir(dir_name, kvm_debugfs_dir);
	mutex_unlock(&kvm_debugfs_lock);
	if (IS_ERR(dent))
		return 0;

	kvm->debugfs_dentry = dent;
	kvm->debugfs_stat_data = kzalloc_objs(*kvm->debugfs_stat_data,
					      kvm_debugfs_num_entries,
					      GFP_KERNEL_ACCOUNT);
	if (!kvm->debugfs_stat_data)
		goto out_err;

	for (i = 0; i < kvm_vm_stats_header.num_desc; ++i) {
		pdesc = &kvm_vm_stats_desc[i];
		stat_data = kzalloc_obj(*stat_data, GFP_KERNEL_ACCOUNT);
		if (!stat_data)
			goto out_err;

		stat_data->kvm = kvm;
		stat_data->desc = pdesc;
		stat_data->kind = KVM_STAT_VM;
		kvm->debugfs_stat_data[i] = stat_data;
		debugfs_create_file(pdesc->name, kvm_stats_debugfs_mode(pdesc),
				    kvm->debugfs_dentry, stat_data,
				    &stat_fops_per_vm);
	}

	for (i = 0; i < kvm_vcpu_stats_header.num_desc; ++i) {
		pdesc = &kvm_vcpu_stats_desc[i];
		stat_data = kzalloc_obj(*stat_data, GFP_KERNEL_ACCOUNT);
		if (!stat_data)
			goto out_err;

		stat_data->kvm = kvm;
		stat_data->desc = pdesc;
		stat_data->kind = KVM_STAT_VCPU;
		kvm->debugfs_stat_data[i + kvm_vm_stats_header.num_desc] = stat_data;
		debugfs_create_file(pdesc->name, kvm_stats_debugfs_mode(pdesc),
				    kvm->debugfs_dentry, stat_data,
				    &stat_fops_per_vm);
	}

	kvm_arch_create_vm_debugfs(kvm);
	return 0;
out_err:
	kvm_destroy_vm_debugfs(kvm);
	return ret;
}

/*
 * Called just after removing the VM from the vm_list, but before doing any
 * other destruction.
 */
void __weak kvm_arch_pre_destroy_vm(struct kvm *kvm)
{
}

/*
 * Called after per-vm debugfs created.  When called kvm->debugfs_dentry should
 * be setup already, so we can create arch-specific debugfs entries under it.
 * Cleanup should be automatic done in kvm_destroy_vm_debugfs() recursively, so
 * a per-arch destroy interface is not needed.
 */
void __weak kvm_arch_create_vm_debugfs(struct kvm *kvm)
{
}

/* Called only on cleanup and destruction paths when there are no users. */
static inline struct kvm_io_bus *kvm_get_bus_for_destruction(struct kvm *kvm,
							     enum kvm_bus idx)
{
	return rcu_dereference_protected(kvm->buses[idx],
					 !refcount_read(&kvm->users_count));
}

static int kvm_enable_virtualization(void);
static void kvm_disable_virtualization(void);

static struct kvm *kvm_create_vm(unsigned long type, const char *fdname)
{
	struct kvm *kvm = kvm_arch_alloc_vm();
	struct kvm_memslots *slots;
	int r, i, j;

	if (!kvm)
		return ERR_PTR(-ENOMEM);

	KVM_MMU_LOCK_INIT(kvm);
	mmgrab(current->mm);
	kvm->mm = current->mm;
	kvm_eventfd_init(kvm);
	mutex_init(&kvm->lock);
	mutex_init(&kvm->irq_lock);
	mutex_init(&kvm->slots_lock);
	mutex_init(&kvm->slots_arch_lock);
	spin_lock_init(&kvm->mn_invalidate_lock);
	rcuwait_init(&kvm->mn_memslots_update_rcuwait);
	xa_init(&kvm->vcpu_array);
#ifdef CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES
	xa_init(&kvm->mem_attr_array);
#endif

	INIT_LIST_HEAD(&kvm->gpc_list);
	spin_lock_init(&kvm->gpc_lock);

	INIT_LIST_HEAD(&kvm->devices);
	kvm->max_vcpus = KVM_MAX_VCPUS;

	BUILD_BUG_ON(KVM_MEM_SLOTS_NUM > SHRT_MAX);

	/*
	 * Force subsequent debugfs file creations to fail if the VM directory
	 * is not created (by kvm_create_vm_debugfs()).
	 */
	kvm->debugfs_dentry = ERR_PTR(-ENOENT);

	snprintf(kvm->stats_id, sizeof(kvm->stats_id), "kvm-%d",
		 task_pid_nr(current));

	r = -ENOMEM;
	if (init_srcu_struct(&kvm->srcu))
		goto out_err_no_srcu;
	if (init_srcu_struct(&kvm->irq_srcu))
		goto out_err_no_irq_srcu;

	r = kvm_init_irq_routing(kvm);
	if (r)
		goto out_err_no_irq_routing;

	refcount_set(&kvm->users_count, 1);

	for (i = 0; i < kvm_arch_nr_memslot_as_ids(kvm); i++) {
		for (j = 0; j < 2; j++) {
			slots = &kvm->__memslots[i][j];

			atomic_long_set(&slots->last_used_slot, (unsigned long)NULL);
			slots->hva_tree = RB_ROOT_CACHED;
			slots->gfn_tree = RB_ROOT;
			hash_init(slots->id_hash);
			slots->node_idx = j;

			/* Generations must be different for each address space. */
			slots->generation = i;
		}

		rcu_assign_pointer(kvm->memslots[i], &kvm->__memslots[i][0]);
	}

	r = -ENOMEM;
	for (i = 0; i < KVM_NR_BUSES; i++) {
		rcu_assign_pointer(kvm->buses[i],
			kzalloc_obj(struct kvm_io_bus, GFP_KERNEL_ACCOUNT));
		if (!kvm->buses[i])
			goto out_err_no_arch_destroy_vm;
	}

	r = kvm_arch_init_vm(kvm, type);
	if (r)
		goto out_err_no_arch_destroy_vm;

	r = kvm_enable_virtualization();
	if (r)
		goto out_err_no_disable;

#ifdef CONFIG_HAVE_KVM_IRQCHIP
	INIT_HLIST_HEAD(&kvm->irq_ack_notifier_list);
#endif

	r = kvm_init_mmu_notifier(kvm);
	if (r)
		goto out_err_no_mmu_notifier;

	r = kvm_coalesced_mmio_init(kvm);
	if (r < 0)
		goto out_no_coalesced_mmio;