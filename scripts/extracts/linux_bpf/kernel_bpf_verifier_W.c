// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 55/132



			if (!btf_struct_ids_match(&env->log, reg->btf, reg->btf_id,
						  reg->var_off.value, btf_vmlinux, *arg_btf_id,
						  strict_type_match)) {
				verbose(env, "R%d is of type %s but %s is expected\n",
					regno, btf_type_name(reg->btf, reg->btf_id),
					btf_type_name(btf_vmlinux, *arg_btf_id));
				return -EACCES;
			}
		}
		break;
	}
	case PTR_TO_BTF_ID | MEM_ALLOC:
	case PTR_TO_BTF_ID | MEM_PERCPU | MEM_ALLOC:
	case PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF:
	case PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF | MEM_RCU:
		if (meta->func_id != BPF_FUNC_spin_lock && meta->func_id != BPF_FUNC_spin_unlock &&
		    meta->func_id != BPF_FUNC_kptr_xchg) {
			verifier_bug(env, "unimplemented handling of MEM_ALLOC");
			return -EFAULT;
		}
		/* Check if local kptr in src arg matches kptr in dst arg */
		if (meta->func_id == BPF_FUNC_kptr_xchg && regno == BPF_REG_2) {
			if (map_kptr_match_type(env, meta->kptr_field, reg, regno))
				return -EACCES;
		}
		break;
	case PTR_TO_BTF_ID | MEM_PERCPU:
	case PTR_TO_BTF_ID | MEM_PERCPU | MEM_RCU:
	case PTR_TO_BTF_ID | MEM_PERCPU | PTR_TRUSTED:
		/* Handled by helper specific checks */
		break;
	default:
		verifier_bug(env, "invalid PTR_TO_BTF_ID register for type match");
		return -EFAULT;
	}
	return 0;
}

static struct btf_field *
reg_find_field_offset(const struct bpf_reg_state *reg, s32 off, u32 fields)
{
	struct btf_field *field;
	struct btf_record *rec;

	rec = reg_btf_record(reg);
	if (!rec)
		return NULL;

	field = btf_record_find(rec, off, fields);
	if (!field)
		return NULL;

	return field;
}

static int check_func_arg_reg_off(struct bpf_verifier_env *env,
				  const struct bpf_reg_state *reg, int regno,
				  enum bpf_arg_type arg_type)
{
	u32 type = reg->type;

	/* When referenced register is passed to release function, its fixed
	 * offset must be 0.
	 *
	 * We will check arg_type_is_release reg has ref_obj_id when storing
	 * meta->release_regno.
	 */
	if (arg_type_is_release(arg_type)) {
		/* ARG_PTR_TO_DYNPTR with OBJ_RELEASE is a bit special, as it
		 * may not directly point to the object being released, but to
		 * dynptr pointing to such object, which might be at some offset
		 * on the stack. In that case, we simply to fallback to the
		 * default handling.
		 */
		if (arg_type_is_dynptr(arg_type) && type == PTR_TO_STACK)
			return 0;

		/* Doing check_ptr_off_reg check for the offset will catch this
		 * because fixed_off_ok is false, but checking here allows us
		 * to give the user a better error message.
		 */
		if (!tnum_is_const(reg->var_off) || reg->var_off.value != 0) {
			verbose(env, "R%d must have zero offset when passed to release func or trusted arg to kfunc\n",
				regno);
			return -EINVAL;
		}
	}

	switch (type) {
	/* Pointer types where both fixed and variable offset is explicitly allowed: */
	case PTR_TO_STACK:
	case PTR_TO_PACKET:
	case PTR_TO_PACKET_META:
	case PTR_TO_MAP_KEY:
	case PTR_TO_MAP_VALUE:
	case PTR_TO_MEM:
	case PTR_TO_MEM | MEM_RDONLY:
	case PTR_TO_MEM | MEM_RINGBUF:
	case PTR_TO_BUF:
	case PTR_TO_BUF | MEM_RDONLY:
	case PTR_TO_ARENA:
	case SCALAR_VALUE:
		return 0;
	/* All the rest must be rejected, except PTR_TO_BTF_ID which allows
	 * fixed offset.
	 */
	case PTR_TO_BTF_ID:
	case PTR_TO_BTF_ID | MEM_ALLOC:
	case PTR_TO_BTF_ID | PTR_TRUSTED:
	case PTR_TO_BTF_ID | MEM_RCU:
	case PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF:
	case PTR_TO_BTF_ID | MEM_ALLOC | NON_OWN_REF | MEM_RCU:
		/* When referenced PTR_TO_BTF_ID is passed to release function,
		 * its fixed offset must be 0. In the other cases, fixed offset
		 * can be non-zero. This was already checked above. So pass
		 * fixed_off_ok as true to allow fixed offset for all other
		 * cases. var_off always must be 0 for PTR_TO_BTF_ID, hence we
		 * still need to do checks instead of returning.
		 */
		return __check_ptr_off_reg(env, reg, regno, true);
	case PTR_TO_CTX:
		/*
		 * Allow fixed and variable offsets for syscall context, but
		 * only when the argument is passed as memory, not ctx,
		 * otherwise we may get modified ctx in tail called programs and
		 * global subprogs (that may act as extension prog hooks).
		 */
		if (arg_type != ARG_PTR_TO_CTX && is_var_ctx_off_allowed(env->prog))
			return 0;
		fallthrough;
	default:
		return __check_ptr_off_reg(env, reg, regno, false);
	}
}

static struct bpf_reg_state *get_dynptr_arg_reg(struct bpf_verifier_env *env,
						const struct bpf_func_proto *fn,
						struct bpf_reg_state *regs)
{
	struct bpf_reg_state *state = NULL;
	int i;

	for (i = 0; i < MAX_BPF_FUNC_REG_ARGS; i++)
		if (arg_type_is_dynptr(fn->arg_type[i])) {
			if (state) {
				verbose(env, "verifier internal error: multiple dynptr args\n");
				return NULL;
			}
			state = &regs[BPF_REG_1 + i];
		}

	if (!state)
		verbose(env, "verifier internal error: no dynptr arg found\n");

	return state;
}

static int dynptr_id(struct bpf_verifier_env *env, struct bpf_reg_state *reg)
{
	struct bpf_func_state *state = bpf_func(env, reg);
	int spi;