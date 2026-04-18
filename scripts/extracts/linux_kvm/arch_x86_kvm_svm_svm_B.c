// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 34/36



	.prepare_switch_to_guest = svm_prepare_switch_to_guest,
	.vcpu_load = svm_vcpu_load,
	.vcpu_put = svm_vcpu_put,
	.vcpu_blocking = avic_vcpu_blocking,
	.vcpu_unblocking = avic_vcpu_unblocking,

	.update_exception_bitmap = svm_update_exception_bitmap,
	.get_feature_msr = svm_get_feature_msr,
	.get_msr = svm_get_msr,
	.set_msr = svm_set_msr,
	.get_segment_base = svm_get_segment_base,
	.get_segment = svm_get_segment,
	.set_segment = svm_set_segment,
	.get_cpl = svm_get_cpl,
	.get_cpl_no_cache = svm_get_cpl,
	.get_cs_db_l_bits = svm_get_cs_db_l_bits,
	.is_valid_cr0 = svm_is_valid_cr0,
	.set_cr0 = svm_set_cr0,
	.post_set_cr3 = sev_post_set_cr3,
	.is_valid_cr4 = svm_is_valid_cr4,
	.set_cr4 = svm_set_cr4,
	.set_efer = svm_set_efer,
	.get_idt = svm_get_idt,
	.set_idt = svm_set_idt,
	.get_gdt = svm_get_gdt,
	.set_gdt = svm_set_gdt,
	.set_dr7 = svm_set_dr7,
	.sync_dirty_debug_regs = svm_sync_dirty_debug_regs,
	.cache_reg = svm_cache_reg,
	.get_rflags = svm_get_rflags,
	.set_rflags = svm_set_rflags,
	.get_if_flag = svm_get_if_flag,

	.flush_tlb_all = svm_flush_tlb_all,
	.flush_tlb_current = svm_flush_tlb_current,
	.flush_tlb_gva = svm_flush_tlb_gva,
	.flush_tlb_guest = svm_flush_tlb_guest,

	.vcpu_pre_run = svm_vcpu_pre_run,
	.vcpu_run = svm_vcpu_run,
	.handle_exit = svm_handle_exit,
	.skip_emulated_instruction = svm_skip_emulated_instruction,
	.update_emulated_instruction = NULL,
	.set_interrupt_shadow = svm_set_interrupt_shadow,
	.get_interrupt_shadow = svm_get_interrupt_shadow,
	.patch_hypercall = svm_patch_hypercall,
	.inject_irq = svm_inject_irq,
	.inject_nmi = svm_inject_nmi,
	.is_vnmi_pending = svm_is_vnmi_pending,
	.set_vnmi_pending = svm_set_vnmi_pending,
	.inject_exception = svm_inject_exception,
	.cancel_injection = svm_cancel_injection,
	.interrupt_allowed = svm_interrupt_allowed,
	.nmi_allowed = svm_nmi_allowed,
	.get_nmi_mask = svm_get_nmi_mask,
	.set_nmi_mask = svm_set_nmi_mask,
	.enable_nmi_window = svm_enable_nmi_window,
	.enable_irq_window = svm_enable_irq_window,
	.update_cr8_intercept = svm_update_cr8_intercept,

	.x2apic_icr_is_split = true,
	.set_virtual_apic_mode = avic_refresh_virtual_apic_mode,
	.refresh_apicv_exec_ctrl = avic_refresh_apicv_exec_ctrl,
	.apicv_post_state_restore = avic_apicv_post_state_restore,
	.required_apicv_inhibits = AVIC_REQUIRED_APICV_INHIBITS,

	.get_exit_info = svm_get_exit_info,
	.get_entry_info = svm_get_entry_info,

	.vcpu_after_set_cpuid = svm_vcpu_after_set_cpuid,

	.has_wbinvd_exit = svm_has_wbinvd_exit,

	.get_l2_tsc_offset = svm_get_l2_tsc_offset,
	.get_l2_tsc_multiplier = svm_get_l2_tsc_multiplier,
	.write_tsc_offset = svm_write_tsc_offset,
	.write_tsc_multiplier = svm_write_tsc_multiplier,

	.load_mmu_pgd = svm_load_mmu_pgd,

	.check_intercept = svm_check_intercept,
	.handle_exit_irqoff = svm_handle_exit_irqoff,

	.nested_ops = &svm_nested_ops,

	.deliver_interrupt = svm_deliver_interrupt,
	.pi_update_irte = avic_pi_update_irte,
	.setup_mce = svm_setup_mce,

#ifdef CONFIG_KVM_SMM
	.smi_allowed = svm_smi_allowed,
	.enter_smm = svm_enter_smm,
	.leave_smm = svm_leave_smm,
	.enable_smi_window = svm_enable_smi_window,
#endif

#ifdef CONFIG_KVM_AMD_SEV
	.dev_get_attr = sev_dev_get_attr,
	.mem_enc_ioctl = sev_mem_enc_ioctl,
	.mem_enc_register_region = sev_mem_enc_register_region,
	.mem_enc_unregister_region = sev_mem_enc_unregister_region,
	.guest_memory_reclaimed = sev_guest_memory_reclaimed,

	.vm_copy_enc_context_from = sev_vm_copy_enc_context_from,
	.vm_move_enc_context_from = sev_vm_move_enc_context_from,
#endif
	.check_emulate_instruction = svm_check_emulate_instruction,

	.apic_init_signal_blocked = svm_apic_init_signal_blocked,

	.recalc_intercepts = svm_recalc_intercepts,
	.complete_emulated_msr = svm_complete_emulated_msr,

	.vcpu_deliver_sipi_vector = svm_vcpu_deliver_sipi_vector,
	.vcpu_get_apicv_inhibit_reasons = avic_vcpu_get_apicv_inhibit_reasons,
	.alloc_apic_backing_page = svm_alloc_apic_backing_page,

	.gmem_prepare = sev_gmem_prepare,
	.gmem_invalidate = sev_gmem_invalidate,
	.gmem_max_mapping_level = sev_gmem_max_mapping_level,
};

/*
 * The default MMIO mask is a single bit (excluding the present bit),
 * which could conflict with the memory encryption bit. Check for
 * memory encryption support and override the default MMIO mask if
 * memory encryption is enabled.
 */
static __init void svm_adjust_mmio_mask(void)
{
	unsigned int enc_bit, mask_bit;
	u64 msr, mask;

	/* If there is no memory encryption support, use existing mask */
	if (cpuid_eax(0x80000000) < 0x8000001f)
		return;

	/* If memory encryption is not enabled, use existing mask */
	rdmsrq(MSR_AMD64_SYSCFG, msr);
	if (!(msr & MSR_AMD64_SYSCFG_MEM_ENCRYPT))
		return;

	enc_bit = cpuid_ebx(0x8000001f) & 0x3f;
	mask_bit = boot_cpu_data.x86_phys_bits;

	/* Increment the mask bit if it is the same as the encryption bit */
	if (enc_bit == mask_bit)
		mask_bit++;