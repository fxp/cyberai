// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 32/132



	/* A full type match is needed, as BTF can be vmlinux, module or prog BTF, and
	 * we also need to take into account the reg->var_off.
	 *
	 * We want to support cases like:
	 *
	 * struct foo {
	 *         struct bar br;
	 *         struct baz bz;
	 * };
	 *
	 * struct foo *v;
	 * v = func();	      // PTR_TO_BTF_ID
	 * val->foo = v;      // reg->var_off is zero, btf and btf_id match type
	 * val->bar = &v->br; // reg->var_off is still zero, but we need to retry with
	 *                    // first member type of struct after comparison fails
	 * val->baz = &v->bz; // reg->var_off is non-zero, so struct needs to be walked
	 *                    // to match type
	 *
	 * In the kptr_ref case, check_func_arg_reg_off already ensures reg->var_off
	 * is zero. We must also ensure that btf_struct_ids_match does not walk
	 * the struct to match type against first member of struct, i.e. reject
	 * second case from above. Hence, when type is BPF_KPTR_REF, we set
	 * strict mode to true for type match.
	 */
	if (!btf_struct_ids_match(&env->log, reg->btf, reg->btf_id, reg->var_off.value,
				  kptr_field->kptr.btf, kptr_field->kptr.btf_id,
				  kptr_field->type != BPF_KPTR_UNREF))
		goto bad_type;
	return 0;
bad_type:
	verbose(env, "invalid kptr access, R%d type=%s%s ", regno,
		reg_type_str(env, reg->type), reg_name);
	verbose(env, "expected=%s%s", reg_type_str(env, PTR_TO_BTF_ID), targ_name);
	if (kptr_field->type == BPF_KPTR_UNREF)
		verbose(env, " or %s%s\n", reg_type_str(env, PTR_TO_BTF_ID | PTR_UNTRUSTED),
			targ_name);
	else
		verbose(env, "\n");
	return -EINVAL;
}

static bool in_sleepable(struct bpf_verifier_env *env)
{
	return env->cur_state->in_sleepable;
}

/* The non-sleepable programs and sleepable programs with explicit bpf_rcu_read_lock()
 * can dereference RCU protected pointers and result is PTR_TRUSTED.
 */
static bool in_rcu_cs(struct bpf_verifier_env *env)
{
	return env->cur_state->active_rcu_locks ||
	       env->cur_state->active_locks ||
	       !in_sleepable(env);
}

/* Once GCC supports btf_type_tag the following mechanism will be replaced with tag check */
BTF_SET_START(rcu_protected_types)
#ifdef CONFIG_NET
BTF_ID(struct, prog_test_ref_kfunc)
#endif
#ifdef CONFIG_CGROUPS
BTF_ID(struct, cgroup)
#endif
#ifdef CONFIG_BPF_JIT
BTF_ID(struct, bpf_cpumask)
#endif
BTF_ID(struct, task_struct)
#ifdef CONFIG_CRYPTO
BTF_ID(struct, bpf_crypto_ctx)
#endif
BTF_SET_END(rcu_protected_types)

static bool rcu_protected_object(const struct btf *btf, u32 btf_id)
{
	if (!btf_is_kernel(btf))
		return true;
	return btf_id_set_contains(&rcu_protected_types, btf_id);
}

static struct btf_record *kptr_pointee_btf_record(struct btf_field *kptr_field)
{
	struct btf_struct_meta *meta;

	if (btf_is_kernel(kptr_field->kptr.btf))
		return NULL;

	meta = btf_find_struct_meta(kptr_field->kptr.btf,
				    kptr_field->kptr.btf_id);

	return meta ? meta->record : NULL;
}

static bool rcu_safe_kptr(const struct btf_field *field)
{
	const struct btf_field_kptr *kptr = &field->kptr;

	return field->type == BPF_KPTR_PERCPU ||
	       (field->type == BPF_KPTR_REF && rcu_protected_object(kptr->btf, kptr->btf_id));
}

static u32 btf_ld_kptr_type(struct bpf_verifier_env *env, struct btf_field *kptr_field)
{
	struct btf_record *rec;
	u32 ret;

	ret = PTR_MAYBE_NULL;
	if (rcu_safe_kptr(kptr_field) && in_rcu_cs(env)) {
		ret |= MEM_RCU;
		if (kptr_field->type == BPF_KPTR_PERCPU)
			ret |= MEM_PERCPU;
		else if (!btf_is_kernel(kptr_field->kptr.btf))
			ret |= MEM_ALLOC;

		rec = kptr_pointee_btf_record(kptr_field);
		if (rec && btf_record_has_field(rec, BPF_GRAPH_NODE))
			ret |= NON_OWN_REF;
	} else {
		ret |= PTR_UNTRUSTED;
	}

	return ret;
}

static int mark_uptr_ld_reg(struct bpf_verifier_env *env, u32 regno,
			    struct btf_field *field)
{
	struct bpf_reg_state *reg;
	const struct btf_type *t;

	t = btf_type_by_id(field->kptr.btf, field->kptr.btf_id);
	mark_reg_known_zero(env, cur_regs(env), regno);
	reg = reg_state(env, regno);
	reg->type = PTR_TO_MEM | PTR_MAYBE_NULL;
	reg->mem_size = t->size;
	reg->id = ++env->id_gen;

	return 0;
}

static int check_map_kptr_access(struct bpf_verifier_env *env, u32 regno,
				 int value_regno, int insn_idx,
				 struct btf_field *kptr_field)
{
	struct bpf_insn *insn = &env->prog->insnsi[insn_idx];
	int class = BPF_CLASS(insn->code);
	struct bpf_reg_state *val_reg;
	int ret;

	/* Things we already checked for in check_map_access and caller:
	 *  - Reject cases where variable offset may touch kptr
	 *  - size of access (must be BPF_DW)
	 *  - tnum_is_const(reg->var_off)
	 *  - kptr_field->offset == off + reg->var_off.value
	 */
	/* Only BPF_[LDX,STX,ST] | BPF_MEM | BPF_DW is supported */
	if (BPF_MODE(insn->code) != BPF_MEM) {
		verbose(env, "kptr in map can only be accessed using BPF_MEM instruction mode\n");
		return -EACCES;
	}