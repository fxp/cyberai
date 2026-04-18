// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 54/132



static const struct bpf_reg_types *compatible_reg_types[__BPF_ARG_TYPE_MAX] = {
	[ARG_PTR_TO_MAP_KEY]		= &mem_types,
	[ARG_PTR_TO_MAP_VALUE]		= &mem_types,
	[ARG_CONST_SIZE]		= &scalar_types,
	[ARG_CONST_SIZE_OR_ZERO]	= &scalar_types,
	[ARG_CONST_ALLOC_SIZE_OR_ZERO]	= &scalar_types,
	[ARG_CONST_MAP_PTR]		= &const_map_ptr_types,
	[ARG_PTR_TO_CTX]		= &context_types,
	[ARG_PTR_TO_SOCK_COMMON]	= &sock_types,
#ifdef CONFIG_NET
	[ARG_PTR_TO_BTF_ID_SOCK_COMMON]	= &btf_id_sock_common_types,
#endif
	[ARG_PTR_TO_SOCKET]		= &fullsock_types,
	[ARG_PTR_TO_BTF_ID]		= &btf_ptr_types,
	[ARG_PTR_TO_SPIN_LOCK]		= &spin_lock_types,
	[ARG_PTR_TO_MEM]		= &mem_types,
	[ARG_PTR_TO_RINGBUF_MEM]	= &ringbuf_mem_types,
	[ARG_PTR_TO_PERCPU_BTF_ID]	= &percpu_btf_ptr_types,
	[ARG_PTR_TO_FUNC]		= &func_ptr_types,
	[ARG_PTR_TO_STACK]		= &stack_ptr_types,
	[ARG_PTR_TO_CONST_STR]		= &const_str_ptr_types,
	[ARG_PTR_TO_TIMER]		= &timer_types,
	[ARG_KPTR_XCHG_DEST]		= &kptr_xchg_dest_types,
	[ARG_PTR_TO_DYNPTR]		= &dynptr_types,
};

static int check_reg_type(struct bpf_verifier_env *env, u32 regno,
			  enum bpf_arg_type arg_type,
			  const u32 *arg_btf_id,
			  struct bpf_call_arg_meta *meta)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	enum bpf_reg_type expected, type = reg->type;
	const struct bpf_reg_types *compatible;
	int i, j, err;

	compatible = compatible_reg_types[base_type(arg_type)];
	if (!compatible) {
		verifier_bug(env, "unsupported arg type %d", arg_type);
		return -EFAULT;
	}

	/* ARG_PTR_TO_MEM + RDONLY is compatible with PTR_TO_MEM and PTR_TO_MEM + RDONLY,
	 * but ARG_PTR_TO_MEM is compatible only with PTR_TO_MEM and NOT with PTR_TO_MEM + RDONLY
	 *
	 * Same for MAYBE_NULL:
	 *
	 * ARG_PTR_TO_MEM + MAYBE_NULL is compatible with PTR_TO_MEM and PTR_TO_MEM + MAYBE_NULL,
	 * but ARG_PTR_TO_MEM is compatible only with PTR_TO_MEM but NOT with PTR_TO_MEM + MAYBE_NULL
	 *
	 * ARG_PTR_TO_MEM is compatible with PTR_TO_MEM that is tagged with a dynptr type.
	 *
	 * Therefore we fold these flags depending on the arg_type before comparison.
	 */
	if (arg_type & MEM_RDONLY)
		type &= ~MEM_RDONLY;
	if (arg_type & PTR_MAYBE_NULL)
		type &= ~PTR_MAYBE_NULL;
	if (base_type(arg_type) == ARG_PTR_TO_MEM)
		type &= ~DYNPTR_TYPE_FLAG_MASK;

	/* Local kptr types are allowed as the source argument of bpf_kptr_xchg */
	if (meta->func_id == BPF_FUNC_kptr_xchg && type_is_alloc(type) && regno == BPF_REG_2) {
		type &= ~MEM_ALLOC;
		type &= ~MEM_PERCPU;
	}

	for (i = 0; i < ARRAY_SIZE(compatible->types); i++) {
		expected = compatible->types[i];
		if (expected == NOT_INIT)
			break;

		if (type == expected)
			goto found;
	}

	verbose(env, "R%d type=%s expected=", regno, reg_type_str(env, reg->type));
	for (j = 0; j + 1 < i; j++)
		verbose(env, "%s, ", reg_type_str(env, compatible->types[j]));
	verbose(env, "%s\n", reg_type_str(env, compatible->types[j]));
	return -EACCES;

found:
	if (base_type(reg->type) != PTR_TO_BTF_ID)
		return 0;

	if (compatible == &mem_types) {
		if (!(arg_type & MEM_RDONLY)) {
			verbose(env,
				"%s() may write into memory pointed by R%d type=%s\n",
				func_id_name(meta->func_id),
				regno, reg_type_str(env, reg->type));
			return -EACCES;
		}
		return 0;
	}

	switch ((int)reg->type) {
	case PTR_TO_BTF_ID:
	case PTR_TO_BTF_ID | PTR_TRUSTED:
	case PTR_TO_BTF_ID | PTR_TRUSTED | PTR_MAYBE_NULL:
	case PTR_TO_BTF_ID | MEM_RCU:
	case PTR_TO_BTF_ID | PTR_MAYBE_NULL:
	case PTR_TO_BTF_ID | PTR_MAYBE_NULL | MEM_RCU:
	{
		/* For bpf_sk_release, it needs to match against first member
		 * 'struct sock_common', hence make an exception for it. This
		 * allows bpf_sk_release to work for multiple socket types.
		 */
		bool strict_type_match = arg_type_is_release(arg_type) &&
					 meta->func_id != BPF_FUNC_sk_release;

		if (type_may_be_null(reg->type) &&
		    (!type_may_be_null(arg_type) || arg_type_is_release(arg_type))) {
			verbose(env, "Possibly NULL pointer passed to helper arg%d\n", regno);
			return -EACCES;
		}

		if (!arg_btf_id) {
			if (!compatible->btf_id) {
				verifier_bug(env, "missing arg compatible BTF ID");
				return -EFAULT;
			}
			arg_btf_id = compatible->btf_id;
		}

		if (meta->func_id == BPF_FUNC_kptr_xchg) {
			if (map_kptr_match_type(env, meta->kptr_field, reg, regno))
				return -EACCES;
		} else {
			if (arg_btf_id == BPF_PTR_POISON) {
				verbose(env, "verifier internal error:");
				verbose(env, "R%d has non-overwritten BPF_PTR_POISON type\n",
					regno);
				return -EACCES;
			}

			err = __check_ptr_off_reg(env, reg, regno, true);
			if (err)
				return err;