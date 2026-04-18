// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 47/86



static int emulator_write_std(struct x86_emulate_ctxt *ctxt, gva_t addr, void *val,
			      unsigned int bytes, struct x86_exception *exception,
			      bool system)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	u64 access = PFERR_WRITE_MASK;

	if (system)
		access |= PFERR_IMPLICIT_ACCESS;
	else if (kvm_x86_call(get_cpl)(vcpu) == 3)
		access |= PFERR_USER_MASK;

	return kvm_write_guest_virt_helper(addr, val, bytes, vcpu,
					   access, exception);
}

int kvm_write_guest_virt_system(struct kvm_vcpu *vcpu, gva_t addr, void *val,
				unsigned int bytes, struct x86_exception *exception)
{
	/* kvm_write_guest_virt_system can pull in tons of pages. */
	kvm_request_l1tf_flush_l1d();

	return kvm_write_guest_virt_helper(addr, val, bytes, vcpu,
					   PFERR_WRITE_MASK, exception);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_write_guest_virt_system);

static int kvm_check_emulate_insn(struct kvm_vcpu *vcpu, int emul_type,
				  void *insn, int insn_len)
{
	return kvm_x86_call(check_emulate_instruction)(vcpu, emul_type,
						       insn, insn_len);
}

int handle_ud(struct kvm_vcpu *vcpu)
{
	static const char kvm_emulate_prefix[] = { __KVM_EMULATE_PREFIX };
	int fep_flags = READ_ONCE(force_emulation_prefix);
	int emul_type = EMULTYPE_TRAP_UD;
	char sig[5]; /* ud2; .ascii "kvm" */
	struct x86_exception e;
	int r;

	r = kvm_check_emulate_insn(vcpu, emul_type, NULL, 0);
	if (r != X86EMUL_CONTINUE)
		return 1;

	if (fep_flags &&
	    kvm_read_guest_virt(vcpu, kvm_get_linear_rip(vcpu),
				sig, sizeof(sig), &e) == 0 &&
	    memcmp(sig, kvm_emulate_prefix, sizeof(sig)) == 0) {
		if (fep_flags & KVM_FEP_CLEAR_RFLAGS_RF)
			kvm_set_rflags(vcpu, kvm_get_rflags(vcpu) & ~X86_EFLAGS_RF);
		kvm_rip_write(vcpu, kvm_rip_read(vcpu) + sizeof(sig));
		emul_type = EMULTYPE_TRAP_UD_FORCED;
	}

	return kvm_emulate_instruction(vcpu, emul_type);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(handle_ud);

static int vcpu_is_mmio_gpa(struct kvm_vcpu *vcpu, unsigned long gva,
			    gpa_t gpa, bool write)
{
	/* For APIC access vmexit */
	if ((gpa & PAGE_MASK) == APIC_DEFAULT_PHYS_BASE)
		return 1;

	if (vcpu_match_mmio_gpa(vcpu, gpa)) {
		trace_vcpu_match_mmio(gva, gpa, write, true);
		return 1;
	}

	return 0;
}

static int vcpu_mmio_gva_to_gpa(struct kvm_vcpu *vcpu, unsigned long gva,
				gpa_t *gpa, struct x86_exception *exception,
				bool write)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;
	u64 access = ((kvm_x86_call(get_cpl)(vcpu) == 3) ? PFERR_USER_MASK : 0)
		     | (write ? PFERR_WRITE_MASK : 0);

	/*
	 * currently PKRU is only applied to ept enabled guest so
	 * there is no pkey in EPT page table for L1 guest or EPT
	 * shadow page table for L2 guest.
	 */
	if (vcpu_match_mmio_gva(vcpu, gva) && (!is_paging(vcpu) ||
	    !permission_fault(vcpu, vcpu->arch.walk_mmu,
			      vcpu->arch.mmio_access, 0, access))) {
		*gpa = vcpu->arch.mmio_gfn << PAGE_SHIFT |
					(gva & (PAGE_SIZE - 1));
		trace_vcpu_match_mmio(gva, *gpa, write, false);
		return 1;
	}

	*gpa = mmu->gva_to_gpa(vcpu, mmu, gva, access, exception);

	if (*gpa == INVALID_GPA)
		return -1;

	return vcpu_is_mmio_gpa(vcpu, gva, *gpa, write);
}

struct read_write_emulator_ops {
	int (*read_write_guest)(struct kvm_vcpu *vcpu, gpa_t gpa,
				void *val, int bytes);
	int (*read_write_mmio)(struct kvm_vcpu *vcpu, gpa_t gpa,
			       int bytes, void *val);
	bool write;
};

static int emulator_read_guest(struct kvm_vcpu *vcpu, gpa_t gpa,
			       void *val, int bytes)
{
	return !kvm_vcpu_read_guest(vcpu, gpa, val, bytes);
}

static int emulator_write_guest(struct kvm_vcpu *vcpu, gpa_t gpa,
				void *val, int bytes)
{
	int ret;

	ret = kvm_vcpu_write_guest(vcpu, gpa, val, bytes);
	if (ret < 0)
		return 0;
	kvm_page_track_write(vcpu, gpa, val, bytes);
	return 1;
}

static int emulator_read_write_onepage(unsigned long addr, void *val,
				       unsigned int bytes,
				       struct x86_exception *exception,
				       struct kvm_vcpu *vcpu,
				       const struct read_write_emulator_ops *ops)
{
	gpa_t gpa;
	int handled, ret;
	bool write = ops->write;
	struct kvm_mmio_fragment *frag;
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;

	/*
	 * If the exit was due to a NPF we may already have a GPA.
	 * If the GPA is present, use it to avoid the GVA to GPA table walk.
	 * Note, this cannot be used on string operations since string
	 * operation using rep will only have the initial GPA from the NPF
	 * occurred.
	 */
	if (ctxt->gpa_available && emulator_can_use_gpa(ctxt) &&
	    (addr & ~PAGE_MASK) == (ctxt->gpa_val & ~PAGE_MASK)) {
		gpa = ctxt->gpa_val;
		ret = vcpu_is_mmio_gpa(vcpu, addr, gpa, write);
	} else {
		ret = vcpu_mmio_gva_to_gpa(vcpu, addr, &gpa, exception, write);
		if (ret < 0)
			return X86EMUL_PROPAGATE_FAULT;
	}