// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 8/33



		/* 16-bit ModR/M decode. */
		switch (ctxt->modrm_mod) {
		case 0:
			if (ctxt->modrm_rm == 6)
				modrm_ea += insn_fetch(u16, ctxt);
			break;
		case 1:
			modrm_ea += insn_fetch(s8, ctxt);
			break;
		case 2:
			modrm_ea += insn_fetch(u16, ctxt);
			break;
		}
		switch (ctxt->modrm_rm) {
		case 0:
			modrm_ea += bx + si;
			break;
		case 1:
			modrm_ea += bx + di;
			break;
		case 2:
			modrm_ea += bp + si;
			break;
		case 3:
			modrm_ea += bp + di;
			break;
		case 4:
			modrm_ea += si;
			break;
		case 5:
			modrm_ea += di;
			break;
		case 6:
			if (ctxt->modrm_mod != 0)
				modrm_ea += bp;
			break;
		case 7:
			modrm_ea += bx;
			break;
		}
		if (ctxt->modrm_rm == 2 || ctxt->modrm_rm == 3 ||
		    (ctxt->modrm_rm == 6 && ctxt->modrm_mod != 0))
			ctxt->modrm_seg = VCPU_SREG_SS;
		modrm_ea = (u16)modrm_ea;
	} else {
		/* 32/64-bit ModR/M decode. */
		if ((ctxt->modrm_rm & 7) == 4) {
			sib = insn_fetch(u8, ctxt);
			index_reg |= (sib >> 3) & 7;
			base_reg |= sib & 7;
			scale = sib >> 6;

			if ((base_reg & 7) == 5 && ctxt->modrm_mod == 0)
				modrm_ea += insn_fetch(s32, ctxt);
			else {
				modrm_ea += reg_read(ctxt, base_reg);
				adjust_modrm_seg(ctxt, base_reg);
				/* Increment ESP on POP [ESP] */
				if ((ctxt->d & IncSP) &&
				    base_reg == VCPU_REGS_RSP)
					modrm_ea += ctxt->op_bytes;
			}
			if (index_reg != 4)
				modrm_ea += reg_read(ctxt, index_reg) << scale;
		} else if ((ctxt->modrm_rm & 7) == 5 && ctxt->modrm_mod == 0) {
			modrm_ea += insn_fetch(s32, ctxt);
			if (ctxt->mode == X86EMUL_MODE_PROT64)
				ctxt->rip_relative = 1;
		} else {
			base_reg = ctxt->modrm_rm;
			modrm_ea += reg_read(ctxt, base_reg);
			adjust_modrm_seg(ctxt, base_reg);
		}
		switch (ctxt->modrm_mod) {
		case 1:
			modrm_ea += insn_fetch(s8, ctxt);
			break;
		case 2:
			modrm_ea += insn_fetch(s32, ctxt);
			break;
		}
	}
	op->addr.mem.ea = modrm_ea;
	if (ctxt->ad_bytes != 8)
		ctxt->memop.addr.mem.ea = (u32)ctxt->memop.addr.mem.ea;

done:
	return rc;
}

static int decode_abs(struct x86_emulate_ctxt *ctxt,
		      struct operand *op)
{
	int rc = X86EMUL_CONTINUE;

	op->type = OP_MEM;
	switch (ctxt->ad_bytes) {
	case 2:
		op->addr.mem.ea = insn_fetch(u16, ctxt);
		break;
	case 4:
		op->addr.mem.ea = insn_fetch(u32, ctxt);
		break;
	case 8:
		op->addr.mem.ea = insn_fetch(u64, ctxt);
		break;
	}
done:
	return rc;
}

static void fetch_bit_operand(struct x86_emulate_ctxt *ctxt)
{
	long sv = 0, mask;

	if (ctxt->dst.type == OP_MEM && ctxt->src.type == OP_REG) {
		mask = ~((long)ctxt->dst.bytes * 8 - 1);

		if (ctxt->src.bytes == 2)
			sv = (s16)ctxt->src.val & (s16)mask;
		else if (ctxt->src.bytes == 4)
			sv = (s32)ctxt->src.val & (s32)mask;
		else
			sv = (s64)ctxt->src.val & (s64)mask;

		ctxt->dst.addr.mem.ea = address_mask(ctxt,
					   ctxt->dst.addr.mem.ea + (sv >> 3));
	}

	/* only subword offset */
	ctxt->src.val &= (ctxt->dst.bytes << 3) - 1;
}

static int read_emulated(struct x86_emulate_ctxt *ctxt,
			 unsigned long addr, void *dest, unsigned size)
{
	int rc;
	struct read_cache *mc = &ctxt->mem_read;

	/*
	 * If the read gets a cache hit, simply copy the value from the cache.
	 * A "hit" here means that there is unused data in the cache, i.e. when
	 * re-emulating an instruction to complete a userspace exit, KVM relies
	 * on "no decode" to ensure the instruction is re-emulated in the same
	 * sequence, so that multiple reads are fulfilled in the correct order.
	 */
	if (mc->pos < mc->end)
		goto read_cached;

	if (KVM_EMULATOR_BUG_ON((mc->end + size) >= sizeof(mc->data), ctxt))
		return X86EMUL_UNHANDLEABLE;

	/*
	 * Route all reads to the cache.  This allows @dest to be an on-stack
	 * variable without triggering use-after-free if KVM needs to exit to
	 * userspace to handle an MMIO read (the MMIO fragment will point at
	 * the current location in the cache).
	 */
	rc = ctxt->ops->read_emulated(ctxt, addr, mc->data + mc->end, size,
				      &ctxt->exception);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	mc->end += size;

read_cached:
	memcpy(dest, mc->data + mc->pos, size);
	mc->pos += size;
	return X86EMUL_CONTINUE;
}

static int segmented_read(struct x86_emulate_ctxt *ctxt,
			  struct segmented_address addr,
			  void *data,
			  unsigned size)
{
	int rc;
	ulong linear;

	rc = linearize(ctxt, addr, size, false, &linear);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	return read_emulated(ctxt, linear, data, size);
}

static int segmented_write(struct x86_emulate_ctxt *ctxt,
			   struct segmented_address addr,
			   const void *data,
			   unsigned size)
{
	int rc;
	ulong linear;

	rc = linearize(ctxt, addr, size, true, &linear);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	return ctxt->ops->write_emulated(ctxt, linear, data, size,
					 &ctxt->exception);
}

static int segmented_cmpxchg(struct x86_emulate_ctxt *ctxt,
			     struct segmented_address addr,
			     const void *orig_data, const void *data,
			     unsigned size)
{
	int rc;
	ulong linear;