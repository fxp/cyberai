// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 33/36



	/*
	 * Detect and workaround Errata 1096 Fam_17h_00_0Fh.
	 *
	 * Errata:
	 * When CPU raises #NPF on guest data access and vCPU CR4.SMAP=1, it is
	 * possible that CPU microcode implementing DecodeAssist will fail to
	 * read guest memory at CS:RIP and vmcb.GuestIntrBytes will incorrectly
	 * be '0'.  This happens because microcode reads CS:RIP using a _data_
	 * loap uop with CPL=0 privileges.  If the load hits a SMAP #PF, ucode
	 * gives up and does not fill the instruction bytes buffer.
	 *
	 * As above, KVM reaches this point iff the VM is an SEV guest, the CPU
	 * supports DecodeAssist, a #NPF was raised, KVM's page fault handler
	 * triggered emulation (e.g. for MMIO), and the CPU returned 0 in the
	 * GuestIntrBytes field of the VMCB.
	 *
	 * This does _not_ mean that the erratum has been encountered, as the
	 * DecodeAssist will also fail if the load for CS:RIP hits a legitimate
	 * #PF, e.g. if the guest attempt to execute from emulated MMIO and
	 * encountered a reserved/not-present #PF.
	 *
	 * To hit the erratum, the following conditions must be true:
	 *    1. CR4.SMAP=1 (obviously).
	 *    2. CR4.SMEP=0 || CPL=3.  If SMEP=1 and CPL<3, the erratum cannot
	 *       have been hit as the guest would have encountered a SMEP
	 *       violation #PF, not a #NPF.
	 *    3. The #NPF is not due to a code fetch, in which case failure to
	 *       retrieve the instruction bytes is legitimate (see abvoe).
	 *
	 * In addition, don't apply the erratum workaround if the #NPF occurred
	 * while translating guest page tables (see below).
	 */
	error_code = svm->vmcb->control.exit_info_1;
	if (error_code & (PFERR_GUEST_PAGE_MASK | PFERR_FETCH_MASK))
		goto resume_guest;

	smep = kvm_is_cr4_bit_set(vcpu, X86_CR4_SMEP);
	smap = kvm_is_cr4_bit_set(vcpu, X86_CR4_SMAP);
	is_user = svm_get_cpl(vcpu) == 3;
	if (smap && (!smep || is_user)) {
		pr_err_ratelimited("SEV Guest triggered AMD Erratum 1096\n");

		/*
		 * If the fault occurred in userspace, arbitrarily inject #GP
		 * to avoid killing the guest and to hopefully avoid confusing
		 * the guest kernel too much, e.g. injecting #PF would not be
		 * coherent with respect to the guest's page tables.  Request
		 * triple fault if the fault occurred in the kernel as there's
		 * no fault that KVM can inject without confusing the guest.
		 * In practice, the triple fault is moot as no sane SEV kernel
		 * will execute from user memory while also running with SMAP=1.
		 */
		if (is_user)
			kvm_inject_gp(vcpu, 0);
		else
			kvm_make_request(KVM_REQ_TRIPLE_FAULT, vcpu);
		return X86EMUL_PROPAGATE_FAULT;
	}

resume_guest:
	/*
	 * If the erratum was not hit, simply resume the guest and let it fault
	 * again.  While awful, e.g. the vCPU may get stuck in an infinite loop
	 * if the fault is at CPL=0, it's the lesser of all evils.  Exiting to
	 * userspace will kill the guest, and letting the emulator read garbage
	 * will yield random behavior and potentially corrupt the guest.
	 *
	 * Simply resuming the guest is technically not a violation of the SEV
	 * architecture.  AMD's APM states that all code fetches and page table
	 * accesses for SEV guest are encrypted, regardless of the C-Bit.  The
	 * APM also states that encrypted accesses to MMIO are "ignored", but
	 * doesn't explicitly define "ignored", i.e. doing nothing and letting
	 * the guest spin is technically "ignoring" the access.
	 */
	return X86EMUL_RETRY_INSTR;
}

static bool svm_apic_init_signal_blocked(struct kvm_vcpu *vcpu)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	return !gif_set(svm);
}

static void svm_vcpu_deliver_sipi_vector(struct kvm_vcpu *vcpu, u8 vector)
{
	if (!is_sev_es_guest(vcpu))
		return kvm_vcpu_deliver_sipi_vector(vcpu, vector);

	sev_vcpu_deliver_sipi_vector(vcpu, vector);
}

static void svm_vm_destroy(struct kvm *kvm)
{
	avic_vm_destroy(kvm);
	sev_vm_destroy(kvm);

	svm_srso_vm_destroy();
}

static int svm_vm_init(struct kvm *kvm)
{
	sev_vm_init(kvm);

	if (!pause_filter_count || !pause_filter_thresh)
		kvm_disable_exits(kvm, KVM_X86_DISABLE_EXITS_PAUSE);

	if (enable_apicv) {
		int ret = avic_vm_init(kvm);
		if (ret)
			return ret;
	}

	svm_srso_vm_init();
	return 0;
}

static void *svm_alloc_apic_backing_page(struct kvm_vcpu *vcpu)
{
	struct page *page = snp_safe_alloc_page();

	if (!page)
		return NULL;

	return page_address(page);
}

struct kvm_x86_ops svm_x86_ops __initdata = {
	.name = KBUILD_MODNAME,

	.check_processor_compatibility = svm_check_processor_compat,

	.hardware_unsetup = svm_hardware_unsetup,
	.enable_virtualization_cpu = svm_enable_virtualization_cpu,
	.disable_virtualization_cpu = svm_disable_virtualization_cpu,
	.emergency_disable_virtualization_cpu = svm_emergency_disable_virtualization_cpu,
	.has_emulated_msr = svm_has_emulated_msr,

	.vcpu_precreate = svm_vcpu_precreate,
	.vcpu_create = svm_vcpu_create,
	.vcpu_free = svm_vcpu_free,
	.vcpu_reset = svm_vcpu_reset,

	.vm_size = sizeof(struct kvm_svm),
	.vm_init = svm_vm_init,
	.vm_destroy = svm_vm_destroy,