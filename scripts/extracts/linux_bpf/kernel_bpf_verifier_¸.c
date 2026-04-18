// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 120/132



	if (map->map_type == BPF_MAP_TYPE_ARENA) {
		if (env->prog->aux->arena) {
			verbose(env, "Only one arena per program\n");
			return -EBUSY;
		}
		if (!env->allow_ptr_leaks || !env->bpf_capable) {
			verbose(env, "CAP_BPF and CAP_PERFMON are required to use arena\n");
			return -EPERM;
		}
		if (!env->prog->jit_requested) {
			verbose(env, "JIT is required to use arena\n");
			return -EOPNOTSUPP;
		}
		if (!bpf_jit_supports_arena()) {
			verbose(env, "JIT doesn't support arena\n");
			return -EOPNOTSUPP;
		}
		env->prog->aux->arena = (void *)map;
		if (!bpf_arena_get_user_vm_start(env->prog->aux->arena)) {
			verbose(env, "arena's user address must be set via map_extra or mmap()\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int __add_used_map(struct bpf_verifier_env *env, struct bpf_map *map)
{
	int i, err;

	/* check whether we recorded this map already */
	for (i = 0; i < env->used_map_cnt; i++)
		if (env->used_maps[i] == map)
			return i;

	if (env->used_map_cnt >= MAX_USED_MAPS) {
		verbose(env, "The total number of maps per program has reached the limit of %u\n",
			MAX_USED_MAPS);
		return -E2BIG;
	}

	err = check_map_prog_compatibility(env, map, env->prog);
	if (err)
		return err;

	if (env->prog->sleepable)
		atomic64_inc(&map->sleepable_refcnt);

	/* hold the map. If the program is rejected by verifier,
	 * the map will be released by release_maps() or it
	 * will be used by the valid program until it's unloaded
	 * and all maps are released in bpf_free_used_maps()
	 */
	bpf_map_inc(map);

	env->used_maps[env->used_map_cnt++] = map;

	if (map->map_type == BPF_MAP_TYPE_INSN_ARRAY) {
		err = bpf_insn_array_init(map, env->prog);
		if (err) {
			verbose(env, "Failed to properly initialize insn array\n");
			return err;
		}
		env->insn_array_maps[env->insn_array_map_cnt++] = map;
	}

	return env->used_map_cnt - 1;
}

/* Add map behind fd to used maps list, if it's not already there, and return
 * its index.
 * Returns <0 on error, or >= 0 index, on success.
 */
static int add_used_map(struct bpf_verifier_env *env, int fd)
{
	struct bpf_map *map;
	CLASS(fd, f)(fd);

	map = __bpf_map_get(f);
	if (IS_ERR(map)) {
		verbose(env, "fd %d is not pointing to valid bpf_map\n", fd);
		return PTR_ERR(map);
	}

	return __add_used_map(env, map);
}

static int check_alu_fields(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	u8 class = BPF_CLASS(insn->code);
	u8 opcode = BPF_OP(insn->code);

	switch (opcode) {
	case BPF_NEG:
		if (BPF_SRC(insn->code) != BPF_K || insn->src_reg != BPF_REG_0 ||
		    insn->off != 0 || insn->imm != 0) {
			verbose(env, "BPF_NEG uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_END:
		if (insn->src_reg != BPF_REG_0 || insn->off != 0 ||
		    (insn->imm != 16 && insn->imm != 32 && insn->imm != 64) ||
		    (class == BPF_ALU64 && BPF_SRC(insn->code) != BPF_TO_LE)) {
			verbose(env, "BPF_END uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_MOV:
		if (BPF_SRC(insn->code) == BPF_X) {
			if (class == BPF_ALU) {
				if ((insn->off != 0 && insn->off != 8 && insn->off != 16) ||
				    insn->imm) {
					verbose(env, "BPF_MOV uses reserved fields\n");
					return -EINVAL;
				}
			} else if (insn->off == BPF_ADDR_SPACE_CAST) {
				if (insn->imm != 1 && insn->imm != 1u << 16) {
					verbose(env, "addr_space_cast insn can only convert between address space 1 and 0\n");
					return -EINVAL;
				}
			} else if ((insn->off != 0 && insn->off != 8 &&
				    insn->off != 16 && insn->off != 32) || insn->imm) {
				verbose(env, "BPF_MOV uses reserved fields\n");
				return -EINVAL;
			}
		} else if (insn->src_reg != BPF_REG_0 || insn->off != 0) {
			verbose(env, "BPF_MOV uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	case BPF_ADD:
	case BPF_SUB:
	case BPF_AND:
	case BPF_OR:
	case BPF_XOR:
	case BPF_LSH:
	case BPF_RSH:
	case BPF_ARSH:
	case BPF_MUL:
	case BPF_DIV:
	case BPF_MOD:
		if (BPF_SRC(insn->code) == BPF_X) {
			if (insn->imm != 0 || (insn->off != 0 && insn->off != 1) ||
			    (insn->off == 1 && opcode != BPF_MOD && opcode != BPF_DIV)) {
				verbose(env, "BPF_ALU uses reserved fields\n");
				return -EINVAL;
			}
		} else if (insn->src_reg != BPF_REG_0 ||
			   (insn->off != 0 && insn->off != 1) ||
			   (insn->off == 1 && opcode != BPF_MOD && opcode != BPF_DIV)) {
			verbose(env, "BPF_ALU uses reserved fields\n");
			return -EINVAL;
		}
		return 0;
	default:
		verbose(env, "invalid BPF_ALU opcode %x\n", opcode);
		return -EINVAL;
	}
}

static int check_jmp_fields(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	u8 class = BPF_CLASS(insn->code);
	u8 opcode = BPF_OP(insn->code);