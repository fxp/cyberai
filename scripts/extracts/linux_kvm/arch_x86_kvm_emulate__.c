// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 31/33



	/*
	 * Decode and fetch the second source operand: register, memory
	 * or immediate.
	 */
	rc = decode_operand(ctxt, &ctxt->src2, (ctxt->d >> Src2Shift) & OpMask);
	if (rc != X86EMUL_CONTINUE)
		goto done;

	/* Decode and fetch the destination operand: register or memory. */
	rc = decode_operand(ctxt, &ctxt->dst, (ctxt->d >> DstShift) & OpMask);

	if (ctxt->rip_relative && likely(ctxt->memopp))
		ctxt->memopp->addr.mem.ea = address_mask(ctxt,
					ctxt->memopp->addr.mem.ea + ctxt->_eip);

done:
	if (rc == X86EMUL_PROPAGATE_FAULT)
		ctxt->have_exception = true;
	return (rc != X86EMUL_CONTINUE) ? EMULATION_FAILED : EMULATION_OK;
}

bool x86_page_table_writing_insn(struct x86_emulate_ctxt *ctxt)
{
	return ctxt->d & PageTable;
}

static bool string_insn_completed(struct x86_emulate_ctxt *ctxt)
{
	/* The second termination condition only applies for REPE
	 * and REPNE. Test if the repeat string operation prefix is
	 * REPE/REPZ or REPNE/REPNZ and if it's the case it tests the
	 * corresponding termination condition according to:
	 * 	- if REPE/REPZ and ZF = 0 then done
	 * 	- if REPNE/REPNZ and ZF = 1 then done
	 */
	if (((ctxt->b == 0xa6) || (ctxt->b == 0xa7) ||
	     (ctxt->b == 0xae) || (ctxt->b == 0xaf))
	    && (((ctxt->rep_prefix == REPE_PREFIX) &&
		 ((ctxt->eflags & X86_EFLAGS_ZF) == 0))
		|| ((ctxt->rep_prefix == REPNE_PREFIX) &&
		    ((ctxt->eflags & X86_EFLAGS_ZF) == X86_EFLAGS_ZF))))
		return true;

	return false;
}

static int flush_pending_x87_faults(struct x86_emulate_ctxt *ctxt)
{
	int rc;

	kvm_fpu_get();
	rc = asm_safe("fwait");
	kvm_fpu_put();

	if (unlikely(rc != X86EMUL_CONTINUE))
		return emulate_exception(ctxt, MF_VECTOR, 0, false);

	return X86EMUL_CONTINUE;
}

static void fetch_possible_mmx_operand(struct operand *op)
{
	if (op->type == OP_MM)
		kvm_read_mmx_reg(op->addr.mm, &op->mm_val);
}

void init_decode_cache(struct x86_emulate_ctxt *ctxt)
{
	/* Clear fields that are set conditionally but read without a guard. */
	ctxt->rip_relative = false;
	ctxt->rex_prefix = REX_NONE;
	ctxt->rex_bits = 0;
	ctxt->lock_prefix = 0;
	ctxt->op_prefix = false;
	ctxt->rep_prefix = 0;
	ctxt->regs_valid = 0;
	ctxt->regs_dirty = 0;

	ctxt->io_read.pos = 0;
	ctxt->io_read.end = 0;
	ctxt->mem_read.end = 0;
}

int x86_emulate_insn(struct x86_emulate_ctxt *ctxt, bool check_intercepts)
{
	const struct x86_emulate_ops *ops = ctxt->ops;
	int rc = X86EMUL_CONTINUE;
	int saved_dst_type = ctxt->dst.type;

	ctxt->mem_read.pos = 0;

	/* LOCK prefix is allowed only with some instructions */
	if (ctxt->lock_prefix && (!(ctxt->d & Lock) || ctxt->dst.type != OP_MEM)) {
		rc = emulate_ud(ctxt);
		goto done;
	}

	if ((ctxt->d & SrcMask) == SrcMemFAddr && ctxt->src.type != OP_MEM) {
		rc = emulate_ud(ctxt);
		goto done;
	}

	if (unlikely(ctxt->d &
		     (No64|Undefined|Avx|Sse|Mmx|Intercept|CheckPerm|Priv|Prot|String))) {
		if ((ctxt->mode == X86EMUL_MODE_PROT64 && (ctxt->d & No64)) ||
				(ctxt->d & Undefined)) {
			rc = emulate_ud(ctxt);
			goto done;
		}

		if ((ctxt->d & (Avx|Sse|Mmx)) && ((ops->get_cr(ctxt, 0) & X86_CR0_EM))) {
			rc = emulate_ud(ctxt);
			goto done;
		}

		if (ctxt->d & Avx) {
			u64 xcr = 0;
			if (!(ops->get_cr(ctxt, 4) & X86_CR4_OSXSAVE)
			    || ops->get_xcr(ctxt, 0, &xcr)
			    || !(xcr & XFEATURE_MASK_YMM)) {
				rc = emulate_ud(ctxt);
				goto done;
			}
		} else if (ctxt->d & Sse) {
			if (!(ops->get_cr(ctxt, 4) & X86_CR4_OSFXSR)) {
				rc = emulate_ud(ctxt);
				goto done;
			}
		}

		if ((ctxt->d & (Avx|Sse|Mmx)) && (ops->get_cr(ctxt, 0) & X86_CR0_TS)) {
			rc = emulate_nm(ctxt);
			goto done;
		}

		if (ctxt->d & Mmx) {
			rc = flush_pending_x87_faults(ctxt);
			if (rc != X86EMUL_CONTINUE)
				goto done;
			/*
			 * Now that we know the fpu is exception safe, we can fetch
			 * operands from it.
			 */
			fetch_possible_mmx_operand(&ctxt->src);
			fetch_possible_mmx_operand(&ctxt->src2);
			if (!(ctxt->d & Mov))
				fetch_possible_mmx_operand(&ctxt->dst);
		}

		if (unlikely(check_intercepts) && ctxt->intercept) {
			rc = emulator_check_intercept(ctxt, ctxt->intercept,
						      X86_ICPT_PRE_EXCEPT);
			if (rc != X86EMUL_CONTINUE)
				goto done;
		}

		/* Instruction can only be executed in protected mode */
		if ((ctxt->d & Prot) && ctxt->mode < X86EMUL_MODE_PROT16) {
			rc = emulate_ud(ctxt);
			goto done;
		}

		/* Privileged instruction can be executed only in CPL=0 */
		if ((ctxt->d & Priv) && ops->cpl(ctxt)) {
			if (ctxt->d & PrivUD)
				rc = emulate_ud(ctxt);
			else
				rc = emulate_gp(ctxt, 0);
			goto done;
		}

		/* Do instruction specific permission checks */
		if (ctxt->d & CheckPerm) {
			rc = ctxt->check_perm(ctxt);
			if (rc != X86EMUL_CONTINUE)
				goto done;
		}

		if (unlikely(check_intercepts) && (ctxt->d & Intercept)) {
			rc = emulator_check_intercept(ctxt, ctxt->intercept,
						      X86_ICPT_POST_EXCEPT);
			if (rc != X86EMUL_CONTINUE)
				goto done;
		}