// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 33/132



	/* We only allow loading referenced kptr, since it will be marked as
	 * untrusted, similar to unreferenced kptr.
	 */
	if (class != BPF_LDX &&
	    (kptr_field->type == BPF_KPTR_REF || kptr_field->type == BPF_KPTR_PERCPU)) {
		verbose(env, "store to referenced kptr disallowed\n");
		return -EACCES;
	}
	if (class != BPF_LDX && kptr_field->type == BPF_UPTR) {
		verbose(env, "store to uptr disallowed\n");
		return -EACCES;
	}

	if (class == BPF_LDX) {
		if (kptr_field->type == BPF_UPTR)
			return mark_uptr_ld_reg(env, value_regno, kptr_field);

		/* We can simply mark the value_regno receiving the pointer
		 * value from map as PTR_TO_BTF_ID, with the correct type.
		 */
		ret = mark_btf_ld_reg(env, cur_regs(env), value_regno, PTR_TO_BTF_ID,
				      kptr_field->kptr.btf, kptr_field->kptr.btf_id,
				      btf_ld_kptr_type(env, kptr_field));
		if (ret < 0)
			return ret;
	} else if (class == BPF_STX) {
		val_reg = reg_state(env, value_regno);
		if (!bpf_register_is_null(val_reg) &&
		    map_kptr_match_type(env, kptr_field, val_reg, value_regno))
			return -EACCES;
	} else if (class == BPF_ST) {
		if (insn->imm) {
			verbose(env, "BPF_ST imm must be 0 when storing to kptr at off=%u\n",
				kptr_field->offset);
			return -EACCES;
		}
	} else {
		verbose(env, "kptr in map can only be accessed using BPF_LDX/BPF_STX/BPF_ST\n");
		return -EACCES;
	}
	return 0;
}

/*
 * Return the size of the memory region accessible from a pointer to map value.
 * For INSN_ARRAY maps whole bpf_insn_array->ips array is accessible.
 */
static u32 map_mem_size(const struct bpf_map *map)
{
	if (map->map_type == BPF_MAP_TYPE_INSN_ARRAY)
		return map->max_entries * sizeof(long);

	return map->value_size;
}

/* check read/write into a map element with possible variable offset */
static int check_map_access(struct bpf_verifier_env *env, u32 regno,
			    int off, int size, bool zero_size_allowed,
			    enum bpf_access_src src)
{
	struct bpf_verifier_state *vstate = env->cur_state;
	struct bpf_func_state *state = vstate->frame[vstate->curframe];
	struct bpf_reg_state *reg = &state->regs[regno];
	struct bpf_map *map = reg->map_ptr;
	u32 mem_size = map_mem_size(map);
	struct btf_record *rec;
	int err, i;

	err = check_mem_region_access(env, regno, off, size, mem_size, zero_size_allowed);
	if (err)
		return err;

	if (IS_ERR_OR_NULL(map->record))
		return 0;
	rec = map->record;
	for (i = 0; i < rec->cnt; i++) {
		struct btf_field *field = &rec->fields[i];
		u32 p = field->offset;

		/* If any part of a field  can be touched by load/store, reject
		 * this program. To check that [x1, x2) overlaps with [y1, y2),
		 * it is sufficient to check x1 < y2 && y1 < x2.
		 */
		if (reg->smin_value + off < p + field->size &&
		    p < reg->umax_value + off + size) {
			switch (field->type) {
			case BPF_KPTR_UNREF:
			case BPF_KPTR_REF:
			case BPF_KPTR_PERCPU:
			case BPF_UPTR:
				if (src != ACCESS_DIRECT) {
					verbose(env, "%s cannot be accessed indirectly by helper\n",
						btf_field_type_name(field->type));
					return -EACCES;
				}
				if (!tnum_is_const(reg->var_off)) {
					verbose(env, "%s access cannot have variable offset\n",
						btf_field_type_name(field->type));
					return -EACCES;
				}
				if (p != off + reg->var_off.value) {
					verbose(env, "%s access misaligned expected=%u off=%llu\n",
						btf_field_type_name(field->type),
						p, off + reg->var_off.value);
					return -EACCES;
				}
				if (size != bpf_size_to_bytes(BPF_DW)) {
					verbose(env, "%s access size must be BPF_DW\n",
						btf_field_type_name(field->type));
					return -EACCES;
				}
				break;
			default:
				verbose(env, "%s cannot be accessed directly by load/store\n",
					btf_field_type_name(field->type));
				return -EACCES;
			}
		}
	}
	return 0;
}

static bool may_access_direct_pkt_data(struct bpf_verifier_env *env,
			       const struct bpf_call_arg_meta *meta,
			       enum bpf_access_type t)
{
	enum bpf_prog_type prog_type = resolve_prog_type(env->prog);

	switch (prog_type) {
	/* Program types only with direct read access go here! */
	case BPF_PROG_TYPE_LWT_IN:
	case BPF_PROG_TYPE_LWT_OUT:
	case BPF_PROG_TYPE_LWT_SEG6LOCAL:
	case BPF_PROG_TYPE_SK_REUSEPORT:
	case BPF_PROG_TYPE_FLOW_DISSECTOR:
	case BPF_PROG_TYPE_CGROUP_SKB:
		if (t == BPF_WRITE)
			return false;
		fallthrough;

	/* Program types with direct read + write access go here! */
	case BPF_PROG_TYPE_SCHED_CLS:
	case BPF_PROG_TYPE_SCHED_ACT:
	case BPF_PROG_TYPE_XDP:
	case BPF_PROG_TYPE_LWT_XMIT:
	case BPF_PROG_TYPE_SK_SKB:
	case BPF_PROG_TYPE_SK_MSG:
		if (meta)
			return meta->pkt_access;

		env->seen_direct_write = true;
		return true;

	case BPF_PROG_TYPE_CGROUP_SOCKOPT:
		if (t == BPF_WRITE)
			env->seen_direct_write = true;

		return true;

	default:
		return false;
	}
}

static int check_packet_access(struct bpf_verifier_env *env, u32 regno, int off,
			       int size, bool zero_size_allowed)
{
	struct bpf_reg_state *reg = reg_state(env, regno);
	int err;