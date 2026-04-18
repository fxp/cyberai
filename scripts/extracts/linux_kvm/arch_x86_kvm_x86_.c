// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 83/86



	if (is_guest_mode(vcpu)) {
		/*
		 * L1 needs to opt into the special #PF vmexits that are
		 * used to deliver async page faults.
		 */
		return vcpu->arch.apf.delivery_as_pf_vmexit;
	} else {
		/*
		 * Play it safe in case the guest temporarily disables paging.
		 * The real mode IDT in particular is unlikely to have a #PF
		 * exception setup.
		 */
		return is_paging(vcpu);
	}
}

bool kvm_can_do_async_pf(struct kvm_vcpu *vcpu)
{
	if (unlikely(!lapic_in_kernel(vcpu) ||
		     kvm_event_needs_reinjection(vcpu) ||
		     kvm_is_exception_pending(vcpu)))
		return false;

	if (kvm_hlt_in_guest(vcpu->kvm) && !kvm_can_deliver_async_pf(vcpu))
		return false;

	/*
	 * If interrupts are off we cannot even use an artificial
	 * halt state.
	 */
	return kvm_arch_interrupt_allowed(vcpu);
}

bool kvm_arch_async_page_not_present(struct kvm_vcpu *vcpu,
				     struct kvm_async_pf *work)
{
	struct x86_exception fault;

	trace_kvm_async_pf_not_present(work->arch.token, work->cr2_or_gpa);
	kvm_add_async_pf_gfn(vcpu, work->arch.gfn);

	if (kvm_can_deliver_async_pf(vcpu) &&
	    !apf_put_user_notpresent(vcpu)) {
		fault.vector = PF_VECTOR;
		fault.error_code_valid = true;
		fault.error_code = 0;
		fault.nested_page_fault = false;
		fault.address = work->arch.token;
		fault.async_page_fault = true;
		kvm_inject_page_fault(vcpu, &fault);
		return true;
	} else {
		/*
		 * It is not possible to deliver a paravirtualized asynchronous
		 * page fault, but putting the guest in an artificial halt state
		 * can be beneficial nevertheless: if an interrupt arrives, we
		 * can deliver it timely and perhaps the guest will schedule
		 * another process.  When the instruction that triggered a page
		 * fault is retried, hopefully the page will be ready in the host.
		 */
		kvm_make_request(KVM_REQ_APF_HALT, vcpu);
		return false;
	}
}

void kvm_arch_async_page_present(struct kvm_vcpu *vcpu,
				 struct kvm_async_pf *work)
{
	struct kvm_lapic_irq irq = {
		.delivery_mode = APIC_DM_FIXED,
		.vector = vcpu->arch.apf.vec
	};

	if (work->wakeup_all)
		work->arch.token = ~0; /* broadcast wakeup */
	else
		kvm_del_async_pf_gfn(vcpu, work->arch.gfn);
	trace_kvm_async_pf_ready(work->arch.token, work->cr2_or_gpa);

	if ((work->wakeup_all || work->notpresent_injected) &&
	    kvm_pv_async_pf_enabled(vcpu) &&
	    !apf_put_user_ready(vcpu, work->arch.token)) {
		WRITE_ONCE(vcpu->arch.apf.pageready_pending, true);
		kvm_apic_set_irq(vcpu, &irq, NULL);
	}

	vcpu->arch.apf.halted = false;
	kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);
}

void kvm_arch_async_page_present_queued(struct kvm_vcpu *vcpu)
{
	kvm_make_request(KVM_REQ_APF_READY, vcpu);

	/* Pairs with smp_store_mb() in kvm_set_msr_common(). */
	smp_mb__after_atomic();

	if (!READ_ONCE(vcpu->arch.apf.pageready_pending))
		kvm_vcpu_kick(vcpu);
}

bool kvm_arch_can_dequeue_async_page_present(struct kvm_vcpu *vcpu)
{
	if (!kvm_pv_async_pf_enabled(vcpu))
		return true;
	else
		return kvm_lapic_enabled(vcpu) && apf_pageready_slot_free(vcpu);
}

static void kvm_noncoherent_dma_assignment_start_or_stop(struct kvm *kvm)
{
	/*
	 * Non-coherent DMA assignment and de-assignment may affect whether or
	 * not KVM honors guest PAT, and thus may cause changes in EPT SPTEs
	 * due to toggling the "ignore PAT" bit.  Zap all SPTEs when the first
	 * (or last) non-coherent device is (un)registered to so that new SPTEs
	 * with the correct "ignore guest PAT" setting are created.
	 *
	 * If KVM always honors guest PAT, however, there is nothing to do.
	 */
	if (kvm_check_has_quirk(kvm, KVM_X86_QUIRK_IGNORE_GUEST_PAT))
		kvm_zap_gfn_range(kvm, gpa_to_gfn(0), gpa_to_gfn(~0ULL));
}

void kvm_arch_register_noncoherent_dma(struct kvm *kvm)
{
	if (atomic_inc_return(&kvm->arch.noncoherent_dma_count) == 1)
		kvm_noncoherent_dma_assignment_start_or_stop(kvm);
}

void kvm_arch_unregister_noncoherent_dma(struct kvm *kvm)
{
	if (!atomic_dec_return(&kvm->arch.noncoherent_dma_count))
		kvm_noncoherent_dma_assignment_start_or_stop(kvm);
}

bool kvm_arch_has_noncoherent_dma(struct kvm *kvm)
{
	return atomic_read(&kvm->arch.noncoherent_dma_count);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_arch_has_noncoherent_dma);

bool kvm_arch_no_poll(struct kvm_vcpu *vcpu)
{
	return (vcpu->arch.msr_kvm_poll_control & 1) == 0;
}

#ifdef CONFIG_KVM_GUEST_MEMFD
/*
 * KVM doesn't yet support initializing guest_memfd memory as shared for VMs
 * with private memory (the private vs. shared tracking needs to be moved into
 * guest_memfd).
 */
bool kvm_arch_supports_gmem_init_shared(struct kvm *kvm)
{
	return !kvm_arch_has_private_mem(kvm);
}

#ifdef CONFIG_HAVE_KVM_ARCH_GMEM_PREPARE
int kvm_arch_gmem_prepare(struct kvm *kvm, gfn_t gfn, kvm_pfn_t pfn, int max_order)
{
	return kvm_x86_call(gmem_prepare)(kvm, pfn, gfn, max_order);
}
#endif

#ifdef CONFIG_HAVE_KVM_ARCH_GMEM_INVALIDATE
void kvm_arch_gmem_invalidate(kvm_pfn_t start, kvm_pfn_t end)
{
	kvm_x86_call(gmem_invalidate)(start, end);
}
#endif
#endif