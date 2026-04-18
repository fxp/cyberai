// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 57/132



	if (bpf_register_is_null(reg) && type_may_be_null(arg_type))
		/* A NULL register has a SCALAR_VALUE type, so skip
		 * type checking.
		 */
		goto skip_type_check;

	/* arg_btf_id and arg_size are in a union. */
	if (base_type(arg_type) == ARG_PTR_TO_BTF_ID ||
	    base_type(arg_type) == ARG_PTR_TO_SPIN_LOCK)
		arg_btf_id = fn->arg_btf_id[arg];

	err = check_reg_type(env, regno, arg_type, arg_btf_id, meta);
	if (err)
		return err;

	err = check_func_arg_reg_off(env, reg, regno, arg_type);
	if (err)
		return err;

skip_type_check:
	if (arg_type_is_release(arg_type)) {
		if (arg_type_is_dynptr(arg_type)) {
			struct bpf_func_state *state = bpf_func(env, reg);
			int spi;

			/* Only dynptr created on stack can be released, thus
			 * the get_spi and stack state checks for spilled_ptr
			 * should only be done before process_dynptr_func for
			 * PTR_TO_STACK.
			 */
			if (reg->type == PTR_TO_STACK) {
				spi = dynptr_get_spi(env, reg);
				if (spi < 0 || !state->stack[spi].spilled_ptr.ref_obj_id) {
					verbose(env, "arg %d is an unacquired reference\n", regno);
					return -EINVAL;
				}
			} else {
				verbose(env, "cannot release unowned const bpf_dynptr\n");
				return -EINVAL;
			}
		} else if (!reg->ref_obj_id && !bpf_register_is_null(reg)) {
			verbose(env, "R%d must be referenced when passed to release function\n",
				regno);
			return -EINVAL;
		}
		if (meta->release_regno) {
			verifier_bug(env, "more than one release argument");
			return -EFAULT;
		}
		meta->release_regno = regno;
	}

	if (reg->ref_obj_id && base_type(arg_type) != ARG_KPTR_XCHG_DEST) {
		if (meta->ref_obj_id) {
			verbose(env, "more than one arg with ref_obj_id R%d %u %u",
				regno, reg->ref_obj_id,
				meta->ref_obj_id);
			return -EACCES;
		}
		meta->ref_obj_id = reg->ref_obj_id;
	}

	switch (base_type(arg_type)) {
	case ARG_CONST_MAP_PTR:
		/* bpf_map_xxx(map_ptr) call: remember that map_ptr */
		if (meta->map.ptr) {
			/* Use map_uid (which is unique id of inner map) to reject:
			 * inner_map1 = bpf_map_lookup_elem(outer_map, key1)
			 * inner_map2 = bpf_map_lookup_elem(outer_map, key2)
			 * if (inner_map1 && inner_map2) {
			 *     timer = bpf_map_lookup_elem(inner_map1);
			 *     if (timer)
			 *         // mismatch would have been allowed
			 *         bpf_timer_init(timer, inner_map2);
			 * }
			 *
			 * Comparing map_ptr is enough to distinguish normal and outer maps.
			 */
			if (meta->map.ptr != reg->map_ptr ||
			    meta->map.uid != reg->map_uid) {
				verbose(env,
					"timer pointer in R1 map_uid=%d doesn't match map pointer in R2 map_uid=%d\n",
					meta->map.uid, reg->map_uid);
				return -EINVAL;
			}
		}
		meta->map.ptr = reg->map_ptr;
		meta->map.uid = reg->map_uid;
		break;
	case ARG_PTR_TO_MAP_KEY:
		/* bpf_map_xxx(..., map_ptr, ..., key) call:
		 * check that [key, key + map->key_size) are within
		 * stack limits and initialized
		 */
		if (!meta->map.ptr) {
			/* in function declaration map_ptr must come before
			 * map_key, so that it's verified and known before
			 * we have to check map_key here. Otherwise it means
			 * that kernel subsystem misconfigured verifier
			 */
			verifier_bug(env, "invalid map_ptr to access map->key");
			return -EFAULT;
		}
		key_size = meta->map.ptr->key_size;
		err = check_helper_mem_access(env, regno, key_size, BPF_READ, false, NULL);
		if (err)
			return err;
		if (can_elide_value_nullness(meta->map.ptr->map_type)) {
			err = get_constant_map_key(env, reg, key_size, &meta->const_map_key);
			if (err < 0) {
				meta->const_map_key = -1;
				if (err == -EOPNOTSUPP)
					err = 0;
				else
					return err;
			}
		}
		break;
	case ARG_PTR_TO_MAP_VALUE:
		if (type_may_be_null(arg_type) && bpf_register_is_null(reg))
			return 0;