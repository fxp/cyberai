// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 14/33



	set_desc_base(ss, 0);	/* flat segment */
	set_desc_limit(ss, 0xfffff);	/* 4GB limit */
	ss->g = 1;		/* 4kb granularity */
	ss->s = 1;
	ss->type = 0x03;	/* Read/Write, Accessed */
	ss->d = 1;		/* 32bit stack segment */
	ss->dpl = 0;
	ss->p = 1;
	ss->l = 0;
	ss->avl = 0;
}

static int em_syscall(struct x86_emulate_ctxt *ctxt)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	struct desc_struct cs, ss;
	u64 msr_data;
	u16 cs_sel, ss_sel;
	u64 efer = 0;

	/* syscall is not available in real mode */
	if (ctxt->mode == X86EMUL_MODE_REAL ||
	    ctxt->mode == X86EMUL_MODE_VM86)
		return emulate_ud(ctxt);

	/*
	 * Intel compatible CPUs only support SYSCALL in 64-bit mode, whereas
	 * AMD allows SYSCALL in any flavor of protected mode.  Note, it's
	 * infeasible to emulate Intel behavior when running on AMD hardware,
	 * as SYSCALL won't fault in the "wrong" mode, i.e. there is no #UD
	 * for KVM to trap-and-emulate, unlike emulating AMD on Intel.
	 */
	if (ctxt->mode != X86EMUL_MODE_PROT64 &&
	    ctxt->ops->guest_cpuid_is_intel_compatible(ctxt))
		return emulate_ud(ctxt);

	ops->get_msr(ctxt, MSR_EFER, &efer);
	if (!(efer & EFER_SCE))
		return emulate_ud(ctxt);

	setup_syscalls_segments(&cs, &ss);
	ops->get_msr(ctxt, MSR_STAR, &msr_data);
	msr_data >>= 32;
	cs_sel = (u16)(msr_data & 0xfffc);
	ss_sel = (u16)(msr_data + 8);

	if (efer & EFER_LMA) {
		cs.d = 0;
		cs.l = 1;
	}
	ops->set_segment(ctxt, cs_sel, &cs, 0, VCPU_SREG_CS);
	ops->set_segment(ctxt, ss_sel, &ss, 0, VCPU_SREG_SS);

	*reg_write(ctxt, VCPU_REGS_RCX) = ctxt->_eip;
	if (efer & EFER_LMA) {
#ifdef CONFIG_X86_64
		*reg_write(ctxt, VCPU_REGS_R11) = ctxt->eflags;

		ops->get_msr(ctxt,
			     ctxt->mode == X86EMUL_MODE_PROT64 ?
			     MSR_LSTAR : MSR_CSTAR, &msr_data);
		ctxt->_eip = msr_data;

		ops->get_msr(ctxt, MSR_SYSCALL_MASK, &msr_data);
		ctxt->eflags &= ~msr_data;
		ctxt->eflags |= X86_EFLAGS_FIXED;
#endif
	} else {
		/* legacy mode */
		ops->get_msr(ctxt, MSR_STAR, &msr_data);
		ctxt->_eip = (u32)msr_data;

		ctxt->eflags &= ~(X86_EFLAGS_VM | X86_EFLAGS_IF);
	}

	ctxt->tf = (ctxt->eflags & X86_EFLAGS_TF) != 0;
	return X86EMUL_CONTINUE;
}

