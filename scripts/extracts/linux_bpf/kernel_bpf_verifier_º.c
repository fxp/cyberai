// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 122/132



			/* In final convert_pseudo_ld_imm64() step, this is
			 * converted into regular 64-bit imm load insn.
			 */
			switch (insn[0].src_reg) {
			case BPF_PSEUDO_MAP_VALUE:
			case BPF_PSEUDO_MAP_IDX_VALUE:
				break;
			case BPF_PSEUDO_MAP_FD:
			case BPF_PSEUDO_MAP_IDX:
				if (insn[1].imm == 0)
					break;
				fallthrough;
			default:
				verbose(env, "unrecognized bpf_ld_imm64 insn\n");
				return -EINVAL;
			}

			switch (insn[0].src_reg) {
			case BPF_PSEUDO_MAP_IDX_VALUE:
			case BPF_PSEUDO_MAP_IDX:
				if (bpfptr_is_null(env->fd_array)) {
					verbose(env, "fd_idx without fd_array is invalid\n");
					return -EPROTO;
				}
				if (copy_from_bpfptr_offset(&fd, env->fd_array,
							    insn[0].imm * sizeof(fd),
							    sizeof(fd)))
					return -EFAULT;
				break;
			default:
				fd = insn[0].imm;
				break;
			}

			map_idx = add_used_map(env, fd);
			if (map_idx < 0)
				return map_idx;
			map = env->used_maps[map_idx];

			aux = &env->insn_aux_data[i];
			aux->map_index = map_idx;

			if (insn[0].src_reg == BPF_PSEUDO_MAP_FD ||
			    insn[0].src_reg == BPF_PSEUDO_MAP_IDX) {
				addr = (unsigned long)map;
			} else {
				u32 off = insn[1].imm;

				if (!map->ops->map_direct_value_addr) {
					verbose(env, "no direct value access support for this map type\n");
					return -EINVAL;
				}

				err = map->ops->map_direct_value_addr(map, &addr, off);
				if (err) {
					verbose(env, "invalid access to map value pointer, value_size=%u off=%u\n",
						map->value_size, off);
					return err;
				}

				aux->map_off = off;
				addr += off;
			}

			insn[0].imm = (u32)addr;
			insn[1].imm = addr >> 32;

next_insn:
			insn++;
			i++;
			continue;
		}

		/* Basic sanity check before we invest more work here. */
		if (!bpf_opcode_in_insntable(insn->code)) {
			verbose(env, "unknown opcode %02x\n", insn->code);
			return -EINVAL;
		}

		err = check_insn_fields(env, insn);
		if (err)
			return err;
	}

	/* now all pseudo BPF_LD_IMM64 instructions load valid
	 * 'struct bpf_map *' into a register instead of user map_fd.
	 * These pointers will be used later by verifier to validate map access.
	 */
	return 0;
}

/* drop refcnt of maps used by the rejected program */
static void release_maps(struct bpf_verifier_env *env)
{
	__bpf_free_used_maps(env->prog->aux, env->used_maps,
			     env->used_map_cnt);
}

/* drop refcnt of maps used by the rejected program */
static void release_btfs(struct bpf_verifier_env *env)
{
	__bpf_free_used_btfs(env->used_btfs, env->used_btf_cnt);
}

/* convert pseudo BPF_LD_IMM64 into generic BPF_LD_IMM64 */
static void convert_pseudo_ld_imm64(struct bpf_verifier_env *env)
{
	struct bpf_insn *insn = env->prog->insnsi;
	int insn_cnt = env->prog->len;
	int i;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->code != (BPF_LD | BPF_IMM | BPF_DW))
			continue;
		if (insn->src_reg == BPF_PSEUDO_FUNC)
			continue;
		insn->src_reg = 0;
	}
}

static void release_insn_arrays(struct bpf_verifier_env *env)
{
	int i;

	for (i = 0; i < env->insn_array_map_cnt; i++)
		bpf_insn_array_release(env->insn_array_maps[i]);
}



/* The verifier does more data flow analysis than llvm and will not
 * explore branches that are dead at run time. Malicious programs can
 * have dead code too. Therefore replace all dead at-run-time code
 * with 'ja -1'.
 *
 * Just nops are not optimal, e.g. if they would sit at the end of the
 * program and through another bug we would manage to jump there, then
 * we'd execute beyond program memory otherwise. Returning exception
 * code also wouldn't work since we can have subprogs where the dead
 * code could be located.
 */
static void sanitize_dead_code(struct bpf_verifier_env *env)
{
	struct bpf_insn_aux_data *aux_data = env->insn_aux_data;
	struct bpf_insn trap = BPF_JMP_IMM(BPF_JA, 0, 0, -1);
	struct bpf_insn *insn = env->prog->insnsi;
	const int insn_cnt = env->prog->len;
	int i;

	for (i = 0; i < insn_cnt; i++) {
		if (aux_data[i].seen)
			continue;
		memcpy(insn + i, &trap, sizeof(trap));
		aux_data[i].zext_dst = false;
	}
}



static void free_states(struct bpf_verifier_env *env)
{
	struct bpf_verifier_state_list *sl;
	struct list_head *head, *pos, *tmp;
	struct bpf_scc_info *info;
	int i, j;

	bpf_free_verifier_state(env->cur_state, true);
	env->cur_state = NULL;
	while (!pop_stack(env, NULL, NULL, false));

	list_for_each_safe(pos, tmp, &env->free_list) {
		sl = container_of(pos, struct bpf_verifier_state_list, node);
		bpf_free_verifier_state(&sl->state, false);
		kfree(sl);
	}
	INIT_LIST_HEAD(&env->free_list);

	for (i = 0; i < env->scc_cnt; ++i) {
		info = env->scc_info[i];
		if (!info)
			continue;
		for (j = 0; j < info->num_visits; j++)
			bpf_free_backedges(&info->visits[j]);
		kvfree(info);
		env->scc_info[i] = NULL;
	}

	if (!env->explored_states)
		return;

	for (i = 0; i < state_htab_size(env); i++) {
		head = &env->explored_states[i];