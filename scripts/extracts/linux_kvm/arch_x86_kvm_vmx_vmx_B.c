// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 34/56



	switch (ex_no) {
	case DB_VECTOR:
		dr6 = vmx_get_exit_qual(vcpu);
		if (!(vcpu->guest_debug &
		      (KVM_GUESTDBG_SINGLESTEP | KVM_GUESTDBG_USE_HW_BP))) {
			/*
			 * If the #DB was due to ICEBP, a.k.a. INT1, skip the
			 * instruction.  ICEBP generates a trap-like #DB, but
			 * despite its interception control being tied to #DB,
			 * is an instruction intercept, i.e. the VM-Exit occurs
			 * on the ICEBP itself.  Use the inner "skip" helper to
			 * avoid single-step #DB and MTF updates, as ICEBP is
			 * higher priority.  Note, skipping ICEBP still clears
			 * STI and MOVSS blocking.
			 *
			 * For all other #DBs, set vmcs.PENDING_DBG_EXCEPTIONS.BS
			 * if single-step is enabled in RFLAGS and STI or MOVSS
			 * blocking is active, as the CPU doesn't set the bit
			 * on VM-Exit due to #DB interception.  VM-Entry has a
			 * consistency check that a single-step #DB is pending
			 * in this scenario as the previous instruction cannot
			 * have toggled RFLAGS.TF 0=>1 (because STI and POP/MOV
			 * don't modify RFLAGS), therefore the one instruction
			 * delay when activating single-step breakpoints must
			 * have already expired.  Note, the CPU sets/clears BS
			 * as appropriate for all other VM-Exits types.
			 */
			if (is_icebp(intr_info))
				WARN_ON(!skip_emulated_instruction(vcpu));
			else if ((vmx_get_rflags(vcpu) & X86_EFLAGS_TF) &&
				 (vmcs_read32(GUEST_INTERRUPTIBILITY_INFO) &
				  (GUEST_INTR_STATE_STI | GUEST_INTR_STATE_MOV_SS)))
				vmcs_writel(GUEST_PENDING_DBG_EXCEPTIONS,
					    vmcs_readl(GUEST_PENDING_DBG_EXCEPTIONS) | DR6_BS);

			kvm_queue_exception_p(vcpu, DB_VECTOR, dr6);
			return 1;
		}
		kvm_run->debug.arch.dr6 = dr6 | DR6_ACTIVE_LOW;
		kvm_run->debug.arch.dr7 = vmcs_readl(GUEST_DR7);
		fallthrough;
	case BP_VECTOR:
		/*
		 * Update instruction length as we may reinject #BP from
		 * user space while in guest debugging mode. Reading it for
		 * #DB as well causes no harm, it is not used in that case.
		 */
		vmx->vcpu.arch.event_exit_inst_len =
			vmcs_read32(VM_EXIT_INSTRUCTION_LEN);
		kvm_run->exit_reason = KVM_EXIT_DEBUG;
		kvm_run->debug.arch.pc = kvm_get_linear_rip(vcpu);
		kvm_run->debug.arch.exception = ex_no;
		break;
	case AC_VECTOR:
		if (vmx_guest_inject_ac(vcpu)) {
			kvm_queue_exception_e(vcpu, AC_VECTOR, error_code);
			return 1;
		}

		/*
		 * Handle split lock. Depending on detection mode this will
		 * either warn and disable split lock detection for this
		 * task or force SIGBUS on it.
		 */
		if (handle_guest_split_lock(kvm_rip_read(vcpu)))
			return 1;
		fallthrough;
	default:
		kvm_run->exit_reason = KVM_EXIT_EXCEPTION;
		kvm_run->ex.exception = ex_no;
		kvm_run->ex.error_code = error_code;
		break;
	}
	return 0;
}

static __always_inline int handle_external_interrupt(struct kvm_vcpu *vcpu)
{
	++vcpu->stat.irq_exits;
	return 1;
}

static int handle_triple_fault(struct kvm_vcpu *vcpu)
{
	vcpu->run->exit_reason = KVM_EXIT_SHUTDOWN;
	vcpu->mmio_needed = 0;
	return 0;
}

static int handle_io(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qualification;
	int size, in, string;
	unsigned port;

	exit_qualification = vmx_get_exit_qual(vcpu);
	string = (exit_qualification & 16) != 0;

	++vcpu->stat.io_exits;

	if (string)
		return kvm_emulate_instruction(vcpu, 0);

	port = exit_qualification >> 16;
	size = (exit_qualification & 7) + 1;
	in = (exit_qualification & 8) != 0;

	return kvm_fast_pio(vcpu, size, port, in);
}

void vmx_patch_hypercall(struct kvm_vcpu *vcpu, unsigned char *hypercall)
{
	/*
	 * Patch in the VMCALL instruction:
	 */
	hypercall[0] = 0x0f;
	hypercall[1] = 0x01;
	hypercall[2] = 0xc1;
}

/* called to set cr0 as appropriate for a mov-to-cr0 exit. */
static int handle_set_cr0(struct kvm_vcpu *vcpu, unsigned long val)
{
	if (is_guest_mode(vcpu)) {
		struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
		unsigned long orig_val = val;

		/*
		 * We get here when L2 changed cr0 in a way that did not change
		 * any of L1's shadowed bits (see nested_vmx_exit_handled_cr),
		 * but did change L0 shadowed bits. So we first calculate the
		 * effective cr0 value that L1 would like to write into the
		 * hardware. It consists of the L2-owned bits from the new
		 * value combined with the L1-owned bits from L1's guest_cr0.
		 */
		val = (val & ~vmcs12->cr0_guest_host_mask) |
			(vmcs12->guest_cr0 & vmcs12->cr0_guest_host_mask);

		if (kvm_set_cr0(vcpu, val))
			return 1;
		vmcs_writel(CR0_READ_SHADOW, orig_val);
		return 0;
	} else {
		return kvm_set_cr0(vcpu, val);
	}
}

static int handle_set_cr4(struct kvm_vcpu *vcpu, unsigned long val)
{
	if (is_guest_mode(vcpu)) {
		struct vmcs12 *vmcs12 = get_vmcs12(vcpu);
		unsigned long orig_val = val;

		/* analogously to handle_set_cr0 */
		val = (val & ~vmcs12->cr4_guest_host_mask) |
			(vmcs12->guest_cr4 & vmcs12->cr4_guest_host_mask);
		if (kvm_set_cr4(vcpu, val))
			return 1;
		vmcs_writel(CR4_READ_SHADOW, orig_val);
		return 0;
	} else
		return kvm_set_cr4(vcpu, val);
}