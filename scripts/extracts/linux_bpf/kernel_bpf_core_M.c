// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 13/20



	/* CALL */
	JMP_CALL:
		/* Function call scratches BPF_R1-BPF_R5 registers,
		 * preserves BPF_R6-BPF_R9, and stores return value
		 * into BPF_R0.
		 */
		BPF_R0 = (__bpf_call_base + insn->imm)(BPF_R1, BPF_R2, BPF_R3,
						       BPF_R4, BPF_R5);
		CONT;

	JMP_CALL_ARGS:
		BPF_R0 = (__bpf_call_base_args + insn->imm)(BPF_R1, BPF_R2,
							    BPF_R3, BPF_R4,
							    BPF_R5,
							    insn + insn->off + 1);
		CONT;

	JMP_TAIL_CALL: {
		struct bpf_map *map = (struct bpf_map *) (unsigned long) BPF_R2;
		struct bpf_array *array = container_of(map, struct bpf_array, map);
		struct bpf_prog *prog;
		u32 index = BPF_R3;

		if (unlikely(index >= array->map.max_entries))
			goto out;

		if (unlikely(tail_call_cnt >= MAX_TAIL_CALL_CNT))
			goto out;

		prog = READ_ONCE(array->ptrs[index]);
		if (!prog)
			goto out;

		tail_call_cnt++;

		/* ARG1 at this point is guaranteed to point to CTX from
		 * the verifier side due to the fact that the tail call is
		 * handled like a helper, that is, bpf_tail_call_proto,
		 * where arg1_type is ARG_PTR_TO_CTX.
		 */
		insn = prog->insnsi;
		goto select_insn;
out:
		CONT;
	}
	JMP_JA:
		insn += insn->off;
		CONT;
	JMP32_JA:
		insn += insn->imm;
		CONT;
	JMP_EXIT:
		return BPF_R0;
	/* JMP */
#define COND_JMP(SIGN, OPCODE, CMP_OP)				\
	JMP_##OPCODE##_X:					\
		if ((SIGN##64) DST CMP_OP (SIGN##64) SRC) {	\
			insn += insn->off;			\
			CONT_JMP;				\
		}						\
		CONT;						\
	JMP32_##OPCODE##_X:					\
		if ((SIGN##32) DST CMP_OP (SIGN##32) SRC) {	\
			insn += insn->off;			\
			CONT_JMP;				\
		}						\
		CONT;						\
	JMP_##OPCODE##_K:					\
		if ((SIGN##64) DST CMP_OP (SIGN##64) IMM) {	\
			insn += insn->off;			\
			CONT_JMP;				\
		}						\
		CONT;						\
	JMP32_##OPCODE##_K:					\
		if ((SIGN##32) DST CMP_OP (SIGN##32) IMM) {	\
			insn += insn->off;			\
			CONT_JMP;				\
		}						\
		CONT;
	COND_JMP(u, JEQ, ==)
	COND_JMP(u, JNE, !=)
	COND_JMP(u, JGT, >)
	COND_JMP(u, JLT, <)
	COND_JMP(u, JGE, >=)
	COND_JMP(u, JLE, <=)
	COND_JMP(u, JSET, &)
	COND_JMP(s, JSGT, >)
	COND_JMP(s, JSLT, <)
	COND_JMP(s, JSGE, >=)
	COND_JMP(s, JSLE, <=)
#undef COND_JMP
	/* ST, STX and LDX*/
	ST_NOSPEC:
		/* Speculation barrier for mitigating Speculative Store Bypass,
		 * Bounds-Check Bypass and Type Confusion. In case of arm64, we
		 * rely on the firmware mitigation as controlled via the ssbd
		 * kernel parameter. Whenever the mitigation is enabled, it
		 * works for all of the kernel code with no need to provide any
		 * additional instructions here. In case of x86, we use 'lfence'
		 * insn for mitigation. We reuse preexisting logic from Spectre
		 * v1 mitigation that happens to produce the required code on
		 * x86 for v4 as well.
		 */
		barrier_nospec();
		CONT;
#define LDST(SIZEOP, SIZE)						\
	STX_MEM_##SIZEOP:						\
		*(SIZE *)(unsigned long) (DST + insn->off) = SRC;	\
		CONT;							\
	ST_MEM_##SIZEOP:						\
		*(SIZE *)(unsigned long) (DST + insn->off) = IMM;	\
		CONT;							\
	LDX_MEM_##SIZEOP:						\
		DST = *(SIZE *)(unsigned long) (SRC + insn->off);	\
		CONT;							\
	LDX_PROBE_MEM_##SIZEOP:						\
		bpf_probe_read_kernel_common(&DST, sizeof(SIZE),	\
			      (const void *)(long) (SRC + insn->off));	\
		DST = *((SIZE *)&DST);					\
		CONT;

	LDST(B,   u8)
	LDST(H,  u16)
	LDST(W,  u32)
	LDST(DW, u64)
#undef LDST

#define LDSX(SIZEOP, SIZE)						\
	LDX_MEMSX_##SIZEOP:						\
		DST = *(SIZE *)(unsigned long) (SRC + insn->off);	\
		CONT;							\
	LDX_PROBE_MEMSX_##SIZEOP:					\
		bpf_probe_read_kernel_common(&DST, sizeof(SIZE),		\
				      (const void *)(long) (SRC + insn->off));	\
		DST = *((SIZE *)&DST);					\
		CONT;

	LDSX(B,   s8)
	LDSX(H,  s16)
	LDSX(W,  s32)
#undef LDSX

#define ATOMIC_ALU_OP(BOP, KOP)						\
		case BOP:						\
			if (BPF_SIZE(insn->code) == BPF_W)		\
				atomic_##KOP((u32) SRC, (atomic_t *)(unsigned long) \
					     (DST + insn->off));	\
			else if (BPF_SIZE(insn->code) == BPF_DW)	\
				atomic64_##KOP((u64) SRC, (atomic64_t *)(unsigned long) \
					       (DST + insn->off));	\
			else						\
				goto default_label;			\
			break;						\
		case BOP | BPF_FETCH:					\
			if (BPF_SIZE(insn->code) == BPF_W)		\
				SRC = (u32) atomic_fetch_##KOP(		\
					(u32) SRC,			\
					(atomic_t *)(unsigned long) (DST + insn->off)); \
			else if (BPF_SIZE(insn->code) == BPF_DW)	\
				SRC = (u64) atomic64_fetch_##KOP(	\
					(u64) SRC,			\
					(atomic64_t *)(unsigned long) (DST + insn->off)); \
			else						\
				goto default_label;			\
			break;

	STX_ATOMIC_DW:
	STX_ATOMIC_W:
	STX_ATOMIC_H:
	STX_ATOMIC_B:
		switch (IMM) {
		/* Atomic read-modify-write instructions support only W and DW
		 * size modifiers.
		 */
		ATOMIC_ALU_OP(BPF_ADD, add)
		ATOMIC_ALU_OP(BPF_AND, and)
		ATOMIC_ALU_OP(BPF_OR, or)
		ATOMIC_ALU_OP(BPF_XOR, xor)
#undef ATOMIC_ALU_OP