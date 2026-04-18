// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 65/86



	/*
	 * Due to sharing page tables across vCPUs, the xAPIC memslot must be
	 * deleted if any vCPU has xAPIC virtualization and x2APIC enabled, but
	 * and hardware doesn't support x2APIC virtualization.  E.g. some AMD
	 * CPUs support AVIC but not x2APIC.  KVM still allows enabling AVIC in
	 * this case so that KVM can use the AVIC doorbell to inject interrupts
	 * to running vCPUs, but KVM must not create SPTEs for the APIC base as
	 * the vCPU would incorrectly be able to access the vAPIC page via MMIO
	 * despite being in x2APIC mode.  For simplicity, inhibiting the APIC
	 * access page is sticky.
	 */
	if (apic_x2apic_mode(vcpu->arch.apic) &&
	    kvm_x86_ops.allow_apicv_in_x2apic_without_x2apic_virtualization)
		kvm_inhibit_apic_access_page(vcpu);

	__kvm_vcpu_update_apicv(vcpu);
}

void __kvm_set_or_clear_apicv_inhibit(struct kvm *kvm,
				      enum kvm_apicv_inhibit reason, bool set)
{
	unsigned long old, new;

	lockdep_assert_held_write(&kvm->arch.apicv_update_lock);

	if (!(kvm_x86_ops.required_apicv_inhibits & BIT(reason)))
		return;

	old = new = kvm->arch.apicv_inhibit_reasons;

	if (reason != APICV_INHIBIT_REASON_IRQWIN)
		set_or_clear_apicv_inhibit(&new, reason, set);

	set_or_clear_apicv_inhibit(&new, APICV_INHIBIT_REASON_IRQWIN,
				   atomic_read(&kvm->arch.apicv_nr_irq_window_req));

	if (!!old != !!new) {
		/*
		 * Kick all vCPUs before setting apicv_inhibit_reasons to avoid
		 * false positives in the sanity check WARN in vcpu_enter_guest().
		 * This task will wait for all vCPUs to ack the kick IRQ before
		 * updating apicv_inhibit_reasons, and all other vCPUs will
		 * block on acquiring apicv_update_lock so that vCPUs can't
		 * redo vcpu_enter_guest() without seeing the new inhibit state.
		 *
		 * Note, holding apicv_update_lock and taking it in the read
		 * side (handling the request) also prevents other vCPUs from
		 * servicing the request with a stale apicv_inhibit_reasons.
		 */
		kvm_make_all_cpus_request(kvm, KVM_REQ_APICV_UPDATE);
		kvm->arch.apicv_inhibit_reasons = new;
		if (new) {
			unsigned long gfn = gpa_to_gfn(APIC_DEFAULT_PHYS_BASE);
			int idx = srcu_read_lock(&kvm->srcu);

			kvm_zap_gfn_range(kvm, gfn, gfn+1);
			srcu_read_unlock(&kvm->srcu, idx);
		}
	} else {
		kvm->arch.apicv_inhibit_reasons = new;
	}
}

void kvm_set_or_clear_apicv_inhibit(struct kvm *kvm,
				    enum kvm_apicv_inhibit reason, bool set)
{
	if (!enable_apicv)
		return;

	down_write(&kvm->arch.apicv_update_lock);
	__kvm_set_or_clear_apicv_inhibit(kvm, reason, set);
	up_write(&kvm->arch.apicv_update_lock);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_set_or_clear_apicv_inhibit);

void kvm_inc_or_dec_irq_window_inhibit(struct kvm *kvm, bool inc)
{
	int add = inc ? 1 : -1;

	if (!enable_apicv)
		return;

	/*
	 * IRQ windows are requested either because of ExtINT injections, or
	 * because APICv is already disabled/inhibited for another reason.
	 * While ExtINT injections are rare and should not happen while the
	 * vCPU is running its actual workload, it's worth avoiding thrashing
	 * if the IRQ window is being requested because APICv is already
	 * inhibited.  So, toggle the actual inhibit (which requires taking
	 * the lock for write) if and only if there's no other inhibit.
	 * kvm_set_or_clear_apicv_inhibit() always evaluates the IRQ window
	 * count; thus the IRQ window inhibit call _will_ be lazily updated on
	 * the next call, if it ever happens.
	 */
	if (READ_ONCE(kvm->arch.apicv_inhibit_reasons) & ~BIT(APICV_INHIBIT_REASON_IRQWIN)) {
		guard(rwsem_read)(&kvm->arch.apicv_update_lock);
		if (READ_ONCE(kvm->arch.apicv_inhibit_reasons) & ~BIT(APICV_INHIBIT_REASON_IRQWIN)) {
			atomic_add(add, &kvm->arch.apicv_nr_irq_window_req);
			return;
		}
	}

	/*
	 * Strictly speaking, the lock is only needed if going 0->1 or 1->0,
	 * a la atomic_dec_and_mutex_lock.  However, ExtINTs are rare and
	 * only target a single CPU, so that is the common case; do not
	 * bother eliding the down_write()/up_write() pair.
	 */
	guard(rwsem_write)(&kvm->arch.apicv_update_lock);
	if (atomic_add_return(add, &kvm->arch.apicv_nr_irq_window_req) == inc)
		__kvm_set_or_clear_apicv_inhibit(kvm, APICV_INHIBIT_REASON_IRQWIN, inc);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_inc_or_dec_irq_window_inhibit);

static void vcpu_scan_ioapic(struct kvm_vcpu *vcpu)
{
	if (!kvm_apic_present(vcpu))
		return;

	bitmap_zero(vcpu->arch.ioapic_handled_vectors, 256);
	vcpu->arch.highest_stale_pending_ioapic_eoi = -1;

	kvm_x86_call(sync_pir_to_irr)(vcpu);

	if (irqchip_split(vcpu->kvm))
		kvm_scan_ioapic_routes(vcpu, vcpu->arch.ioapic_handled_vectors);
#ifdef CONFIG_KVM_IOAPIC
	else if (ioapic_in_kernel(vcpu->kvm))
		kvm_ioapic_scan_entry(vcpu, vcpu->arch.ioapic_handled_vectors);
#endif

	if (is_guest_mode(vcpu))
		vcpu->arch.load_eoi_exitmap_pending = true;
	else
		kvm_make_request(KVM_REQ_LOAD_EOI_EXITMAP, vcpu);
}

static void vcpu_load_eoi_exitmap(struct kvm_vcpu *vcpu)
{
	if (!kvm_apic_hw_enabled(vcpu->arch.apic))
		return;