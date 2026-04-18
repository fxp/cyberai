// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 52/86



static const struct x86_emulate_ops emulate_ops = {
	.vm_bugged           = emulator_vm_bugged,
	.read_gpr            = emulator_read_gpr,
	.write_gpr           = emulator_write_gpr,
	.read_std            = emulator_read_std,
	.write_std           = emulator_write_std,
	.fetch               = kvm_fetch_guest_virt,
	.read_emulated       = emulator_read_emulated,
	.write_emulated      = emulator_write_emulated,
	.cmpxchg_emulated    = emulator_cmpxchg_emulated,
	.invlpg              = emulator_invlpg,
	.pio_in_emulated     = emulator_pio_in_emulated,
	.pio_out_emulated    = emulator_pio_out_emulated,
	.get_segment         = emulator_get_segment,
	.set_segment         = emulator_set_segment,
	.get_cached_segment_base = emulator_get_cached_segment_base,
	.get_gdt             = emulator_get_gdt,
	.get_idt	     = emulator_get_idt,
	.set_gdt             = emulator_set_gdt,
	.set_idt	     = emulator_set_idt,
	.get_cr              = emulator_get_cr,
	.set_cr              = emulator_set_cr,
	.cpl                 = emulator_get_cpl,
	.get_dr              = emulator_get_dr,
	.set_dr              = emulator_set_dr,
	.set_msr_with_filter = emulator_set_msr_with_filter,
	.get_msr_with_filter = emulator_get_msr_with_filter,
	.get_msr             = emulator_get_msr,
	.check_rdpmc_early   = emulator_check_rdpmc_early,
	.read_pmc            = emulator_read_pmc,
	.halt                = emulator_halt,
	.wbinvd              = emulator_wbinvd,
	.fix_hypercall       = emulator_fix_hypercall,
	.intercept           = emulator_intercept,
	.get_cpuid           = emulator_get_cpuid,
	.guest_has_movbe     = emulator_guest_has_movbe,
	.guest_has_fxsr      = emulator_guest_has_fxsr,
	.guest_has_rdpid     = emulator_guest_has_rdpid,
	.guest_cpuid_is_intel_compatible = emulator_guest_cpuid_is_intel_compatible,
	.set_nmi_mask        = emulator_set_nmi_mask,
	.is_smm              = emulator_is_smm,
	.leave_smm           = emulator_leave_smm,
	.triple_fault        = emulator_triple_fault,
	.get_xcr             = emulator_get_xcr,
	.set_xcr             = emulator_set_xcr,
	.get_untagged_addr   = emulator_get_untagged_addr,
	.is_canonical_addr   = emulator_is_canonical_addr,
	.page_address_valid  = emulator_page_address_valid,
};

static void toggle_interruptibility(struct kvm_vcpu *vcpu, u32 mask)
{
	u32 int_shadow = kvm_x86_call(get_interrupt_shadow)(vcpu);
	/*
	 * an sti; sti; sequence only disable interrupts for the first
	 * instruction. So, if the last instruction, be it emulated or
	 * not, left the system with the INT_STI flag enabled, it
	 * means that the last instruction is an sti. We should not
	 * leave the flag on in this case. The same goes for mov ss
	 */
	if (int_shadow & mask)
		mask = 0;
	if (unlikely(int_shadow || mask)) {
		kvm_x86_call(set_interrupt_shadow)(vcpu, mask);
		if (!mask)
			kvm_make_request(KVM_REQ_EVENT, vcpu);
	}
}

static void inject_emulated_exception(struct kvm_vcpu *vcpu)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;

	if (ctxt->exception.vector == PF_VECTOR)
		kvm_inject_emulated_page_fault(vcpu, &ctxt->exception);
	else if (ctxt->exception.error_code_valid)
		kvm_queue_exception_e(vcpu, ctxt->exception.vector,
				      ctxt->exception.error_code);
	else
		kvm_queue_exception(vcpu, ctxt->exception.vector);
}

static struct x86_emulate_ctxt *alloc_emulate_ctxt(struct kvm_vcpu *vcpu)
{
	struct x86_emulate_ctxt *ctxt;

	ctxt = kmem_cache_zalloc(x86_emulator_cache, GFP_KERNEL_ACCOUNT);
	if (!ctxt) {
		pr_err("failed to allocate vcpu's emulator\n");
		return NULL;
	}

	ctxt->vcpu = vcpu;
	ctxt->ops = &emulate_ops;
	vcpu->arch.emulate_ctxt = ctxt;

	return ctxt;
}

static void init_emulate_ctxt(struct kvm_vcpu *vcpu)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	int cs_db, cs_l;

	kvm_x86_call(get_cs_db_l_bits)(vcpu, &cs_db, &cs_l);

	ctxt->gpa_available = false;
	ctxt->eflags = kvm_get_rflags(vcpu);
	ctxt->tf = (ctxt->eflags & X86_EFLAGS_TF) != 0;

	ctxt->eip = kvm_rip_read(vcpu);
	ctxt->mode = (!is_protmode(vcpu))		? X86EMUL_MODE_REAL :
		     (ctxt->eflags & X86_EFLAGS_VM)	? X86EMUL_MODE_VM86 :
		     (cs_l && is_long_mode(vcpu))	? X86EMUL_MODE_PROT64 :
		     cs_db				? X86EMUL_MODE_PROT32 :
							  X86EMUL_MODE_PROT16;
	ctxt->interruptibility = 0;
	ctxt->have_exception = false;
	ctxt->exception.vector = -1;
	ctxt->perm_ok = false;

	init_decode_cache(ctxt);
	vcpu->arch.emulate_regs_need_sync_from_vcpu = false;
}

void kvm_inject_realmode_interrupt(struct kvm_vcpu *vcpu, int irq, int inc_eip)
{
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	int ret;

	init_emulate_ctxt(vcpu);

	ctxt->op_bytes = 2;
	ctxt->ad_bytes = 2;
	ctxt->_eip = ctxt->eip + inc_eip;
	ret = emulate_int_real(ctxt, irq);

	if (ret != X86EMUL_CONTINUE) {
		kvm_make_request(KVM_REQ_TRIPLE_FAULT, vcpu);
	} else {
		ctxt->eip = ctxt->_eip;
		kvm_rip_write(vcpu, ctxt->eip);
		kvm_set_rflags(vcpu, ctxt->eflags);
	}
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_inject_realmode_interrupt);