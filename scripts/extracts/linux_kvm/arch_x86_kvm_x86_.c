// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 77/86



	vcpu->arch.smi_pending = 0;
	vcpu->arch.smi_count = 0;
	atomic_set(&vcpu->arch.nmi_queued, 0);
	vcpu->arch.nmi_pending = 0;
	vcpu->arch.nmi_injected = false;
	kvm_clear_interrupt_queue(vcpu);
	kvm_clear_exception_queue(vcpu);

	memset(vcpu->arch.db, 0, sizeof(vcpu->arch.db));
	kvm_update_dr0123(vcpu);
	vcpu->arch.dr6 = DR6_ACTIVE_LOW;
	vcpu->arch.dr7 = DR7_FIXED_1;
	kvm_update_dr7(vcpu);

	vcpu->arch.cr2 = 0;

	kvm_make_request(KVM_REQ_EVENT, vcpu);
	vcpu->arch.apf.msr_en_val = 0;
	vcpu->arch.apf.msr_int_val = 0;
	vcpu->arch.st.msr_val = 0;

	kvmclock_reset(vcpu);

	kvm_clear_async_pf_completion_queue(vcpu);
	kvm_async_pf_hash_reset(vcpu);
	vcpu->arch.apf.halted = false;

	kvm_xstate_reset(vcpu, init_event);

	if (!init_event) {
		vcpu->arch.smbase = 0x30000;

		vcpu->arch.pat = MSR_IA32_CR_PAT_DEFAULT;

		vcpu->arch.msr_misc_features_enables = 0;
		vcpu->arch.ia32_misc_enable_msr = MSR_IA32_MISC_ENABLE_PEBS_UNAVAIL |
						  MSR_IA32_MISC_ENABLE_BTS_UNAVAIL;

		__kvm_set_xcr(vcpu, 0, XFEATURE_MASK_FP);
		kvm_msr_write(vcpu, MSR_IA32_XSS, 0);
	}

	/* All GPRs except RDX (handled below) are zeroed on RESET/INIT. */
	memset(vcpu->arch.regs, 0, sizeof(vcpu->arch.regs));
	kvm_register_mark_dirty(vcpu, VCPU_REGS_RSP);

	/*
	 * Fall back to KVM's default Family/Model/Stepping of 0x600 (P6/Athlon)
	 * if no CPUID match is found.  Note, it's impossible to get a match at
	 * RESET since KVM emulates RESET before exposing the vCPU to userspace,
	 * i.e. it's impossible for kvm_find_cpuid_entry() to find a valid entry
	 * on RESET.  But, go through the motions in case that's ever remedied.
	 */
	cpuid_0x1 = kvm_find_cpuid_entry(vcpu, 1);
	kvm_rdx_write(vcpu, cpuid_0x1 ? cpuid_0x1->eax : 0x600);

	kvm_x86_call(vcpu_reset)(vcpu, init_event);

	kvm_set_rflags(vcpu, X86_EFLAGS_FIXED);
	kvm_rip_write(vcpu, 0xfff0);

	vcpu->arch.cr3 = 0;
	kvm_register_mark_dirty(vcpu, VCPU_EXREG_CR3);

	/*
	 * CR0.CD/NW are set on RESET, preserved on INIT.  Note, some versions
	 * of Intel's SDM list CD/NW as being set on INIT, but they contradict
	 * (or qualify) that with a footnote stating that CD/NW are preserved.
	 */
	new_cr0 = X86_CR0_ET;
	if (init_event)
		new_cr0 |= (old_cr0 & (X86_CR0_NW | X86_CR0_CD));
	else
		new_cr0 |= X86_CR0_NW | X86_CR0_CD;

	kvm_x86_call(set_cr0)(vcpu, new_cr0);
	kvm_x86_call(set_cr4)(vcpu, 0);
	kvm_x86_call(set_efer)(vcpu, 0);
	kvm_x86_call(update_exception_bitmap)(vcpu);

	/*
	 * On the standard CR0/CR4/EFER modification paths, there are several
	 * complex conditions determining whether the MMU has to be reset and/or
	 * which PCIDs have to be flushed.  However, CR0.WP and the paging-related
	 * bits in CR4 and EFER are irrelevant if CR0.PG was '0'; and a reset+flush
	 * is needed anyway if CR0.PG was '1' (which can only happen for INIT, as
	 * CR0 will be '0' prior to RESET).  So we only need to check CR0.PG here.
	 */
	if (old_cr0 & X86_CR0_PG) {
		kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
		kvm_mmu_reset_context(vcpu);
	}

	/*
	 * Intel's SDM states that all TLB entries are flushed on INIT.  AMD's
	 * APM states the TLBs are untouched by INIT, but it also states that
	 * the TLBs are flushed on "External initialization of the processor."
	 * Flush the guest TLB regardless of vendor, there is no meaningful
	 * benefit in relying on the guest to flush the TLB immediately after
	 * INIT.  A spurious TLB flush is benign and likely negligible from a
	 * performance perspective.
	 */
	if (init_event)
		kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_reset);

void kvm_vcpu_deliver_sipi_vector(struct kvm_vcpu *vcpu, u8 vector)
{
	struct kvm_segment cs;

	kvm_get_segment(vcpu, &cs, VCPU_SREG_CS);
	cs.selector = vector << 8;
	cs.base = vector << 12;
	kvm_set_segment(vcpu, &cs, VCPU_SREG_CS);
	kvm_rip_write(vcpu, 0);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_deliver_sipi_vector);

void kvm_arch_enable_virtualization(void)
{
	x86_virt_register_emergency_callback(kvm_x86_ops.emergency_disable_virtualization_cpu);
}

void kvm_arch_disable_virtualization(void)
{
	x86_virt_unregister_emergency_callback(kvm_x86_ops.emergency_disable_virtualization_cpu);
}

int kvm_arch_enable_virtualization_cpu(void)
{
	struct kvm *kvm;
	struct kvm_vcpu *vcpu;
	unsigned long i;
	int ret;
	u64 local_tsc;
	u64 max_tsc = 0;
	bool stable, backwards_tsc = false;

	kvm_user_return_msr_cpu_online();

	ret = kvm_x86_check_processor_compatibility();
	if (ret)
		return ret;

	ret = kvm_x86_call(enable_virtualization_cpu)();
	if (ret != 0)
		return ret;

	local_tsc = rdtsc();
	stable = !kvm_check_tsc_unstable();
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_for_each_vcpu(i, vcpu, kvm) {
			if (!stable && vcpu->cpu == smp_processor_id())
				kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
			if (stable && vcpu->arch.last_host_tsc > local_tsc) {
				backwards_tsc = true;
				if (vcpu->arch.last_host_tsc > max_tsc)
					max_tsc = vcpu->arch.last_host_tsc;
			}
		}
	}