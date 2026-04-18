// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 53/132



	if (cur_iter->iter.state == BPF_ITER_STATE_ACTIVE) {
		/* Because iter_next() call is a checkpoint is_state_visitied()
		 * should guarantee parent state with same call sites and insn_idx.
		 */
		if (!cur_st->parent || cur_st->parent->insn_idx != insn_idx ||
		    !same_callsites(cur_st->parent, cur_st)) {
			verifier_bug(env, "bad parent state for iter next call");
			return -EFAULT;
		}
		/* Note cur_st->parent in the call below, it is necessary to skip
		 * checkpoint created for cur_st by is_state_visited()
		 * right at this instruction.
		 */
		prev_st = find_prev_entry(env, cur_st->parent, insn_idx);
		/* branch out active iter state */
		queued_st = push_stack(env, insn_idx + 1, insn_idx, false);
		if (IS_ERR(queued_st))
			return PTR_ERR(queued_st);

		queued_iter = get_iter_from_state(queued_st, meta);
		queued_iter->iter.state = BPF_ITER_STATE_ACTIVE;
		queued_iter->iter.depth++;
		if (prev_st)
			widen_imprecise_scalars(env, prev_st, queued_st);

		queued_fr = queued_st->frame[queued_st->curframe];
		mark_ptr_not_null_reg(&queued_fr->regs[BPF_REG_0]);
	}

	/* switch to DRAINED state, but keep the depth unchanged */
	/* mark current iter state as drained and assume returned NULL */
	cur_iter->iter.state = BPF_ITER_STATE_DRAINED;
	__mark_reg_const_zero(env, &cur_fr->regs[BPF_REG_0]);

	return 0;
}

static bool arg_type_is_mem_size(enum bpf_arg_type type)
{
	return type == ARG_CONST_SIZE ||
	       type == ARG_CONST_SIZE_OR_ZERO;
}

static bool arg_type_is_raw_mem(enum bpf_arg_type type)
{
	return base_type(type) == ARG_PTR_TO_MEM &&
	       type & MEM_UNINIT;
}

static bool arg_type_is_release(enum bpf_arg_type type)
{
	return type & OBJ_RELEASE;
}

static bool arg_type_is_dynptr(enum bpf_arg_type type)
{
	return base_type(type) == ARG_PTR_TO_DYNPTR;
}

static int resolve_map_arg_type(struct bpf_verifier_env *env,
				 const struct bpf_call_arg_meta *meta,
				 enum bpf_arg_type *arg_type)
{
	if (!meta->map.ptr) {
		/* kernel subsystem misconfigured verifier */
		verifier_bug(env, "invalid map_ptr to access map->type");
		return -EFAULT;
	}

	switch (meta->map.ptr->map_type) {
	case BPF_MAP_TYPE_SOCKMAP:
	case BPF_MAP_TYPE_SOCKHASH:
		if (*arg_type == ARG_PTR_TO_MAP_VALUE) {
			*arg_type = ARG_PTR_TO_BTF_ID_SOCK_COMMON;
		} else {
			verbose(env, "invalid arg_type for sockmap/sockhash\n");
			return -EINVAL;
		}
		break;
	case BPF_MAP_TYPE_BLOOM_FILTER:
		if (meta->func_id == BPF_FUNC_map_peek_elem)
			*arg_type = ARG_PTR_TO_MAP_VALUE;
		break;
	default:
		break;
	}
	return 0;
}

struct bpf_reg_types {
	const enum bpf_reg_type types[10];
	u32 *btf_id;
};

static const struct bpf_reg_types sock_types = {
	.types = {
		PTR_TO_SOCK_COMMON,
		PTR_TO_SOCKET,
		PTR_TO_TCP_SOCK,
		PTR_TO_XDP_SOCK,
	},
};

#ifdef CONFIG_NET
static const struct bpf_reg_types btf_id_sock_common_types = {
	.types = {
		PTR_TO_SOCK_COMMON,
		PTR_TO_SOCKET,
		PTR_TO_TCP_SOCK,
		PTR_TO_XDP_SOCK,
		PTR_TO_BTF_ID,
		PTR_TO_BTF_ID | PTR_TRUSTED,
	},
	.btf_id = &btf_sock_ids[BTF_SOCK_TYPE_SOCK_COMMON],
};
#endif

static const struct bpf_reg_types mem_types = {
	.types = {
		PTR_TO_STACK,
		PTR_TO_PACKET,
		PTR_TO_PACKET_META,
		PTR_TO_MAP_KEY,
		PTR_TO_MAP_VALUE,
		PTR_TO_MEM,
		PTR_TO_MEM | MEM_RINGBUF,
		PTR_TO_BUF,
		PTR_TO_BTF_ID | PTR_TRUSTED,
		PTR_TO_CTX,
	},
};

static const struct bpf_reg_types spin_lock_types = {
	.types = {
		PTR_TO_MAP_VALUE,
		PTR_TO_BTF_ID | MEM_ALLOC,
	}
};

static const struct bpf_reg_types fullsock_types = { .types = { PTR_TO_SOCKET } };
static const struct bpf_reg_types scalar_types = { .types = { SCALAR_VALUE } };
static const struct bpf_reg_types context_types = { .types = { PTR_TO_CTX } };
static const struct bpf_reg_types ringbuf_mem_types = { .types = { PTR_TO_MEM | MEM_RINGBUF } };
static const struct bpf_reg_types const_map_ptr_types = { .types = { CONST_PTR_TO_MAP } };
static const struct bpf_reg_types btf_ptr_types = {
	.types = {
		PTR_TO_BTF_ID,
		PTR_TO_BTF_ID | PTR_TRUSTED,
		PTR_TO_BTF_ID | MEM_RCU,
	},
};
static const struct bpf_reg_types percpu_btf_ptr_types = {
	.types = {
		PTR_TO_BTF_ID | MEM_PERCPU,
		PTR_TO_BTF_ID | MEM_PERCPU | MEM_RCU,
		PTR_TO_BTF_ID | MEM_PERCPU | PTR_TRUSTED,
	}
};
static const struct bpf_reg_types func_ptr_types = { .types = { PTR_TO_FUNC } };
static const struct bpf_reg_types stack_ptr_types = { .types = { PTR_TO_STACK } };
static const struct bpf_reg_types const_str_ptr_types = { .types = { PTR_TO_MAP_VALUE } };
static const struct bpf_reg_types timer_types = { .types = { PTR_TO_MAP_VALUE } };
static const struct bpf_reg_types kptr_xchg_dest_types = {
	.types = {
		PTR_TO_MAP_VALUE,
		PTR_TO_BTF_ID | MEM_ALLOC,
		PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF,
		PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF | MEM_RCU,
	}
};
static const struct bpf_reg_types dynptr_types = {
	.types = {
		PTR_TO_STACK,
		CONST_PTR_TO_DYNPTR,
	}
};