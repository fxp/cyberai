// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 5/33



static int emulate_de(struct x86_emulate_ctxt *ctxt)
{
	return emulate_exception(ctxt, DE_VECTOR, 0, false);
}

static int emulate_nm(struct x86_emulate_ctxt *ctxt)
{
	return emulate_exception(ctxt, NM_VECTOR, 0, false);
}

static u16 get_segment_selector(struct x86_emulate_ctxt *ctxt, unsigned seg)
{
	u16 selector;
	struct desc_struct desc;

	ctxt->ops->get_segment(ctxt, &selector, &desc, NULL, seg);
	return selector;
}

static void set_segment_selector(struct x86_emulate_ctxt *ctxt, u16 selector,
				 unsigned seg)
{
	u16 dummy;
	u32 base3;
	struct desc_struct desc;

	ctxt->ops->get_segment(ctxt, &dummy, &desc, &base3, seg);
	ctxt->ops->set_segment(ctxt, selector, &desc, base3, seg);
}

static inline u8 ctxt_virt_addr_bits(struct x86_emulate_ctxt *ctxt)
{
	return (ctxt->ops->get_cr(ctxt, 4) & X86_CR4_LA57) ? 57 : 48;
}

static inline bool emul_is_noncanonical_address(u64 la,
						struct x86_emulate_ctxt *ctxt,
						unsigned int flags)
{
	return !ctxt->ops->is_canonical_addr(ctxt, la, flags);
}

/*
 * x86 defines three classes of vector instructions: explicitly
 * aligned, explicitly unaligned, and the rest, which change behaviour
 * depending on whether they're AVX encoded or not.
 *
 * Also included is CMPXCHG16B which is not a vector instruction, yet it is
 * subject to the same check.  FXSAVE and FXRSTOR are checked here too as their
 * 512 bytes of data must be aligned to a 16 byte boundary.
 */
static unsigned insn_alignment(struct x86_emulate_ctxt *ctxt, unsigned size)
{
	u64 alignment = ctxt->d & AlignMask;

	if (likely(size < 16))
		return 1;

	switch (alignment) {
	case Unaligned:
		return 1;
	case Aligned16:
		return 16;
	case Aligned:
	default:
		return size;
	}
}

static __always_inline int __linearize(struct x86_emulate_ctxt *ctxt,
				       struct segmented_address addr,
				       unsigned *max_size, unsigned size,
				       enum x86emul_mode mode, ulong *linear,
				       unsigned int flags)
{
	struct desc_struct desc;
	bool usable;
	ulong la;
	u32 lim;
	u16 sel;
	u8  va_bits;

	la = seg_base(ctxt, addr.seg) + addr.ea;
	*max_size = 0;
	switch (mode) {
	case X86EMUL_MODE_PROT64:
		*linear = la = ctxt->ops->get_untagged_addr(ctxt, la, flags);
		va_bits = ctxt_virt_addr_bits(ctxt);
		if (!__is_canonical_address(la, va_bits))
			goto bad;

		*max_size = min_t(u64, ~0u, (1ull << va_bits) - la);
		if (size > *max_size)
			goto bad;
		break;
	default:
		*linear = la = (u32)la;
		usable = ctxt->ops->get_segment(ctxt, &sel, &desc, NULL,
						addr.seg);
		if (!usable)
			goto bad;
		/* code segment in protected mode or read-only data segment */
		if ((((ctxt->mode != X86EMUL_MODE_REAL) && (desc.type & 8)) || !(desc.type & 2)) &&
		    (flags & X86EMUL_F_WRITE))
			goto bad;
		/* unreadable code segment */
		if (!(flags & X86EMUL_F_FETCH) && (desc.type & 8) && !(desc.type & 2))
			goto bad;
		lim = desc_limit_scaled(&desc);
		if (!(desc.type & 8) && (desc.type & 4)) {
			/* expand-down segment */
			if (addr.ea <= lim)
				goto bad;
			lim = desc.d ? 0xffffffff : 0xffff;
		}
		if (addr.ea > lim)
			goto bad;
		if (lim == 0xffffffff)
			*max_size = ~0u;
		else {
			*max_size = (u64)lim + 1 - addr.ea;
			if (size > *max_size)
				goto bad;
		}
		break;
	}
	if (la & (insn_alignment(ctxt, size) - 1))
		return emulate_gp(ctxt, 0);
	return X86EMUL_CONTINUE;
bad:
	if (addr.seg == VCPU_SREG_SS)
		return emulate_ss(ctxt, 0);
	else
		return emulate_gp(ctxt, 0);
}

static int linearize(struct x86_emulate_ctxt *ctxt,
		     struct segmented_address addr,
		     unsigned size, bool write,
		     ulong *linear)
{
	unsigned max_size;
	return __linearize(ctxt, addr, &max_size, size, ctxt->mode, linear,
			   write ? X86EMUL_F_WRITE : 0);
}

static inline int assign_eip(struct x86_emulate_ctxt *ctxt, ulong dst)
{
	ulong linear;
	int rc;
	unsigned max_size;
	struct segmented_address addr = { .seg = VCPU_SREG_CS,
					   .ea = dst };

	if (ctxt->op_bytes != sizeof(unsigned long))
		addr.ea = dst & ((1UL << (ctxt->op_bytes << 3)) - 1);
	rc = __linearize(ctxt, addr, &max_size, 1, ctxt->mode, &linear,
			 X86EMUL_F_FETCH);
	if (rc == X86EMUL_CONTINUE)
		ctxt->_eip = addr.ea;
	return rc;
}

static inline int emulator_recalc_and_set_mode(struct x86_emulate_ctxt *ctxt)
{
	u64 efer;
	struct desc_struct cs;
	u16 selector;
	u32 base3;

	ctxt->ops->get_msr(ctxt, MSR_EFER, &efer);

	if (!(ctxt->ops->get_cr(ctxt, 0) & X86_CR0_PE)) {
		/* Real mode. cpu must not have long mode active */
		if (efer & EFER_LMA)
			return X86EMUL_UNHANDLEABLE;
		ctxt->mode = X86EMUL_MODE_REAL;
		return X86EMUL_CONTINUE;
	}

	if (ctxt->eflags & X86_EFLAGS_VM) {
		/* Protected/VM86 mode. cpu must not have long mode active */
		if (efer & EFER_LMA)
			return X86EMUL_UNHANDLEABLE;
		ctxt->mode = X86EMUL_MODE_VM86;
		return X86EMUL_CONTINUE;
	}

	if (!ctxt->ops->get_segment(ctxt, &selector, &cs, &base3, VCPU_SREG_CS))
		return X86EMUL_UNHANDLEABLE;