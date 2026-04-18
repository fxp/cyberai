// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 2/33



/* Operand sizes: 8-bit operands or specified/overridden size. */
#define ByteOp      (1<<0)      /* 8-bit operands. */
#define DstShift    1           /* Destination operand type at bits 1-5 */
#define ImplicitOps (OpImplicit << DstShift)
#define DstReg      (OpReg << DstShift)
#define DstMem      (OpMem << DstShift)
#define DstAcc      (OpAcc << DstShift)
#define DstDI       (OpDI << DstShift)
#define DstMem64    (OpMem64 << DstShift)
#define DstMem16    (OpMem16 << DstShift)
#define DstImmUByte (OpImmUByte << DstShift)
#define DstDX       (OpDX << DstShift)
#define DstAccLo    (OpAccLo << DstShift)
#define DstMask     (OpMask << DstShift)
#define SrcShift    6           /* Source operand type at bits 6-10 */
#define SrcNone     (OpNone << SrcShift)
#define SrcReg      (OpReg << SrcShift)
#define SrcMem      (OpMem << SrcShift)
#define SrcMem16    (OpMem16 << SrcShift)
#define SrcMem32    (OpMem32 << SrcShift)
#define SrcImm      (OpImm << SrcShift)
#define SrcImmByte  (OpImmByte << SrcShift)
#define SrcOne      (OpOne << SrcShift)
#define SrcImmUByte (OpImmUByte << SrcShift)
#define SrcImmU     (OpImmU << SrcShift)
#define SrcSI       (OpSI << SrcShift)
#define SrcXLat     (OpXLat << SrcShift)
#define SrcImmFAddr (OpImmFAddr << SrcShift)
#define SrcMemFAddr (OpMemFAddr << SrcShift)
#define SrcAcc      (OpAcc << SrcShift)
#define SrcImmU16   (OpImmU16 << SrcShift)
#define SrcImm64    (OpImm64 << SrcShift)
#define SrcDX       (OpDX << SrcShift)
#define SrcMem8     (OpMem8 << SrcShift)
#define SrcAccHi    (OpAccHi << SrcShift)
#define SrcMask     (OpMask << SrcShift)
#define BitOp       (1<<11)
#define MemAbs      (1<<12)     /* Memory operand is absolute displacement */
#define String      (1<<13)     /* String instruction (rep capable) */
#define Stack       (1<<14)     /* Stack instruction (push/pop) */
#define GroupMask   (7<<15)     /* Group mechanisms, at bits 15-17 */
#define Group       (1<<15)     /* Bits 3:5 of modrm byte extend opcode */
#define GroupDual   (2<<15)     /* Alternate decoding of mod == 3 */
#define Prefix      (3<<15)     /* Instruction varies with 66/f2/f3 prefix */
#define RMExt       (4<<15)     /* Opcode extension in ModRM r/m if mod == 3 */
#define Escape      (5<<15)     /* Escape to coprocessor instruction */
#define InstrDual   (6<<15)     /* Alternate instruction decoding of mod == 3 */
#define ModeDual    (7<<15)     /* Different instruction for 32/64 bit */
#define Sse         (1<<18)     /* SSE Vector instruction */
#define ModRM       (1<<19)     /* Generic ModRM decode. */
#define Mov         (1<<20)     /* Destination is only written; never read. */
#define Prot        (1<<21) /* instruction generates #UD if not in prot-mode */
#define EmulateOnUD (1<<22) /* Emulate if unsupported by the host */
#define NoAccess    (1<<23) /* Don't access memory (lea/invlpg/verr etc) */
#define Op3264      (1<<24) /* Operand is 64b in long mode, 32b otherwise */
#define Undefined   (1<<25) /* No Such Instruction */
#define Lock        (1<<26) /* lock prefix is allowed for the instruction */
#define Priv        (1<<27) /* instruction generates #GP if current CPL != 0 */
#define No64        (1<<28)     /* Instruction generates #UD in 64-bit mode */
#define PageTable   (1 << 29)   /* instruction used to write page table */
#define NotImpl     (1 << 30)   /* instruction is not implemented */
#define Avx         ((u64)1 << 31)   /* Instruction uses VEX prefix */
#define Src2Shift   (32)        /* Source 2 operand type at bits 32-36 */
#define Src2None    (OpNone << Src2Shift)
#define Src2Mem     (OpMem << Src2Shift)
#define Src2CL      (OpCL << Src2Shift)
#define Src2ImmByte (OpImmByte << Src2Shift)
#define Src2One     (OpOne << Src2Shift)
#define Src2Imm     (OpImm << Src2Shift)
#define Src2ES      (OpES << Src2Shift)
#define Src2CS      (OpCS << Src2Shift)
#define Src2SS      (OpSS << Src2Shift)
#define Src2DS      (OpDS << Src2Shift)
#define Src2FS      (OpFS << Src2Shift)
#define Src2GS      (OpGS << Src2Shift)
#define Src2Mask    (OpMask << Src2Shift)
/* free: 37-39 */
#define Mmx         ((u64)1 << 40)  /* MMX Vector instruction */
#define AlignMask   ((u64)3 << 41)  /* Memory alignment requirement at bits 41-42 */
#define Aligned     ((u64)1 << 41)  /* Explicitly aligned (e.g. MOVDQA) */
#define Unaligned   ((u64)2 << 41)  /* Explicitly unaligned (e.g. MOVDQU) */
#define Aligned16   ((u64)3 << 41)  /* Aligned to 16 byte boundary (e.g. FXSAVE) */
/* free: 43-44 */
#define NoWrite     ((u64)1 << 45)  /* No writeback */
#define SrcWrite    ((u64)1 << 46)  /* Write back src operand */
#define NoMod	    ((u64)1 << 47)  /* Mod field is ignored */
#define Intercept   ((u64)1 << 48)  /* Has valid intercept field */
#define CheckPerm   ((u64)1 << 49)  /* Has valid check_perm field */
#define PrivUD      ((u64)1 << 51)  /* #UD instead of #GP on CPL > 0 */
#define NearBranch  ((u64)1 << 52)  /* Near branches */
#define No16	    ((u64)1 << 53)  /* No 16 bit operand */
#define IncSP       ((u