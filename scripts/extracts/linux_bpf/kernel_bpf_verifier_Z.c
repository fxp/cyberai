// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 58/132



		/* bpf_map_xxx(..., map_ptr, ..., value) call:
		 * check [value, value + map->value_size) validity
		 */
		if (!meta->map.ptr) {
			/* kernel subsystem misconfigured verifier */
			verifier_bug(env, "invalid map_ptr to access map->value");
			return -EFAULT;
		}
		meta->raw_mode = arg_type & MEM_UNINIT;
		err = check_helper_mem_access(env, regno, meta->map.ptr->value_size,
					      arg_type & MEM_WRITE ? BPF_WRITE : BPF_READ,
					      false, meta);
		break;
	case ARG_PTR_TO_PERCPU_BTF_ID:
		if (!reg->btf_id) {
			verbose(env, "Helper has invalid btf_id in R%d\n", regno);
			return -EACCES;
		}
		meta->ret_btf = reg->btf;
		meta->ret_btf_id = reg->btf_id;
		break;
	case ARG_PTR_TO_SPIN_LOCK:
		if (in_rbtree_lock_required_cb(env)) {
			verbose(env, "can't spin_{lock,unlock} in rbtree cb\n");
			return -EACCES;
		}
		if (meta->func_id == BPF_FUNC_spin_lock) {
			err = process_spin_lock(env, regno, PROCESS_SPIN_LOCK);
			if (err)
				return err;
		} else if (meta->func_id == BPF_FUNC_spin_unlock) {
			err = process_spin_lock(env, regno, 0);
			if (err)
				return err;
		} else {
			verifier_bug(env, "spin lock arg on unexpected helper");
			return -EFAULT;
		}
		break;
	case ARG_PTR_TO_TIMER:
		err = process_timer_helper(env, regno, meta);
		if (err)
			return err;
		break;
	case ARG_PTR_TO_FUNC:
		meta->subprogno = reg->subprogno;
		break;
	case ARG_PTR_TO_MEM:
		/* The access to this pointer is only checked when we hit the
		 * next is_mem_size argument below.
		 */
		meta->raw_mode = arg_type & MEM_UNINIT;
		if (arg_type & MEM_FIXED_SIZE) {
			err = check_helper_mem_access(env, regno, fn->arg_size[arg],
						      arg_type & MEM_WRITE ? BPF_WRITE : BPF_READ,
						      false, meta);
			if (err)
				return err;
			if (arg_type & MEM_ALIGNED)
				err = check_ptr_alignment(env, reg, 0, fn->arg_size[arg], true);
		}
		break;
	case ARG_CONST_SIZE:
		err = check_mem_size_reg(env, reg, regno,
					 fn->arg_type[arg - 1] & MEM_WRITE ?
					 BPF_WRITE : BPF_READ,
					 false, meta);
		break;
	case ARG_CONST_SIZE_OR_ZERO:
		err = check_mem_size_reg(env, reg, regno,
					 fn->arg_type[arg - 1] & MEM_WRITE ?
					 BPF_WRITE : BPF_READ,
					 true, meta);
		break;
	case ARG_PTR_TO_DYNPTR:
		err = process_dynptr_func(env, regno, insn_idx, arg_type, 0);
		if (err)
			return err;
		break;
	case ARG_CONST_ALLOC_SIZE_OR_ZERO:
		if (!tnum_is_const(reg->var_off)) {
			verbose(env, "R%d is not a known constant'\n",
				regno);
			return -EACCES;
		}
		meta->mem_size = reg->var_off.value;
		err = mark_chain_precision(env, regno);
		if (err)
			return err;
		break;
	case ARG_PTR_TO_CONST_STR:
	{
		err = check_reg_const_str(env, reg, regno);
		if (err)
			return err;
		break;
	}
	case ARG_KPTR_XCHG_DEST:
		err = process_kptr_func(env, regno, meta);
		if (err)
			return err;
		break;
	}

	return err;
}

static bool may_update_sockmap(struct bpf_verifier_env *env, int func_id)
{
	enum bpf_attach_type eatype = env->prog->expected_attach_type;
	enum bpf_prog_type type = resolve_prog_type(env->prog);

	if (func_id != BPF_FUNC_map_update_elem &&
	    func_id != BPF_FUNC_map_delete_elem)
		return false;

	/* It's not possible to get access to a locked struct sock in these
	 * contexts, so updating is safe.
	 */
	switch (type) {
	case BPF_PROG_TYPE_TRACING:
		if (eatype == BPF_TRACE_ITER)
			return true;
		break;
	case BPF_PROG_TYPE_SOCK_OPS:
		/* map_update allowed only via dedicated helpers with event type checks */
		if (func_id == BPF_FUNC_map_delete_elem)
			return true;
		break;
	case BPF_PROG_TYPE_SOCKET_FILTER:
	case BPF_PROG_TYPE_SCHED_CLS:
	case BPF_PROG_TYPE_SCHED_ACT:
	case BPF_PROG_TYPE_XDP:
	case BPF_PROG_TYPE_SK_REUSEPORT:
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
	case BPF_PROG_TYPE_SK_LOOKUP:
		return true;
	default:
		break;
	}

	verbose(env, "cannot update sockmap in this context\n");
	return false;
}

bool bpf_allow_tail_call_in_subprogs(struct bpf_verifier_env *env)
{
	return env->prog->jit_requested &&
	       bpf_jit_supports_subprog_tailcalls();
}

static int check_map_func_compatibility(struct bpf_verifier_env *env,
					struct bpf_map *map, int func_id)
{
	if (!map)
		return 0;