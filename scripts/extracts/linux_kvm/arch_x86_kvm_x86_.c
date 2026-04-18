// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 79/86



	kvm->arch.vm_type = type;
	kvm->arch.has_private_mem =
		(type == KVM_X86_SW_PROTECTED_VM);
	/* Decided by the vendor code for other VM types.  */
	kvm->arch.pre_fault_allowed =
		type == KVM_X86_DEFAULT_VM || type == KVM_X86_SW_PROTECTED_VM;
	kvm->arch.disabled_quirks = kvm_caps.inapplicable_quirks & kvm_caps.supported_quirks;

	ret = kvm_page_track_init(kvm);
	if (ret)
		goto out;

	ret = kvm_mmu_init_vm(kvm);
	if (ret)
		goto out_cleanup_page_track;

	ret = kvm_x86_call(vm_init)(kvm);
	if (ret)
		goto out_uninit_mmu;

	atomic_set(&kvm->arch.noncoherent_dma_count, 0);

	raw_spin_lock_init(&kvm->arch.tsc_write_lock);
	mutex_init(&kvm->arch.apic_map_lock);
	seqcount_raw_spinlock_init(&kvm->arch.pvclock_sc, &kvm->arch.tsc_write_lock);
	kvm->arch.kvmclock_offset = -get_kvmclock_base_ns();

	raw_spin_lock_irqsave(&kvm->arch.tsc_write_lock, flags);
	pvclock_update_vm_gtod_copy(kvm);
	raw_spin_unlock_irqrestore(&kvm->arch.tsc_write_lock, flags);

	kvm->arch.default_tsc_khz = max_tsc_khz ? : tsc_khz;
	kvm->arch.apic_bus_cycle_ns = APIC_BUS_CYCLE_NS_DEFAULT;
	kvm->arch.guest_can_read_msr_platform_info = true;
	kvm->arch.enable_pmu = enable_pmu;

#if IS_ENABLED(CONFIG_HYPERV)
	spin_lock_init(&kvm->arch.hv_root_tdp_lock);
	kvm->arch.hv_root_tdp = INVALID_PAGE;
#endif

	kvm_apicv_init(kvm);
	kvm_hv_init_vm(kvm);
	kvm_xen_init_vm(kvm);

	if (ignore_msrs && !report_ignored_msrs) {
		pr_warn_once("Running KVM with ignore_msrs=1 and report_ignored_msrs=0 is not a\n"
			     "a supported configuration.  Lying to the guest about the existence of MSRs\n"
			     "may cause the guest operating system to hang or produce errors.  If a guest\n"
			     "does not run without ignore_msrs=1, please report it to kvm@vger.kernel.org.\n");
	}

	once_init(&kvm->arch.nx_once);
	return 0;

out_uninit_mmu:
	kvm_mmu_uninit_vm(kvm);
out_cleanup_page_track:
	kvm_page_track_cleanup(kvm);
out:
	return ret;
}

/**
 * __x86_set_memory_region: Setup KVM internal memory slot
 *
 * @kvm: the kvm pointer to the VM.
 * @id: the slot ID to setup.
 * @gpa: the GPA to install the slot (unused when @size == 0).
 * @size: the size of the slot. Set to zero to uninstall a slot.
 *
 * This function helps to setup a KVM internal memory slot.  Specify
 * @size > 0 to install a new slot, while @size == 0 to uninstall a
 * slot.  The return code can be one of the following:
 *
 *   HVA:           on success (uninstall will return a bogus HVA)
 *   -errno:        on error
 *
 * The caller should always use IS_ERR() to check the return value
 * before use.  Note, the KVM internal memory slots are guaranteed to
 * remain valid and unchanged until the VM is destroyed, i.e., the
 * GPA->HVA translation will not change.  However, the HVA is a user
 * address, i.e. its accessibility is not guaranteed, and must be
 * accessed via __copy_{to,from}_user().
 */
void __user * __x86_set_memory_region(struct kvm *kvm, int id, gpa_t gpa,
				      u32 size)
{
	int i, r;
	unsigned long hva, old_npages;
	struct kvm_memslots *slots = kvm_memslots(kvm);
	struct kvm_memory_slot *slot;

	lockdep_assert_held(&kvm->slots_lock);

	if (WARN_ON(id >= KVM_MEM_SLOTS_NUM))
		return ERR_PTR_USR(-EINVAL);

	slot = id_to_memslot(slots, id);
	if (size) {
		if (slot && slot->npages)
			return ERR_PTR_USR(-EEXIST);

		/*
		 * MAP_SHARED to prevent internal slot pages from being moved
		 * by fork()/COW.
		 */
		hva = vm_mmap(NULL, 0, size, PROT_READ | PROT_WRITE,
			      MAP_SHARED | MAP_ANONYMOUS, 0);
		if (IS_ERR_VALUE(hva))
			return (void __user *)hva;
	} else {
		if (!slot || !slot->npages)
			return NULL;

		old_npages = slot->npages;
		hva = slot->userspace_addr;
	}

	for (i = 0; i < kvm_arch_nr_memslot_as_ids(kvm); i++) {
		struct kvm_userspace_memory_region2 m;

		m.slot = id | (i << 16);
		m.flags = 0;
		m.guest_phys_addr = gpa;
		m.userspace_addr = hva;
		m.memory_size = size;
		r = kvm_set_internal_memslot(kvm, &m);
		if (r < 0)
			return ERR_PTR_USR(r);
	}

	if (!size)
		vm_munmap(hva, old_npages * PAGE_SIZE);

	return (void __user *)hva;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__x86_set_memory_region);

void kvm_arch_pre_destroy_vm(struct kvm *kvm)
{
	/*
	 * Stop all background workers and kthreads before destroying vCPUs, as
	 * iterating over vCPUs in a different task while vCPUs are being freed
	 * is unsafe, i.e. will lead to use-after-free.  The PIT also needs to
	 * be stopped before IRQ routing is freed.
	 */
#ifdef CONFIG_KVM_IOAPIC
	kvm_free_pit(kvm);
#endif

	kvm_mmu_pre_destroy_vm(kvm);
	kvm_x86_call(vm_pre_destroy)(kvm);
}