// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 86/86



EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_entry);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_exit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_mmio);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_fast_mmio);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_inj_virq);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_page_fault);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_msr);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_cr);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_vmenter);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_vmexit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_vmexit_inject);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_intr_vmexit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_vmenter_failed);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_invlpga);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_skinit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_nested_intercepts);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_write_tsc_offset);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_ple_window_update);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_pml_full);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_avic_unaccelerated_access);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_avic_incomplete_ipi);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_avic_ga_log);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_avic_kick_vcpu_slowpath);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_avic_doorbell);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_apicv_accept_irq);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_vmgexit_enter);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_vmgexit_exit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_vmgexit_msr_protocol_enter);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_vmgexit_msr_protocol_exit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_rmp_fault);

static int __init kvm_x86_init(void)
{
	kvm_init_xstate_sizes();

	kvm_mmu_x86_module_init();
	mitigate_smt_rsb &= boot_cpu_has_bug(X86_BUG_SMT_RSB) && cpu_smt_possible();
	return 0;
}
module_init(kvm_x86_init);

static void __exit kvm_x86_exit(void)
{
	WARN_ON_ONCE(static_branch_unlikely(&kvm_has_noapic_vcpu));
}
module_exit(kvm_x86_exit);
