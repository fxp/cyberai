// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 64/86



	if (vcpu->arch.nmi_pending) {
		r = can_inject ? kvm_x86_call(nmi_allowed)(vcpu, true) :
				 -EBUSY;
		if (r < 0)
			goto out;
		if (r) {
			--vcpu->arch.nmi_pending;
			vcpu->arch.nmi_injected = true;
			kvm_x86_call(inject_nmi)(vcpu);
			can_inject = false;
			WARN_ON(kvm_x86_call(nmi_allowed)(vcpu, true) < 0);
		}
		if (vcpu->arch.nmi_pending)
			kvm_x86_call(enable_nmi_window)(vcpu);
	}

	if (kvm_cpu_has_injectable_intr(vcpu)) {
		r = can_inject ? kvm_x86_call(interrupt_allowed)(vcpu, true) :
				 -EBUSY;
		if (r < 0)
			goto out;
		if (r) {
			int irq = kvm_cpu_get_interrupt(vcpu);

			if (!WARN_ON_ONCE(irq == -1)) {
				kvm_queue_interrupt(vcpu, irq, false);
				kvm_x86_call(inject_irq)(vcpu, false);
				WARN_ON(kvm_x86_call(interrupt_allowed)(vcpu, true) < 0);
			}
		}
		if (kvm_cpu_has_injectable_intr(vcpu))
			kvm_x86_call(enable_irq_window)(vcpu);
	}

	if (is_guest_mode(vcpu) &&
	    kvm_x86_ops.nested_ops->has_events &&
	    kvm_x86_ops.nested_ops->has_events(vcpu, true))
		*req_immediate_exit = true;

	/*
	 * KVM must never queue a new exception while injecting an event; KVM
	 * is done emulating and should only propagate the to-be-injected event
	 * to the VMCS/VMCB.  Queueing a new exception can put the vCPU into an
	 * infinite loop as KVM will bail from VM-Enter to inject the pending
	 * exception and start the cycle all over.
	 *
	 * Exempt triple faults as they have special handling and won't put the
	 * vCPU into an infinite loop.  Triple fault can be queued when running
	 * VMX without unrestricted guest, as that requires KVM to emulate Real
	 * Mode events (see kvm_inject_realmode_interrupt()).
	 */
	WARN_ON_ONCE(vcpu->arch.exception.pending ||
		     vcpu->arch.exception_vmexit.pending);
	return 0;

out:
	if (r == -EBUSY) {
		*req_immediate_exit = true;
		r = 0;
	}
	return r;
}

static void process_nmi(struct kvm_vcpu *vcpu)
{
	unsigned int limit;

	/*
	 * x86 is limited to one NMI pending, but because KVM can't react to
	 * incoming NMIs as quickly as bare metal, e.g. if the vCPU is
	 * scheduled out, KVM needs to play nice with two queued NMIs showing
	 * up at the same time.  To handle this scenario, allow two NMIs to be
	 * (temporarily) pending so long as NMIs are not blocked and KVM is not
	 * waiting for a previous NMI injection to complete (which effectively
	 * blocks NMIs).  KVM will immediately inject one of the two NMIs, and
	 * will request an NMI window to handle the second NMI.
	 */
	if (kvm_x86_call(get_nmi_mask)(vcpu) || vcpu->arch.nmi_injected)
		limit = 1;
	else
		limit = 2;

	/*
	 * Adjust the limit to account for pending virtual NMIs, which aren't
	 * tracked in vcpu->arch.nmi_pending.
	 */
	if (kvm_x86_call(is_vnmi_pending)(vcpu))
		limit--;

	vcpu->arch.nmi_pending += atomic_xchg(&vcpu->arch.nmi_queued, 0);
	vcpu->arch.nmi_pending = min(vcpu->arch.nmi_pending, limit);

	if (vcpu->arch.nmi_pending &&
	    (kvm_x86_call(set_vnmi_pending)(vcpu)))
		vcpu->arch.nmi_pending--;

	if (vcpu->arch.nmi_pending)
		kvm_make_request(KVM_REQ_EVENT, vcpu);
}

/* Return total number of NMIs pending injection to the VM */
int kvm_get_nr_pending_nmis(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.nmi_pending +
	       kvm_x86_call(is_vnmi_pending)(vcpu);
}

void kvm_make_scan_ioapic_request_mask(struct kvm *kvm,
				       unsigned long *vcpu_bitmap)
{
	kvm_make_vcpus_request_mask(kvm, KVM_REQ_SCAN_IOAPIC, vcpu_bitmap);
}

void kvm_make_scan_ioapic_request(struct kvm *kvm)
{
	kvm_make_all_cpus_request(kvm, KVM_REQ_SCAN_IOAPIC);
}

void __kvm_vcpu_update_apicv(struct kvm_vcpu *vcpu)
{
	struct kvm_lapic *apic = vcpu->arch.apic;
	bool activate;

	if (!lapic_in_kernel(vcpu))
		return;

	down_read(&vcpu->kvm->arch.apicv_update_lock);
	preempt_disable();

	/* Do not activate APICV when APIC is disabled */
	activate = kvm_vcpu_apicv_activated(vcpu) &&
		   (kvm_get_apic_mode(vcpu) != LAPIC_MODE_DISABLED);

	if (apic->apicv_active == activate)
		goto out;

	apic->apicv_active = activate;
	kvm_apic_update_apicv(vcpu);
	kvm_x86_call(refresh_apicv_exec_ctrl)(vcpu);

	/*
	 * When APICv gets disabled, we may still have injected interrupts
	 * pending. At the same time, KVM_REQ_EVENT may not be set as APICv was
	 * still active when the interrupt got accepted. Make sure
	 * kvm_check_and_inject_events() is called to check for that.
	 */
	if (!apic->apicv_active)
		kvm_make_request(KVM_REQ_EVENT, vcpu);

out:
	preempt_enable();
	up_read(&vcpu->kvm->arch.apicv_update_lock);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_vcpu_update_apicv);

static void kvm_vcpu_update_apicv(struct kvm_vcpu *vcpu)
{
	if (!lapic_in_kernel(vcpu))
		return;