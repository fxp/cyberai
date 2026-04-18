// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 56/132



	if (reg->type == CONST_PTR_TO_DYNPTR)
		return reg->id;
	spi = dynptr_get_spi(env, reg);
	if (spi < 0)
		return spi;
	return state->stack[spi].spilled_ptr.id;
}

static int dynptr_ref_obj_id(struct bpf_verifier_env *env, struct bpf_reg_state *reg)
{
	struct bpf_func_state *state = bpf_func(env, reg);
	int spi;

	if (reg->type == CONST_PTR_TO_DYNPTR)
		return reg->ref_obj_id;
	spi = dynptr_get_spi(env, reg);
	if (spi < 0)
		return spi;
	return state->stack[spi].spilled_ptr.ref_obj_id;
}

static enum bpf_dynptr_type dynptr_get_type(struct bpf_verifier_env *env,
					    struct bpf_reg_state *reg)
{
	struct bpf_func_state *state = bpf_func(env, reg);
	int spi;

	if (reg->type == CONST_PTR_TO_DYNPTR)
		return reg->dynptr.type;

	spi = bpf_get_spi(reg->var_off.value);
	if (spi < 0) {
		verbose(env, "verifier internal error: invalid spi when querying dynptr type\n");
		return BPF_DYNPTR_TYPE_INVALID;
	}

	return state->stack[spi].spilled_ptr.dynptr.type;
}

static int check_reg_const_str(struct bpf_verifier_env *env,
			       struct bpf_reg_state *reg, u32 regno)
{
	struct bpf_map *map = reg->map_ptr;
	int err;
	int map_off;
	u64 map_addr;
	char *str_ptr;

	if (reg->type != PTR_TO_MAP_VALUE)
		return -EINVAL;

	if (map->map_type == BPF_MAP_TYPE_INSN_ARRAY) {
		verbose(env, "R%d points to insn_array map which cannot be used as const string\n", regno);
		return -EACCES;
	}

	if (!bpf_map_is_rdonly(map)) {
		verbose(env, "R%d does not point to a readonly map'\n", regno);
		return -EACCES;
	}

	if (!tnum_is_const(reg->var_off)) {
		verbose(env, "R%d is not a constant address'\n", regno);
		return -EACCES;
	}

	if (!map->ops->map_direct_value_addr) {
		verbose(env, "no direct value access support for this map type\n");
		return -EACCES;
	}

	err = check_map_access(env, regno, 0,
			       map->value_size - reg->var_off.value, false,
			       ACCESS_HELPER);
	if (err)
		return err;

	map_off = reg->var_off.value;
	err = map->ops->map_direct_value_addr(map, &map_addr, map_off);
	if (err) {
		verbose(env, "direct value access on string failed\n");
		return err;
	}

	str_ptr = (char *)(long)(map_addr);
	if (!strnchr(str_ptr + map_off, map->value_size - map_off, 0)) {
		verbose(env, "string is not zero-terminated\n");
		return -EINVAL;
	}
	return 0;
}

/* Returns constant key value in `value` if possible, else negative error */
static int get_constant_map_key(struct bpf_verifier_env *env,
				struct bpf_reg_state *key,
				u32 key_size,
				s64 *value)
{
	struct bpf_func_state *state = bpf_func(env, key);
	struct bpf_reg_state *reg;
	int slot, spi, off;
	int spill_size = 0;
	int zero_size = 0;
	int stack_off;
	int i, err;
	u8 *stype;

	if (!env->bpf_capable)
		return -EOPNOTSUPP;
	if (key->type != PTR_TO_STACK)
		return -EOPNOTSUPP;
	if (!tnum_is_const(key->var_off))
		return -EOPNOTSUPP;

	stack_off = key->var_off.value;
	slot = -stack_off - 1;
	spi = slot / BPF_REG_SIZE;
	off = slot % BPF_REG_SIZE;
	stype = state->stack[spi].slot_type;

	/* First handle precisely tracked STACK_ZERO */
	for (i = off; i >= 0 && stype[i] == STACK_ZERO; i--)
		zero_size++;
	if (zero_size >= key_size) {
		*value = 0;
		return 0;
	}

	/* Check that stack contains a scalar spill of expected size */
	if (!bpf_is_spilled_scalar_reg(&state->stack[spi]))
		return -EOPNOTSUPP;
	for (i = off; i >= 0 && stype[i] == STACK_SPILL; i--)
		spill_size++;
	if (spill_size != key_size)
		return -EOPNOTSUPP;

	reg = &state->stack[spi].spilled_ptr;
	if (!tnum_is_const(reg->var_off))
		/* Stack value not statically known */
		return -EOPNOTSUPP;

	/* We are relying on a constant value. So mark as precise
	 * to prevent pruning on it.
	 */
	bpf_bt_set_frame_slot(&env->bt, key->frameno, spi);
	err = mark_chain_precision_batch(env, env->cur_state);
	if (err < 0)
		return err;

	*value = reg->var_off.value;
	return 0;
}

static bool can_elide_value_nullness(enum bpf_map_type type);

static int check_func_arg(struct bpf_verifier_env *env, u32 arg,
			  struct bpf_call_arg_meta *meta,
			  const struct bpf_func_proto *fn,
			  int insn_idx)
{
	u32 regno = BPF_REG_1 + arg;
	struct bpf_reg_state *reg = reg_state(env, regno);
	enum bpf_arg_type arg_type = fn->arg_type[arg];
	enum bpf_reg_type type = reg->type;
	u32 *arg_btf_id = NULL;
	u32 key_size;
	int err = 0;

	if (arg_type == ARG_DONTCARE)
		return 0;

	err = check_reg_arg(env, regno, SRC_OP);
	if (err)
		return err;

	if (arg_type == ARG_ANYTHING) {
		if (is_pointer_value(env, regno)) {
			verbose(env, "R%d leaks addr into helper function\n",
				regno);
			return -EACCES;
		}
		return 0;
	}

	if (type_is_pkt_pointer(type) &&
	    !may_access_direct_pkt_data(env, meta, BPF_READ)) {
		verbose(env, "helper access to the packet is not allowed\n");
		return -EACCES;
	}

	if (base_type(arg_type) == ARG_PTR_TO_MAP_VALUE) {
		err = resolve_map_arg_type(env, meta, &arg_type);
		if (err)
			return err;
	}