static int em_sysenter(struct x86_emulate_ctxt *ctxt)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	struct desc_struct cs, ss;
	u64 msr_data;
	u16 cs_sel, ss_sel;
	u64 efer = 0;

	ops->get_msr(ctxt, MSR_EFER, &efer);
	/* inject #GP if in real mode */
	if (ctxt->mode == X86EMUL_MODE_REAL)
		return emulate_gp(ctxt, 0);

	/*
	 * Intel's architecture allows SYSENTER in compatibility mode, but AMD
	 * does not.  Note, AMD does allow SYSENTER in legacy protected mode.
	 */
	if ((ctxt->mode != X86EMUL_MODE_PROT64) && (efer & EFER_LMA) &&
	    !ctxt->ops->guest_cpuid_is_intel_compatible(ctxt))
		return emulate_ud(ctxt);

	/* sysenter/sysexit have not been tested in 64bit mode. */
	if (ctxt->mode == X86EMUL_MODE_PROT64)
		return X86EMUL_UNHANDLEABLE;

	ops->get_msr(ctxt, MSR_IA32_SYSENTER_CS, &msr_data);
	if ((msr_data & 0xfffc) == 0x0)
		return emulate_gp(ctxt, 0);

	setup_syscalls_segments(&cs, &ss);
	ctxt->eflags &= ~(X86_EFLAGS_VM | X86_EFLAGS_IF);
	cs_sel = (u16)msr_data & ~SEGMENT_RPL_MASK;
	ss_sel = cs_sel + 8;
	if (efer & EFER_LMA) {
		cs.d = 0;
		cs.l = 1;
	}

	ops->set_segment(ctxt, cs_sel, &cs, 0, VCPU_SREG_CS);
	ops->set_segment(ctxt, ss_sel, &ss, 0, VCPU_SREG_SS);

	ops->get_msr(ctxt, MSR_IA32_SYSENTER_EIP, &msr_data);
	ctxt->_eip = (efer & EFER_LMA) ? msr_data : (u32)msr_data;

	ops->get_msr(ctxt, MSR_IA32_SYSENTER_ESP, &msr_data);
	*reg_write(ctxt, VCPU_REGS_RSP) = (efer & EFER_LMA) ? msr_data :
							      (u32)msr_data;
	if (efer & EFER_LMA)
		ctxt->mode = X86EMUL_MODE_PROT64;

	return X86EMUL_CONTINUE;
}

static int em_sysexit(struct x86_emulate_ctxt *ctxt)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	struct desc_struct cs, ss;
	u64 msr_data, rcx, rdx;
	int usermode;
	u16 cs_sel = 0, ss_sel = 0;

	/* inject #GP if in real mode or Virtual 8086 mode */
	if (ctxt->mode == X86EMUL_MODE_REAL ||
	    ctxt->mode == X86EMUL_MODE_VM86)
		return emulate_gp(ctxt, 0);

	setup_syscalls_segments(&cs, &ss);

	if (ctxt->rex_bits & REX_W)
		usermode = X86EMUL_MODE_PROT64;
	else
		usermode = X86EMUL_MODE_PROT32;

	rcx = reg_read(ctxt, VCPU_REGS_RCX);
	rdx = reg_read(ctxt, VCPU_REGS_RDX);

	cs.dpl = 3;
	ss.dpl = 3;
	ops->get_msr(ctxt, MSR_IA32_SYSENTER_CS, &msr_data);
	switch (usermode) {
	case X86EMUL_MODE_PROT32:
		cs_sel = (u16)(msr_data + 16);
		if ((msr_data & 0xfffc) == 0x0)
			return emulate_gp(ctxt, 0);
		ss_sel = (u16)(msr_data + 24);
		rcx = (u32)rcx;
		rdx = (u32)rdx;
		break;
	case X86EMUL_MODE_PROT64:
		cs_sel = (u16)(msr_data + 32);
		if (msr_data == 0x0)
			return emulate_gp(ctxt, 0);
		ss_sel = cs_sel + 8;
		cs.d = 0;
		cs.l = 1;
		if (emul_is_noncanonical_address(rcx, ctxt, 0) ||
		    emul_is_noncanonical_address(rdx, ctxt, 0))
			return emulate_gp(ctxt, 0);
		break;
	}
	cs_sel |= SEGMENT_RPL_MASK;
	ss_sel |= SEGMENT_RPL_MASK;