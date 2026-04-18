// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 15/33



	ops->set_segment(ctxt, cs_sel, &cs, 0, VCPU_SREG_CS);
	ops->set_segment(ctxt, ss_sel, &ss, 0, VCPU_SREG_SS);

	ctxt->_eip = rdx;
	ctxt->mode = usermode;
	*reg_write(ctxt, VCPU_REGS_RSP) = rcx;

	return X86EMUL_CONTINUE;
}

static bool emulator_bad_iopl(struct x86_emulate_ctxt *ctxt)
{
	int iopl;
	if (ctxt->mode == X86EMUL_MODE_REAL)
		return false;
	if (ctxt->mode == X86EMUL_MODE_VM86)
		return true;
	iopl = (ctxt->eflags & X86_EFLAGS_IOPL) >> X86_EFLAGS_IOPL_BIT;
	return ctxt->ops->cpl(ctxt) > iopl;
}

#define VMWARE_PORT_VMPORT	(0x5658)
#define VMWARE_PORT_VMRPC	(0x5659)

static bool emulator_io_port_access_allowed(struct x86_emulate_ctxt *ctxt,
					    u16 port, u16 len)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	struct desc_struct tr_seg;
	u32 base3;
	int r;
	u16 tr, io_bitmap_ptr, perm, bit_idx = port & 0x7;
	unsigned mask = (1 << len) - 1;
	unsigned long base;

	/*
	 * VMware allows access to these ports even if denied
	 * by TSS I/O permission bitmap. Mimic behavior.
	 */
	if (enable_vmware_backdoor &&
	    ((port == VMWARE_PORT_VMPORT) || (port == VMWARE_PORT_VMRPC)))
		return true;

	ops->get_segment(ctxt, &tr, &tr_seg, &base3, VCPU_SREG_TR);
	if (!tr_seg.p)
		return false;
	if (desc_limit_scaled(&tr_seg) < 103)
		return false;
	base = get_desc_base(&tr_seg);
#ifdef CONFIG_X86_64
	base |= ((u64)base3) << 32;
#endif
	r = ops->read_std(ctxt, base + 102, &io_bitmap_ptr, 2, NULL, true);
	if (r != X86EMUL_CONTINUE)
		return false;
	if (io_bitmap_ptr + port/8 > desc_limit_scaled(&tr_seg))
		return false;
	r = ops->read_std(ctxt, base + io_bitmap_ptr + port/8, &perm, 2, NULL, true);
	if (r != X86EMUL_CONTINUE)
		return false;
	if ((perm >> bit_idx) & mask)
		return false;
	return true;
}

static bool emulator_io_permitted(struct x86_emulate_ctxt *ctxt,
				  u16 port, u16 len)
{
	if (ctxt->perm_ok)
		return true;

	if (emulator_bad_iopl(ctxt))
		if (!emulator_io_port_access_allowed(ctxt, port, len))
			return false;

	ctxt->perm_ok = true;

	return true;
}

static void string_registers_quirk(struct x86_emulate_ctxt *ctxt)
{
	/*
	 * Intel CPUs mask the counter and pointers in quite strange
	 * manner when ECX is zero due to REP-string optimizations.
	 */
#ifdef CONFIG_X86_64
	u32 eax, ebx, ecx, edx;

	if (ctxt->ad_bytes != 4)
		return;

	eax = ecx = 0;
	ctxt->ops->get_cpuid(ctxt, &eax, &ebx, &ecx, &edx, true);
	if (!is_guest_vendor_intel(ebx, ecx, edx))
		return;

	*reg_write(ctxt, VCPU_REGS_RCX) = 0;

	switch (ctxt->b) {
	case 0xa4:	/* movsb */
	case 0xa5:	/* movsd/w */
		*reg_rmw(ctxt, VCPU_REGS_RSI) &= (u32)-1;
		fallthrough;
	case 0xaa:	/* stosb */
	case 0xab:	/* stosd/w */
		*reg_rmw(ctxt, VCPU_REGS_RDI) &= (u32)-1;
	}
#endif
}

static void save_state_to_tss16(struct x86_emulate_ctxt *ctxt,
				struct tss_segment_16 *tss)
{
	tss->ip = ctxt->_eip;
	tss->flag = ctxt->eflags;
	tss->ax = reg_read(ctxt, VCPU_REGS_RAX);
	tss->cx = reg_read(ctxt, VCPU_REGS_RCX);
	tss->dx = reg_read(ctxt, VCPU_REGS_RDX);
	tss->bx = reg_read(ctxt, VCPU_REGS_RBX);
	tss->sp = reg_read(ctxt, VCPU_REGS_RSP);
	tss->bp = reg_read(ctxt, VCPU_REGS_RBP);
	tss->si = reg_read(ctxt, VCPU_REGS_RSI);
	tss->di = reg_read(ctxt, VCPU_REGS_RDI);

	tss->es = get_segment_selector(ctxt, VCPU_SREG_ES);
	tss->cs = get_segment_selector(ctxt, VCPU_SREG_CS);
	tss->ss = get_segment_selector(ctxt, VCPU_SREG_SS);
	tss->ds = get_segment_selector(ctxt, VCPU_SREG_DS);
	tss->ldt = get_segment_selector(ctxt, VCPU_SREG_LDTR);
}

static int load_state_from_tss16(struct x86_emulate_ctxt *ctxt,
				 struct tss_segment_16 *tss)
{
	int ret;
	u8 cpl;

	ctxt->_eip = tss->ip;
	ctxt->eflags = tss->flag | 2;
	*reg_write(ctxt, VCPU_REGS_RAX) = tss->ax;
	*reg_write(ctxt, VCPU_REGS_RCX) = tss->cx;
	*reg_write(ctxt, VCPU_REGS_RDX) = tss->dx;
	*reg_write(ctxt, VCPU_REGS_RBX) = tss->bx;
	*reg_write(ctxt, VCPU_REGS_RSP) = tss->sp;
	*reg_write(ctxt, VCPU_REGS_RBP) = tss->bp;
	*reg_write(ctxt, VCPU_REGS_RSI) = tss->si;
	*reg_write(ctxt, VCPU_REGS_RDI) = tss->di;

	/*
	 * SDM says that segment selectors are loaded before segment
	 * descriptors
	 */
	set_segment_selector(ctxt, tss->ldt, VCPU_SREG_LDTR);
	set_segment_selector(ctxt, tss->es, VCPU_SREG_ES);
	set_segment_selector(ctxt, tss->cs, VCPU_SREG_CS);
	set_segment_selector(ctxt, tss->ss, VCPU_SREG_SS);
	set_segment_selector(ctxt, tss->ds, VCPU_SREG_DS);

	cpl = tss->cs & 3;