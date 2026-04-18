// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 23/33



static const struct opcode group7_rm3[] = {
	DIP(SrcNone | Prot | Priv,		vmrun,		check_svme_pa),
	II(SrcNone  | Prot | EmulateOnUD,	em_hypercall,	vmmcall),
	DIP(SrcNone | Prot | Priv,		vmload,		check_svme_pa),
	DIP(SrcNone | Prot | Priv,		vmsave,		check_svme_pa),
	DIP(SrcNone | Prot | Priv,		stgi,		check_svme),
	DIP(SrcNone | Prot | Priv,		clgi,		check_svme),
	DIP(SrcNone | Prot | Priv,		skinit,		check_svme),
	DIP(SrcNone | Prot | Priv,		invlpga,	check_svme),
};

static const struct opcode group7_rm7[] = {
	N,
	DIP(SrcNone, rdtscp, check_rdtsc),
	N, N, N, N, N, N,
};

static const struct opcode group1[] = {
	I(Lock, em_add),
	I(Lock | PageTable, em_or),
	I(Lock, em_adc),
	I(Lock, em_sbb),
	I(Lock | PageTable, em_and),
	I(Lock, em_sub),
	I(Lock, em_xor),
	I(NoWrite, em_cmp),
};

static const struct opcode group1A[] = {
	I(DstMem | SrcNone | Mov | Stack | IncSP | TwoMemOp, em_pop), N, N, N, N, N, N, N,
};

static const struct opcode group2[] = {
	I(DstMem | ModRM, em_rol),
	I(DstMem | ModRM, em_ror),
	I(DstMem | ModRM, em_rcl),
	I(DstMem | ModRM, em_rcr),
	I(DstMem | ModRM, em_shl),
	I(DstMem | ModRM, em_shr),
	I(DstMem | ModRM, em_shl),
	I(DstMem | ModRM, em_sar),
};

static const struct opcode group3[] = {
	I(DstMem | SrcImm | NoWrite, em_test),
	I(DstMem | SrcImm | NoWrite, em_test),
	I(DstMem | SrcNone | Lock, em_not),
	I(DstMem | SrcNone | Lock, em_neg),
	I(DstXacc | Src2Mem, em_mul_ex),
	I(DstXacc | Src2Mem, em_imul_ex),
	I(DstXacc | Src2Mem, em_div_ex),
	I(DstXacc | Src2Mem, em_idiv_ex),
};

static const struct opcode group4[] = {
	I(ByteOp | DstMem | SrcNone | Lock, em_inc),
	I(ByteOp | DstMem | SrcNone | Lock, em_dec),
	N, N, N, N, N, N,
};

static const struct opcode group5[] = {
	I(DstMem | SrcNone | Lock,		em_inc),
	I(DstMem | SrcNone | Lock,		em_dec),
	I(SrcMem | NearBranch | IsBranch | ShadowStack, em_call_near_abs),
	I(SrcMemFAddr | ImplicitOps | IsBranch | ShadowStack, em_call_far),
	I(SrcMem | NearBranch | IsBranch,       em_jmp_abs),
	I(SrcMemFAddr | ImplicitOps | IsBranch, em_jmp_far),
	I(SrcMem | Stack | TwoMemOp,		em_push), D(Undefined),
};

static const struct opcode group6[] = {
	II(Prot | DstMem,	   em_sldt, sldt),
	II(Prot | DstMem,	   em_str, str),
	II(Prot | Priv | SrcMem16, em_lldt, lldt),
	II(Prot | Priv | SrcMem16, em_ltr, ltr),
	N, N, N, N,
};

