// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 4/33



/* 2-operand, using "a" (dst) and CL (src2) */
#define EM_ASM_2CL(op) \
	EM_ASM_START(op) \
	case 1: __EM_ASM_2(op##b, al, cl); break; \
	case 2: __EM_ASM_2(op##w, ax, cl); break; \
	case 4: __EM_ASM_2(op##l, eax, cl); break; \
	ON64(case 8: __EM_ASM_2(op##q, rax, cl); break;) \
	EM_ASM_END

/* 3-operand, using "a" (dst), "d" (src) and CL (src2) */
#define EM_ASM_3WCL(op) \
	EM_ASM_START(op) \
	case 1: break; \
	case 2: __EM_ASM_3(op##w, ax, dx, cl); break; \
	case 4: __EM_ASM_3(op##l, eax, edx, cl); break; \
	ON64(case 8: __EM_ASM_3(op##q, rax, rdx, cl); break;) \
	EM_ASM_END

static int em_salc(struct x86_emulate_ctxt *ctxt)
{
	/*
	 * Set AL 0xFF if CF is set, or 0x00 when clear.
	 */
	ctxt->dst.val = 0xFF * !!(ctxt->eflags & X86_EFLAGS_CF);
	return X86EMUL_CONTINUE;
}

/*
 * XXX: inoutclob user must know where the argument is being expanded.
 *      Using asm goto would allow us to remove _fault.
 */
#define asm_safe(insn, inoutclob...) \
({ \
	int _fault = 0; \
 \
	asm volatile("1:" insn "\n" \
	             "2:\n" \
		     _ASM_EXTABLE_TYPE_REG(1b, 2b, EX_TYPE_ONE_REG, %[_fault]) \
	             : [_fault] "+r"(_fault) inoutclob ); \
 \
	_fault ? X86EMUL_UNHANDLEABLE : X86EMUL_CONTINUE; \
})

static int emulator_check_intercept(struct x86_emulate_ctxt *ctxt,
				    enum x86_intercept intercept,
				    enum x86_intercept_stage stage)
{
	struct x86_instruction_info info = {
		.intercept  = intercept,
		.rep_prefix = ctxt->rep_prefix,
		.modrm_mod  = ctxt->modrm_mod,
		.modrm_reg  = ctxt->modrm_reg,
		.modrm_rm   = ctxt->modrm_rm,
		.src_val    = ctxt->src.val64,
		.dst_val    = ctxt->dst.val64,
		.src_bytes  = ctxt->src.bytes,
		.dst_bytes  = ctxt->dst.bytes,
		.src_type   = ctxt->src.type,
		.dst_type   = ctxt->dst.type,
		.ad_bytes   = ctxt->ad_bytes,
		.rip	    = ctxt->eip,
		.next_rip   = ctxt->_eip,
	};

	return ctxt->ops->intercept(ctxt, &info, stage);
}

static void assign_masked(ulong *dest, ulong src, ulong mask)
{
	*dest = (*dest & ~mask) | (src & mask);
}

static void assign_register(unsigned long *reg, u64 val, int bytes)
{
	/* The 4-byte case *is* correct: in 64-bit mode we zero-extend. */
	switch (bytes) {
	case 1:
		*(u8 *)reg = (u8)val;
		break;
	case 2:
		*(u16 *)reg = (u16)val;
		break;
	case 4:
		*reg = (u32)val;
		break;	/* 64b: zero-extend */
	case 8:
		*reg = val;
		break;
	}
}

static inline unsigned long ad_mask(struct x86_emulate_ctxt *ctxt)
{
	return (1UL << (ctxt->ad_bytes << 3)) - 1;
}

static ulong stack_mask(struct x86_emulate_ctxt *ctxt)
{
	u16 sel;
	struct desc_struct ss;

	if (ctxt->mode == X86EMUL_MODE_PROT64)
		return ~0UL;
	ctxt->ops->get_segment(ctxt, &sel, &ss, NULL, VCPU_SREG_SS);
	return ~0U >> ((ss.d ^ 1) * 16);  /* d=0: 0xffff; d=1: 0xffffffff */
}

static int stack_size(struct x86_emulate_ctxt *ctxt)
{
	return (__fls(stack_mask(ctxt)) + 1) >> 3;
}

/* Access/update address held in a register, based on addressing mode. */
static inline unsigned long
address_mask(struct x86_emulate_ctxt *ctxt, unsigned long reg)
{
	if (ctxt->ad_bytes == sizeof(unsigned long))
		return reg;
	else
		return reg & ad_mask(ctxt);
}

static inline unsigned long
register_address(struct x86_emulate_ctxt *ctxt, int reg)
{
	return address_mask(ctxt, reg_read(ctxt, reg));
}

static void masked_increment(ulong *reg, ulong mask, int inc)
{
	assign_masked(reg, *reg + inc, mask);
}

static inline void
register_address_increment(struct x86_emulate_ctxt *ctxt, int reg, int inc)
{
	ulong *preg = reg_rmw(ctxt, reg);

	assign_register(preg, *preg + inc, ctxt->ad_bytes);
}

static void rsp_increment(struct x86_emulate_ctxt *ctxt, int inc)
{
	masked_increment(reg_rmw(ctxt, VCPU_REGS_RSP), stack_mask(ctxt), inc);
}

static u32 desc_limit_scaled(struct desc_struct *desc)
{
	u32 limit = get_desc_limit(desc);

	return desc->g ? (limit << 12) | 0xfff : limit;
}

static unsigned long seg_base(struct x86_emulate_ctxt *ctxt, int seg)
{
	if (ctxt->mode == X86EMUL_MODE_PROT64 && seg < VCPU_SREG_FS)
		return 0;

	return ctxt->ops->get_cached_segment_base(ctxt, seg);
}

static int emulate_exception(struct x86_emulate_ctxt *ctxt, int vec,
			     u32 error, bool valid)
{
	if (KVM_EMULATOR_BUG_ON(vec > 0x1f, ctxt))
		return X86EMUL_UNHANDLEABLE;

	ctxt->exception.vector = vec;
	ctxt->exception.error_code = error;
	ctxt->exception.error_code_valid = valid;
	return X86EMUL_PROPAGATE_FAULT;
}

static int emulate_db(struct x86_emulate_ctxt *ctxt)
{
	return emulate_exception(ctxt, DB_VECTOR, 0, false);
}

static int emulate_gp(struct x86_emulate_ctxt *ctxt, int err)
{
	return emulate_exception(ctxt, GP_VECTOR, err, true);
}

static int emulate_ss(struct x86_emulate_ctxt *ctxt, int err)
{
	return emulate_exception(ctxt, SS_VECTOR, err, true);
}

static int emulate_ud(struct x86_emulate_ctxt *ctxt)
{
	return emulate_exception(ctxt, UD_VECTOR, 0, false);
}

static int emulate_ts(struct x86_emulate_ctxt *ctxt, int err)
{
	return emulate_exception(ctxt, TS_VECTOR, err, true);
}