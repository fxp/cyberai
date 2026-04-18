// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 34/132



	if (reg->range < 0) {
		verbose(env, "R%d offset is outside of the packet\n", regno);
		return -EINVAL;
	}

	err = check_mem_region_access(env, regno, off, size, reg->range, zero_size_allowed);
	if (err)
		return err;

	/* __check_mem_access has made sure "off + size - 1" is within u16.
	 * reg->umax_value can't be bigger than MAX_PACKET_OFF which is 0xffff,
	 * otherwise find_good_pkt_pointers would have refused to set range info
	 * that __check_mem_access would have rejected this pkt access.
	 * Therefore, "off + reg->umax_value + size - 1" won't overflow u32.
	 */
	env->prog->aux->max_pkt_offset =
		max_t(u32, env->prog->aux->max_pkt_offset,
		      off + reg->umax_value + size - 1);

	return 0;
}

static bool is_var_ctx_off_allowed(struct bpf_prog *prog)
{
	return resolve_prog_type(prog) == BPF_PROG_TYPE_SYSCALL;
}

/* check access to 'struct bpf_context' fields.  Supports fixed offsets only */
static int __check_ctx_access(struct bpf_verifier_env *env, int insn_idx, int off, int size,
			      enum bpf_access_type t, struct bpf_insn_access_aux *info)
{
	if (env->ops->is_valid_access &&
	    env->ops->is_valid_access(off, size, t, env->prog, info)) {
		/* A non zero info.ctx_field_size indicates that this field is a
		 * candidate for later verifier transformation to load the whole
		 * field and then apply a mask when accessed with a narrower
		 * access than actual ctx access size. A zero info.ctx_field_size
		 * will only allow for whole field access and rejects any other
		 * type of narrower access.
		 */
		if (base_type(info->reg_type) == PTR_TO_BTF_ID) {
			if (info->ref_obj_id &&
			    !find_reference_state(env->cur_state, info->ref_obj_id)) {
				verbose(env, "invalid bpf_context access off=%d. Reference may already be released\n",
					off);
				return -EACCES;
			}
		} else {
			env->insn_aux_data[insn_idx].ctx_field_size = info->ctx_field_size;
		}
		/* remember the offset of last byte accessed in ctx */
		if (env->prog->aux->max_ctx_offset < off + size)
			env->prog->aux->max_ctx_offset = off + size;
		return 0;
	}

	verbose(env, "invalid bpf_context access off=%d size=%d\n", off, size);
	return -EACCES;
}

static int check_ctx_access(struct bpf_verifier_env *env, int insn_idx, u32 regno,
			    int off, int access_size, enum bpf_access_type t,
			    struct bpf_insn_access_aux *info)
{
	/*
	 * Program types that don't rewrite ctx accesses can safely
	 * dereference ctx pointers with fixed offsets.
	 */
	bool var_off_ok = is_var_ctx_off_allowed(env->prog);
	bool fixed_off_ok = !env->ops->convert_ctx_access;
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *reg = regs + regno;
	int err;

	if (var_off_ok)
		err = check_mem_region_access(env, regno, off, access_size, U16_MAX, false);
	else
		err = __check_ptr_off_reg(env, reg, regno, fixed_off_ok);
	if (err)
		return err;
	off += reg->umax_value;

	err = __check_ctx_access(env, insn_idx, off, access_size, t, info);
	if (err)
		verbose_linfo(env, insn_idx, "; ");
	return err;
}

static int check_flow_keys_access(struct bpf_verifier_env *env, int off,
				  int size)
{
	if (size < 0 || off < 0 ||
	    (u64)off + size > sizeof(struct bpf_flow_keys)) {
		verbose(env, "invalid access to flow keys off=%d size=%d\n",
			off, size);
		return -EACCES;
	}
	return 0;
}

static int check_sock_access(struct bpf_verifier_env *env, int insn_idx,
			     u32 regno, int off, int size,
			     enum bpf_access_type t)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct bpf_insn_access_aux info = {};
	bool valid;

	if (reg->smin_value < 0) {
		verbose(env, "R%d min value is negative, either use unsigned index or do a if (index >=0) check.\n",
			regno);
		return -EACCES;
	}

	switch (reg->type) {
	case PTR_TO_SOCK_COMMON:
		valid = bpf_sock_common_is_valid_access(off, size, t, &info);
		break;
	case PTR_TO_SOCKET:
		valid = bpf_sock_is_valid_access(off, size, t, &info);
		break;
	case PTR_TO_TCP_SOCK:
		valid = bpf_tcp_sock_is_valid_access(off, size, t, &info);
		break;
	case PTR_TO_XDP_SOCK:
		valid = bpf_xdp_sock_is_valid_access(off, size, t, &info);
		break;
	default:
		valid = false;
	}


	if (valid) {
		env->insn_aux_data[insn_idx].ctx_field_size =
			info.ctx_field_size;
		return 0;
	}

	verbose(env, "R%d invalid %s access off=%d size=%d\n",
		regno, reg_type_str(env, reg->type), off, size);

	return -EACCES;
}

static bool is_pointer_value(struct bpf_verifier_env *env, int regno)
{
	return __is_pointer_value(env->allow_ptr_leaks, reg_state(env, regno));
}

static bool is_ctx_reg(struct bpf_verifier_env *env, int regno)
{
	const struct bpf_reg_state *reg = reg_state(env, regno);

	return reg->type == PTR_TO_CTX;
}

static bool is_sk_reg(struct bpf_verifier_env *env, int regno)
{
	const struct bpf_reg_state *reg = reg_state(env, regno);

	return type_is_sk_pointer(reg->type);
}

static bool is_pkt_reg(struct bpf_verifier_env *env, int regno)
{
	const struct bpf_reg_state *reg = reg_state(env, regno);