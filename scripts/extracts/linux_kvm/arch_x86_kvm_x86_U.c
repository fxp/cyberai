// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 53/86



static void prepare_emulation_failure_exit(struct kvm_vcpu *vcpu, u64 *data,
					   u8 ndata, u8 *insn_bytes, u8 insn_size)
{
	struct kvm_run *run = vcpu->run;
	u64 info[5];
	u8 info_start;

	/*
	 * Zero the whole array used to retrieve the exit info, as casting to
	 * u32 for select entries will leave some chunks uninitialized.
	 */
	memset(&info, 0, sizeof(info));

	kvm_x86_call(get_exit_info)(vcpu, (u32 *)&info[0], &info[1], &info[2],
				    (u32 *)&info[3], (u32 *)&info[4]);

	run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
	run->emulation_failure.suberror = KVM_INTERNAL_ERROR_EMULATION;

	/*
	 * There's currently space for 13 entries, but 5 are used for the exit
	 * reason and info.  Restrict to 4 to reduce the maintenance burden
	 * when expanding kvm_run.emulation_failure in the future.
	 */
	if (WARN_ON_ONCE(ndata > 4))
		ndata = 4;

	/* Always include the flags as a 'data' entry. */
	info_start = 1;
	run->emulation_failure.flags = 0;

	if (insn_size) {
		BUILD_BUG_ON((sizeof(run->emulation_failure.insn_size) +
			      sizeof(run->emulation_failure.insn_bytes) != 16));
		info_start += 2;
		run->emulation_failure.flags |=
			KVM_INTERNAL_ERROR_EMULATION_FLAG_INSTRUCTION_BYTES;
		run->emulation_failure.insn_size = insn_size;
		memset(run->emulation_failure.insn_bytes, 0x90,
		       sizeof(run->emulation_failure.insn_bytes));
		memcpy(run->emulation_failure.insn_bytes, insn_bytes, insn_size);
	}

	memcpy(&run->internal.data[info_start], info, sizeof(info));
	memcpy(&run->internal.data[info_start + ARRAY_SIZE(info)], data,
	       ndata * sizeof(data[0]));

	run->emulation_failure.ndata = info_start + ARRAY_SIZE(info) + ndata;
}

static void prepare_emulation_ctxt_failure_exit(struct kvm_vcpu *vcpu)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;

	prepare_emulation_failure_exit(vcpu, NULL, 0, ctxt->fetch.data,
				       ctxt->fetch.end - ctxt->fetch.data);
}

void __kvm_prepare_emulation_failure_exit(struct kvm_vcpu *vcpu, u64 *data,
					  u8 ndata)
{
	prepare_emulation_failure_exit(vcpu, data, ndata, NULL, 0);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_prepare_emulation_failure_exit);

void kvm_prepare_emulation_failure_exit(struct kvm_vcpu *vcpu)
{
	__kvm_prepare_emulation_failure_exit(vcpu, NULL, 0);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_prepare_emulation_failure_exit);

void kvm_prepare_event_vectoring_exit(struct kvm_vcpu *vcpu, gpa_t gpa)
{
	u32 reason, intr_info, error_code;
	struct kvm_run *run = vcpu->run;
	u64 info1, info2;
	int ndata = 0;

	kvm_x86_call(get_exit_info)(vcpu, &reason, &info1, &info2,
				    &intr_info, &error_code);

	run->internal.data[ndata++] = info2;
	run->internal.data[ndata++] = reason;
	run->internal.data[ndata++] = info1;
	run->internal.data[ndata++] = gpa;
	run->internal.data[ndata++] = vcpu->arch.last_vmentry_cpu;

	run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
	run->internal.suberror = KVM_INTERNAL_ERROR_DELIVERY_EV;
	run->internal.ndata = ndata;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_prepare_event_vectoring_exit);

void kvm_prepare_unexpected_reason_exit(struct kvm_vcpu *vcpu, u64 exit_reason)
{
	vcpu_unimpl(vcpu, "unexpected exit reason 0x%llx\n", exit_reason);

	vcpu->run->exit_reason = KVM_EXIT_INTERNAL_ERROR;
	vcpu->run->internal.suberror = KVM_INTERNAL_ERROR_UNEXPECTED_EXIT_REASON;
	vcpu->run->internal.ndata = 2;
	vcpu->run->internal.data[0] = exit_reason;
	vcpu->run->internal.data[1] = vcpu->arch.last_vmentry_cpu;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_prepare_unexpected_reason_exit);

static int handle_emulation_failure(struct kvm_vcpu *vcpu, int emulation_type)
{
	struct kvm *kvm = vcpu->kvm;

	++vcpu->stat.insn_emulation_fail;
	trace_kvm_emulate_insn_failed(vcpu);

	if (emulation_type & EMULTYPE_VMWARE_GP) {
		kvm_queue_exception_e(vcpu, GP_VECTOR, 0);
		return 1;
	}

	if (kvm->arch.exit_on_emulation_error ||
	    (emulation_type & EMULTYPE_SKIP)) {
		prepare_emulation_ctxt_failure_exit(vcpu);
		return 0;
	}

	kvm_queue_exception(vcpu, UD_VECTOR);

	if (!is_guest_mode(vcpu) && kvm_x86_call(get_cpl)(vcpu) == 0) {
		prepare_emulation_ctxt_failure_exit(vcpu);
		return 0;
	}

	return 1;
}

static bool kvm_unprotect_and_retry_on_failure(struct kvm_vcpu *vcpu,
					       gpa_t cr2_or_gpa,
					       int emulation_type)
{
	if (!(emulation_type & EMULTYPE_ALLOW_RETRY_PF))
		return false;

	/*
	 * If the failed instruction faulted on an access to page tables that
	 * are used to translate any part of the instruction, KVM can't resolve
	 * the issue by unprotecting the gfn, as zapping the shadow page will
	 * result in the instruction taking a !PRESENT page fault and thus put
	 * the vCPU into an infinite loop of page faults.  E.g. KVM will create
	 * a SPTE and write-protect the gfn to resolve the !PRESENT fault, and
	 * then zap the SPTE to unprotect the gfn, and then do it all over
	 * again.  Report the error to userspace.
	 */
	if (emulation_type & EMULTYPE_WRITE_PF_TO_SP)
		return false;