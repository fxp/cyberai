// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 29/56



static u64 vmx_tertiary_exec_control(struct vcpu_vmx *vmx)
{
	u64 exec_control = vmcs_config.cpu_based_3rd_exec_ctrl;

	/*
	 * IPI virtualization relies on APICv. Disable IPI virtualization if
	 * APICv is inhibited.
	 */
	if (!enable_ipiv || !kvm_vcpu_apicv_active(&vmx->vcpu))
		exec_control &= ~TERTIARY_EXEC_IPI_VIRT;

	return exec_control;
}

/*
 * Adjust a single secondary execution control bit to intercept/allow an
 * instruction in the guest.  This is usually done based on whether or not a
 * feature has been exposed to the guest in order to correctly emulate faults.
 */
static inline void
vmx_adjust_secondary_exec_control(struct vcpu_vmx *vmx, u32 *exec_control,
				  u32 control, bool enabled, bool exiting)
{
	/*
	 * If the control is for an opt-in feature, clear the control if the
	 * feature is not exposed to the guest, i.e. not enabled.  If the
	 * control is opt-out, i.e. an exiting control, clear the control if
	 * the feature _is_ exposed to the guest, i.e. exiting/interception is
	 * disabled for the associated instruction.  Note, the caller is
	 * responsible presetting exec_control to set all supported bits.
	 */
	if (enabled == exiting)
		*exec_control &= ~control;

	/*
	 * Update the nested MSR settings so that a nested VMM can/can't set
	 * controls for features that are/aren't exposed to the guest.
	 */
	if (nested &&
	    kvm_check_has_quirk(vmx->vcpu.kvm, KVM_X86_QUIRK_STUFF_FEATURE_MSRS)) {
		/*
		 * All features that can be added or removed to VMX MSRs must
		 * be supported in the first place for nested virtualization.
		 */
		if (WARN_ON_ONCE(!(vmcs_config.nested.secondary_ctls_high & control)))
			enabled = false;

		if (enabled)
			vmx->nested.msrs.secondary_ctls_high |= control;
		else
			vmx->nested.msrs.secondary_ctls_high &= ~control;
	}
}

/*
 * Wrapper macro for the common case of adjusting a secondary execution control
 * based on a single guest CPUID bit, with a dedicated feature bit.  This also
 * verifies that the control is actually supported by KVM and hardware.
 */
#define vmx_adjust_sec_exec_control(vmx, exec_control, name, feat_name, ctrl_name, exiting)	\
({												\
	struct kvm_vcpu *__vcpu = &(vmx)->vcpu;							\
	bool __enabled;										\
												\
	if (cpu_has_vmx_##name()) {								\
		__enabled = guest_cpu_cap_has(__vcpu, X86_FEATURE_##feat_name);			\
		vmx_adjust_secondary_exec_control(vmx, exec_control, SECONDARY_EXEC_##ctrl_name,\
						  __enabled, exiting);				\
	}											\
})

/* More macro magic for ENABLE_/opt-in versus _EXITING/opt-out controls. */
#define vmx_adjust_sec_exec_feature(vmx, exec_control, lname, uname) \
	vmx_adjust_sec_exec_control(vmx, exec_control, lname, uname, ENABLE_##uname, false)

#define vmx_adjust_sec_exec_exiting(vmx, exec_control, lname, uname) \
	vmx_adjust_sec_exec_control(vmx, exec_control, lname, uname, uname##_EXITING, true)

static u32 vmx_secondary_exec_control(struct vcpu_vmx *vmx)
{
	struct kvm_vcpu *vcpu = &vmx->vcpu;

	u32 exec_control = vmcs_config.cpu_based_2nd_exec_ctrl;

	if (vmx_pt_mode_is_system())
		exec_control &= ~(SECONDARY_EXEC_PT_USE_GPA | SECONDARY_EXEC_PT_CONCEAL_VMX);
	if (!cpu_need_virtualize_apic_accesses(vcpu))
		exec_control &= ~SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES;
	if (vmx->vpid == 0)
		exec_control &= ~SECONDARY_EXEC_ENABLE_VPID;
	if (!enable_ept) {
		exec_control &= ~SECONDARY_EXEC_ENABLE_EPT;
		exec_control &= ~SECONDARY_EXEC_EPT_VIOLATION_VE;
		enable_unrestricted_guest = 0;
	}
	if (!enable_unrestricted_guest)
		exec_control &= ~SECONDARY_EXEC_UNRESTRICTED_GUEST;
	if (kvm_pause_in_guest(vmx->vcpu.kvm))
		exec_control &= ~SECONDARY_EXEC_PAUSE_LOOP_EXITING;
	if (!kvm_vcpu_apicv_active(vcpu))
		exec_control &= ~(SECONDARY_EXEC_APIC_REGISTER_VIRT |
				  SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY);
	exec_control &= ~SECONDARY_EXEC_VIRTUALIZE_X2APIC_MODE;

	/*
	 * KVM doesn't support VMFUNC for L1, but the control is set in KVM's
	 * base configuration as KVM emulates VMFUNC[EPTP_SWITCHING] for L2.
	 */
	exec_control &= ~SECONDARY_EXEC_ENABLE_VMFUNC;

	/* SECONDARY_EXEC_DESC is enabled/disabled on writes to CR4.UMIP,
	 * in vmx_set_cr4.  */
	exec_control &= ~SECONDARY_EXEC_DESC;

	/* SECONDARY_EXEC_SHADOW_VMCS is enabled when L1 executes VMPTRLD
	   (handle_vmptrld).
	   We can NOT enable shadow_vmcs here because we don't have yet
	   a current VMCS12
	*/
	exec_control &= ~SECONDARY_EXEC_SHADOW_VMCS;

	/*
	 * PML is enabled/disabled when dirty logging of memsmlots changes, but
	 * it needs to be set here when dirty logging is already active, e.g.
	 * if this vCPU was created after dirty logging was enabled.
	 */
	if (!enable_pml || !atomic_read(&vcpu->kvm->nr_memslots_dirty_logging))
		exec_control &= ~SECONDARY_EXEC_ENABLE_PML;

	vmx_adjust_sec_exec_feature(vmx, &exec_control, xsaves, XSAVES);