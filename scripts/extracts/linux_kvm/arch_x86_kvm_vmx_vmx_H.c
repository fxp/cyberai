// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/vmx/vmx.c
// Segment 40/56



	/* Do nothing if PML buffer is empty */
	if (pml_idx == PML_HEAD_INDEX)
		return;
	/*
	 * PML index always points to the next available PML buffer entity
	 * unless PML log has just overflowed.
	 */
	pml_tail_index = (pml_idx >= PML_LOG_NR_ENTRIES) ? 0 : pml_idx + 1;

	/*
	 * PML log is written backwards: the CPU first writes the entry 511
	 * then the entry 510, and so on.
	 *
	 * Read the entries in the same order they were written, to ensure that
	 * the dirty ring is filled in the same order the CPU wrote them.
	 */
	pml_buf = page_address(vmx->pml_pg);

	for (i = PML_HEAD_INDEX; i >= pml_tail_index; i--) {
		u64 gpa;

		gpa = pml_buf[i];
		WARN_ON(gpa & (PAGE_SIZE - 1));
		kvm_vcpu_mark_page_dirty(vcpu, gpa >> PAGE_SHIFT);
	}

	/* reset PML index */
	vmcs_write16(GUEST_PML_INDEX, PML_HEAD_INDEX);
}

static void nested_vmx_mark_all_vmcs12_pages_dirty(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);

	kvm_vcpu_map_mark_dirty(vcpu, &vmx->nested.apic_access_page_map);
	kvm_vcpu_map_mark_dirty(vcpu, &vmx->nested.virtual_apic_map);
	kvm_vcpu_map_mark_dirty(vcpu, &vmx->nested.pi_desc_map);
}

static void vmx_dump_sel(char *name, uint32_t sel)
{
	pr_err("%s sel=0x%04x, attr=0x%05x, limit=0x%08x, base=0x%016lx\n",
	       name, vmcs_read16(sel),
	       vmcs_read32(sel + GUEST_ES_AR_BYTES - GUEST_ES_SELECTOR),
	       vmcs_read32(sel + GUEST_ES_LIMIT - GUEST_ES_SELECTOR),
	       vmcs_readl(sel + GUEST_ES_BASE - GUEST_ES_SELECTOR));
}

static void vmx_dump_dtsel(char *name, uint32_t limit)
{
	pr_err("%s                           limit=0x%08x, base=0x%016lx\n",
	       name, vmcs_read32(limit),
	       vmcs_readl(limit + GUEST_GDTR_BASE - GUEST_GDTR_LIMIT));
}

static void vmx_dump_msrs(char *name, struct vmx_msrs *m)
{
	unsigned int i;
	struct vmx_msr_entry *e;

	pr_err("MSR %s:\n", name);
	for (i = 0, e = m->val; i < m->nr; ++i, ++e)
		pr_err("  %2d: msr=0x%08x value=0x%016llx\n", i, e->index, e->value);
}

void dump_vmcs(struct kvm_vcpu *vcpu)
{
	struct vcpu_vmx *vmx = to_vmx(vcpu);
	u32 vmentry_ctl, vmexit_ctl;
	u32 cpu_based_exec_ctrl, pin_based_exec_ctrl, secondary_exec_control;
	u64 tertiary_exec_control;
	unsigned long cr4;
	int efer_slot;

	if (!dump_invalid_vmcs) {
		pr_warn_ratelimited("set kvm_intel.dump_invalid_vmcs=1 to dump internal KVM state.\n");
		return;
	}

	vmentry_ctl = vmcs_read32(VM_ENTRY_CONTROLS);
	vmexit_ctl = vmcs_read32(VM_EXIT_CONTROLS);
	cpu_based_exec_ctrl = vmcs_read32(CPU_BASED_VM_EXEC_CONTROL);
	pin_based_exec_ctrl = vmcs_read32(PIN_BASED_VM_EXEC_CONTROL);
	cr4 = vmcs_readl(GUEST_CR4);

	if (cpu_has_secondary_exec_ctrls())
		secondary_exec_control = vmcs_read32(SECONDARY_VM_EXEC_CONTROL);
	else
		secondary_exec_control = 0;

	if (cpu_has_tertiary_exec_ctrls())
		tertiary_exec_control = vmcs_read64(TERTIARY_VM_EXEC_CONTROL);
	else
		tertiary_exec_control = 0;