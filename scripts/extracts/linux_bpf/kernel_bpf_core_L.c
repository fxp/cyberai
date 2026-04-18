// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 12/20



	/* Explicitly mask the register-based shift amounts with 63 or 31
	 * to avoid undefined behavior. Normally this won't affect the
	 * generated code, for example, in case of native 64 bit archs such
	 * as x86-64 or arm64, the compiler is optimizing the AND away for
	 * the interpreter. In case of JITs, each of the JIT backends compiles
	 * the BPF shift operations to machine instructions which produce
	 * implementation-defined results in such a case; the resulting
	 * contents of the register may be arbitrary, but program behaviour
	 * as a whole remains defined. In other words, in case of JIT backends,
	 * the AND must /not/ be added to the emitted LSH/RSH/ARSH translation.
	 */
	/* ALU (shifts) */
#define SHT(OPCODE, OP)					\
	ALU64_##OPCODE##_X:				\
		DST = DST OP (SRC & 63);		\
		CONT;					\
	ALU_##OPCODE##_X:				\
		DST = (u32) DST OP ((u32) SRC & 31);	\
		CONT;					\
	ALU64_##OPCODE##_K:				\
		DST = DST OP IMM;			\
		CONT;					\
	ALU_##OPCODE##_K:				\
		DST = (u32) DST OP (u32) IMM;		\
		CONT;
	/* ALU (rest) */
#define ALU(OPCODE, OP)					\
	ALU64_##OPCODE##_X:				\
		DST = DST OP SRC;			\
		CONT;					\
	ALU_##OPCODE##_X:				\
		DST = (u32) DST OP (u32) SRC;		\
		CONT;					\
	ALU64_##OPCODE##_K:				\
		DST = DST OP IMM;			\
		CONT;					\
	ALU_##OPCODE##_K:				\
		DST = (u32) DST OP (u32) IMM;		\
		CONT;
	ALU(ADD,  +)
	ALU(SUB,  -)
	ALU(AND,  &)
	ALU(OR,   |)
	ALU(XOR,  ^)
	ALU(MUL,  *)
	SHT(LSH, <<)
	SHT(RSH, >>)
#undef SHT
#undef ALU
	ALU_NEG:
		DST = (u32) -DST;
		CONT;
	ALU64_NEG:
		DST = -DST;
		CONT;
	ALU_MOV_X:
		switch (OFF) {
		case 0:
			DST = (u32) SRC;
			break;
		case 8:
			DST = (u32)(s8) SRC;
			break;
		case 16:
			DST = (u32)(s16) SRC;
			break;
		}
		CONT;
	ALU_MOV_K:
		DST = (u32) IMM;
		CONT;
	ALU64_MOV_X:
		switch (OFF) {
		case 0:
			DST = SRC;
			break;
		case 8:
			DST = (s8) SRC;
			break;
		case 16:
			DST = (s16) SRC;
			break;
		case 32:
			DST = (s32) SRC;
			break;
		}
		CONT;
	ALU64_MOV_K:
		DST = IMM;
		CONT;
	LD_IMM_DW:
		DST = (u64) (u32) insn[0].imm | ((u64) (u32) insn[1].imm) << 32;
		insn++;
		CONT;
	ALU_ARSH_X:
		DST = (u64) (u32) (((s32) DST) >> (SRC & 31));
		CONT;
	ALU_ARSH_K:
		DST = (u64) (u32) (((s32) DST) >> IMM);
		CONT;
	ALU64_ARSH_X:
		(*(s64 *) &DST) >>= (SRC & 63);
		CONT;
	ALU64_ARSH_K:
		(*(s64 *) &DST) >>= IMM;
		CONT;
	ALU64_MOD_X:
		switch (OFF) {
		case 0:
			div64_u64_rem(DST, SRC, &AX);
			DST = AX;
			break;
		case 1:
			AX = div64_s64(DST, SRC);
			DST = DST - AX * SRC;
			break;
		}
		CONT;
	ALU_MOD_X:
		switch (OFF) {
		case 0:
			AX = (u32) DST;
			DST = do_div(AX, (u32) SRC);
			break;
		case 1:
			AX = abs_s32((s32)DST);
			AX = do_div(AX, abs_s32((s32)SRC));
			if ((s32)DST < 0)
				DST = (u32)-AX;
			else
				DST = (u32)AX;
			break;
		}
		CONT;
	ALU64_MOD_K:
		switch (OFF) {
		case 0:
			div64_u64_rem(DST, IMM, &AX);
			DST = AX;
			break;
		case 1:
			AX = div64_s64(DST, IMM);
			DST = DST - AX * IMM;
			break;
		}
		CONT;
	ALU_MOD_K:
		switch (OFF) {
		case 0:
			AX = (u32) DST;
			DST = do_div(AX, (u32) IMM);
			break;
		case 1:
			AX = abs_s32((s32)DST);
			AX = do_div(AX, abs_s32((s32)IMM));
			if ((s32)DST < 0)
				DST = (u32)-AX;
			else
				DST = (u32)AX;
			break;
		}
		CONT;
	ALU64_DIV_X:
		switch (OFF) {
		case 0:
			DST = div64_u64(DST, SRC);
			break;
		case 1:
			DST = div64_s64(DST, SRC);
			break;
		}
		CONT;
	ALU_DIV_X:
		switch (OFF) {
		case 0:
			AX = (u32) DST;
			do_div(AX, (u32) SRC);
			DST = (u32) AX;
			break;
		case 1:
			AX = abs_s32((s32)DST);
			do_div(AX, abs_s32((s32)SRC));
			if (((s32)DST < 0) == ((s32)SRC < 0))
				DST = (u32)AX;
			else
				DST = (u32)-AX;
			break;
		}
		CONT;
	ALU64_DIV_K:
		switch (OFF) {
		case 0:
			DST = div64_u64(DST, IMM);
			break;
		case 1:
			DST = div64_s64(DST, IMM);
			break;
		}
		CONT;
	ALU_DIV_K:
		switch (OFF) {
		case 0:
			AX = (u32) DST;
			do_div(AX, (u32) IMM);
			DST = (u32) AX;
			break;
		case 1:
			AX = abs_s32((s32)DST);
			do_div(AX, abs_s32((s32)IMM));
			if (((s32)DST < 0) == ((s32)IMM < 0))
				DST = (u32)AX;
			else
				DST = (u32)-AX;
			break;
		}
		CONT;
	ALU_END_TO_BE:
		switch (IMM) {
		case 16:
			DST = (__force u16) cpu_to_be16(DST);
			break;
		case 32:
			DST = (__force u32) cpu_to_be32(DST);
			break;
		case 64:
			DST = (__force u64) cpu_to_be64(DST);
			break;
		}
		CONT;
	ALU_END_TO_LE:
		switch (IMM) {
		case 16:
			DST = (__force u16) cpu_to_le16(DST);
			break;
		case 32:
			DST = (__force u32) cpu_to_le32(DST);
			break;
		case 64:
			DST = (__force u64) cpu_to_le64(DST);
			break;
		}
		CONT;
	ALU64_END_TO_LE:
		switch (IMM) {
		case 16:
			DST = (__force u16) __swab16(DST);
			break;
		case 32:
			DST = (__force u32) __swab32(DST);
			break;
		case 64:
			DST = (__force u64) __swab64(DST);
			break;
		}
		CONT;