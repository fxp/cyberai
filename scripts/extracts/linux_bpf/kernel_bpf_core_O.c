// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 15/20



static bool __bpf_prog_map_compatible(struct bpf_map *map,
				      const struct bpf_prog *fp)
{
	enum bpf_prog_type prog_type = resolve_prog_type(fp);
	struct bpf_prog_aux *aux = fp->aux;
	enum bpf_cgroup_storage_type i;
	bool ret = false;
	u64 cookie;

	if (fp->kprobe_override)
		return ret;

	spin_lock(&map->owner_lock);
	/* There's no owner yet where we could check for compatibility. */
	if (!map->owner) {
		map->owner = bpf_map_owner_alloc(map);
		if (!map->owner)
			goto err;
		map->owner->type  = prog_type;
		map->owner->jited = fp->jited;
		map->owner->xdp_has_frags = aux->xdp_has_frags;
		map->owner->sleepable = fp->sleepable;
		map->owner->expected_attach_type = fp->expected_attach_type;
		map->owner->attach_func_proto = aux->attach_func_proto;
		for_each_cgroup_storage_type(i) {
			map->owner->storage_cookie[i] =
				aux->cgroup_storage[i] ?
				aux->cgroup_storage[i]->cookie : 0;
		}
		ret = true;
	} else {
		ret = map->owner->type  == prog_type &&
		      map->owner->jited == fp->jited &&
		      map->owner->xdp_has_frags == aux->xdp_has_frags &&
		      map->owner->sleepable == fp->sleepable;
		if (ret &&
		    map->map_type == BPF_MAP_TYPE_PROG_ARRAY &&
		    map->owner->expected_attach_type != fp->expected_attach_type)
			ret = false;
		for_each_cgroup_storage_type(i) {
			if (!ret)
				break;
			cookie = aux->cgroup_storage[i] ?
				 aux->cgroup_storage[i]->cookie : 0;
			ret = map->owner->storage_cookie[i] == cookie ||
			      !cookie;
		}
		if (ret &&
		    map->owner->attach_func_proto != aux->attach_func_proto) {
			switch (prog_type) {
			case BPF_PROG_TYPE_TRACING:
			case BPF_PROG_TYPE_LSM:
			case BPF_PROG_TYPE_EXT:
			case BPF_PROG_TYPE_STRUCT_OPS:
				ret = false;
				break;
			default:
				break;
			}
		}
	}
err:
	spin_unlock(&map->owner_lock);
	return ret;
}

bool bpf_prog_map_compatible(struct bpf_map *map, const struct bpf_prog *fp)
{
	/* XDP programs inserted into maps are not guaranteed to run on
	 * a particular netdev (and can run outside driver context entirely
	 * in the case of devmap and cpumap). Until device checks
	 * are implemented, prohibit adding dev-bound programs to program maps.
	 */
	if (bpf_prog_is_dev_bound(fp->aux))
		return false;

	return __bpf_prog_map_compatible(map, fp);
}

static int bpf_check_tail_call(const struct bpf_prog *fp)
{
	struct bpf_prog_aux *aux = fp->aux;
	int i, ret = 0;

	mutex_lock(&aux->used_maps_mutex);
	for (i = 0; i < aux->used_map_cnt; i++) {
		struct bpf_map *map = aux->used_maps[i];

		if (!map_type_contains_progs(map))
			continue;

		if (!__bpf_prog_map_compatible(map, fp)) {
			ret = -EINVAL;
			goto out;
		}
	}

out:
	mutex_unlock(&aux->used_maps_mutex);
	return ret;
}

static bool bpf_prog_select_interpreter(struct bpf_prog *fp)
{
	bool select_interpreter = false;
#ifndef CONFIG_BPF_JIT_ALWAYS_ON
	u32 stack_depth = max_t(u32, fp->aux->stack_depth, 1);
	u32 idx = (round_up(stack_depth, 32) / 32) - 1;

	/* may_goto may cause stack size > 512, leading to idx out-of-bounds.
	 * But for non-JITed programs, we don't need bpf_func, so no bounds
	 * check needed.
	 */
	if (idx < ARRAY_SIZE(interpreters)) {
		fp->bpf_func = interpreters[idx];
		select_interpreter = true;
	} else {
		fp->bpf_func = __bpf_prog_ret0_warn;
	}
#else
	fp->bpf_func = __bpf_prog_ret0_warn;
#endif
	return select_interpreter;
}

static struct bpf_prog *bpf_prog_jit_compile(struct bpf_verifier_env *env, struct bpf_prog *prog)
{
#ifdef CONFIG_BPF_JIT
	struct bpf_prog *orig_prog;
	struct bpf_insn_aux_data *orig_insn_aux;

	if (!bpf_prog_need_blind(prog))
		return bpf_int_jit_compile(env, prog);

	if (env) {
		/*
		 * If env is not NULL, we are called from the end of bpf_check(), at this
		 * point, only insn_aux_data is used after failure, so it should be restored
		 * on failure.
		 */
		orig_insn_aux = bpf_dup_insn_aux_data(env);
		if (!orig_insn_aux)
			return prog;
	}

	orig_prog = prog;
	prog = bpf_jit_blind_constants(env, prog);
	/*
	 * If blinding was requested and we failed during blinding, we must fall
	 * back to the interpreter.
	 */
	if (IS_ERR(prog))
		goto out_restore;

	prog = bpf_int_jit_compile(env, prog);
	if (prog->jited) {
		bpf_jit_prog_release_other(prog, orig_prog);
		if (env)
			vfree(orig_insn_aux);
		return prog;
	}

	bpf_jit_prog_release_other(orig_prog, prog);

out_restore:
	prog = orig_prog;
	if (env)
		bpf_restore_insn_aux_data(env, orig_insn_aux);
#endif
	return prog;
}

struct bpf_prog *__bpf_prog_select_runtime(struct bpf_verifier_env *env, struct bpf_prog *fp,
					   int *err)
{
	/* In case of BPF to BPF calls, verifier did all the prep
	 * work with regards to JITing, etc.
	 */
	bool jit_needed = false;

	if (fp->bpf_func)
		goto finalize;

	if (IS_ENABLED(CONFIG_BPF_JIT_ALWAYS_ON) ||
	    bpf_prog_has_kfunc_call(fp))
		jit_needed = true;

	if (!bpf_prog_select_interpreter(fp))
		jit_needed = true;