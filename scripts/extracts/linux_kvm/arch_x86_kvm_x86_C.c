// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 35/86



#ifdef CONFIG_KVM_SMM
	events->smi.smm = is_smm(vcpu);
	events->smi.pending = vcpu->arch.smi_pending;
	events->smi.smm_inside_nmi =
		!!(vcpu->arch.hflags & HF_SMM_INSIDE_NMI_MASK);
#endif
	events->smi.latched_init = kvm_lapic_latched_init(vcpu);

	events->flags = (KVM_VCPUEVENT_VALID_NMI_PENDING
			 | KVM_VCPUEVENT_VALID_SHADOW
			 | KVM_VCPUEVENT_VALID_SMM);
	if (vcpu->kvm->arch.exception_payload_enabled)
		events->flags |= KVM_VCPUEVENT_VALID_PAYLOAD;
	if (vcpu->kvm->arch.triple_fault_event) {
		events->triple_fault.pending = kvm_test_request(KVM_REQ_TRIPLE_FAULT, vcpu);
		events->flags |= KVM_VCPUEVENT_VALID_TRIPLE_FAULT;
	}
}

static int kvm_vcpu_ioctl_x86_set_vcpu_events(struct kvm_vcpu *vcpu,
					      struct kvm_vcpu_events *events)
{
	if (events->flags & ~(KVM_VCPUEVENT_VALID_NMI_PENDING
			      | KVM_VCPUEVENT_VALID_SIPI_VECTOR
			      | KVM_VCPUEVENT_VALID_SHADOW
			      | KVM_VCPUEVENT_VALID_SMM
			      | KVM_VCPUEVENT_VALID_PAYLOAD
			      | KVM_VCPUEVENT_VALID_TRIPLE_FAULT))
		return -EINVAL;

	if (events->flags & KVM_VCPUEVENT_VALID_PAYLOAD) {
		if (!vcpu->kvm->arch.exception_payload_enabled)
			return -EINVAL;
		if (events->exception.pending)
			events->exception.injected = 0;
		else
			events->exception_has_payload = 0;
	} else {
		events->exception.pending = 0;
		events->exception_has_payload = 0;
	}

	if ((events->exception.injected || events->exception.pending) &&
	    (events->exception.nr > 31 || events->exception.nr == NMI_VECTOR))
		return -EINVAL;

	process_nmi(vcpu);

	/*
	 * Flag that userspace is stuffing an exception, the next KVM_RUN will
	 * morph the exception to a VM-Exit if appropriate.  Do this only for
	 * pending exceptions, already-injected exceptions are not subject to
	 * intercpetion.  Note, userspace that conflates pending and injected
	 * is hosed, and will incorrectly convert an injected exception into a
	 * pending exception, which in turn may cause a spurious VM-Exit.
	 */
	vcpu->arch.exception_from_userspace = events->exception.pending;

	vcpu->arch.exception_vmexit.pending = false;

	vcpu->arch.exception.injected = events->exception.injected;
	vcpu->arch.exception.pending = events->exception.pending;
	vcpu->arch.exception.vector = events->exception.nr;
	vcpu->arch.exception.has_error_code = events->exception.has_error_code;
	vcpu->arch.exception.error_code = events->exception.error_code;
	vcpu->arch.exception.has_payload = events->exception_has_payload;
	vcpu->arch.exception.payload = events->exception_payload;

	vcpu->arch.interrupt.injected = events->interrupt.injected;
	vcpu->arch.interrupt.nr = events->interrupt.nr;
	vcpu->arch.interrupt.soft = events->interrupt.soft;
	if (events->flags & KVM_VCPUEVENT_VALID_SHADOW)
		kvm_x86_call(set_interrupt_shadow)(vcpu,
						   events->interrupt.shadow);

	vcpu->arch.nmi_injected = events->nmi.injected;
	if (events->flags & KVM_VCPUEVENT_VALID_NMI_PENDING) {
		vcpu->arch.nmi_pending = 0;
		atomic_set(&vcpu->arch.nmi_queued, events->nmi.pending);
		if (events->nmi.pending)
			kvm_make_request(KVM_REQ_NMI, vcpu);
	}
	kvm_x86_call(set_nmi_mask)(vcpu, events->nmi.masked);

	if (events->flags & KVM_VCPUEVENT_VALID_SIPI_VECTOR &&
	    lapic_in_kernel(vcpu))
		vcpu->arch.apic->sipi_vector = events->sipi_vector;

	if (events->flags & KVM_VCPUEVENT_VALID_SMM) {
#ifdef CONFIG_KVM_SMM
		if (!!(vcpu->arch.hflags & HF_SMM_MASK) != events->smi.smm) {
			kvm_leave_nested(vcpu);
			kvm_smm_changed(vcpu, events->smi.smm);
		}

		vcpu->arch.smi_pending = events->smi.pending;

		if (events->smi.smm) {
			if (events->smi.smm_inside_nmi)
				vcpu->arch.hflags |= HF_SMM_INSIDE_NMI_MASK;
			else
				vcpu->arch.hflags &= ~HF_SMM_INSIDE_NMI_MASK;
		}

#else
		if (events->smi.smm || events->smi.pending ||
		    events->smi.smm_inside_nmi)
			return -EINVAL;
#endif

		if (lapic_in_kernel(vcpu)) {
			if (events->smi.latched_init)
				set_bit(KVM_APIC_INIT, &vcpu->arch.apic->pending_events);
			else
				clear_bit(KVM_APIC_INIT, &vcpu->arch.apic->pending_events);
		}
	}

	if (events->flags & KVM_VCPUEVENT_VALID_TRIPLE_FAULT) {
		if (!vcpu->kvm->arch.triple_fault_event)
			return -EINVAL;
		if (events->triple_fault.pending)
			kvm_make_request(KVM_REQ_TRIPLE_FAULT, vcpu);
		else
			kvm_clear_request(KVM_REQ_TRIPLE_FAULT, vcpu);
	}

	kvm_make_request(KVM_REQ_EVENT, vcpu);

	return 0;
}

static int kvm_vcpu_ioctl_x86_get_debugregs(struct kvm_vcpu *vcpu,
					    struct kvm_debugregs *dbgregs)
{
	unsigned int i;

	if (vcpu->kvm->arch.has_protected_state &&
	    vcpu->arch.guest_state_protected)
		return -EINVAL;

	kvm_handle_exception_payload_quirk(vcpu);

	memset(dbgregs, 0, sizeof(*dbgregs));

	BUILD_BUG_ON(ARRAY_SIZE(vcpu->arch.db) != ARRAY_SIZE(dbgregs->db));
	for (i = 0; i < ARRAY_SIZE(vcpu->arch.db); i++)
		dbgregs->db[i] = vcpu->arch.db[i];

	dbgregs->dr6 = vcpu->arch.dr6;
	dbgregs->dr7 = vcpu->arch.dr7;
	return 0;
}