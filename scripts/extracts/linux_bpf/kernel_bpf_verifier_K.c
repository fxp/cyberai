// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 43/132



				err = bpf_map_direct_read(map, map_off, size,
							  &val, is_ldsx);
				if (err)
					return err;

				regs[value_regno].type = SCALAR_VALUE;
				__mark_reg_known(&regs[value_regno], val);
			} else if (map->map_type == BPF_MAP_TYPE_INSN_ARRAY) {
				if (bpf_size != BPF_DW) {
					verbose(env, "Invalid read of %d bytes from insn_array\n",
						     size);
					return -EACCES;
				}
				copy_register_state(&regs[value_regno], reg);
				add_scalar_to_reg(&regs[value_regno], off);
				regs[value_regno].type = PTR_TO_INSN;
			} else {
				mark_reg_unknown(env, regs, value_regno);
			}
		}
	} else if (base_type(reg->type) == PTR_TO_MEM) {
		bool rdonly_mem = type_is_rdonly_mem(reg->type);
		bool rdonly_untrusted = rdonly_mem && (reg->type & PTR_UNTRUSTED);

		if (type_may_be_null(reg->type)) {
			verbose(env, "R%d invalid mem access '%s'\n", regno,
				reg_type_str(env, reg->type));
			return -EACCES;
		}

		if (t == BPF_WRITE && rdonly_mem) {
			verbose(env, "R%d cannot write into %s\n",
				regno, reg_type_str(env, reg->type));
			return -EACCES;
		}

		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into mem\n", value_regno);
			return -EACCES;
		}

		/*
		 * Accesses to untrusted PTR_TO_MEM are done through probe
		 * instructions, hence no need to check bounds in that case.
		 */
		if (!rdonly_untrusted)
			err = check_mem_region_access(env, regno, off, size,
						      reg->mem_size, false);
		if (!err && value_regno >= 0 && (t == BPF_READ || rdonly_mem))
			mark_reg_unknown(env, regs, value_regno);
	} else if (reg->type == PTR_TO_CTX) {
		struct bpf_insn_access_aux info = {
			.reg_type = SCALAR_VALUE,
			.is_ldsx = is_ldsx,
			.log = &env->log,
		};
		struct bpf_retval_range range;

		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into ctx\n", value_regno);
			return -EACCES;
		}

		err = check_ctx_access(env, insn_idx, regno, off, size, t, &info);
		if (!err && t == BPF_READ && value_regno >= 0) {
			/* ctx access returns either a scalar, or a
			 * PTR_TO_PACKET[_META,_END]. In the latter
			 * case, we know the offset is zero.
			 */
			if (info.reg_type == SCALAR_VALUE) {
				if (info.is_retval && get_func_retval_range(env->prog, &range)) {
					err = __mark_reg_s32_range(env, regs, value_regno,
								   range.minval, range.maxval);
					if (err)
						return err;
				} else {
					mark_reg_unknown(env, regs, value_regno);
				}
			} else {
				mark_reg_known_zero(env, regs,
						    value_regno);
				if (type_may_be_null(info.reg_type))
					regs[value_regno].id = ++env->id_gen;
				/* A load of ctx field could have different
				 * actual load size with the one encoded in the
				 * insn. When the dst is PTR, it is for sure not
				 * a sub-register.
				 */
				regs[value_regno].subreg_def = DEF_NOT_SUBREG;
				if (base_type(info.reg_type) == PTR_TO_BTF_ID) {
					regs[value_regno].btf = info.btf;
					regs[value_regno].btf_id = info.btf_id;
					regs[value_regno].ref_obj_id = info.ref_obj_id;
				}
			}
			regs[value_regno].type = info.reg_type;
		}

	} else if (reg->type == PTR_TO_STACK) {
		/* Basic bounds checks. */
		err = check_stack_access_within_bounds(env, regno, off, size, t);
		if (err)
			return err;

		if (t == BPF_READ)
			err = check_stack_read(env, regno, off, size,
					       value_regno);
		else
			err = check_stack_write(env, regno, off, size,
						value_regno, insn_idx);
	} else if (reg_is_pkt_pointer(reg)) {
		if (t == BPF_WRITE && !may_access_direct_pkt_data(env, NULL, t)) {
			verbose(env, "cannot write into packet\n");
			return -EACCES;
		}
		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into packet\n",
				value_regno);
			return -EACCES;
		}
		err = check_packet_access(env, regno, off, size, false);
		if (!err && t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else if (reg->type == PTR_TO_FLOW_KEYS) {
		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into flow keys\n",
				value_regno);
			return -EACCES;
		}