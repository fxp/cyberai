// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 28/56



	/*
	 * Supervisor shadow stack is not enabled on host side, i.e.,
	 * host IA32_S_CET.SHSTK_EN bit is guaranteed to 0 now, per SDM
	 * description(RDSSP instruction), SSP is not readable in CPL0,
	 * so resetting the two registers to 0s at VM-Exit does no harm
	 * to kernel execution. When execution flow exits to userspace,
	 * SSP is reloaded from IA32_PL3_SSP. Check SDM Vol.2A/B Chapter
	 * 3 and 4 for details.
	 */
	if (cpu_has_load_cet_ctrl()) {
		vmcs_writel(HOST_S_CET, kvm_host.s_cet);
		vmcs_writel(HOST_SSP, 0);
		vmcs_writel(HOST_INTR_SSP_TABLE, 0);
	}

	/*
	 * When running a guest with a mediated PMU, guest state is resident in
	 * hardware after VM-Exit.  Zero PERF_GLOBAL_CTRL on exit so that host
	 * activity doesn't bleed into the guest counters.  When running with
	 * an emulated PMU, PERF_GLOBAL_CTRL is dynamically computed on every
	 * entry/exit to merge guest and host PMU usage.
	 */
	if (enable_mediated_pmu)
		vmcs_write64(HOST_IA32_PERF_GLOBAL_CTRL, 0);
}

void set_cr4_guest_host_mask(struct vcpu_vmx *vmx)
{
	struct kvm_vcpu *vcpu = &vmx->vcpu;

	vcpu->arch.cr4_guest_owned_bits = KVM_POSSIBLE_CR4_GUEST_BITS &
					  ~vcpu->arch.cr4_guest_rsvd_bits;
	if (!enable_ept) {
		vcpu->arch.cr4_guest_owned_bits &= ~X86_CR4_TLBFLUSH_BITS;
		vcpu->arch.cr4_guest_owned_bits &= ~X86_CR4_PDPTR_BITS;
	}
	if (is_guest_mode(&vmx->vcpu))
		vcpu->arch.cr4_guest_owned_bits &=
			~get_vmcs12(vcpu)->cr4_guest_host_mask;
	vmcs_writel(CR4_GUEST_HOST_MASK, ~vcpu->arch.cr4_guest_owned_bits);
}

static u32 vmx_pin_based_exec_ctrl(struct vcpu_vmx *vmx)
{
	u32 pin_based_exec_ctrl = vmcs_config.pin_based_exec_ctrl;

	if (!kvm_vcpu_apicv_active(&vmx->vcpu))
		pin_based_exec_ctrl &= ~PIN_BASED_POSTED_INTR;

	if (!enable_vnmi)
		pin_based_exec_ctrl &= ~PIN_BASED_VIRTUAL_NMIS;

	if (!enable_preemption_timer)
		pin_based_exec_ctrl &= ~PIN_BASED_VMX_PREEMPTION_TIMER;

	return pin_based_exec_ctrl;
}

static u32 vmx_get_initial_vmentry_ctrl(void)
{
	u32 vmentry_ctrl = vmcs_config.vmentry_ctrl;

	if (vmx_pt_mode_is_system())
		vmentry_ctrl &= ~(VM_ENTRY_PT_CONCEAL_PIP |
				  VM_ENTRY_LOAD_IA32_RTIT_CTL);
	/*
	 * IA32e mode, and loading of EFER and PERF_GLOBAL_CTRL are toggled dynamically.
	 */
	vmentry_ctrl &= ~(VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL |
			  VM_ENTRY_LOAD_IA32_EFER |
			  VM_ENTRY_IA32E_MODE);

	return vmentry_ctrl;
}

static u32 vmx_get_initial_vmexit_ctrl(void)
{
	u32 vmexit_ctrl = vmcs_config.vmexit_ctrl;

	/*
	 * Not used by KVM and never set in vmcs01 or vmcs02, but emulated for
	 * nested virtualization and thus allowed to be set in vmcs12.
	 */
	vmexit_ctrl &= ~(VM_EXIT_SAVE_IA32_PAT | VM_EXIT_SAVE_IA32_EFER |
			 VM_EXIT_SAVE_VMX_PREEMPTION_TIMER);

	if (vmx_pt_mode_is_system())
		vmexit_ctrl &= ~(VM_EXIT_PT_CONCEAL_PIP |
				 VM_EXIT_CLEAR_IA32_RTIT_CTL);
	/* Loading of EFER and PERF_GLOBAL_CTRL are toggled dynamically */
	return vmexit_ctrl &
		~(VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL | VM_EXIT_LOAD_IA32_EFER |
		  VM_EXIT_SAVE_IA32_PERF_GLOBAL_CTRL);
}

void vmx_refresh_apicv_exec_ctrl(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	guard(vmx_vmcs01)(vcpu);

	pin_controls_set(vmx, vmx_pin_based_exec_ctrl(vmx));

	secondary_exec_controls_changebit(vmx,
					  SECONDARY_EXEC_APIC_REGISTER_VIRT |
					  SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY,
					  kvm_vcpu_apicv_active(vcpu));
	if (enable_ipiv)
		tertiary_exec_controls_changebit(vmx, TERTIARY_EXEC_IPI_VIRT,
						 kvm_vcpu_apicv_active(vcpu));

	vmx_update_msr_bitmap_x2apic(vcpu);
}

static u32 vmx_exec_control(struct vcpu_vmx *vmx)
{
	u32 exec_control = vmcs_config.cpu_based_exec_ctrl;

	/*
	 * Not used by KVM, but fully supported for nesting, i.e. are allowed in
	 * vmcs12 and propagated to vmcs02 when set in vmcs12.
	 */
	exec_control &= ~(CPU_BASED_RDTSC_EXITING |
			  CPU_BASED_USE_IO_BITMAPS |
			  CPU_BASED_MONITOR_TRAP_FLAG |
			  CPU_BASED_PAUSE_EXITING);

	/* INTR_WINDOW_EXITING and NMI_WINDOW_EXITING are toggled dynamically */
	exec_control &= ~(CPU_BASED_INTR_WINDOW_EXITING |
			  CPU_BASED_NMI_WINDOW_EXITING);

	if (vmx->vcpu.arch.switch_db_regs & KVM_DEBUGREG_WONT_EXIT)
		exec_control &= ~CPU_BASED_MOV_DR_EXITING;

	if (!cpu_need_tpr_shadow(&vmx->vcpu))
		exec_control &= ~CPU_BASED_TPR_SHADOW;

#ifdef CONFIG_X86_64
	if (exec_control & CPU_BASED_TPR_SHADOW)
		exec_control &= ~(CPU_BASED_CR8_LOAD_EXITING |
				  CPU_BASED_CR8_STORE_EXITING);
	else
		exec_control |= CPU_BASED_CR8_STORE_EXITING |
				CPU_BASED_CR8_LOAD_EXITING;
#endif
	/* No need to intercept CR3 access or INVPLG when using EPT. */
	if (enable_ept)
		exec_control &= ~(CPU_BASED_CR3_LOAD_EXITING |
				  CPU_BASED_CR3_STORE_EXITING |
				  CPU_BASED_INVLPG_EXITING);
	if (kvm_mwait_in_guest(vmx->vcpu.kvm))
		exec_control &= ~(CPU_BASED_MWAIT_EXITING |
				CPU_BASED_MONITOR_EXITING);
	if (kvm_hlt_in_guest(vmx->vcpu.kvm))
		exec_control &= ~CPU_BASED_HLT_EXITING;
	return exec_control;
}