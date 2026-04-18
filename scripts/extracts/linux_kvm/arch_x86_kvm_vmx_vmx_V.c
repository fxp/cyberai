// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 54/56



	/*
	 * TODO: Implement custom flows for forcing the vCPU out/in of L2 on
	 * SMI and RSM.  Using the common VM-Exit + VM-Enter routines is wrong
	 * SMI and RSM only modify state that is saved and restored via SMRAM.
	 * E.g. most MSRs are left untouched, but many are modified by VM-Exit
	 * and VM-Enter, and thus L2's values may be corrupted on SMI+RSM.
	 */
	vmx->nested.smm.guest_mode = is_guest_mode(vcpu);
	if (vmx->nested.smm.guest_mode)
		nested_vmx_vmexit(vcpu, -1, 0, 0);

	vmx->nested.smm.vmxon = vmx->nested.vmxon;
	vmx->nested.vmxon = false;
	vmx_clear_hlt(vcpu);
	return 0;
}

int vmx_leave_smm(struct kvm_vcpu *vcpu, const union kvm_smram *smram)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	int ret;

	if (vmx->nested.smm.vmxon) {
		vmx->nested.vmxon = true;
		vmx->nested.smm.vmxon = false;
	}

	if (vmx->nested.smm.guest_mode) {
		/* Triple fault if the state is invalid.  */
		if (nested_vmx_check_restored_vmcs12(vcpu) < 0)
			return 1;

		ret = nested_vmx_enter_non_root_mode(vcpu, false);
		if (ret != NVMX_VMENTRY_SUCCESS)
			return 1;

		vcpu->arch.nested_run_pending = KVM_NESTED_RUN_PENDING;
		vmx->nested.smm.guest_mode = false;
	}
	return 0;
}

void vmx_enable_smi_window(struct kvm_vcpu *vcpu)
{
	/* RSM will cause a vmexit anyway.  */
}
#endif

bool vmx_apic_init_signal_blocked(struct kvm_vcpu *vcpu)
{
	return to_vmx(vcpu)->nested.vmxon && !is_guest_mode(vcpu);
}

void vmx_migrate_timers(struct kvm_vcpu *vcpu)
{
	if (is_guest_mode(vcpu)) {
		struct hrtimer *timer = &to_vmx(vcpu)->nested.preemption_timer;

		if (hrtimer_try_to_cancel(timer) == 1)
			hrtimer_start_expires(timer, HRTIMER_MODE_ABS_PINNED);
	}
}

void vmx_hardware_unsetup(void)
{
	kvm_set_posted_intr_wakeup_handler(NULL);

	if (nested)
		nested_vmx_hardware_unsetup();
}

void vmx_vm_destroy(struct kvm *kvm)
{
	struct kvm_vmx *kvm_vmx = to_kvm_vmx(kvm);

	free_pages((unsigned long)kvm_vmx->pid_table, vmx_get_pid_table_order(kvm));
}

/*
 * Note, the SDM states that the linear address is masked *after* the modified
 * canonicality check, whereas KVM masks (untags) the address and then performs
 * a "normal" canonicality check.  Functionally, the two methods are identical,
 * and when the masking occurs relative to the canonicality check isn't visible
 * to software, i.e. KVM's behavior doesn't violate the SDM.
 */
gva_t vmx_get_untagged_addr(struct kvm_vcpu *vcpu, gva_t gva, unsigned int flags)
{
	int lam_bit;
	unsigned long cr3_bits;

	if (flags & (X86EMUL_F_FETCH | X86EMUL_F_IMPLICIT | X86EMUL_F_INVLPG))
		return gva;

	if (!is_64_bit_mode(vcpu))
		return gva;

	/*
	 * Bit 63 determines if the address should be treated as user address
	 * or a supervisor address.
	 */
	if (!(gva & BIT_ULL(63))) {
		cr3_bits = kvm_get_active_cr3_lam_bits(vcpu);
		if (!(cr3_bits & (X86_CR3_LAM_U57 | X86_CR3_LAM_U48)))
			return gva;

		/* LAM_U48 is ignored if LAM_U57 is set. */
		lam_bit = cr3_bits & X86_CR3_LAM_U57 ? 56 : 47;
	} else {
		if (!kvm_is_cr4_bit_set(vcpu, X86_CR4_LAM_SUP))
			return gva;

		lam_bit = kvm_is_cr4_bit_set(vcpu, X86_CR4_LA57) ? 56 : 47;
	}

	/*
	 * Untag the address by sign-extending the lam_bit, but NOT to bit 63.
	 * Bit 63 is retained from the raw virtual address so that untagging
	 * doesn't change a user access to a supervisor access, and vice versa.
	 */
	return (sign_extend64(gva, lam_bit) & ~BIT_ULL(63)) | (gva & BIT_ULL(63));
}

static unsigned int vmx_handle_intel_pt_intr(void)
{
	struct kvm_vcpu *vcpu = kvm_get_running_vcpu();

	/* '0' on failure so that the !PT case can use a RET0 static call. */
	if (!vcpu || !kvm_handling_nmi_from_guest(vcpu))
		return 0;

	kvm_make_request(KVM_REQ_PMI, vcpu);
	__set_bit(MSR_CORE_PERF_GLOBAL_OVF_CTRL_TRACE_TOPA_PMI_BIT,
		  (unsigned long *)&vcpu->arch.pmu.global_status);
	return 1;
}

static __init void vmx_setup_user_return_msrs(void)
{

	/*
	 * Though SYSCALL is only supported in 64-bit mode on Intel CPUs, kvm
	 * will emulate SYSCALL in legacy mode if the vendor string in guest
	 * CPUID.0:{EBX,ECX,EDX} is "AuthenticAMD" or "AMDisbetter!" To
	 * support this emulation, MSR_STAR is included in the list for i386,
	 * but is never loaded into hardware.  MSR_CSTAR is also never loaded
	 * into hardware and is here purely for emulation purposes.
	 */
	const u32 vmx_uret_msrs_list[] = {
	#ifdef CONFIG_X86_64
		MSR_SYSCALL_MASK, MSR_LSTAR, MSR_CSTAR,
	#endif
		MSR_EFER, MSR_TSC_AUX, MSR_STAR,
		MSR_IA32_TSX_CTRL,
	};
	int i;

	BUILD_BUG_ON(ARRAY_SIZE(vmx_uret_msrs_list) != MAX_NR_USER_RETURN_MSRS);

	for (i = 0; i < ARRAY_SIZE(vmx_uret_msrs_list); ++i)
		kvm_add_user_return_msr(vmx_uret_msrs_list[i]);
}

static void __init vmx_setup_me_spte_mask(void)
{
	u64 me_mask = 0;