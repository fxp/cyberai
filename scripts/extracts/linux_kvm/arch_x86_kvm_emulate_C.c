// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 3/33

64)1 << 54)  /* SP is incremented before ModRM calc */
#define TwoMemOp    ((u64)1 << 55)  /* Instruction has two memory operand */
#define IsBranch    ((u64)1 << 56)  /* Instruction is considered a branch. */
#define ShadowStack ((u64)1 << 57)  /* Instruction affects Shadow Stacks. */

#define DstXacc     (DstAccLo | SrcAccHi | SrcWrite)

#define X2(x...) x, x
#define X3(x...) X2(x), x
#define X4(x...) X2(x), X2(x)
#define X5(x...) X4(x), x
#define X6(x...) X4(x), X2(x)
#define X7(x...) X4(x), X3(x)
#define X8(x...) X4(x), X4(x)
#define X16(x...) X8(x), X8(x)

struct opcode {
	u64 flags;
	u8 intercept;
	u8 pad[7];
	union {
		int (*execute)(struct x86_emulate_ctxt *ctxt);
		const struct opcode *group;
		const struct group_dual *gdual;
		const struct gprefix *gprefix;
		const struct escape *esc;
		const struct instr_dual *idual;
		const struct mode_dual *mdual;
	} u;
	int (*check_perm)(struct x86_emulate_ctxt *ctxt);
};

struct group_dual {
	struct opcode mod012[8];
	struct opcode mod3[8];
};

struct gprefix {
	struct opcode pfx_no;
	struct opcode pfx_66;
	struct opcode pfx_f2;
	struct opcode pfx_f3;
};

struct escape {
	struct opcode op[8];
	struct opcode high[64];
};

struct instr_dual {
	struct opcode mod012;
	struct opcode mod3;
};

struct mode_dual {
	struct opcode mode32;
	struct opcode mode64;
};

#define EFLG_RESERVED_ZEROS_MASK 0xffc0802a

enum x86_transfer_type {
	X86_TRANSFER_NONE,
	X86_TRANSFER_CALL_JMP,
	X86_TRANSFER_RET,
	X86_TRANSFER_TASK_SWITCH,
};

enum rex_bits {
	REX_B = 1,
	REX_X = 2,
	REX_R = 4,
	REX_W = 8,
};

static void writeback_registers(struct x86_emulate_ctxt *ctxt)
{
	unsigned long dirty = ctxt->regs_dirty;
	unsigned reg;

	for_each_set_bit(reg, &dirty, NR_EMULATOR_GPRS)
		ctxt->ops->write_gpr(ctxt, reg, ctxt->_regs[reg]);
}

static void invalidate_registers(struct x86_emulate_ctxt *ctxt)
{
	ctxt->regs_dirty = 0;
	ctxt->regs_valid = 0;
}

/*
 * These EFLAGS bits are restored from saved value during emulation, and
 * any changes are written back to the saved value after emulation.
 */
#define EFLAGS_MASK (X86_EFLAGS_OF|X86_EFLAGS_SF|X86_EFLAGS_ZF|X86_EFLAGS_AF|\
		     X86_EFLAGS_PF|X86_EFLAGS_CF)

#ifdef CONFIG_X86_64
#define ON64(x...) x
#else
#define ON64(x...)
#endif

#define EM_ASM_START(op) \
static int em_##op(struct x86_emulate_ctxt *ctxt) \
{ \
	unsigned long flags = (ctxt->eflags & EFLAGS_MASK) | X86_EFLAGS_IF; \
	int bytes = 1, ok = 1; \
	if (!(ctxt->d & ByteOp)) \
		bytes = ctxt->dst.bytes; \
	switch (bytes) {

#define __EM_ASM(str) \
		asm("push %[flags]; popf \n\t" \
		    "10: " str \
		    "pushf; pop %[flags] \n\t" \
		    "11: \n\t" \
		    : "+a" (ctxt->dst.val), \
		      "+d" (ctxt->src.val), \
		      [flags] "+D" (flags), \
		      "+S" (ok) \
		    : "c" (ctxt->src2.val))

#define __EM_ASM_1(op, dst) \
		__EM_ASM(#op " %%" #dst " \n\t")

#define __EM_ASM_1_EX(op, dst) \
		__EM_ASM(#op " %%" #dst " \n\t" \
			 _ASM_EXTABLE_TYPE_REG(10b, 11f, EX_TYPE_ZERO_REG, %%esi))

#define __EM_ASM_2(op, dst, src) \
		__EM_ASM(#op " %%" #src ", %%" #dst " \n\t")

#define __EM_ASM_3(op, dst, src, src2) \
		__EM_ASM(#op " %%" #src2 ", %%" #src ", %%" #dst " \n\t")

#define EM_ASM_END \
	} \
	ctxt->eflags = (ctxt->eflags & ~EFLAGS_MASK) | (flags & EFLAGS_MASK); \
	return !ok ? emulate_de(ctxt) : X86EMUL_CONTINUE; \
}

/* 1-operand, using "a" (dst) */
#define EM_ASM_1(op) \
	EM_ASM_START(op) \
	case 1: __EM_ASM_1(op##b, al); break; \
	case 2: __EM_ASM_1(op##w, ax); break; \
	case 4: __EM_ASM_1(op##l, eax); break; \
	ON64(case 8: __EM_ASM_1(op##q, rax); break;) \
	EM_ASM_END

/* 1-operand, using "c" (src2) */
#define EM_ASM_1SRC2(op, name) \
	EM_ASM_START(name) \
	case 1: __EM_ASM_1(op##b, cl); break; \
	case 2: __EM_ASM_1(op##w, cx); break; \
	case 4: __EM_ASM_1(op##l, ecx); break; \
	ON64(case 8: __EM_ASM_1(op##q, rcx); break;) \
	EM_ASM_END

/* 1-operand, using "c" (src2) with exception */
#define EM_ASM_1SRC2EX(op, name) \
	EM_ASM_START(name) \
	case 1: __EM_ASM_1_EX(op##b, cl); break; \
	case 2: __EM_ASM_1_EX(op##w, cx); break; \
	case 4: __EM_ASM_1_EX(op##l, ecx); break; \
	ON64(case 8: __EM_ASM_1_EX(op##q, rcx); break;) \
	EM_ASM_END

/* 2-operand, using "a" (dst), "d" (src) */
#define EM_ASM_2(op) \
	EM_ASM_START(op) \
	case 1: __EM_ASM_2(op##b, al, dl); break; \
	case 2: __EM_ASM_2(op##w, ax, dx); break; \
	case 4: __EM_ASM_2(op##l, eax, edx); break; \
	ON64(case 8: __EM_ASM_2(op##q, rax, rdx); break;) \
	EM_ASM_END

/* 2-operand, reversed */
#define EM_ASM_2R(op, name) \
	EM_ASM_START(name) \
	case 1: __EM_ASM_2(op##b, dl, al); break; \
	case 2: __EM_ASM_2(op##w, dx, ax); break; \
	case 4: __EM_ASM_2(op##l, edx, eax); break; \
	ON64(case 8: __EM_ASM_2(op##q, rdx, rax); break;) \
	EM_ASM_END

/* 2-operand, word only (no byte op) */
#define EM_ASM_2W(op) \
	EM_ASM_START(op) \
	case 1: break; \
	case 2: __EM_ASM_2(op##w, ax, dx); break; \
	case 4: __EM_ASM_2(op##l, eax, edx); break; \
	ON64(case 8: __EM_ASM_2(op##q, rax, rdx); break;) \
	EM_ASM_END