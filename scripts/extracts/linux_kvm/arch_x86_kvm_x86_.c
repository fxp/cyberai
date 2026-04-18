// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 70/86



/* Called within kvm->srcu read side.  */
static int vcpu_run(struct kvm_vcpu *vcpu)
{
	int r;

	vcpu->run->exit_reason = KVM_EXIT_UNKNOWN;

	for (;;) {
		/*
		 * If another guest vCPU requests a PV TLB flush in the middle
		 * of instruction emulation, the rest of the emulation could
		 * use a stale page translation. Assume that any code after
		 * this point can start executing an instruction.
		 */
		vcpu->arch.at_instruction_boundary = false;
		if (kvm_vcpu_running(vcpu)) {
			r = vcpu_enter_guest(vcpu);
		} else {
			r = vcpu_block(vcpu);
		}

		if (r <= 0)
			break;

		kvm_clear_request(KVM_REQ_UNBLOCK, vcpu);
		if (kvm_xen_has_pending_events(vcpu))
			kvm_xen_inject_pending_events(vcpu);

		if (kvm_cpu_has_pending_timer(vcpu))
			kvm_inject_pending_timer_irqs(vcpu);

		if (dm_request_for_irq_injection(vcpu) &&
			kvm_vcpu_ready_for_interrupt_injection(vcpu)) {
			r = 0;
			vcpu->run->exit_reason = KVM_EXIT_IRQ_WINDOW_OPEN;
			++vcpu->stat.request_irq_exits;
			break;
		}

		if (__xfer_to_guest_mode_work_pending()) {
			kvm_vcpu_srcu_read_unlock(vcpu);
			r = kvm_xfer_to_guest_mode_handle_work(vcpu);
			kvm_vcpu_srcu_read_lock(vcpu);
			if (r)
				return r;
		}
	}

	return r;
}

static int __kvm_emulate_halt(struct kvm_vcpu *vcpu, int state, int reason)
{
	/*
	 * The vCPU has halted, e.g. executed HLT.  Update the run state if the
	 * local APIC is in-kernel, the run loop will detect the non-runnable
	 * state and halt the vCPU.  Exit to userspace if the local APIC is
	 * managed by userspace, in which case userspace is responsible for
	 * handling wake events.
	 */
	++vcpu->stat.halt_exits;
	if (lapic_in_kernel(vcpu)) {
		if (kvm_vcpu_has_events(vcpu) || vcpu->arch.pv.pv_unhalted)
			state = KVM_MP_STATE_RUNNABLE;
		kvm_set_mp_state(vcpu, state);
		return 1;
	} else {
		vcpu->run->exit_reason = reason;
		return 0;
	}
}

int kvm_emulate_halt_noskip(struct kvm_vcpu *vcpu)
{
	return __kvm_emulate_halt(vcpu, KVM_MP_STATE_HALTED, KVM_EXIT_HLT);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_halt_noskip);

int kvm_emulate_halt(struct kvm_vcpu *vcpu)
{
	int ret = kvm_skip_emulated_instruction(vcpu);
	/*
	 * TODO: we might be squashing a GUESTDBG_SINGLESTEP-triggered
	 * KVM_EXIT_DEBUG here.
	 */
	return kvm_emulate_halt_noskip(vcpu) && ret;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_halt);

fastpath_t handle_fastpath_hlt(struct kvm_vcpu *vcpu)
{
	if (!kvm_pmu_is_fastpath_emulation_allowed(vcpu))
		return EXIT_FASTPATH_NONE;

	if (!kvm_emulate_halt(vcpu))
		return EXIT_FASTPATH_EXIT_USERSPACE;

	if (kvm_vcpu_running(vcpu))
		return EXIT_FASTPATH_REENTER_GUEST;

	return EXIT_FASTPATH_EXIT_HANDLED;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(handle_fastpath_hlt);

int kvm_emulate_ap_reset_hold(struct kvm_vcpu *vcpu)
{
	int ret = kvm_skip_emulated_instruction(vcpu);

	return __kvm_emulate_halt(vcpu, KVM_MP_STATE_AP_RESET_HOLD,
					KVM_EXIT_AP_RESET_HOLD) && ret;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_ap_reset_hold);

bool kvm_arch_dy_has_pending_interrupt(struct kvm_vcpu *vcpu)
{
	return kvm_vcpu_apicv_active(vcpu) &&
	       kvm_x86_call(dy_apicv_has_pending_interrupt)(vcpu);
}

bool kvm_arch_vcpu_preempted_in_kernel(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.preempted_in_kernel;
}

bool kvm_arch_dy_runnable(struct kvm_vcpu *vcpu)
{
	if (READ_ONCE(vcpu->arch.pv.pv_unhalted))
		return true;

	if (kvm_test_request(KVM_REQ_NMI, vcpu) ||
#ifdef CONFIG_KVM_SMM
		kvm_test_request(KVM_REQ_SMI, vcpu) ||
#endif
		 kvm_test_request(KVM_REQ_EVENT, vcpu))
		return true;

	return kvm_arch_dy_has_pending_interrupt(vcpu);
}

static inline int complete_emulated_io(struct kvm_vcpu *vcpu)
{
	return kvm_emulate_instruction(vcpu, EMULTYPE_NO_DECODE);
}

static int complete_emulated_pio(struct kvm_vcpu *vcpu)
{
	if (KVM_BUG_ON(!vcpu->arch.pio.count, vcpu->kvm))
		return -EIO;

	return complete_emulated_io(vcpu);
}

/*
 * Implements the following, as a state machine:
 *
 * read:
 *   for each fragment
 *     for each mmio piece in the fragment
 *       write gpa, len
 *       exit
 *       copy data
 *   execute insn
 *
 * write:
 *   for each fragment
 *     for each mmio piece in the fragment
 *       write gpa, len
 *       copy data
 *       exit
 */
static int complete_emulated_mmio(struct kvm_vcpu *vcpu)
{
	struct kvm_run *run = vcpu->run;
	struct kvm_mmio_fragment *frag;
	unsigned len;

	if (KVM_BUG_ON(!vcpu->mmio_needed, vcpu->kvm))
		return -EIO;

	/* Complete previous fragment */
	frag = &vcpu->mmio_fragments[vcpu->mmio_cur_fragment];
	len = min(8u, frag->len);
	if (!vcpu->mmio_is_write)
		memcpy(frag->data, run->mmio.data, len);

	if (frag->len <= 8) {
		/* Switch to the next fragment. */
		frag++;
		vcpu->mmio_cur_fragment++;
	} else {
		if (WARN_ON_ONCE(frag->data == &frag->val))
			return -EIO;

		/* Go forward to the next mmio piece. */
		frag->data += len;
		frag->gpa += len;
		frag->len -= len;
	}

	if (vcpu->mmio_cur_fragment >= vcpu->mmio_nr_fragments) {
		vcpu->mmio_needed = 0;