// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 9/33



	rc = linearize(ctxt, addr, size, true, &linear);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	return ctxt->ops->cmpxchg_emulated(ctxt, linear, orig_data, data,
					   size, &ctxt->exception);
}

static int pio_in_emulated(struct x86_emulate_ctxt *ctxt,
			   unsigned int size, unsigned short port,
			   void *dest)
{
	struct read_cache *rc = &ctxt->io_read;

	if (rc->pos == rc->end) { /* refill pio read ahead */
		unsigned int in_page, n;
		unsigned int count = ctxt->rep_prefix ?
			address_mask(ctxt, reg_read(ctxt, VCPU_REGS_RCX)) : 1;
		in_page = (ctxt->eflags & X86_EFLAGS_DF) ?
			offset_in_page(reg_read(ctxt, VCPU_REGS_RDI)) :
			PAGE_SIZE - offset_in_page(reg_read(ctxt, VCPU_REGS_RDI));
		n = min3(in_page, (unsigned int)sizeof(rc->data) / size, count);
		if (n == 0)
			n = 1;
		rc->pos = rc->end = 0;
		if (!ctxt->ops->pio_in_emulated(ctxt, size, port, rc->data, n))
			return 0;
		rc->end = n * size;
	}

	if (ctxt->rep_prefix && (ctxt->d & String) &&
	    !(ctxt->eflags & X86_EFLAGS_DF)) {
		ctxt->dst.data = rc->data + rc->pos;
		ctxt->dst.type = OP_MEM_STR;
		ctxt->dst.count = (rc->end - rc->pos) / size;
		rc->pos = rc->end;
	} else {
		memcpy(dest, rc->data + rc->pos, size);
		rc->pos += size;
	}
	return 1;
}

static int read_interrupt_descriptor(struct x86_emulate_ctxt *ctxt,
				     u16 index, struct desc_struct *desc)
{
	struct desc_ptr dt;
	ulong addr;

	ctxt->ops->get_idt(ctxt, &dt);

	if (dt.size < index * 8 + 7)
		return emulate_gp(ctxt, index << 3 | 0x2);

	addr = dt.address + index * 8;
	return linear_read_system(ctxt, addr, desc, sizeof(*desc));
}

static void get_descriptor_table_ptr(struct x86_emulate_ctxt *ctxt,
				     u16 selector, struct desc_ptr *dt)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	u32 base3 = 0;

	if (selector & 1 << 2) {
		struct desc_struct desc;
		u16 sel;

		memset(dt, 0, sizeof(*dt));
		if (!ops->get_segment(ctxt, &sel, &desc, &base3,
				      VCPU_SREG_LDTR))
			return;

		dt->size = desc_limit_scaled(&desc); /* what if limit > 65535? */
		dt->address = get_desc_base(&desc) | ((u64)base3 << 32);
	} else
		ops->get_gdt(ctxt, dt);
}

static int get_descriptor_ptr(struct x86_emulate_ctxt *ctxt,
			      u16 selector, ulong *desc_addr_p)
{
	struct desc_ptr dt;
	u16 index = selector >> 3;
	ulong addr;

	get_descriptor_table_ptr(ctxt, selector, &dt);

	if (dt.size < index * 8 + 7)
		return emulate_gp(ctxt, selector & 0xfffc);

	addr = dt.address + index * 8;

#ifdef CONFIG_X86_64
	if (addr >> 32 != 0) {
		u64 efer = 0;

		ctxt->ops->get_msr(ctxt, MSR_EFER, &efer);
		if (!(efer & EFER_LMA))
			addr &= (u32)-1;
	}
#endif

	*desc_addr_p = addr;
	return X86EMUL_CONTINUE;
}

/* allowed just for 8 bytes segments */
static int read_segment_descriptor(struct x86_emulate_ctxt *ctxt,
				   u16 selector, struct desc_struct *desc,
				   ulong *desc_addr_p)
{
	int rc;

	rc = get_descriptor_ptr(ctxt, selector, desc_addr_p);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	return linear_read_system(ctxt, *desc_addr_p, desc, sizeof(*desc));
}

/* allowed just for 8 bytes segments */
static int write_segment_descriptor(struct x86_emulate_ctxt *ctxt,
				    u16 selector, struct desc_struct *desc)
{
	int rc;
	ulong addr;

	rc = get_descriptor_ptr(ctxt, selector, &addr);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	return linear_write_system(ctxt, addr, desc, sizeof(*desc));
}

static bool emulator_is_ssp_invalid(struct x86_emulate_ctxt *ctxt, u8 cpl)
{
	const u32 MSR_IA32_X_CET = cpl == 3 ? MSR_IA32_U_CET : MSR_IA32_S_CET;
	u64 efer = 0, cet = 0, ssp = 0;

	if (!(ctxt->ops->get_cr(ctxt, 4) & X86_CR4_CET))
		return false;

	if (ctxt->ops->get_msr(ctxt, MSR_EFER, &efer))
		return true;

	/* SSP is guaranteed to be valid if the vCPU was already in 32-bit mode. */
	if (!(efer & EFER_LMA))
		return false;

	if (ctxt->ops->get_msr(ctxt, MSR_IA32_X_CET, &cet))
		return true;

	if (!(cet & CET_SHSTK_EN))
		return false;

	if (ctxt->ops->get_msr(ctxt, MSR_KVM_INTERNAL_GUEST_SSP, &ssp))
		return true;

	/*
	 * On transfer from 64-bit mode to compatibility mode, SSP[63:32] must
	 * be 0, i.e. SSP must be a 32-bit value outside of 64-bit mode.
	 */
	return ssp >> 32;
}

static int __load_segment_descriptor(struct x86_emulate_ctxt *ctxt,
				     u16 selector, int seg, u8 cpl,
				     enum x86_transfer_type transfer,
				     struct desc_struct *desc)
{
	struct desc_struct seg_desc, old_desc;
	u8 dpl, rpl;
	unsigned err_vec = GP_VECTOR;
	u32 err_code = 0;
	bool null_selector = !(selector & ~0x3); /* 0000-0003 are null */
	ulong desc_addr;
	int ret;
	u16 dummy;
	u32 base3 = 0;

	memset(&seg_desc, 0, sizeof(seg_desc));