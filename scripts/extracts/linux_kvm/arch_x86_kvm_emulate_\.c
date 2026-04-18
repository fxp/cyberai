// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 28/33



	switch (d) {
	case OpReg:
		decode_register_operand(ctxt, op);
		break;
	case OpImmUByte:
		rc = decode_imm(ctxt, op, 1, false);
		break;
	case OpMem:
		ctxt->memop.bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
	mem_common:
		*op = ctxt->memop;
		ctxt->memopp = op;
		if (ctxt->d & BitOp)
			fetch_bit_operand(ctxt);
		op->orig_val = op->val;
		break;
	case OpMem64:
		ctxt->memop.bytes = (ctxt->op_bytes == 8) ? 16 : 8;
		goto mem_common;
	case OpAcc:
		op->type = OP_REG;
		op->bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
		op->addr.reg = reg_rmw(ctxt, VCPU_REGS_RAX);
		fetch_register_operand(op);
		break;
	case OpAccLo:
		op->type = OP_REG;
		op->bytes = (ctxt->d & ByteOp) ? 2 : ctxt->op_bytes;
		op->addr.reg = reg_rmw(ctxt, VCPU_REGS_RAX);
		fetch_register_operand(op);
		break;
	case OpAccHi:
		if (ctxt->d & ByteOp) {
			op->type = OP_NONE;
			break;
		}
		op->type = OP_REG;
		op->bytes = ctxt->op_bytes;
		op->addr.reg = reg_rmw(ctxt, VCPU_REGS_RDX);
		fetch_register_operand(op);
		break;
	case OpDI:
		op->type = OP_MEM;
		op->bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
		op->addr.mem.ea =
			register_address(ctxt, VCPU_REGS_RDI);
		op->addr.mem.seg = VCPU_SREG_ES;
		op->val = 0;
		op->count = 1;
		break;
	case OpDX:
		op->type = OP_REG;
		op->bytes = 2;
		op->addr.reg = reg_rmw(ctxt, VCPU_REGS_RDX);
		fetch_register_operand(op);
		break;
	case OpCL:
		op->type = OP_IMM;
		op->bytes = 1;
		op->val = reg_read(ctxt, VCPU_REGS_RCX) & 0xff;
		break;
	case OpImmByte:
		rc = decode_imm(ctxt, op, 1, true);
		break;
	case OpOne:
		op->type = OP_IMM;
		op->bytes = 1;
		op->val = 1;
		break;
	case OpImm:
		rc = decode_imm(ctxt, op, imm_size(ctxt), true);
		break;
	case OpImm64:
		rc = decode_imm(ctxt, op, ctxt->op_bytes, true);
		break;
	case OpMem8:
		ctxt->memop.bytes = 1;
		if (ctxt->memop.type == OP_REG) {
			ctxt->memop.addr.reg = decode_register(ctxt,
					ctxt->modrm_rm, true);
			fetch_register_operand(&ctxt->memop);
		}
		goto mem_common;
	case OpMem16:
		ctxt->memop.bytes = 2;
		goto mem_common;
	case OpMem32:
		ctxt->memop.bytes = 4;
		goto mem_common;
	case OpImmU16:
		rc = decode_imm(ctxt, op, 2, false);
		break;
	case OpImmU:
		rc = decode_imm(ctxt, op, imm_size(ctxt), false);
		break;
	case OpSI:
		op->type = OP_MEM;
		op->bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
		op->addr.mem.ea =
			register_address(ctxt, VCPU_REGS_RSI);
		op->addr.mem.seg = ctxt->seg_override;
		op->val = 0;
		op->count = 1;
		break;
	case OpXLat:
		op->type = OP_MEM;
		op->bytes = (ctxt->d & ByteOp) ? 1 : ctxt->op_bytes;
		op->addr.mem.ea =
			address_mask(ctxt,
				reg_read(ctxt, VCPU_REGS_RBX) +
				(reg_read(ctxt, VCPU_REGS_RAX) & 0xff));
		op->addr.mem.seg = ctxt->seg_override;
		op->val = 0;
		break;
	case OpImmFAddr:
		op->type = OP_IMM;
		op->addr.mem.ea = ctxt->_eip;
		op->bytes = ctxt->op_bytes + 2;
		insn_fetch_arr(op->valptr, op->bytes, ctxt);
		break;
	case OpMemFAddr:
		ctxt->memop.bytes = ctxt->op_bytes + 2;
		goto mem_common;
	case OpES:
		op->type = OP_IMM;
		op->val = VCPU_SREG_ES;
		break;
	case OpCS:
		op->type = OP_IMM;
		op->val = VCPU_SREG_CS;
		break;
	case OpSS:
		op->type = OP_IMM;
		op->val = VCPU_SREG_SS;
		break;
	case OpDS:
		op->type = OP_IMM;
		op->val = VCPU_SREG_DS;
		break;
	case OpFS:
		op->type = OP_IMM;
		op->val = VCPU_SREG_FS;
		break;
	case OpGS:
		op->type = OP_IMM;
		op->val = VCPU_SREG_GS;
		break;
	case OpImplicit:
		/* Special instructions do their own operand decoding. */
	default:
		op->type = OP_NONE; /* Disable writeback. */
		break;
	}

done:
	return rc;
}

static int x86_decode_avx(struct x86_emulate_ctxt *ctxt,
			  u8 vex_1st, u8 vex_2nd, struct opcode *opcode)
{
	u8 vex_3rd, map, pp, l, v;
	int rc = X86EMUL_CONTINUE;

	if (ctxt->rep_prefix || ctxt->op_prefix || ctxt->rex_prefix)
		goto ud;

	if (vex_1st == 0xc5) {
		/* Expand RVVVVlpp to VEX3 format */
		vex_3rd = vex_2nd & ~0x80;         /* VVVVlpp from VEX2, w=0 */
		vex_2nd = (vex_2nd & 0x80) | 0x61; /* R from VEX2, X=1 B=1 mmmmm=00001 */
	} else {
		vex_3rd = insn_fetch(u8, ctxt);
	}

	/* vex_2nd = RXBmmmmm, vex_3rd = wVVVVlpp.  Fix polarity */
	vex_2nd ^= 0xE0; /* binary 11100000 */
	vex_3rd ^= 0x78; /* binary 01111000 */

	ctxt->rex_prefix = REX_PREFIX;
	ctxt->rex_bits = (vex_2nd & 0xE0) >> 5; /* RXB */
	ctxt->rex_bits |= (vex_3rd & 0x80) >> 4; /* w */
	if (ctxt->rex_bits && ctxt->mode != X86EMUL_MODE_PROT64)
		goto ud;

	map = vex_2nd & 0x1f;
	v = (vex_3rd >> 3) & 0xf;
	l = vex_3rd & 0x4;
	pp = vex_3rd & 0x3;

	ctxt->b = insn_fetch(u8, ctxt);
	switch (map) {
	case 1:
		ctxt->opcode_len = 2;
		*opcode = twobyte_table[ctxt->b];
		break;
	case 2:
		ctxt->opcode_len = 3;
		*opcode = opcode_map_0f_38[ctxt->b];
		break;
	case 3:
		/* no 0f 3a instructions are supported yet */
		return X86EMUL_UNHANDLEABLE;
	default:
		goto ud;
	}

	/*
	 * No three operand instructions are supported yet; those that
	 * *are* marked with the Avx flag reserve the VVVV flag.
	 */
	if (v)
		goto ud;