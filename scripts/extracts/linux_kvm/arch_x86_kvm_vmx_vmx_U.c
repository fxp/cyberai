// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 53/56



	case x86_intercept_lgdt:
	case x86_intercept_lidt:
	case x86_intercept_lldt:
	case x86_intercept_ltr:
	case x86_intercept_sgdt:
	case x86_intercept_sidt:
	case x86_intercept_sldt:
	case x86_intercept_str:
		if (!nested_cpu_has2(vmcs12, SECONDARY_EXEC_DESC))
			return X86EMUL_CONTINUE;

		if (info->intercept == x86_intercept_lldt ||
		    info->intercept == x86_intercept_ltr ||
		    info->intercept == x86_intercept_sldt ||
		    info->intercept == x86_intercept_str)
			vm_exit_reason = EXIT_REASON_LDTR_TR;
		else
			vm_exit_reason = EXIT_REASON_GDTR_IDTR;
		/*
		 * FIXME: Decode the ModR/M to generate the correct exit
		 *        qualification for memory operands.
		 */
		break;

	case x86_intercept_hlt:
		if (!nested_cpu_has(vmcs12, CPU_BASED_HLT_EXITING))
			return X86EMUL_CONTINUE;

		vm_exit_reason = EXIT_REASON_HLT;
		break;

	case x86_intercept_pause:
		/*
		 * PAUSE is a single-byte NOP with a REPE prefix, i.e. collides
		 * with vanilla NOPs in the emulator.  Apply the interception
		 * check only to actual PAUSE instructions.  Don't check
		 * PAUSE-loop-exiting, software can't expect a given PAUSE to
		 * exit, i.e. KVM is within its rights to allow L2 to execute
		 * the PAUSE.
		 */
		if ((info->rep_prefix != REPE_PREFIX) ||
		    !nested_cpu_has(vmcs12, CPU_BASED_PAUSE_EXITING))
			return X86EMUL_CONTINUE;

		vm_exit_reason = EXIT_REASON_PAUSE_INSTRUCTION;
		break;

	/* TODO: check more intercepts... */
	default:
		return X86EMUL_UNHANDLEABLE;
	}

	exit_insn_len = abs_diff((s64)info->next_rip, (s64)info->rip);
	if (!exit_insn_len || exit_insn_len > X86_MAX_INSTRUCTION_LENGTH)
		return X86EMUL_UNHANDLEABLE;

	__nested_vmx_vmexit(vcpu, vm_exit_reason, 0, exit_qualification,
			    exit_insn_len);
	return X86EMUL_INTERCEPTED;
}

#ifdef CONFIG_X86_64
/* (a << shift) / divisor, return 1 if overflow otherwise 0 */
static inline int u64_shl_div_u64(u64 a, unsigned int shift,
				  u64 divisor, u64 *result)
{
	u64 low = a << shift, high = a >> (64 - shift);

	/* To avoid the overflow on divq */
	if (high >= divisor)
		return 1;

	/* Low hold the result, high hold rem which is discarded */
	asm("divq %2\n\t" : "=a" (low), "=d" (high) :
	    "rm" (divisor), "0" (low), "1" (high));
	*result = low;

	return 0;
}

int vmx_set_hv_timer(struct kvm_vcpu *vcpu, u64 guest_deadline_tsc,
		     bool *expired)
{
	struct vcpu_vmx *vmx;
	u64 tscl, guest_tscl, delta_tsc, lapic_timer_advance_cycles;
	struct kvm_timer *ktimer = &vcpu->arch.apic->lapic_timer;

	vmx = to_vmx(vcpu);
	tscl = rdtsc();
	guest_tscl = kvm_read_l1_tsc(vcpu, tscl);
	delta_tsc = max(guest_deadline_tsc, guest_tscl) - guest_tscl;
	lapic_timer_advance_cycles = nsec_to_cycles(vcpu,
						    ktimer->timer_advance_ns);

	if (delta_tsc > lapic_timer_advance_cycles)
		delta_tsc -= lapic_timer_advance_cycles;
	else
		delta_tsc = 0;

	/* Convert to host delta tsc if tsc scaling is enabled */
	if (vcpu->arch.l1_tsc_scaling_ratio != kvm_caps.default_tsc_scaling_ratio &&
	    delta_tsc && u64_shl_div_u64(delta_tsc,
				kvm_caps.tsc_scaling_ratio_frac_bits,
				vcpu->arch.l1_tsc_scaling_ratio, &delta_tsc))
		return -ERANGE;

	/*
	 * If the delta tsc can't fit in the 32 bit after the multi shift,
	 * we can't use the preemption timer.
	 * It's possible that it fits on later vmentries, but checking
	 * on every vmentry is costly so we just use an hrtimer.
	 */
	if (delta_tsc >> (cpu_preemption_timer_multi + 32))
		return -ERANGE;

	vmx->hv_deadline_tsc = tscl + delta_tsc;
	*expired = !delta_tsc;
	return 0;
}

void vmx_cancel_hv_timer(struct kvm_vcpu *vcpu)
{
	to_vmx(vcpu)->hv_deadline_tsc = -1;
}
#endif

void vmx_update_cpu_dirty_logging(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	if (WARN_ON_ONCE(!enable_pml))
		return;

	guard(vmx_vmcs01)(vcpu);

	/*
	 * Note, nr_memslots_dirty_logging can be changed concurrent with this
	 * code, but in that case another update request will be made and so
	 * the guest will never run with a stale PML value.
	 */
	if (atomic_read(&vcpu->kvm->nr_memslots_dirty_logging))
		secondary_exec_controls_setbit(vmx, SECONDARY_EXEC_ENABLE_PML);
	else
		secondary_exec_controls_clearbit(vmx, SECONDARY_EXEC_ENABLE_PML);
}

void vmx_setup_mce(struct kvm_vcpu *vcpu)
{
	if (vcpu->arch.mcg_cap & MCG_LMCE_P)
		to_vmx(vcpu)->msr_ia32_feature_control_valid_bits |=
			FEAT_CTL_LMCE_ENABLED;
	else
		to_vmx(vcpu)->msr_ia32_feature_control_valid_bits &=
			~FEAT_CTL_LMCE_ENABLED;
}

#ifdef CONFIG_KVM_SMM
int vmx_smi_allowed(struct kvm_vcpu *vcpu, bool for_injection)
{
	/* we need a nested vmexit to enter SMM, postpone if run is pending */
	if (vcpu->arch.nested_run_pending)
		return -EBUSY;
	return !is_smm(vcpu);
}

int vmx_enter_smm(struct kvm_vcpu *vcpu, union kvm_smram *smram)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);