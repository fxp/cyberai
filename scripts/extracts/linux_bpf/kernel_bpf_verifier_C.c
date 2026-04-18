// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 35/132



	return type_is_pkt_pointer(reg->type);
}

static bool is_flow_key_reg(struct bpf_verifier_env *env, int regno)
{
	const struct bpf_reg_state *reg = reg_state(env, regno);

	/* Separate to is_ctx_reg() since we still want to allow BPF_ST here. */
	return reg->type == PTR_TO_FLOW_KEYS;
}

static bool is_arena_reg(struct bpf_verifier_env *env, int regno)
{
	const struct bpf_reg_state *reg = reg_state(env, regno);

	return reg->type == PTR_TO_ARENA;
}

/* Return false if @regno contains a pointer whose type isn't supported for
 * atomic instruction @insn.
 */
static bool atomic_ptr_type_ok(struct bpf_verifier_env *env, int regno,
			       struct bpf_insn *insn)
{
	if (is_ctx_reg(env, regno))
		return false;
	if (is_pkt_reg(env, regno))
		return false;
	if (is_flow_key_reg(env, regno))
		return false;
	if (is_sk_reg(env, regno))
		return false;
	if (is_arena_reg(env, regno))
		return bpf_jit_supports_insn(insn, true);

	return true;
}

static u32 *reg2btf_ids[__BPF_REG_TYPE_MAX] = {
#ifdef CONFIG_NET
	[PTR_TO_SOCKET] = &btf_sock_ids[BTF_SOCK_TYPE_SOCK],
	[PTR_TO_SOCK_COMMON] = &btf_sock_ids[BTF_SOCK_TYPE_SOCK_COMMON],
	[PTR_TO_TCP_SOCK] = &btf_sock_ids[BTF_SOCK_TYPE_TCP],
#endif
	[CONST_PTR_TO_MAP] = btf_bpf_map_id,
};

static bool is_trusted_reg(const struct bpf_reg_state *reg)
{
	/* A referenced register is always trusted. */
	if (reg->ref_obj_id)
		return true;

	/* Types listed in the reg2btf_ids are always trusted */
	if (reg2btf_ids[base_type(reg->type)] &&
	    !bpf_type_has_unsafe_modifiers(reg->type))
		return true;

	/* If a register is not referenced, it is trusted if it has the
	 * MEM_ALLOC or PTR_TRUSTED type modifiers, and no others. Some of the
	 * other type modifiers may be safe, but we elect to take an opt-in
	 * approach here as some (e.g. PTR_UNTRUSTED and PTR_MAYBE_NULL) are
	 * not.
	 *
	 * Eventually, we should make PTR_TRUSTED the single source of truth
	 * for whether a register is trusted.
	 */
	return type_flag(reg->type) & BPF_REG_TRUSTED_MODIFIERS &&
	       !bpf_type_has_unsafe_modifiers(reg->type);
}

static bool is_rcu_reg(const struct bpf_reg_state *reg)
{
	return reg->type & MEM_RCU;
}

static void clear_trusted_flags(enum bpf_type_flag *flag)
{
	*flag &= ~(BPF_REG_TRUSTED_MODIFIERS | MEM_RCU);
}

static int check_pkt_ptr_alignment(struct bpf_verifier_env *env,
				   const struct bpf_reg_state *reg,
				   int off, int size, bool strict)
{
	struct tnum reg_off;
	int ip_align;

	/* Byte size accesses are always allowed. */
	if (!strict || size == 1)
		return 0;

	/* For platforms that do not have a Kconfig enabling
	 * CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS the value of
	 * NET_IP_ALIGN is universally set to '2'.  And on platforms
	 * that do set CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS, we get
	 * to this code only in strict mode where we want to emulate
	 * the NET_IP_ALIGN==2 checking.  Therefore use an
	 * unconditional IP align value of '2'.
	 */
	ip_align = 2;

	reg_off = tnum_add(reg->var_off, tnum_const(ip_align + off));
	if (!tnum_is_aligned(reg_off, size)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
		verbose(env,
			"misaligned packet access off %d+%s+%d size %d\n",
			ip_align, tn_buf, off, size);
		return -EACCES;
	}

	return 0;
}

static int check_generic_ptr_alignment(struct bpf_verifier_env *env,
				       const struct bpf_reg_state *reg,
				       const char *pointer_desc,
				       int off, int size, bool strict)
{
	struct tnum reg_off;

	/* Byte size accesses are always allowed. */
	if (!strict || size == 1)
		return 0;

	reg_off = tnum_add(reg->var_off, tnum_const(off));
	if (!tnum_is_aligned(reg_off, size)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
		verbose(env, "misaligned %saccess off %s+%d size %d\n",
			pointer_desc, tn_buf, off, size);
		return -EACCES;
	}

	return 0;
}

static int check_ptr_alignment(struct bpf_verifier_env *env,
			       const struct bpf_reg_state *reg, int off,
			       int size, bool strict_alignment_once)
{
	bool strict = env->strict_alignment || strict_alignment_once;
	const char *pointer_desc = "";