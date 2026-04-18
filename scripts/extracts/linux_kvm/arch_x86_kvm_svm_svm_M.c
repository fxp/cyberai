// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 13/36



static bool is_erratum_383(void)
{
	int i;
	u64 value;

	if (!erratum_383_found)
		return false;

	if (native_read_msr_safe(MSR_IA32_MC0_STATUS, &value))
		return false;

	/* Bit 62 may or may not be set for this mce */
	value &= ~(1ULL << 62);

	if (value != 0xb600000000010015ULL)
		return false;

	/* Clear MCi_STATUS registers */
	for (i = 0; i < 6; ++i)
		native_write_msr_safe(MSR_IA32_MCx_STATUS(i), 0);

	if (!native_read_msr_safe(MSR_IA32_MCG_STATUS, &value)) {
		value &= ~(1ULL << 2);
		native_write_msr_safe(MSR_IA32_MCG_STATUS, value);
	}

	/* Flush tlb to evict multi-match entries */
	__flush_tlb_all();

	return true;
}

static void svm_handle_mce(struct kvm_vcpu *vcpu)
{
	if (is_erratum_383()) {
		/*
		 * Erratum 383 triggered. Guest state is corrupt so kill the
		 * guest.
		 */
		pr_err("Guest triggered AMD Erratum 383\n");

		kvm_make_request(KVM_REQ_TRIPLE_FAULT, vcpu);

		return;
	}

	/*
	 * On an #MC intercept the MCE handler is not called automatically in
	 * the host. So do it by hand here.
	 */
	kvm_machine_check();
}

static int mc_interception(struct kvm_vcpu *vcpu)
{
	return 1;
}

static int shutdown_interception(struct kvm_vcpu *vcpu)
{
	struct kvm_run *kvm_run = vcpu->run;
	struct vcpu_svm *svm = to_svm(vcpu);


	/*
	 * VMCB is undefined after a SHUTDOWN intercept.  INIT the vCPU to put
	 * the VMCB in a known good state.  Unfortuately, KVM doesn't have
	 * KVM_MP_STATE_SHUTDOWN and can't add it without potentially breaking
	 * userspace.  At a platform view, INIT is acceptable behavior as
	 * there exist bare metal platforms that automatically INIT the CPU
	 * in response to shutdown.
	 *
	 * The VM save area for SEV-ES guests has already been encrypted so it
	 * cannot be reinitialized, i.e. synthesizing INIT is futile.
	 */
	if (!is_sev_es_guest(vcpu)) {
		clear_page(svm->vmcb);
#ifdef CONFIG_KVM_SMM
		if (is_smm(vcpu))
			kvm_smm_changed(vcpu, false);
#endif
		kvm_vcpu_reset(vcpu, true);
	}

	kvm_run->exit_reason = KVM_EXIT_SHUTDOWN;
	return 0;
}

static int io_interception(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u32 io_info = svm->vmcb->control.exit_info_1; /* address size bug? */
	int size, in, string;
	unsigned port;

	++vcpu->stat.io_exits;
	string = (io_info & SVM_IOIO_STR_MASK) != 0;
	in = (io_info & SVM_IOIO_TYPE_MASK) != 0;
	port = io_info >> 16;
	size = (io_info & SVM_IOIO_SIZE_MASK) >> SVM_IOIO_SIZE_SHIFT;

	if (string) {
		if (is_sev_es_guest(vcpu))
			return sev_es_string_io(svm, size, port, in);
		else
			return kvm_emulate_instruction(vcpu, 0);
	}

	svm->next_rip = svm->vmcb->control.exit_info_2;

	return kvm_fast_pio(vcpu, size, port, in);
}

static int nmi_interception(struct kvm_vcpu *vcpu)
{
	return 1;
}

static int smi_interception(struct kvm_vcpu *vcpu)
{
	return 1;
}

static int intr_interception(struct kvm_vcpu *vcpu)
{
	++vcpu->stat.irq_exits;
	return 1;
}

static int vmload_vmsave_interception(struct kvm_vcpu *vcpu, bool vmload)
{
	u64 vmcb12_gpa = kvm_register_read(vcpu, VCPU_REGS_RAX);
	struct vcpu_svm *svm = to_svm(vcpu);
	struct vmcb *vmcb12;
	struct kvm_host_map map;
	int ret;

	if (nested_svm_check_permissions(vcpu))
		return 1;

	if (!page_address_valid(vcpu, vmcb12_gpa)) {
		kvm_inject_gp(vcpu, 0);
		return 1;
	}

	if (kvm_vcpu_map(vcpu, gpa_to_gfn(vmcb12_gpa), &map))
		return kvm_handle_memory_failure(vcpu, X86EMUL_IO_NEEDED, NULL);

	vmcb12 = map.hva;

	ret = kvm_skip_emulated_instruction(vcpu);

	/* KVM always performs VMLOAD/VMSAVE on VMCB01 (see __svm_vcpu_run()) */
	if (vmload) {
		svm_copy_vmloadsave_state(svm->vmcb01.ptr, vmcb12);
		svm->sysenter_eip_hi = 0;
		svm->sysenter_esp_hi = 0;
	} else {
		svm_copy_vmloadsave_state(vmcb12, svm->vmcb01.ptr);
	}

	kvm_vcpu_unmap(vcpu, &map);

	return ret;
}

static int vmload_interception(struct kvm_vcpu *vcpu)
{
	return vmload_vmsave_interception(vcpu, true);
}

static int vmsave_interception(struct kvm_vcpu *vcpu)
{
	return vmload_vmsave_interception(vcpu, false);
}

static int vmrun_interception(struct kvm_vcpu *vcpu)
{
	if (nested_svm_check_permissions(vcpu))
		return 1;

	return nested_svm_vmrun(vcpu);
}

/* Return 0 if not SVM instr, otherwise return associated exit_code */
static u64 svm_get_decoded_instr_exit_code(struct kvm_vcpu *vcpu)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;

	if (ctxt->b != 0x1 || ctxt->opcode_len != 2)
		return 0;

	BUILD_BUG_ON(!SVM_EXIT_VMRUN || !SVM_EXIT_VMLOAD || !SVM_EXIT_VMSAVE);

	switch (ctxt->modrm) {
	case 0xd8: /* VMRUN */
		return SVM_EXIT_VMRUN;
	case 0xda: /* VMLOAD */
		return SVM_EXIT_VMLOAD;
	case 0xdb: /* VMSAVE */
		return SVM_EXIT_VMSAVE;
	default:
		break;
	}

	return 0;
}