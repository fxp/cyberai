// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 26/33

mplicitOps, em_sti),
	D(ImplicitOps), D(ImplicitOps), G(0, group4), G(0, group5),
};

static const struct opcode twobyte_table[256] = {
	/* 0x00 - 0x0F */
	G(0, group6), GD(0, &group7), N, N,
	N, I(ImplicitOps | EmulateOnUD | IsBranch | ShadowStack, em_syscall),
	II(ImplicitOps | Priv, em_clts, clts), N,
	DI(ImplicitOps | Priv, invd), DI(ImplicitOps | Priv, wbinvd), N, N,
	N, D(ImplicitOps | ModRM | SrcMem | NoAccess), N, N,
	/* 0x10 - 0x1F */
	GP(ModRM | DstReg | SrcMem | Mov | Sse | Avx, &pfx_0f_10_0f_11),
	GP(ModRM | DstMem | SrcReg | Mov | Sse | Avx, &pfx_0f_10_0f_11),
	N, N, N, N, N, N,
	D(ImplicitOps | ModRM | SrcMem | NoAccess), /* 4 * prefetch + 4 * reserved NOP */
	D(ImplicitOps | ModRM | SrcMem | NoAccess), N, N,
	D(ImplicitOps | ModRM | SrcMem | NoAccess), /* 8 * reserved NOP */
	D(ImplicitOps | ModRM | SrcMem | NoAccess), /* 8 * reserved NOP */
	D(ImplicitOps | ModRM | SrcMem | NoAccess), /* 8 * reserved NOP */
	D(ImplicitOps | ModRM | SrcMem | NoAccess), /* NOP + 7 * reserved NOP */
	/* 0x20 - 0x2F */
	DIP(ModRM | DstMem | Priv | Op3264 | NoMod, cr_read, check_cr_access),
	DIP(ModRM | DstMem | Priv | Op3264 | NoMod, dr_read, check_dr_read),
	IIP(ModRM | SrcMem | Priv | Op3264 | NoMod, em_cr_write, cr_write,
						check_cr_access),
	IIP(ModRM | SrcMem | Priv | Op3264 | NoMod, em_dr_write, dr_write,
						check_dr_write),
	N, N, N, N,
	GP(ModRM | DstReg | SrcMem | Mov | Sse | Avx, &pfx_0f_28_0f_29),
	GP(ModRM | DstMem | SrcReg | Mov | Sse | Avx, &pfx_0f_28_0f_29),
	N, GP(ModRM | DstMem | SrcReg | Mov | Sse | Avx, &pfx_0f_2b),
	N, N, N, N,
	/* 0x30 - 0x3F */
	II(ImplicitOps | Priv, em_wrmsr, wrmsr),
	IIP(ImplicitOps, em_rdtsc, rdtsc, check_rdtsc),
	II(ImplicitOps | Priv, em_rdmsr, rdmsr),
	IIP(ImplicitOps, em_rdpmc, rdpmc, check_rdpmc),
	I(ImplicitOps | EmulateOnUD | IsBranch | ShadowStack, em_sysenter),
	I(ImplicitOps | Priv | EmulateOnUD | IsBranch | ShadowStack, em_sysexit),
	N, N,
	N, N, N, N, N, N, N, N,
	/* 0x40 - 0x4F */
	X16(D(DstReg | SrcMem | ModRM)),
	/* 0x50 - 0x5F */
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	/* 0x60 - 0x6F */
	N, N, N, N,
	N, N, N, N,
	N, N, N, N,
	N, N, N, GP(SrcMem | DstReg | ModRM | Mov, &pfx_0f_6f_0f_7f),
	/* 0x70 - 0x7F */
	N, N, N, N,
	N, N, N, N,
	N, N, N, N,
	N, N, N, GP(SrcReg | DstMem | ModRM | Mov, &pfx_0f_6f_0f_7f),
	/* 0x80 - 0x8F */
	X16(D(SrcImm | NearBranch | IsBranch)),
	/* 0x90 - 0x9F */
	X16(D(ByteOp | DstMem | SrcNone | ModRM| Mov)),
	/* 0xA0 - 0xA7 */
	I(Stack | Src2FS, em_push_sreg), I(Stack | Src2FS, em_pop_sreg),
	II(ImplicitOps, em_cpuid, cpuid),
	I(DstMem | SrcReg | ModRM | BitOp | NoWrite, em_bt),
	I(DstMem | SrcReg | Src2ImmByte | ModRM, em_shld),
	I(DstMem | SrcReg | Src2CL | ModRM, em_shld), N, N,
	/* 0xA8 - 0xAF */
	I(Stack | Src2GS, em_push_sreg), I(Stack | Src2GS, em_pop_sreg),
	II(EmulateOnUD | ImplicitOps, em_rsm, rsm),
	I(DstMem | SrcReg | ModRM | BitOp | Lock | PageTable, em_bts),
	I(DstMem | SrcReg | Src2ImmByte | ModRM, em_shrd),
	I(DstMem | SrcReg | Src2CL | ModRM, em_shrd),
	GD(0, &group15), I(DstReg | SrcMem | ModRM, em_imul),
	/* 0xB0 - 0xB7 */
	I2bv(DstMem | SrcReg | ModRM | Lock | PageTable | SrcWrite, em_cmpxchg),
	I(DstReg | SrcMemFAddr | ModRM | Src2SS, em_lseg),
	I(DstMem | SrcReg | ModRM | BitOp | Lock, em_btr),
	I(DstReg | SrcMemFAddr | ModRM | Src2FS, em_lseg),
	I(DstReg | SrcMemFAddr | ModRM | Src2GS, em_lseg),
	D(DstReg | SrcMem8 | ModRM | Mov), D(DstReg | SrcMem16 | ModRM | Mov),
	/* 0xB8 - 0xBF */
	N, N,
	G(BitOp, group8),
	I(DstMem | SrcReg | ModRM | BitOp | Lock | PageTable, em_btc),
	I(DstReg | SrcMem | ModRM, em_bsf_c),
	I(DstReg | SrcMem | ModRM, em_bsr_c),
	D(DstReg | SrcMem8 | ModRM | Mov), D(DstReg | SrcMem16 | ModRM | Mov),
	/* 0xC0 - 0xC7 */
	I2bv(DstMem | SrcReg | ModRM | SrcWrite | Lock, em_xadd),
	N, ID(0, &instr_dual_0f_c3),
	N, N, N, GD(0, &group9),
	/* 0xC8 - 0xCF */
	X8(I(DstReg, em_bswap)),
	/* 0xD0 - 0xDF */
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	/* 0xE0 - 0xEF */
	N, N, N, N, N, N, N, GP(SrcReg | DstMem | ModRM | Mov, &pfx_0f_e7_0f_38_2a),
	N, N, N, N, N, N, N, N,
	/* 0xF0 - 0xFF */
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
};

static const struct instr_dual instr_dual_0f_38_f0 = {
	I(DstReg | SrcMem | Mov, em_movbe), N
};

static const struct instr_dual instr_dual_0f_38_f1 = {
	I(DstMem | SrcReg | Mov, em_movbe), N
};

static const struct gprefix three_byte_0f_38_f0 = {
	ID(0, &instr_dual_0f_38_f0), ID(0, &instr_dual_0f_38_f0), N, N
};

static const struct gprefix three_byte_0f_38_f1 = {
	ID(0, &instr_dual_0f_38_f1), ID(0, &instr_dual_0f_38_f1), N, N
};