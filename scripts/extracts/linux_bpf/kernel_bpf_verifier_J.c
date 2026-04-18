// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 42/132



/* Check that the stack access at 'regno + off' falls within the maximum stack
 * bounds.
 *
 * 'off' includes `regno->offset`, but not its dynamic part (if any).
 */
static int check_stack_access_within_bounds(
		struct bpf_verifier_env *env,
		int regno, int off, int access_size,
		enum bpf_access_type type)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	struct bpf_func_state *state = bpf_func(env, reg);
	s64 min_off, max_off;
	int err;
	char *err_extra;

	if (type == BPF_READ)
		err_extra = " read from";
	else
		err_extra = " write to";

	if (tnum_is_const(reg->var_off)) {
		min_off = (s64)reg->var_off.value + off;
		max_off = min_off + access_size;
	} else {
		if (reg->smax_value >= BPF_MAX_VAR_OFF ||
		    reg->smin_value <= -BPF_MAX_VAR_OFF) {
			verbose(env, "invalid unbounded variable-offset%s stack R%d\n",
				err_extra, regno);
			return -EACCES;
		}
		min_off = reg->smin_value + off;
		max_off = reg->smax_value + off + access_size;
	}

	err = check_stack_slot_within_bounds(env, min_off, state, type);
	if (!err && max_off > 0)
		err = -EINVAL; /* out of stack access into non-negative offsets */
	if (!err && access_size < 0)
		/* access_size should not be negative (or overflow an int); others checks
		 * along the way should have prevented such an access.
		 */
		err = -EFAULT; /* invalid negative access size; integer overflow? */

	if (err) {
		if (tnum_is_const(reg->var_off)) {
			verbose(env, "invalid%s stack R%d off=%lld size=%d\n",
				err_extra, regno, min_off, access_size);
		} else {
			char tn_buf[48];

			tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
			verbose(env, "invalid variable-offset%s stack R%d var_off=%s off=%d size=%d\n",
				err_extra, regno, tn_buf, off, access_size);
		}
		return err;
	}

	/* Note that there is no stack access with offset zero, so the needed stack
	 * size is -min_off, not -min_off+1.
	 */
	return grow_stack_state(env, state, -min_off /* size */);
}

static bool get_func_retval_range(struct bpf_prog *prog,
				  struct bpf_retval_range *range)
{
	if (prog->type == BPF_PROG_TYPE_LSM &&
		prog->expected_attach_type == BPF_LSM_MAC &&
		!bpf_lsm_get_retval_range(prog, range)) {
		return true;
	}
	return false;
}

static void add_scalar_to_reg(struct bpf_reg_state *dst_reg, s64 val)
{
	struct bpf_reg_state fake_reg;

	if (!val)
		return;

	fake_reg.type = SCALAR_VALUE;
	__mark_reg_known(&fake_reg, val);

	scalar32_min_max_add(dst_reg, &fake_reg);
	scalar_min_max_add(dst_reg, &fake_reg);
	dst_reg->var_off = tnum_add(dst_reg->var_off, fake_reg.var_off);

	reg_bounds_sync(dst_reg);
}

/* check whether memory at (regno + off) is accessible for t = (read | write)
 * if t==write, value_regno is a register which value is stored into memory
 * if t==read, value_regno is a register which will receive the value from memory
 * if t==write && value_regno==-1, some unknown value is stored into memory
 * if t==read && value_regno==-1, don't care what we read from memory
 */
static int check_mem_access(struct bpf_verifier_env *env, int insn_idx, u32 regno,
			    int off, int bpf_size, enum bpf_access_type t,
			    int value_regno, bool strict_alignment_once, bool is_ldsx)
{
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *reg = regs + regno;
	int size, err = 0;

	size = bpf_size_to_bytes(bpf_size);
	if (size < 0)
		return size;

	err = check_ptr_alignment(env, reg, off, size, strict_alignment_once);
	if (err)
		return err;

	if (reg->type == PTR_TO_MAP_KEY) {
		if (t == BPF_WRITE) {
			verbose(env, "write to change key R%d not allowed\n", regno);
			return -EACCES;
		}

		err = check_mem_region_access(env, regno, off, size,
					      reg->map_ptr->key_size, false);
		if (err)
			return err;
		if (value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else if (reg->type == PTR_TO_MAP_VALUE) {
		struct btf_field *kptr_field = NULL;

		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into map\n", value_regno);
			return -EACCES;
		}
		err = check_map_access_type(env, regno, off, size, t);
		if (err)
			return err;
		err = check_map_access(env, regno, off, size, false, ACCESS_DIRECT);
		if (err)
			return err;
		if (tnum_is_const(reg->var_off))
			kptr_field = btf_record_find(reg->map_ptr->record,
						     off + reg->var_off.value, BPF_KPTR | BPF_UPTR);
		if (kptr_field) {
			err = check_map_kptr_access(env, regno, value_regno, insn_idx, kptr_field);
		} else if (t == BPF_READ && value_regno >= 0) {
			struct bpf_map *map = reg->map_ptr;

			/*
			 * If map is read-only, track its contents as scalars,
			 * unless it is an insn array (see the special case below)
			 */
			if (tnum_is_const(reg->var_off) &&
			    bpf_map_is_rdonly(map) &&
			    map->ops->map_direct_value_addr &&
			    map->map_type != BPF_MAP_TYPE_INSN_ARRAY) {
				int map_off = off + reg->var_off.value;
				u64 val = 0;