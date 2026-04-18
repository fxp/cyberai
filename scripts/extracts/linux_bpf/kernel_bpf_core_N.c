// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 14/20



		case BPF_XCHG:
			if (BPF_SIZE(insn->code) == BPF_W)
				SRC = (u32) atomic_xchg(
					(atomic_t *)(unsigned long) (DST + insn->off),
					(u32) SRC);
			else if (BPF_SIZE(insn->code) == BPF_DW)
				SRC = (u64) atomic64_xchg(
					(atomic64_t *)(unsigned long) (DST + insn->off),
					(u64) SRC);
			else
				goto default_label;
			break;
		case BPF_CMPXCHG:
			if (BPF_SIZE(insn->code) == BPF_W)
				BPF_R0 = (u32) atomic_cmpxchg(
					(atomic_t *)(unsigned long) (DST + insn->off),
					(u32) BPF_R0, (u32) SRC);
			else if (BPF_SIZE(insn->code) == BPF_DW)
				BPF_R0 = (u64) atomic64_cmpxchg(
					(atomic64_t *)(unsigned long) (DST + insn->off),
					(u64) BPF_R0, (u64) SRC);
			else
				goto default_label;
			break;
		/* Atomic load and store instructions support all size
		 * modifiers.
		 */
		case BPF_LOAD_ACQ:
			switch (BPF_SIZE(insn->code)) {
#define LOAD_ACQUIRE(SIZEOP, SIZE)				\
			case BPF_##SIZEOP:			\
				DST = (SIZE)smp_load_acquire(	\
					(SIZE *)(unsigned long)(SRC + insn->off));	\
				break;
			LOAD_ACQUIRE(B,   u8)
			LOAD_ACQUIRE(H,  u16)
			LOAD_ACQUIRE(W,  u32)
#ifdef CONFIG_64BIT
			LOAD_ACQUIRE(DW, u64)
#endif
#undef LOAD_ACQUIRE
			default:
				goto default_label;
			}
			break;
		case BPF_STORE_REL:
			switch (BPF_SIZE(insn->code)) {
#define STORE_RELEASE(SIZEOP, SIZE)			\
			case BPF_##SIZEOP:		\
				smp_store_release(	\
					(SIZE *)(unsigned long)(DST + insn->off), (SIZE)SRC);	\
				break;
			STORE_RELEASE(B,   u8)
			STORE_RELEASE(H,  u16)
			STORE_RELEASE(W,  u32)
#ifdef CONFIG_64BIT
			STORE_RELEASE(DW, u64)
#endif
#undef STORE_RELEASE
			default:
				goto default_label;
			}
			break;

		default:
			goto default_label;
		}
		CONT;

	default_label:
		/* If we ever reach this, we have a bug somewhere. Die hard here
		 * instead of just returning 0; we could be somewhere in a subprog,
		 * so execution could continue otherwise which we do /not/ want.
		 *
		 * Note, verifier whitelists all opcodes in bpf_opcode_in_insntable().
		 */
		pr_warn("BPF interpreter: unknown opcode %02x (imm: 0x%x)\n",
			insn->code, insn->imm);
		BUG_ON(1);
		return 0;
}

#define PROG_NAME(stack_size) __bpf_prog_run##stack_size
#define DEFINE_BPF_PROG_RUN(stack_size) \
static unsigned int PROG_NAME(stack_size)(const void *ctx, const struct bpf_insn *insn) \
{ \
	u64 stack[stack_size / sizeof(u64)]; \
	u64 regs[MAX_BPF_EXT_REG] = {}; \
\
	kmsan_unpoison_memory(stack, sizeof(stack)); \
	FP = (u64) (unsigned long) &stack[ARRAY_SIZE(stack)]; \
	ARG1 = (u64) (unsigned long) ctx; \
	return ___bpf_prog_run(regs, insn); \
}

#define PROG_NAME_ARGS(stack_size) __bpf_prog_run_args##stack_size
#define DEFINE_BPF_PROG_RUN_ARGS(stack_size) \
static u64 PROG_NAME_ARGS(stack_size)(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5, \
				      const struct bpf_insn *insn) \
{ \
	u64 stack[stack_size / sizeof(u64)]; \
	u64 regs[MAX_BPF_EXT_REG]; \
\
	kmsan_unpoison_memory(stack, sizeof(stack)); \
	FP = (u64) (unsigned long) &stack[ARRAY_SIZE(stack)]; \
	BPF_R1 = r1; \
	BPF_R2 = r2; \
	BPF_R3 = r3; \
	BPF_R4 = r4; \
	BPF_R5 = r5; \
	return ___bpf_prog_run(regs, insn); \
}

#define EVAL1(FN, X) FN(X)
#define EVAL2(FN, X, Y...) FN(X) EVAL1(FN, Y)
#define EVAL3(FN, X, Y...) FN(X) EVAL2(FN, Y)
#define EVAL4(FN, X, Y...) FN(X) EVAL3(FN, Y)
#define EVAL5(FN, X, Y...) FN(X) EVAL4(FN, Y)
#define EVAL6(FN, X, Y...) FN(X) EVAL5(FN, Y)

EVAL6(DEFINE_BPF_PROG_RUN, 32, 64, 96, 128, 160, 192);
EVAL6(DEFINE_BPF_PROG_RUN, 224, 256, 288, 320, 352, 384);
EVAL4(DEFINE_BPF_PROG_RUN, 416, 448, 480, 512);

EVAL6(DEFINE_BPF_PROG_RUN_ARGS, 32, 64, 96, 128, 160, 192);
EVAL6(DEFINE_BPF_PROG_RUN_ARGS, 224, 256, 288, 320, 352, 384);
EVAL4(DEFINE_BPF_PROG_RUN_ARGS, 416, 448, 480, 512);

#define PROG_NAME_LIST(stack_size) PROG_NAME(stack_size),

static unsigned int (*interpreters[])(const void *ctx,
				      const struct bpf_insn *insn) = {
EVAL6(PROG_NAME_LIST, 32, 64, 96, 128, 160, 192)
EVAL6(PROG_NAME_LIST, 224, 256, 288, 320, 352, 384)
EVAL4(PROG_NAME_LIST, 416, 448, 480, 512)
};
#undef PROG_NAME_LIST
#define PROG_NAME_LIST(stack_size) PROG_NAME_ARGS(stack_size),
static __maybe_unused
u64 (*interpreters_args[])(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5,
			   const struct bpf_insn *insn) = {
EVAL6(PROG_NAME_LIST, 32, 64, 96, 128, 160, 192)
EVAL6(PROG_NAME_LIST, 224, 256, 288, 320, 352, 384)
EVAL4(PROG_NAME_LIST, 416, 448, 480, 512)
};
#undef PROG_NAME_LIST

#ifdef CONFIG_BPF_SYSCALL
void bpf_patch_call_args(struct bpf_insn *insn, u32 stack_depth)
{
	stack_depth = max_t(u32, stack_depth, 1);
	insn->off = (s16) insn->imm;
	insn->imm = interpreters_args[(round_up(stack_depth, 32) / 32) - 1] -
		__bpf_call_base_args;
	insn->code = BPF_JMP | BPF_CALL_ARGS;
}
#endif
#endif

static unsigned int __bpf_prog_ret0_warn(const void *ctx,
					 const struct bpf_insn *insn)
{
	/* If this handler ever gets executed, then BPF_JIT_ALWAYS_ON
	 * is not working properly, so warn about it!
	 */
	WARN_ON_ONCE(1);
	return 0;
}