// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 10/20



/* All UAPI available opcodes. */
#define BPF_INSN_MAP(INSN_2, INSN_3)		\
	/* 32 bit ALU operations. */		\
	/*   Register based. */			\
	INSN_3(ALU, ADD,  X),			\
	INSN_3(ALU, SUB,  X),			\
	INSN_3(ALU, AND,  X),			\
	INSN_3(ALU, OR,   X),			\
	INSN_3(ALU, LSH,  X),			\
	INSN_3(ALU, RSH,  X),			\
	INSN_3(ALU, XOR,  X),			\
	INSN_3(ALU, MUL,  X),			\
	INSN_3(ALU, MOV,  X),			\
	INSN_3(ALU, ARSH, X),			\
	INSN_3(ALU, DIV,  X),			\
	INSN_3(ALU, MOD,  X),			\
	INSN_2(ALU, NEG),			\
	INSN_3(ALU, END, TO_BE),		\
	INSN_3(ALU, END, TO_LE),		\
	/*   Immediate based. */		\
	INSN_3(ALU, ADD,  K),			\
	INSN_3(ALU, SUB,  K),			\
	INSN_3(ALU, AND,  K),			\
	INSN_3(ALU, OR,   K),			\
	INSN_3(ALU, LSH,  K),			\
	INSN_3(ALU, RSH,  K),			\
	INSN_3(ALU, XOR,  K),			\
	INSN_3(ALU, MUL,  K),			\
	INSN_3(ALU, MOV,  K),			\
	INSN_3(ALU, ARSH, K),			\
	INSN_3(ALU, DIV,  K),			\
	INSN_3(ALU, MOD,  K),			\
	/* 64 bit ALU operations. */		\
	/*   Register based. */			\
	INSN_3(ALU64, ADD,  X),			\
	INSN_3(ALU64, SUB,  X),			\
	INSN_3(ALU64, AND,  X),			\
	INSN_3(ALU64, OR,   X),			\
	INSN_3(ALU64, LSH,  X),			\
	INSN_3(ALU64, RSH,  X),			\
	INSN_3(ALU64, XOR,  X),			\
	INSN_3(ALU64, MUL,  X),			\
	INSN_3(ALU64, MOV,  X),			\
	INSN_3(ALU64, ARSH, X),			\
	INSN_3(ALU64, DIV,  X),			\
	INSN_3(ALU64, MOD,  X),			\
	INSN_2(ALU64, NEG),			\
	INSN_3(ALU64, END, TO_LE),		\
	/*   Immediate based. */		\
	INSN_3(ALU64, ADD,  K),			\
	INSN_3(ALU64, SUB,  K),			\
	INSN_3(ALU64, AND,  K),			\
	INSN_3(ALU64, OR,   K),			\
	INSN_3(ALU64, LSH,  K),			\
	INSN_3(ALU64, RSH,  K),			\
	INSN_3(ALU64, XOR,  K),			\
	INSN_3(ALU64, MUL,  K),			\
	INSN_3(ALU64, MOV,  K),			\
	INSN_3(ALU64, ARSH, K),			\
	INSN_3(ALU64, DIV,  K),			\
	INSN_3(ALU64, MOD,  K),			\
	/* Call instruction. */			\
	INSN_2(JMP, CALL),			\
	/* Exit instruction. */			\
	INSN_2(JMP, EXIT),			\
	/* 32-bit Jump instructions. */		\
	/*   Register based. */			\
	INSN_3(JMP32, JEQ,  X),			\
	INSN_3(JMP32, JNE,  X),			\
	INSN_3(JMP32, JGT,  X),			\
	INSN_3(JMP32, JLT,  X),			\
	INSN_3(JMP32, JGE,  X),			\
	INSN_3(JMP32, JLE,  X),			\
	INSN_3(JMP32, JSGT, X),			\
	INSN_3(JMP32, JSLT, X),			\
	INSN_3(JMP32, JSGE, X),			\
	INSN_3(JMP32, JSLE, X),			\
	INSN_3(JMP32, JSET, X),			\
	/*   Immediate based. */		\
	INSN_3(JMP32, JEQ,  K),			\
	INSN_3(JMP32, JNE,  K),			\
	INSN_3(JMP32, JGT,  K),			\
	INSN_3(JMP32, JLT,  K),			\
	INSN_3(JMP32, JGE,  K),			\
	INSN_3(JMP32, JLE,  K),			\
	INSN_3(JMP32, JSGT, K),			\
	INSN_3(JMP32, JSLT, K),			\
	INSN_3(JMP32, JSGE, K),			\
	INSN_3(JMP32, JSLE, K),			\
	INSN_3(JMP32, JSET, K),			\
	/* Jump instructions. */		\
	/*   Register based. */			\
	INSN_3(JMP, JEQ,  X),			\
	INSN_3(JMP, JNE,  X),			\
	INSN_3(JMP, JGT,  X),			\
	INSN_3(JMP, JLT,  X),			\
	INSN_3(JMP, JGE,  X),			\
	INSN_3(JMP, JLE,  X),			\
	INSN_3(JMP, JSGT, X),			\
	INSN_3(JMP, JSLT, X),			\
	INSN_3(JMP, JSGE, X),			\
	INSN_3(JMP, JSLE, X),			\
	INSN_3(JMP, JSET, X),			\
	/*   Immediate based. */		\
	INSN_3(JMP, JEQ,  K),			\
	INSN_3(JMP, JNE,  K),			\
	INSN_3(JMP, JGT,  K),			\
	INSN_3(JMP, JLT,  K),			\
	INSN_3(JMP, JGE,  K),			\
	INSN_3(JMP, JLE,  K),			\
	INSN_3(JMP, JSGT, K),			\
	INSN_3(JMP, JSLT, K),			\
	INSN_3(JMP, JSGE, K),			\
	INSN_3(JMP, JSLE, K),			\
	INSN_3(JMP, JSET, K),			\
	INSN_2(JMP, JA),			\
	INSN_2(JMP32, JA),			\
	/* Atomic operations. */		\
	INSN_3(STX, ATOMIC, B),			\
	INSN_3(STX, ATOMIC, H),			\
	INSN_3(STX, ATOMIC, W),			\
	INSN_3(STX, ATOMIC, DW),		\
	/* Store instructions. */		\
	/*   Register based. */			\
	INSN_3(STX, MEM,  B),			\
	INSN_3(STX, MEM,  H),			\
	INSN_3(STX, MEM,  W),			\
	INSN_3(STX, MEM,  DW),			\
	/*   Immediate based. */		\
	INSN_3(ST, MEM, B),			\
	INSN_3(ST, MEM, H),			\
	INSN_3(ST, MEM, W),			\
	INSN_3(ST, MEM, DW),			\
	/* Load instructions. */		\
	/*   Register based. */			\
	INSN_3(LDX, MEM, B),			\
	INSN_3(LDX, MEM, H),			\
	INSN_3(LDX, MEM, W),			\
	INSN_3(LDX, MEM, DW),			\
	INSN_3(LDX, MEMSX, B),			\
	INSN_3(LDX, MEMSX, H),			\
	INSN_3(LDX, MEMSX, W),			\
	/*   Immediate based. */		\
	INSN_3(LD, IMM, DW)

