// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 32/36



	BUILD_BUG_ON(offsetof(struct vmcb, save) != 0x400);

	svm_copy_vmrun_state(map_save.hva + 0x400,
			     &svm->vmcb01.ptr->save);

	kvm_vcpu_unmap(vcpu, &map_save);
	return 0;
}

static int svm_leave_smm(struct kvm_vcpu *vcpu, const union kvm_smram *smram)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	struct kvm_host_map map, map_save;
	struct vmcb *vmcb12;
	int ret;

	const struct kvm_smram_state_64 *smram64 = &smram->smram64;

	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_LM))
		return 0;

	/* Non-zero if SMI arrived while vCPU was in guest mode. */
	if (!smram64->svm_guest_flag)
		return 0;

	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_SVM))
		return 1;

	if (!(smram64->efer & EFER_SVME))
		return 1;

	if (kvm_vcpu_map(vcpu, gpa_to_gfn(smram64->svm_guest_vmcb_gpa), &map))
		return 1;

	ret = 1;
	if (kvm_vcpu_map(vcpu, gpa_to_gfn(svm->nested.hsave_msr), &map_save))
		goto unmap_map;

	if (svm_allocate_nested(svm))
		goto unmap_save;

	/*
	 * Restore L1 host state from L1 HSAVE area as VMCB01 was
	 * used during SMM (see svm_enter_smm())
	 */

	svm_copy_vmrun_state(&svm->vmcb01.ptr->save, map_save.hva + 0x400);

	/*
	 * Enter the nested guest now
	 */

	vmcb_mark_all_dirty(svm->vmcb01.ptr);

	vmcb12 = map.hva;
	nested_copy_vmcb_control_to_cache(svm, &vmcb12->control);
	nested_copy_vmcb_save_to_cache(svm, &vmcb12->save);

	if (nested_svm_check_cached_vmcb12(vcpu) < 0)
		goto unmap_save;

	if (enter_svm_guest_mode(vcpu, smram64->svm_guest_vmcb_gpa, false) != 0)
		goto unmap_save;

	ret = 0;
	vcpu->arch.nested_run_pending = KVM_NESTED_RUN_PENDING;

unmap_save:
	kvm_vcpu_unmap(vcpu, &map_save);
unmap_map:
	kvm_vcpu_unmap(vcpu, &map);
	return ret;
}

static void svm_enable_smi_window(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	if (!gif_set(svm)) {
		if (vgif)
			svm_set_intercept(svm, INTERCEPT_STGI);
		/* STGI will cause a vm exit */
	} else {
		/* We must be in SMM; RSM will cause a vmexit anyway.  */
	}
}
#endif

static int svm_check_emulate_instruction(struct kvm_vcpu *vcpu, int emul_type,
					 void *insn, int insn_len)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	bool smep, smap, is_user;
	u64 error_code;

	/* Check that emulation is possible during event vectoring */
	if ((svm->vmcb->control.exit_int_info & SVM_EXITINTINFO_TYPE_MASK) &&
	    !kvm_can_emulate_event_vectoring(emul_type))
		return X86EMUL_UNHANDLEABLE_VECTORING;

	/* Emulation is always possible when KVM has access to all guest state. */
	if (!is_sev_guest(vcpu))
		return X86EMUL_CONTINUE;

	/* #UD and #GP should never be intercepted for SEV guests. */
	WARN_ON_ONCE(emul_type & (EMULTYPE_TRAP_UD |
				  EMULTYPE_TRAP_UD_FORCED |
				  EMULTYPE_VMWARE_GP));

	/*
	 * Emulation is impossible for SEV-ES guests as KVM doesn't have access
	 * to guest register state.
	 */
	if (is_sev_es_guest(vcpu))
		return X86EMUL_RETRY_INSTR;

	/*
	 * Emulation is possible if the instruction is already decoded, e.g.
	 * when completing I/O after returning from userspace.
	 */
	if (emul_type & EMULTYPE_NO_DECODE)
		return X86EMUL_CONTINUE;

	/*
	 * Emulation is possible for SEV guests if and only if a prefilled
	 * buffer containing the bytes of the intercepted instruction is
	 * available. SEV guest memory is encrypted with a guest specific key
	 * and cannot be decrypted by KVM, i.e. KVM would read ciphertext and
	 * decode garbage.
	 *
	 * If KVM is NOT trying to simply skip an instruction, inject #UD if
	 * KVM reached this point without an instruction buffer.  In practice,
	 * this path should never be hit by a well-behaved guest, e.g. KVM
	 * doesn't intercept #UD or #GP for SEV guests, but this path is still
	 * theoretically reachable, e.g. via unaccelerated fault-like AVIC
	 * access, and needs to be handled by KVM to avoid putting the guest
	 * into an infinite loop.   Injecting #UD is somewhat arbitrary, but
	 * its the least awful option given lack of insight into the guest.
	 *
	 * If KVM is trying to skip an instruction, simply resume the guest.
	 * If a #NPF occurs while the guest is vectoring an INT3/INTO, then KVM
	 * will attempt to re-inject the INT3/INTO and skip the instruction.
	 * In that scenario, retrying the INT3/INTO and hoping the guest will
	 * make forward progress is the only option that has a chance of
	 * success (and in practice it will work the vast majority of the time).
	 */
	if (unlikely(!insn)) {
		if (emul_type & EMULTYPE_SKIP)
			return X86EMUL_UNHANDLEABLE;

		kvm_queue_exception(vcpu, UD_VECTOR);
		return X86EMUL_PROPAGATE_FAULT;
	}

	/*
	 * Emulate for SEV guests if the insn buffer is not empty.  The buffer
	 * will be empty if the DecodeAssist microcode cannot fetch bytes for
	 * the faulting instruction because the code fetch itself faulted, e.g.
	 * the guest attempted to fetch from emulated MMIO or a guest page
	 * table used to translate CS:RIP resides in emulated MMIO.
	 */
	if (likely(insn_len))
		return X86EMUL_CONTINUE;