static const struct group_dual group7 = { {
	II(Mov | DstMem,			em_sgdt, sgdt),
	II(Mov | DstMem,			em_sidt, sidt),
	II(SrcMem | Priv,			em_lgdt, lgdt),
	II(SrcMem | Priv,			em_lidt, lidt),
	II(SrcNone | DstMem | Mov,		em_smsw, smsw), N,
	II(SrcMem16 | Mov | Priv,		em_lmsw, lmsw),
	II(SrcMem | ByteOp | Priv | NoAccess,	em_invlpg, invlpg),
}, {
	EXT(0, group7_rm0),
	EXT(0, group7_rm1),
	EXT(0, group7_rm2),
	EXT(0, group7_rm3),
	II(SrcNone | DstMem | Mov,		em_smsw, smsw), N,
	II(SrcMem16 | Mov | Priv,		em_lmsw, lmsw),
	EXT(0, group7_rm7),
} };

static const struct opcode group8[] = {
	N, N, N, N,
	I(DstMem | SrcImmByte | NoWrite,		em_bt),
	I(DstMem | SrcImmByte | Lock | PageTable,	em_bts),
	I(DstMem | SrcImmByte | Lock,			em_btr),
	I(DstMem | SrcImmByte | Lock | PageTable,	em_btc),
};

/*
 * The "memory" destination is actually always a register, since we come
 * from the register case of group9.
 */
static const struct gprefix pfx_0f_c7_7 = {
	N, N, N, II(DstMem | ModRM | Op3264 | EmulateOnUD, em_rdpid, rdpid),
};


static const struct group_dual group9 = { {
	N, I(DstMem64 | Lock | PageTable, em_cmpxchg8b), N, N, N, N, N, N,
}, {
	N, N, N, N, N, N, N,
	GP(0, &pfx_0f_c7_7),
} };

static const struct opcode group11[] = {
	I(DstMem | SrcImm | Mov | PageTable, em_mov),
	X7(D(Undefined)),
};

static const struct gprefix pfx_0f_ae_7 = {
	I(SrcMem | ByteOp, em_clflush), I(SrcMem | ByteOp, em_clflushopt), N, N,
};

static const struct group_dual group15 = { {
	I(ModRM | Aligned16, em_fxsave),
	I(ModRM | Aligned16, em_fxrstor),
	N, N, N, N, N, GP(0, &pfx_0f_ae_7),
}, {
	N, N, N, N, N, N, N, N,
} };

static const struct gprefix pfx_0f_6f_0f_7f = {
	I(Mmx, em_mov), I(Sse | Avx | Aligned, em_mov), N, I(Sse | Avx | Unaligned, em_mov),
};

static const struct instr_dual instr_dual_0f_2b = {
	I(0, em_mov), N
};

static const struct gprefix pfx_0f_2b = {
	ID(0, &instr_dual_0f_2b), ID(0, &instr_dual_0f_2b), N, N,
};

static const struct gprefix pfx_0f_10_0f_11 = {
	I(Unaligned, em_mov), I(Unaligned, em_mov), N, N,
};

static const struct gprefix pfx_0f_28_0f_29 = {
	I(Aligned, em_mov), I(Aligned, em_mov), N, N,
};

static const struct gprefix pfx_0f_e7_0f_38_2a = {
	N, I(Sse | Avx, em_mov), N, N,
};

static const struct escape escape_d9 = { {
	N, N, N, N, N, N, N, I(DstMem16 | Mov, em_fnstcw),
}, {
	/* 0xC0 - 0xC7 */
	N, N, N, N, N, N, N, N,
	/* 0xC8 - 0xCF */
	N, N, N, N, N, N, N, N,
	/* 0xD0 - 0xC7 */
	N, N, N, N, N, N, N, N,
	/* 0xD8 - 0xDF */
	N, N, N, N, N, N, N, N,
	/* 0xE0 - 0xE7 */
	N, N, N, N, N, N, N, N,
	/* 0xE8 - 0xEF */
	N, N, N, N, N, N, N, N,
	/* 0xF0 - 0xF7 */
	N, N, N, N, N, N, N, N,
	/* 0xF8 - 0xFF */
	N, N, N, N, N, N, N, N,
} };