bool bpf_opcode_in_insntable(u8 code)
{
#define BPF_INSN_2_TBL(x, y)    [BPF_##x | BPF_##y] = true
#define BPF_INSN_3_TBL(x, y, z) [BPF_##x | BPF_##y | BPF_##z] = true
	static const bool public_insntable[256] = {
		[0 ... 255] = false,
		/* Now overwrite non-defaults ... */
		BPF_INSN_MAP(BPF_INSN_2_TBL, BPF_INSN_3_TBL),
		/* UAPI exposed, but rewritten opcodes. cBPF carry-over. */
		[BPF_LD | BPF_ABS | BPF_B] = true,
		[BPF_LD | BPF_ABS | BPF_H] = true,
		[BPF_LD | BPF_ABS | BPF_W] = true,
		[BPF_LD | BPF_IND | BPF_B] = true,
		[BPF_LD | BPF_IND | BPF_H] = true,
		[BPF_LD | BPF_IND | BPF_W] = true,
		[BPF_JMP | BPF_JA | BPF_X] = true,
		[BPF_JMP | BPF_JCOND] = true,
	};
#undef BPF_INSN_3_TBL
#undef BPF_INSN_2_TBL
	return public_insntable[code];
}

#ifndef CONFIG_BPF_JIT_ALWAYS_ON
/* Absolute value of s32 without undefined behavior for S32_MIN */
static u32 abs_s32(s32 x)
{
	return x >= 0 ? (u32)x : -(u32)x;
}