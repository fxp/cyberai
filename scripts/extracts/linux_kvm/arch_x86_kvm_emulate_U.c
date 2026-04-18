// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 21/33



	flags = X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF | X86_EFLAGS_ZF |
		X86_EFLAGS_SF;
	flags &= *reg_rmw(ctxt, VCPU_REGS_RAX) >> 8;

	ctxt->eflags &= ~0xffUL;
	ctxt->eflags |= flags | X86_EFLAGS_FIXED;
	return X86EMUL_CONTINUE;
}

static int em_lahf(struct x86_emulate_ctxt *ctxt)
{
	*reg_rmw(ctxt, VCPU_REGS_RAX) &= ~0xff00UL;
	*reg_rmw(ctxt, VCPU_REGS_RAX) |= (ctxt->eflags & 0xff) << 8;
	return X86EMUL_CONTINUE;
}

static int em_bswap(struct x86_emulate_ctxt *ctxt)
{
	switch (ctxt->op_bytes) {
#ifdef CONFIG_X86_64
	case 8:
		asm("bswap %0" : "+r"(ctxt->dst.val));
		break;
#endif
	default:
		asm("bswap %0" : "+r"(*(u32 *)&ctxt->dst.val));
		break;
	}
	return X86EMUL_CONTINUE;
}

static int em_clflush(struct x86_emulate_ctxt *ctxt)
{
	/* emulating clflush regardless of cpuid */
	return X86EMUL_CONTINUE;
}

static int em_clflushopt(struct x86_emulate_ctxt *ctxt)
{
	/* emulating clflushopt regardless of cpuid */
	return X86EMUL_CONTINUE;
}

static int em_movsxd(struct x86_emulate_ctxt *ctxt)
{
	ctxt->dst.val = (s32) ctxt->src.val;
	return X86EMUL_CONTINUE;
}

static int check_fxsr(struct x86_emulate_ctxt *ctxt)
{
	if (!ctxt->ops->guest_has_fxsr(ctxt))
		return emulate_ud(ctxt);

	if (ctxt->ops->get_cr(ctxt, 0) & (X86_CR0_TS | X86_CR0_EM))
		return emulate_nm(ctxt);

	/*
	 * Don't emulate a case that should never be hit, instead of working
	 * around a lack of fxsave64/fxrstor64 on old compilers.
	 */
	if (ctxt->mode >= X86EMUL_MODE_PROT64)
		return X86EMUL_UNHANDLEABLE;

	return X86EMUL_CONTINUE;
}

/*
 * Hardware doesn't save and restore XMM 0-7 without CR4.OSFXSR, but does save
 * and restore MXCSR.
 */
static size_t __fxstate_size(int nregs)
{
	return offsetof(struct fxregs_state, xmm_space[0]) + nregs * 16;
}

static inline size_t fxstate_size(struct x86_emulate_ctxt *ctxt)
{
	bool cr4_osfxsr;
	if (ctxt->mode == X86EMUL_MODE_PROT64)
		return __fxstate_size(16);

	cr4_osfxsr = ctxt->ops->get_cr(ctxt, 4) & X86_CR4_OSFXSR;
	return __fxstate_size(cr4_osfxsr ? 8 : 0);
}

/*
 * FXSAVE and FXRSTOR have 4 different formats depending on execution mode,
 *  1) 16 bit mode
 *  2) 32 bit mode
 *     - like (1), but FIP and FDP (foo) are only 16 bit.  At least Intel CPUs
 *       preserve whole 32 bit values, though, so (1) and (2) are the same wrt.
 *       save and restore
 *  3) 64-bit mode with REX.W prefix
 *     - like (2), but XMM 8-15 are being saved and restored
 *  4) 64-bit mode without REX.W prefix
 *     - like (3), but FIP and FDP are 64 bit
 *
 * Emulation uses (3) for (1) and (2) and preserves XMM 8-15 to reach the
 * desired result.  (4) is not emulated.
 *
 * Note: Guest and host CPUID.(EAX=07H,ECX=0H):EBX[bit 13] (deprecate FPU CS
 * and FPU DS) should match.
 */
static int em_fxsave(struct x86_emulate_ctxt *ctxt)
{
	struct fxregs_state fx_state = {};
	int rc;

	rc = check_fxsr(ctxt);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	kvm_fpu_get();

	rc = asm_safe("fxsave %[fx]", , [fx] "+m"(fx_state));

	kvm_fpu_put();

	if (rc != X86EMUL_CONTINUE)
		return rc;

	return segmented_write_std(ctxt, ctxt->memop.addr.mem, &fx_state,
		                   fxstate_size(ctxt));
}

/*
 * FXRSTOR might restore XMM registers not provided by the guest. Fill
 * in the host registers (via FXSAVE) instead, so they won't be modified.
 * (preemption has to stay disabled until FXRSTOR).
 *
 * Use noinline to keep the stack for other functions called by callers small.
 */
static noinline int fxregs_fixup(struct fxregs_state *fx_state,
				 const size_t used_size)
{
	struct fxregs_state fx_tmp = {};
	int rc;

	rc = asm_safe("fxsave %[fx]", , [fx] "+m"(fx_tmp));
	memcpy((void *)fx_state + used_size, (void *)&fx_tmp + used_size,
	       __fxstate_size(16) - used_size);

	return rc;
}

static int em_fxrstor(struct x86_emulate_ctxt *ctxt)
{
	struct fxregs_state fx_state;
	int rc;
	size_t size;

	rc = check_fxsr(ctxt);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	size = fxstate_size(ctxt);
	rc = segmented_read_std(ctxt, ctxt->memop.addr.mem, &fx_state, size);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	kvm_fpu_get();

	if (size < __fxstate_size(16)) {
		rc = fxregs_fixup(&fx_state, size);
		if (rc != X86EMUL_CONTINUE)
			goto out;
	}

	if (fx_state.mxcsr >> 16) {
		rc = emulate_gp(ctxt, 0);
		goto out;
	}

	if (rc == X86EMUL_CONTINUE)
		rc = asm_safe("fxrstor %[fx]", : [fx] "m"(fx_state));

out:
	kvm_fpu_put();

	return rc;
}

static int em_xsetbv(struct x86_emulate_ctxt *ctxt)
{
	u32 eax, ecx, edx;

	if (!(ctxt->ops->get_cr(ctxt, 4) & X86_CR4_OSXSAVE))
		return emulate_ud(ctxt);

	eax = reg_read(ctxt, VCPU_REGS_RAX);
	edx = reg_read(ctxt, VCPU_REGS_RDX);
	ecx = reg_read(ctxt, VCPU_REGS_RCX);

	if (ctxt->ops->set_xcr(ctxt, ecx, ((u64)edx << 32) | eax))
		return emulate_gp(ctxt, 0);

	return X86EMUL_CONTINUE;
}

static bool valid_cr(int nr)
{
	switch (nr) {
	case 0:
	case 2 ... 4:
	case 8:
		return true;
	default:
		return false;
	}
}