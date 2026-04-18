// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 66/86



#ifdef CONFIG_KVM_HYPERV
	if (to_hv_vcpu(vcpu)) {
		u64 eoi_exit_bitmap[4];

		bitmap_or((ulong *)eoi_exit_bitmap,
			  vcpu->arch.ioapic_handled_vectors,
			  to_hv_synic(vcpu)->vec_bitmap, 256);
		kvm_x86_call(load_eoi_exitmap)(vcpu, eoi_exit_bitmap);
		return;
	}
#endif
	kvm_x86_call(load_eoi_exitmap)(
		vcpu, (u64 *)vcpu->arch.ioapic_handled_vectors);
}

void kvm_arch_guest_memory_reclaimed(struct kvm *kvm)
{
	kvm_x86_call(guest_memory_reclaimed)(kvm);
}

static void kvm_vcpu_reload_apic_access_page(struct kvm_vcpu *vcpu)
{
	if (!lapic_in_kernel(vcpu))
		return;

	kvm_x86_call(set_apic_access_page_addr)(vcpu);
}

/*
 * Called within kvm->srcu read side.
 * Returns 1 to let vcpu_run() continue the guest execution loop without
 * exiting to the userspace.  Otherwise, the value will be returned to the
 * userspace.
 */
static int vcpu_enter_guest(struct kvm_vcpu *vcpu)
{
	int r;
	bool req_int_win =
		dm_request_for_irq_injection(vcpu) &&
		kvm_cpu_accept_dm_intr(vcpu);
	fastpath_t exit_fastpath;
	u64 run_flags, debug_ctl;

	bool req_immediate_exit = false;

	if (kvm_request_pending(vcpu)) {
		if (kvm_check_request(KVM_REQ_VM_DEAD, vcpu)) {
			r = -EIO;
			goto out;
		}

		if (kvm_dirty_ring_check_request(vcpu)) {
			r = 0;
			goto out;
		}

		if (kvm_check_request(KVM_REQ_GET_NESTED_STATE_PAGES, vcpu)) {
			if (unlikely(!kvm_x86_ops.nested_ops->get_nested_state_pages(vcpu))) {
				r = 0;
				goto out;
			}
		}
		if (kvm_check_request(KVM_REQ_MMU_FREE_OBSOLETE_ROOTS, vcpu))
			kvm_mmu_free_obsolete_roots(vcpu);
		if (kvm_check_request(KVM_REQ_MIGRATE_TIMER, vcpu))
			__kvm_migrate_timers(vcpu);
		if (kvm_check_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu))
			kvm_update_masterclock(vcpu->kvm);
		if (kvm_check_request(KVM_REQ_GLOBAL_CLOCK_UPDATE, vcpu))
			kvm_gen_kvmclock_update(vcpu);
		if (kvm_check_request(KVM_REQ_CLOCK_UPDATE, vcpu)) {
			r = kvm_guest_time_update(vcpu);
			if (unlikely(r))
				goto out;
		}
		if (kvm_check_request(KVM_REQ_MMU_SYNC, vcpu))
			kvm_mmu_sync_roots(vcpu);
		if (kvm_check_request(KVM_REQ_LOAD_MMU_PGD, vcpu))
			kvm_mmu_load_pgd(vcpu);

		/*
		 * Note, the order matters here, as flushing "all" TLB entries
		 * also flushes the "current" TLB entries, i.e. servicing the
		 * flush "all" will clear any request to flush "current".
		 */
		if (kvm_check_request(KVM_REQ_TLB_FLUSH, vcpu))
			kvm_vcpu_flush_tlb_all(vcpu);

		kvm_service_local_tlb_flush_requests(vcpu);

		/*
		 * Fall back to a "full" guest flush if Hyper-V's precise
		 * flushing fails.  Note, Hyper-V's flushing is per-vCPU, but
		 * the flushes are considered "remote" and not "local" because
		 * the requests can be initiated from other vCPUs.
		 */
#ifdef CONFIG_KVM_HYPERV
		if (kvm_check_request(KVM_REQ_HV_TLB_FLUSH, vcpu) &&
		    kvm_hv_vcpu_flush_tlb(vcpu))
			kvm_vcpu_flush_tlb_guest(vcpu);
#endif

		if (kvm_check_request(KVM_REQ_REPORT_TPR_ACCESS, vcpu)) {
			vcpu->run->exit_reason = KVM_EXIT_TPR_ACCESS;
			r = 0;
			goto out;
		}
		if (kvm_test_request(KVM_REQ_TRIPLE_FAULT, vcpu)) {
			if (is_guest_mode(vcpu))
				kvm_x86_ops.nested_ops->triple_fault(vcpu);