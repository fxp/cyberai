// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 37/56



	/*
	 * A nested guest cannot optimize MMIO vmexits, because we have an
	 * nGPA here instead of the required GPA.
	 */
	gpa = vmcs_read64(GUEST_PHYSICAL_ADDRESS);
	if (!is_guest_mode(vcpu) &&
	    !kvm_io_bus_write(vcpu, KVM_FAST_MMIO_BUS, gpa, 0, NULL)) {
		trace_kvm_fast_mmio(gpa);
		return kvm_skip_emulated_instruction(vcpu);
	}

	return kvm_mmu_page_fault(vcpu, gpa, PFERR_RSVD_MASK, NULL, 0);
}

static int handle_nmi_window(struct kvm_vcpu *vcpu)
{
	if (KVM_BUG_ON(!enable_vnmi, vcpu->kvm))
		return -EIO;

	exec_controls_clearbit(to_vmx(vcpu), CPU_BASED_NMI_WINDOW_EXITING);
	++vcpu->stat.nmi_window_exits;
	kvm_make_request(KVM_REQ_EVENT, vcpu);

	return 1;
}

/*
 * Returns true if emulation is required (due to the vCPU having invalid state
 * with unsrestricted guest mode disabled) and KVM can't faithfully emulate the
 * current vCPU state.
 */
static bool vmx_unhandleable_emulation_required(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	if (!vmx->vt.emulation_required)
		return false;

	/*
	 * It is architecturally impossible for emulation to be required when a
	 * nested VM-Enter is pending completion, as VM-Enter will VM-Fail if
	 * guest state is invalid and unrestricted guest is disabled, i.e. KVM
	 * should synthesize VM-Fail instead emulation L2 code.  This path is
	 * only reachable if userspace modifies L2 guest state after KVM has
	 * performed the nested VM-Enter consistency checks.
	 */
	if (vcpu->arch.nested_run_pending)
		return true;

	/*
	 * KVM only supports emulating exceptions if the vCPU is in Real Mode.
	 * If emulation is required, KVM can't perform a successful VM-Enter to
	 * inject the exception.
	 */
	return !vmx->rmode.vm86_active &&
	       (kvm_is_exception_pending(vcpu) || vcpu->arch.exception.injected);
}

static int handle_invalid_guest_state(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	bool intr_window_requested;
	unsigned count = 130;

	intr_window_requested = exec_controls_get(vmx) &
				CPU_BASED_INTR_WINDOW_EXITING;

	while (vmx->vt.emulation_required && count-- != 0) {
		if (intr_window_requested && !vmx_interrupt_blocked(vcpu))
			return handle_interrupt_window(&vmx->vcpu);

		if (kvm_test_request(KVM_REQ_EVENT, vcpu))
			return 1;

		/*
		 * Ensure that any updates to kvm->buses[] observed by the
		 * previous instruction (emulated or otherwise) are also
		 * visible to the instruction KVM is about to emulate.
		 */
		smp_rmb();

		if (!kvm_emulate_instruction(vcpu, 0))
			return 0;

		if (vmx_unhandleable_emulation_required(vcpu)) {
			kvm_prepare_emulation_failure_exit(vcpu);
			return 0;
		}

		if (vcpu->arch.halt_request) {
			vcpu->arch.halt_request = 0;
			return kvm_emulate_halt_noskip(vcpu);
		}

		/*
		 * Note, return 1 and not 0, vcpu_run() will invoke
		 * xfer_to_guest_mode() which will create a proper return
		 * code.
		 */
		if (__xfer_to_guest_mode_work_pending())
			return 1;
	}

	return 1;
}

int vmx_vcpu_pre_run(struct kvm_vcpu *vcpu)
{
	if (vmx_unhandleable_emulation_required(vcpu)) {
		kvm_prepare_emulation_failure_exit(vcpu);
		return 0;
	}

	return 1;
}

/*
 * Indicate a busy-waiting vcpu in spinlock. We do not enable the PAUSE
 * exiting, so only get here on cpu with PAUSE-Loop-Exiting.
 */
static int handle_pause(struct kvm_vcpu *vcpu)
{
	if (!kvm_pause_in_guest(vcpu->kvm))
		grow_ple_window(vcpu);

	/*
	 * Intel sdm vol3 ch-25.1.3 says: The "PAUSE-loop exiting"
	 * VM-execution control is ignored if CPL > 0. OTOH, KVM
	 * never set PAUSE_EXITING and just set PLE if supported,
	 * so the vcpu must be CPL=0 if it gets a PAUSE exit.
	 */
	kvm_vcpu_on_spin(vcpu, true);
	return kvm_skip_emulated_instruction(vcpu);
}

static int handle_monitor_trap(struct kvm_vcpu *vcpu)
{
	return 1;
}

static int handle_invpcid(struct kvm_vcpu *vcpu)
{
	u32 vmx_instruction_info;
	unsigned long type;
	gva_t gva;
	struct {
		u64 pcid;
		u64 gla;
	} operand;
	int gpr_index;

	if (!guest_cpu_cap_has(vcpu, X86_FEATURE_INVPCID)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

	vmx_instruction_info = vmcs_read32(VMX_INSTRUCTION_INFO);
	gpr_index = vmx_get_instr_info_reg2(vmx_instruction_info);
	type = kvm_register_read(vcpu, gpr_index);

	/* According to the Intel instruction reference, the memory operand
	 * is read even if it isn't needed (e.g., for type==all)
	 */
	if (get_vmx_mem_address(vcpu, vmx_get_exit_qual(vcpu),
				vmx_instruction_info, false,
				sizeof(operand), &gva))
		return 1;

	return kvm_handle_invpcid(vcpu, type, gva);
}

static int handle_pml_full(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qualification;

	trace_kvm_pml_full(vcpu->vcpu_id);

	exit_qualification = vmx_get_exit_qual(vcpu);