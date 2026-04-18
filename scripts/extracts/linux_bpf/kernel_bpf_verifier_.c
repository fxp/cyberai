// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 91/132



	if (ptr_reg->type & PTR_MAYBE_NULL) {
		verbose(env, "R%d pointer arithmetic on %s prohibited, null-check it first\n",
			dst, reg_type_str(env, ptr_reg->type));
		return -EACCES;
	}

	/*
	 * Accesses to untrusted PTR_TO_MEM are done through probe
	 * instructions, hence no need to track offsets.
	 */
	if (base_type(ptr_reg->type) == PTR_TO_MEM && (ptr_reg->type & PTR_UNTRUSTED))
		return 0;

	switch (base_type(ptr_reg->type)) {
	case PTR_TO_CTX:
	case PTR_TO_MAP_VALUE:
	case PTR_TO_MAP_KEY:
	case PTR_TO_STACK:
	case PTR_TO_PACKET_META:
	case PTR_TO_PACKET:
	case PTR_TO_TP_BUFFER:
	case PTR_TO_BTF_ID:
	case PTR_TO_MEM:
	case PTR_TO_BUF:
	case PTR_TO_FUNC:
	case CONST_PTR_TO_DYNPTR:
		break;
	case PTR_TO_FLOW_KEYS:
		if (known)
			break;
		fallthrough;
	case CONST_PTR_TO_MAP:
		/* smin_val represents the known value */
		if (known && smin_val == 0 && opcode == BPF_ADD)
			break;
		fallthrough;
	default:
		verbose(env, "R%d pointer arithmetic on %s prohibited\n",
			dst, reg_type_str(env, ptr_reg->type));
		return -EACCES;
	}

	/* In case of 'scalar += pointer', dst_reg inherits pointer type and id.
	 * The id may be overwritten later if we create a new variable offset.
	 */
	dst_reg->type = ptr_reg->type;
	dst_reg->id = ptr_reg->id;

	if (!check_reg_sane_offset_scalar(env, off_reg, ptr_reg->type) ||
	    !check_reg_sane_offset_ptr(env, ptr_reg, ptr_reg->type))
		return -EINVAL;

	/* pointer types do not carry 32-bit bounds at the moment. */
	__mark_reg32_unbounded(dst_reg);

	if (sanitize_needed(opcode)) {
		ret = sanitize_ptr_alu(env, insn, ptr_reg, off_reg, dst_reg,
				       &info, false);
		if (ret < 0)
			return sanitize_err(env, insn, ret, off_reg, dst_reg);
	}