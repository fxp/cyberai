// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 54/86



	/*
	 * If emulation may have been triggered by a write to a shadowed page
	 * table, unprotect the gfn (zap any relevant SPTEs) and re-enter the
	 * guest to let the CPU re-execute the instruction in the hope that the
	 * CPU can cleanly execute the instruction that KVM failed to emulate.
	 */
	__kvm_mmu_unprotect_gfn_and_retry(vcpu, cr2_or_gpa, true);

	/*
	 * Retry even if _this_ vCPU didn't unprotect the gfn, as it's possible
	 * all SPTEs were already zapped by a different task.  The alternative
	 * is to report the error to userspace and likely terminate the guest,
	 * and the last_retry_{eip,addr} checks will prevent retrying the page
	 * fault indefinitely, i.e. there's nothing to lose by retrying.
	 */
	return true;
}

static int complete_emulated_mmio(struct kvm_vcpu *vcpu);
static int complete_emulated_pio(struct kvm_vcpu *vcpu);

static int kvm_vcpu_check_hw_bp(unsigned long addr, u32 type, u32 dr7,
				unsigned long *db)
{
	u32 dr6 = 0;
	int i;
	u32 enable, rwlen;

	enable = dr7;
	rwlen = dr7 >> 16;
	for (i = 0; i < 4; i++, enable >>= 2, rwlen >>= 4)
		if ((enable & 3) && (rwlen & 15) == type && db[i] == addr)
			dr6 |= (1 << i);
	return dr6;
}

static int kvm_vcpu_do_singlestep(struct kvm_vcpu *vcpu)
{
	struct kvm_run *kvm_run = vcpu->run;

	if (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP) {
		kvm_run->debug.arch.dr6 = DR6_BS | DR6_ACTIVE_LOW;
		kvm_run->debug.arch.pc = kvm_get_linear_rip(vcpu);
		kvm_run->debug.arch.exception = DB_VECTOR;
		kvm_run->exit_reason = KVM_EXIT_DEBUG;
		return 0;
	}
	kvm_queue_exception_p(vcpu, DB_VECTOR, DR6_BS);
	return 1;
}

int kvm_skip_emulated_instruction(struct kvm_vcpu *vcpu)
{
	unsigned long rflags = kvm_x86_call(get_rflags)(vcpu);
	int r;

	r = kvm_x86_call(skip_emulated_instruction)(vcpu);
	if (unlikely(!r))
		return 0;

	kvm_pmu_instruction_retired(vcpu);

	/*
	 * rflags is the old, "raw" value of the flags.  The new value has
	 * not been saved yet.
	 *
	 * This is correct even for TF set by the guest, because "the
	 * processor will not generate this exception after the instruction
	 * that sets the TF flag".
	 */
	if (unlikely(rflags & X86_EFLAGS_TF))
		r = kvm_vcpu_do_singlestep(vcpu);
	return r;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_skip_emulated_instruction);

static bool kvm_is_code_breakpoint_inhibited(struct kvm_vcpu *vcpu)
{
	if (kvm_get_rflags(vcpu) & X86_EFLAGS_RF)
		return true;

	/*
	 * Intel compatible CPUs inhibit code #DBs when MOV/POP SS blocking is
	 * active, but AMD compatible CPUs do not.
	 */
	if (!guest_cpuid_is_intel_compatible(vcpu))
		return false;

	return kvm_x86_call(get_interrupt_shadow)(vcpu) & KVM_X86_SHADOW_INT_MOV_SS;
}

static bool kvm_vcpu_check_code_breakpoint(struct kvm_vcpu *vcpu,
					   int emulation_type, int *r)
{
	WARN_ON_ONCE(emulation_type & EMULTYPE_NO_DECODE);

	/*
	 * Do not check for code breakpoints if hardware has already done the
	 * checks, as inferred from the emulation type.  On NO_DECODE and SKIP,
	 * the instruction has passed all exception checks, and all intercepted
	 * exceptions that trigger emulation have lower priority than code
	 * breakpoints, i.e. the fact that the intercepted exception occurred
	 * means any code breakpoints have already been serviced.
	 *
	 * Note, KVM needs to check for code #DBs on EMULTYPE_TRAP_UD_FORCED as
	 * hardware has checked the RIP of the magic prefix, but not the RIP of
	 * the instruction being emulated.  The intent of forced emulation is
	 * to behave as if KVM intercepted the instruction without an exception
	 * and without a prefix.
	 */
	if (emulation_type & (EMULTYPE_NO_DECODE | EMULTYPE_SKIP |
			      EMULTYPE_TRAP_UD | EMULTYPE_VMWARE_GP | EMULTYPE_PF))
		return false;

	if (unlikely(vcpu->guest_debug & KVM_GUESTDBG_USE_HW_BP) &&
	    (vcpu->arch.guest_debug_dr7 & DR7_BP_EN_MASK)) {
		struct kvm_run *kvm_run = vcpu->run;
		unsigned long eip = kvm_get_linear_rip(vcpu);
		u32 dr6 = kvm_vcpu_check_hw_bp(eip, 0,
					   vcpu->arch.guest_debug_dr7,
					   vcpu->arch.eff_db);

		if (dr6 != 0) {
			kvm_run->debug.arch.dr6 = dr6 | DR6_ACTIVE_LOW;
			kvm_run->debug.arch.pc = eip;
			kvm_run->debug.arch.exception = DB_VECTOR;
			kvm_run->exit_reason = KVM_EXIT_DEBUG;
			*r = 0;
			return true;
		}
	}

	if (unlikely(vcpu->arch.dr7 & DR7_BP_EN_MASK) &&
	    !kvm_is_code_breakpoint_inhibited(vcpu)) {
		unsigned long eip = kvm_get_linear_rip(vcpu);
		u32 dr6 = kvm_vcpu_check_hw_bp(eip, 0,
					   vcpu->arch.dr7,
					   vcpu->arch.db);

		if (dr6 != 0) {
			kvm_queue_exception_p(vcpu, DB_VECTOR, dr6);
			*r = 1;
			return true;
		}
	}

	return false;
}