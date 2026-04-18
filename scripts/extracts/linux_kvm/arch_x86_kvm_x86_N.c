// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 46/86



gpa_t translate_nested_gpa(struct kvm_vcpu *vcpu, gpa_t gpa, u64 access,
			   struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.mmu;
	gpa_t t_gpa;

	BUG_ON(!mmu_is_nested(vcpu));

	/* NPT walks are always user-walks */
	access |= PFERR_USER_MASK;
	t_gpa  = mmu->gva_to_gpa(vcpu, mmu, gpa, access, exception);

	return t_gpa;
}

gpa_t kvm_mmu_gva_to_gpa_read(struct kvm_vcpu *vcpu, gva_t gva,
			      struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;

	u64 access = (kvm_x86_call(get_cpl)(vcpu) == 3) ? PFERR_USER_MASK : 0;
	return mmu->gva_to_gpa(vcpu, mmu, gva, access, exception);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_mmu_gva_to_gpa_read);

gpa_t kvm_mmu_gva_to_gpa_write(struct kvm_vcpu *vcpu, gva_t gva,
			       struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;

	u64 access = (kvm_x86_call(get_cpl)(vcpu) == 3) ? PFERR_USER_MASK : 0;
	access |= PFERR_WRITE_MASK;
	return mmu->gva_to_gpa(vcpu, mmu, gva, access, exception);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_mmu_gva_to_gpa_write);

/* uses this to access any guest's mapped memory without checking CPL */
gpa_t kvm_mmu_gva_to_gpa_system(struct kvm_vcpu *vcpu, gva_t gva,
				struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;

	return mmu->gva_to_gpa(vcpu, mmu, gva, 0, exception);
}

static int kvm_read_guest_virt_helper(gva_t addr, void *val, unsigned int bytes,
				      struct kvm_vcpu *vcpu, u64 access,
				      struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;
	void *data = val;
	int r = X86EMUL_CONTINUE;

	while (bytes) {
		gpa_t gpa = mmu->gva_to_gpa(vcpu, mmu, addr, access, exception);
		unsigned offset = addr & (PAGE_SIZE-1);
		unsigned toread = min(bytes, (unsigned)PAGE_SIZE - offset);
		int ret;

		if (gpa == INVALID_GPA)
			return X86EMUL_PROPAGATE_FAULT;
		ret = kvm_vcpu_read_guest_page(vcpu, gpa >> PAGE_SHIFT, data,
					       offset, toread);
		if (ret < 0) {
			r = X86EMUL_IO_NEEDED;
			goto out;
		}

		bytes -= toread;
		data += toread;
		addr += toread;
	}
out:
	return r;
}

/* used for instruction fetching */
static int kvm_fetch_guest_virt(struct x86_emulate_ctxt *ctxt,
				gva_t addr, void *val, unsigned int bytes,
				struct x86_exception *exception)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;
	u64 access = (kvm_x86_call(get_cpl)(vcpu) == 3) ? PFERR_USER_MASK : 0;
	unsigned offset;
	int ret;

	/* Inline kvm_read_guest_virt_helper for speed.  */
	gpa_t gpa = mmu->gva_to_gpa(vcpu, mmu, addr, access|PFERR_FETCH_MASK,
				    exception);
	if (unlikely(gpa == INVALID_GPA))
		return X86EMUL_PROPAGATE_FAULT;

	offset = addr & (PAGE_SIZE-1);
	if (WARN_ON(offset + bytes > PAGE_SIZE))
		bytes = (unsigned)PAGE_SIZE - offset;
	ret = kvm_vcpu_read_guest_page(vcpu, gpa >> PAGE_SHIFT, val,
				       offset, bytes);
	if (unlikely(ret < 0))
		return X86EMUL_IO_NEEDED;

	return X86EMUL_CONTINUE;
}

int kvm_read_guest_virt(struct kvm_vcpu *vcpu,
			       gva_t addr, void *val, unsigned int bytes,
			       struct x86_exception *exception)
{
	u64 access = (kvm_x86_call(get_cpl)(vcpu) == 3) ? PFERR_USER_MASK : 0;

	/*
	 * FIXME: this should call handle_emulation_failure if X86EMUL_IO_NEEDED
	 * is returned, but our callers are not ready for that and they blindly
	 * call kvm_inject_page_fault.  Ensure that they at least do not leak
	 * uninitialized kernel stack memory into cr2 and error code.
	 */
	memset(exception, 0, sizeof(*exception));
	return kvm_read_guest_virt_helper(addr, val, bytes, vcpu, access,
					  exception);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_read_guest_virt);

static int emulator_read_std(struct x86_emulate_ctxt *ctxt,
			     gva_t addr, void *val, unsigned int bytes,
			     struct x86_exception *exception, bool system)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	u64 access = 0;

	if (system)
		access |= PFERR_IMPLICIT_ACCESS;
	else if (kvm_x86_call(get_cpl)(vcpu) == 3)
		access |= PFERR_USER_MASK;

	return kvm_read_guest_virt_helper(addr, val, bytes, vcpu, access, exception);
}

static int kvm_write_guest_virt_helper(gva_t addr, void *val, unsigned int bytes,
				      struct kvm_vcpu *vcpu, u64 access,
				      struct x86_exception *exception)
{
	struct kvm_mmu *mmu = vcpu->arch.walk_mmu;
	void *data = val;
	int r = X86EMUL_CONTINUE;

	while (bytes) {
		gpa_t gpa = mmu->gva_to_gpa(vcpu, mmu, addr, access, exception);
		unsigned offset = addr & (PAGE_SIZE-1);
		unsigned towrite = min(bytes, (unsigned)PAGE_SIZE - offset);
		int ret;

		if (gpa == INVALID_GPA)
			return X86EMUL_PROPAGATE_FAULT;
		ret = kvm_vcpu_write_guest(vcpu, gpa, data, towrite);
		if (ret < 0) {
			r = X86EMUL_IO_NEEDED;
			goto out;
		}

		bytes -= towrite;
		data += towrite;
		addr += towrite;
	}
out:
	return r;
}