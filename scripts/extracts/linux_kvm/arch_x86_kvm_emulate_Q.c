// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 17/33



	/*
	 * Now load segment descriptors. If fault happens at this stage
	 * it is handled in a context of new task
	 */
	ret = __load_segment_descriptor(ctxt, tss->ldt_selector, VCPU_SREG_LDTR,
					cpl, X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->es, VCPU_SREG_ES, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->cs, VCPU_SREG_CS, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->ss, VCPU_SREG_SS, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->ds, VCPU_SREG_DS, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->fs, VCPU_SREG_FS, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = __load_segment_descriptor(ctxt, tss->gs, VCPU_SREG_GS, cpl,
					X86_TRANSFER_TASK_SWITCH, NULL);

	return ret;
}

static int task_switch_32(struct x86_emulate_ctxt *ctxt, u16 old_tss_sel,
			  ulong old_tss_base, struct desc_struct *new_desc)
{
	struct tss_segment_32 tss_seg;
	int ret;
	u32 new_tss_base = get_desc_base(new_desc);
	u32 eip_offset = offsetof(struct tss_segment_32, eip);
	u32 ldt_sel_offset = offsetof(struct tss_segment_32, ldt_selector);

	ret = linear_read_system(ctxt, old_tss_base, &tss_seg, sizeof(tss_seg));
	if (ret != X86EMUL_CONTINUE)
		return ret;

	save_state_to_tss32(ctxt, &tss_seg);

	/* Only GP registers and segment selectors are saved */
	ret = linear_write_system(ctxt, old_tss_base + eip_offset, &tss_seg.eip,
				  ldt_sel_offset - eip_offset);
	if (ret != X86EMUL_CONTINUE)
		return ret;

	ret = linear_read_system(ctxt, new_tss_base, &tss_seg, sizeof(tss_seg));
	if (ret != X86EMUL_CONTINUE)
		return ret;

	if (old_tss_sel != 0xffff) {
		tss_seg.prev_task_link = old_tss_sel;

		ret = linear_write_system(ctxt, new_tss_base,
					  &tss_seg.prev_task_link,
					  sizeof(tss_seg.prev_task_link));
		if (ret != X86EMUL_CONTINUE)
			return ret;
	}

	return load_state_from_tss32(ctxt, &tss_seg);
}

static int emulator_do_task_switch(struct x86_emulate_ctxt *ctxt,
				   u16 tss_selector, int idt_index, int reason,
				   bool has_error_code, u32 error_code)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	struct desc_struct curr_tss_desc, next_tss_desc;
	int ret;
	u16 old_tss_sel = get_segment_selector(ctxt, VCPU_SREG_TR);
	ulong old_tss_base =
		ops->get_cached_segment_base(ctxt, VCPU_SREG_TR);
	u32 desc_limit;
	ulong desc_addr, dr7;

	/* FIXME: old_tss_base == ~0 ? */

	ret = read_segment_descriptor(ctxt, tss_selector, &next_tss_desc, &desc_addr);
	if (ret != X86EMUL_CONTINUE)
		return ret;
	ret = read_segment_descriptor(ctxt, old_tss_sel, &curr_tss_desc, &desc_addr);
	if (ret != X86EMUL_CONTINUE)
		return ret;

	/* FIXME: check that next_tss_desc is tss */

	/*
	 * Check privileges. The three cases are task switch caused by...
	 *
	 * 1. jmp/call/int to task gate: Check against DPL of the task gate
	 * 2. Exception/IRQ/iret: No check is performed
	 * 3. jmp/call to TSS/task-gate: No check is performed since the
	 *    hardware checks it before exiting.
	 */
	if (reason == TASK_SWITCH_GATE) {
		if (idt_index != -1) {
			/* Software interrupts */
			struct desc_struct task_gate_desc;
			int dpl;

			ret = read_interrupt_descriptor(ctxt, idt_index,
							&task_gate_desc);
			if (ret != X86EMUL_CONTINUE)
				return ret;

			dpl = task_gate_desc.dpl;
			if ((tss_selector & 3) > dpl || ops->cpl(ctxt) > dpl)
				return emulate_gp(ctxt, (idt_index << 3) | 0x2);
		}
	}

	desc_limit = desc_limit_scaled(&next_tss_desc);
	if (!next_tss_desc.p ||
	    ((desc_limit < 0x67 && (next_tss_desc.type & 8)) ||
	     desc_limit < 0x2b)) {
		return emulate_ts(ctxt, tss_selector & 0xfffc);
	}

	if (reason == TASK_SWITCH_IRET || reason == TASK_SWITCH_JMP) {
		curr_tss_desc.type &= ~(1 << 1); /* clear busy flag */
		write_segment_descriptor(ctxt, old_tss_sel, &curr_tss_desc);
	}

	if (reason == TASK_SWITCH_IRET)
		ctxt->eflags = ctxt->eflags & ~X86_EFLAGS_NT;

	/* set back link to prev task only if NT bit is set in eflags
	   note that old_tss_sel is not used after this point */
	if (reason != TASK_SWITCH_CALL && reason != TASK_SWITCH_GATE)
		old_tss_sel = 0xffff;

	if (next_tss_desc.type & 8)
		ret = task_switch_32(ctxt, old_tss_sel, old_tss_base, &next_tss_desc);
	else
		ret = task_switch_16(ctxt, old_tss_sel,
				     old_tss_base, &next_tss_desc);
	if (ret != X86EMUL_CONTINUE)
		return ret;

	if (reason == TASK_SWITCH_CALL || reason == TASK_SWITCH_GATE)
		ctxt->eflags = ctxt->eflags | X86_EFLAGS_NT;

	if (reason != TASK_SWITCH_IRET) {
		next_tss_desc.type |= (1 << 1); /* set busy flag */
		write_segment_descriptor(ctxt, tss_selector, &next_tss_desc);
	}