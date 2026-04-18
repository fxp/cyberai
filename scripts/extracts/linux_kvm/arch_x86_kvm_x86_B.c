// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 34/86



	if (lapic_in_kernel(vcpu))
		kvm_apic_local_deliver(vcpu->arch.apic, APIC_LVTCMCI);

	return 0;
}

static int kvm_vcpu_ioctl_x86_set_mce(struct kvm_vcpu *vcpu,
				      struct kvm_x86_mce *mce)
{
	u64 mcg_cap = vcpu->arch.mcg_cap;
	unsigned bank_num = mcg_cap & 0xff;
	u64 *banks = vcpu->arch.mce_banks;

	if (mce->bank >= bank_num || !(mce->status & MCI_STATUS_VAL))
		return -EINVAL;

	banks += array_index_nospec(4 * mce->bank, 4 * bank_num);

	if (is_ucna(mce))
		return kvm_vcpu_x86_set_ucna(vcpu, mce, banks);

	/*
	 * if IA32_MCG_CTL is not all 1s, the uncorrected error
	 * reporting is disabled
	 */
	if ((mce->status & MCI_STATUS_UC) && (mcg_cap & MCG_CTL_P) &&
	    vcpu->arch.mcg_ctl != ~(u64)0)
		return 0;
	/*
	 * if IA32_MCi_CTL is not all 1s, the uncorrected error
	 * reporting is disabled for the bank
	 */
	if ((mce->status & MCI_STATUS_UC) && banks[0] != ~(u64)0)
		return 0;
	if (mce->status & MCI_STATUS_UC) {
		if ((vcpu->arch.mcg_status & MCG_STATUS_MCIP) ||
		    !kvm_is_cr4_bit_set(vcpu, X86_CR4_MCE)) {
			kvm_make_request(KVM_REQ_TRIPLE_FAULT, vcpu);
			return 0;
		}
		if (banks[1] & MCI_STATUS_VAL)
			mce->status |= MCI_STATUS_OVER;
		banks[2] = mce->addr;
		banks[3] = mce->misc;
		vcpu->arch.mcg_status = mce->mcg_status;
		banks[1] = mce->status;
		kvm_queue_exception(vcpu, MC_VECTOR);
	} else if (!(banks[1] & MCI_STATUS_VAL)
		   || !(banks[1] & MCI_STATUS_UC)) {
		if (banks[1] & MCI_STATUS_VAL)
			mce->status |= MCI_STATUS_OVER;
		banks[2] = mce->addr;
		banks[3] = mce->misc;
		banks[1] = mce->status;
	} else
		banks[1] |= MCI_STATUS_OVER;
	return 0;
}

static struct kvm_queued_exception *kvm_get_exception_to_save(struct kvm_vcpu *vcpu)
{
	/*
	 * KVM's ABI only allows for one exception to be migrated.  Luckily,
	 * the only time there can be two queued exceptions is if there's a
	 * non-exiting _injected_ exception, and a pending exiting exception.
	 * In that case, ignore the VM-Exiting exception as it's an extension
	 * of the injected exception.
	 */
	if (vcpu->arch.exception_vmexit.pending &&
	    !vcpu->arch.exception.pending &&
	    !vcpu->arch.exception.injected)
		return &vcpu->arch.exception_vmexit;

	return &vcpu->arch.exception;
}

static void kvm_handle_exception_payload_quirk(struct kvm_vcpu *vcpu)
{
	struct kvm_queued_exception *ex = kvm_get_exception_to_save(vcpu);

	/*
	 * If KVM_CAP_EXCEPTION_PAYLOAD is disabled, then (prematurely) deliver
	 * the pending exception payload when userspace saves *any* vCPU state
	 * that interacts with exception payloads to avoid breaking userspace.
	 *
	 * Architecturally, KVM must not deliver an exception payload until the
	 * exception is actually injected, e.g. to avoid losing pending #DB
	 * information (which VMX tracks in the VMCS), and to avoid clobbering
	 * state if the exception is never injected for whatever reason.  But
	 * if KVM_CAP_EXCEPTION_PAYLOAD isn't enabled, then userspace may or
	 * may not propagate the payload across save+restore, and so KVM can't
	 * safely defer delivery of the payload.
	 */
	if (!vcpu->kvm->arch.exception_payload_enabled &&
	    ex->pending && ex->has_payload)
		kvm_deliver_exception_payload(vcpu, ex);
}

static void kvm_vcpu_ioctl_x86_get_vcpu_events(struct kvm_vcpu *vcpu,
					       struct kvm_vcpu_events *events)
{
	struct kvm_queued_exception *ex = kvm_get_exception_to_save(vcpu);

	process_nmi(vcpu);

#ifdef CONFIG_KVM_SMM
	if (kvm_check_request(KVM_REQ_SMI, vcpu))
		process_smi(vcpu);
#endif

	kvm_handle_exception_payload_quirk(vcpu);

	memset(events, 0, sizeof(*events));

	/*
	 * The API doesn't provide the instruction length for software
	 * exceptions, so don't report them. As long as the guest RIP
	 * isn't advanced, we should expect to encounter the exception
	 * again.
	 */
	if (!kvm_exception_is_soft(ex->vector)) {
		events->exception.injected = ex->injected;
		events->exception.pending = ex->pending;
		/*
		 * For ABI compatibility, deliberately conflate
		 * pending and injected exceptions when
		 * KVM_CAP_EXCEPTION_PAYLOAD isn't enabled.
		 */
		if (!vcpu->kvm->arch.exception_payload_enabled)
			events->exception.injected |= ex->pending;
	}
	events->exception.nr = ex->vector;
	events->exception.has_error_code = ex->has_error_code;
	events->exception.error_code = ex->error_code;
	events->exception_has_payload = ex->has_payload;
	events->exception_payload = ex->payload;

	events->interrupt.injected =
		vcpu->arch.interrupt.injected && !vcpu->arch.interrupt.soft;
	events->interrupt.nr = vcpu->arch.interrupt.nr;
	events->interrupt.shadow = kvm_x86_call(get_interrupt_shadow)(vcpu);

	events->nmi.injected = vcpu->arch.nmi_injected;
	events->nmi.pending = kvm_get_nr_pending_nmis(vcpu);
	events->nmi.masked = kvm_x86_call(get_nmi_mask)(vcpu);

	/* events->sipi_vector is never valid when reporting to user space */