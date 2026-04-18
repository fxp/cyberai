// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 21/36



	pr_err("%s vCPU%u VMCB %p, last attempted VMRUN on CPU %d\n",
	       vm_type, vcpu->vcpu_id, svm->current_vmcb->ptr, vcpu->arch.last_vmentry_cpu);
	pr_err("VMCB Control Area:\n");
	pr_err("%-20s%04x\n", "cr_read:", control->intercepts[INTERCEPT_CR] & 0xffff);
	pr_err("%-20s%04x\n", "cr_write:", control->intercepts[INTERCEPT_CR] >> 16);
	pr_err("%-20s%04x\n", "dr_read:", control->intercepts[INTERCEPT_DR] & 0xffff);
	pr_err("%-20s%04x\n", "dr_write:", control->intercepts[INTERCEPT_DR] >> 16);
	pr_err("%-20s%08x\n", "exceptions:", control->intercepts[INTERCEPT_EXCEPTION]);
	pr_err("%-20s%08x %08x\n", "intercepts:",
              control->intercepts[INTERCEPT_WORD3],
	       control->intercepts[INTERCEPT_WORD4]);
	pr_err("%-20s%d\n", "pause filter count:", control->pause_filter_count);
	pr_err("%-20s%d\n", "pause filter threshold:",
	       control->pause_filter_thresh);
	pr_err("%-20s%016llx\n", "iopm_base_pa:", control->iopm_base_pa);
	pr_err("%-20s%016llx\n", "msrpm_base_pa:", control->msrpm_base_pa);
	pr_err("%-20s%016llx\n", "tsc_offset:", control->tsc_offset);
	pr_err("%-20s%d\n", "asid:", control->asid);
	pr_err("%-20s%d\n", "tlb_ctl:", control->tlb_ctl);
	pr_err("%-20s%d\n", "erap_ctl:", control->erap_ctl);
	pr_err("%-20s%08x\n", "int_ctl:", control->int_ctl);
	pr_err("%-20s%08x\n", "int_vector:", control->int_vector);
	pr_err("%-20s%08x\n", "int_state:", control->int_state);
	pr_err("%-20s%016llx\n", "exit_code:", control->exit_code);
	pr_err("%-20s%016llx\n", "exit_info1:", control->exit_info_1);
	pr_err("%-20s%016llx\n", "exit_info2:", control->exit_info_2);
	pr_err("%-20s%08x\n", "exit_int_info:", control->exit_int_info);
	pr_err("%-20s%08x\n", "exit_int_info_err:", control->exit_int_info_err);
	pr_err("%-20s%lld\n", "misc_ctl:", control->misc_ctl);
	pr_err("%-20s%016llx\n", "nested_cr3:", control->nested_cr3);
	pr_err("%-20s%016llx\n", "avic_vapic_bar:", control->avic_vapic_bar);
	pr_err("%-20s%016llx\n", "ghcb:", control->ghcb_gpa);
	pr_err("%-20s%08x\n", "event_inj:", control->event_inj);
	pr_err("%-20s%08x\n", "event_inj_err:", control->event_inj_err);
	pr_err("%-20s%lld\n", "misc_ctl2:", control->misc_ctl2);
	pr_err("%-20s%016llx\n", "next_rip:", control->next_rip);
	pr_err("%-20s%016llx\n", "avic_backing_page:", control->avic_backing_page);
	pr_err("%-20s%016llx\n", "avic_logical_id:", control->avic_logical_id);
	pr_err("%-20s%016llx\n", "avic_physical_id:", control->avic_physical_id);
	pr_err("%-20s%016llx\n", "vmsa_pa:", control->vmsa_pa);
	pr_err("%-20s%016llx\n", "allowed_sev_features:", control->allowed_sev_features);
	pr_err("%-20s%016llx\n", "guest_sev_features:", control->guest_sev_features);

	if (is_sev_es_guest(vcpu)) {
		save = sev_decrypt_vmsa(vcpu);
		if (!save)
			goto no_vmsa;

		save01 = save;
	}