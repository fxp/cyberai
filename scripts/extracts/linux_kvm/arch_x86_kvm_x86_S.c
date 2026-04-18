// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 51/86



	r = kvm_emulate_msr_read(vcpu, msr_index, pdata);
	if (r < 0)
		return X86EMUL_UNHANDLEABLE;

	if (r) {
		if (kvm_msr_user_space(vcpu, msr_index, KVM_EXIT_X86_RDMSR, 0,
				       complete_emulated_rdmsr, r))
			return X86EMUL_IO_NEEDED;

		trace_kvm_msr_read_ex(msr_index);
		return X86EMUL_PROPAGATE_FAULT;
	}

	trace_kvm_msr_read(msr_index, *pdata);
	return X86EMUL_CONTINUE;
}

static int emulator_set_msr_with_filter(struct x86_emulate_ctxt *ctxt,
					u32 msr_index, u64 data)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	int r;

	r = kvm_emulate_msr_write(vcpu, msr_index, data);
	if (r < 0)
		return X86EMUL_UNHANDLEABLE;

	if (r) {
		if (kvm_msr_user_space(vcpu, msr_index, KVM_EXIT_X86_WRMSR, data,
				       complete_emulated_msr_access, r))
			return X86EMUL_IO_NEEDED;

		trace_kvm_msr_write_ex(msr_index, data);
		return X86EMUL_PROPAGATE_FAULT;
	}

	trace_kvm_msr_write(msr_index, data);
	return X86EMUL_CONTINUE;
}

static int emulator_get_msr(struct x86_emulate_ctxt *ctxt,
			    u32 msr_index, u64 *pdata)
{
	/*
	 * Treat emulator accesses to the current shadow stack pointer as host-
	 * initiated, as they aren't true MSR accesses (SSP is a "just a reg"),
	 * and this API is used only for implicit accesses, i.e. not RDMSR, and
	 * so the index is fully KVM-controlled.
	 */
	if (unlikely(msr_index == MSR_KVM_INTERNAL_GUEST_SSP))
		return kvm_msr_read(emul_to_vcpu(ctxt), msr_index, pdata);

	return __kvm_emulate_msr_read(emul_to_vcpu(ctxt), msr_index, pdata);
}

static int emulator_check_rdpmc_early(struct x86_emulate_ctxt *ctxt, u32 pmc)
{
	return kvm_pmu_check_rdpmc_early(emul_to_vcpu(ctxt), pmc);
}

static int emulator_read_pmc(struct x86_emulate_ctxt *ctxt,
			     u32 pmc, u64 *pdata)
{
	return kvm_pmu_rdpmc(emul_to_vcpu(ctxt), pmc, pdata);
}

static void emulator_halt(struct x86_emulate_ctxt *ctxt)
{
	emul_to_vcpu(ctxt)->arch.halt_request = 1;
}

static int emulator_intercept(struct x86_emulate_ctxt *ctxt,
			      struct x86_instruction_info *info,
			      enum x86_intercept_stage stage)
{
	return kvm_x86_call(check_intercept)(emul_to_vcpu(ctxt), info, stage,
					     &ctxt->exception);
}

static bool emulator_get_cpuid(struct x86_emulate_ctxt *ctxt,
			      u32 *eax, u32 *ebx, u32 *ecx, u32 *edx,
			      bool exact_only)
{
	return kvm_cpuid(emul_to_vcpu(ctxt), eax, ebx, ecx, edx, exact_only);
}

static bool emulator_guest_has_movbe(struct x86_emulate_ctxt *ctxt)
{
	return guest_cpu_cap_has(emul_to_vcpu(ctxt), X86_FEATURE_MOVBE);
}

static bool emulator_guest_has_fxsr(struct x86_emulate_ctxt *ctxt)
{
	return guest_cpu_cap_has(emul_to_vcpu(ctxt), X86_FEATURE_FXSR);
}

static bool emulator_guest_has_rdpid(struct x86_emulate_ctxt *ctxt)
{
	return guest_cpu_cap_has(emul_to_vcpu(ctxt), X86_FEATURE_RDPID);
}

static bool emulator_guest_cpuid_is_intel_compatible(struct x86_emulate_ctxt *ctxt)
{
	return guest_cpuid_is_intel_compatible(emul_to_vcpu(ctxt));
}

static ulong emulator_read_gpr(struct x86_emulate_ctxt *ctxt, unsigned reg)
{
	return kvm_register_read_raw(emul_to_vcpu(ctxt), reg);
}

static void emulator_write_gpr(struct x86_emulate_ctxt *ctxt, unsigned reg, ulong val)
{
	kvm_register_write_raw(emul_to_vcpu(ctxt), reg, val);
}

static void emulator_set_nmi_mask(struct x86_emulate_ctxt *ctxt, bool masked)
{
	kvm_x86_call(set_nmi_mask)(emul_to_vcpu(ctxt), masked);
}

static bool emulator_is_smm(struct x86_emulate_ctxt *ctxt)
{
	return is_smm(emul_to_vcpu(ctxt));
}

#ifndef CONFIG_KVM_SMM
static int emulator_leave_smm(struct x86_emulate_ctxt *ctxt)
{
	WARN_ON_ONCE(1);
	return X86EMUL_UNHANDLEABLE;
}
#endif

static void emulator_triple_fault(struct x86_emulate_ctxt *ctxt)
{
	kvm_make_request(KVM_REQ_TRIPLE_FAULT, emul_to_vcpu(ctxt));
}

static int emulator_get_xcr(struct x86_emulate_ctxt *ctxt, u32 index, u64 *xcr)
{
	if (index != XCR_XFEATURE_ENABLED_MASK)
		return 1;
	*xcr = emul_to_vcpu(ctxt)->arch.xcr0;
	return 0;
}

static int emulator_set_xcr(struct x86_emulate_ctxt *ctxt, u32 index, u64 xcr)
{
	return __kvm_set_xcr(emul_to_vcpu(ctxt), index, xcr);
}

static void emulator_vm_bugged(struct x86_emulate_ctxt *ctxt)
{
	struct kvm *kvm = emul_to_vcpu(ctxt)->kvm;

	if (!kvm->vm_bugged)
		kvm_vm_bugged(kvm);
}

static gva_t emulator_get_untagged_addr(struct x86_emulate_ctxt *ctxt,
					gva_t addr, unsigned int flags)
{
	if (!kvm_x86_ops.get_untagged_addr)
		return addr;

	return kvm_x86_call(get_untagged_addr)(emul_to_vcpu(ctxt),
					       addr, flags);
}

static bool emulator_is_canonical_addr(struct x86_emulate_ctxt *ctxt,
				       gva_t addr, unsigned int flags)
{
	return !is_noncanonical_address(addr, emul_to_vcpu(ctxt), flags);
}

static bool emulator_page_address_valid(struct x86_emulate_ctxt *ctxt, gpa_t gpa)
{
	return page_address_valid(emul_to_vcpu(ctxt), gpa);
}