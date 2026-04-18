// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/syscall.c
// Segment 28/37



static const struct bpf_map *bpf_map_from_imm(const struct bpf_prog *prog,
					      unsigned long addr, u32 *off,
					      u32 *type)
{
	const struct bpf_map *map;
	int i;

	mutex_lock(&prog->aux->used_maps_mutex);
	for (i = 0, *off = 0; i < prog->aux->used_map_cnt; i++) {
		map = prog->aux->used_maps[i];
		if (map == (void *)addr) {
			*type = BPF_PSEUDO_MAP_FD;
			goto out;
		}
		if (!map->ops->map_direct_value_meta)
			continue;
		if (!map->ops->map_direct_value_meta(map, addr, off)) {
			*type = BPF_PSEUDO_MAP_VALUE;
			goto out;
		}
	}
	map = NULL;

out:
	mutex_unlock(&prog->aux->used_maps_mutex);
	return map;
}

static struct bpf_insn *bpf_insn_prepare_dump(const struct bpf_prog *prog,
					      const struct cred *f_cred)
{
	const struct bpf_map *map;
	struct bpf_insn *insns;
	u32 off, type;
	u64 imm;
	u8 code;
	int i;

	insns = kmemdup(prog->insnsi, bpf_prog_insn_size(prog),
			GFP_USER);
	if (!insns)
		return insns;

	for (i = 0; i < prog->len; i++) {
		code = insns[i].code;

		if (code == (BPF_JMP | BPF_TAIL_CALL)) {
			insns[i].code = BPF_JMP | BPF_CALL;
			insns[i].imm = BPF_FUNC_tail_call;
			/* fall-through */
		}
		if (code == (BPF_JMP | BPF_CALL) ||
		    code == (BPF_JMP | BPF_CALL_ARGS)) {
			if (code == (BPF_JMP | BPF_CALL_ARGS))
				insns[i].code = BPF_JMP | BPF_CALL;
			if (!bpf_dump_raw_ok(f_cred))
				insns[i].imm = 0;
			continue;
		}
		if (BPF_CLASS(code) == BPF_LDX && BPF_MODE(code) == BPF_PROBE_MEM) {
			insns[i].code = BPF_LDX | BPF_SIZE(code) | BPF_MEM;
			continue;
		}

		if ((BPF_CLASS(code) == BPF_LDX || BPF_CLASS(code) == BPF_STX ||
		     BPF_CLASS(code) == BPF_ST) && BPF_MODE(code) == BPF_PROBE_MEM32) {
			insns[i].code = BPF_CLASS(code) | BPF_SIZE(code) | BPF_MEM;
			continue;
		}

		if (code != (BPF_LD | BPF_IMM | BPF_DW))
			continue;

		imm = ((u64)insns[i + 1].imm << 32) | (u32)insns[i].imm;
		map = bpf_map_from_imm(prog, imm, &off, &type);
		if (map) {
			insns[i].src_reg = type;
			insns[i].imm = map->id;
			insns[i + 1].imm = off;
			continue;
		}
	}

	return insns;
}

static int set_info_rec_size(struct bpf_prog_info *info)
{
	/*
	 * Ensure info.*_rec_size is the same as kernel expected size
	 *
	 * or
	 *
	 * Only allow zero *_rec_size if both _rec_size and _cnt are
	 * zero.  In this case, the kernel will set the expected
	 * _rec_size back to the info.
	 */

	if ((info->nr_func_info || info->func_info_rec_size) &&
	    info->func_info_rec_size != sizeof(struct bpf_func_info))
		return -EINVAL;

	if ((info->nr_line_info || info->line_info_rec_size) &&
	    info->line_info_rec_size != sizeof(struct bpf_line_info))
		return -EINVAL;

	if ((info->nr_jited_line_info || info->jited_line_info_rec_size) &&
	    info->jited_line_info_rec_size != sizeof(__u64))
		return -EINVAL;

	info->func_info_rec_size = sizeof(struct bpf_func_info);
	info->line_info_rec_size = sizeof(struct bpf_line_info);
	info->jited_line_info_rec_size = sizeof(__u64);

	return 0;
}

static int bpf_prog_get_info_by_fd(struct file *file,
				   struct bpf_prog *prog,
				   const union bpf_attr *attr,
				   union bpf_attr __user *uattr)
{
	struct bpf_prog_info __user *uinfo = u64_to_user_ptr(attr->info.info);
	struct btf *attach_btf = bpf_prog_get_target_btf(prog);
	struct bpf_prog_info info;
	u32 info_len = attr->info.info_len;
	struct bpf_prog_kstats stats;
	char __user *uinsns;
	u32 ulen;
	int err;

	err = bpf_check_uarg_tail_zero(USER_BPFPTR(uinfo), sizeof(info), info_len);
	if (err)
		return err;
	info_len = min_t(u32, sizeof(info), info_len);

	memset(&info, 0, sizeof(info));
	if (copy_from_user(&info, uinfo, info_len))
		return -EFAULT;

	info.type = prog->type;
	info.id = prog->aux->id;
	info.load_time = prog->aux->load_time;
	info.created_by_uid = from_kuid_munged(current_user_ns(),
					       prog->aux->user->uid);
	info.gpl_compatible = prog->gpl_compatible;

	memcpy(info.tag, prog->tag, sizeof(prog->tag));
	memcpy(info.name, prog->aux->name, sizeof(prog->aux->name));

	mutex_lock(&prog->aux->used_maps_mutex);
	ulen = info.nr_map_ids;
	info.nr_map_ids = prog->aux->used_map_cnt;
	ulen = min_t(u32, info.nr_map_ids, ulen);
	if (ulen) {
		u32 __user *user_map_ids = u64_to_user_ptr(info.map_ids);
		u32 i;

		for (i = 0; i < ulen; i++)
			if (put_user(prog->aux->used_maps[i]->id,
				     &user_map_ids[i])) {
				mutex_unlock(&prog->aux->used_maps_mutex);
				return -EFAULT;
			}
	}
	mutex_unlock(&prog->aux->used_maps_mutex);

	err = set_info_rec_size(&info);
	if (err)
		return err;

	bpf_prog_get_stats(prog, &stats);
	info.run_time_ns = stats.nsecs;
	info.run_cnt = stats.cnt;
	info.recursion_misses = stats.misses;

	info.verified_insns = prog->aux->verified_insns;
	if (prog->aux->btf)
		info.btf_id = btf_obj_id(prog->aux->btf);

	if (!bpf_capable()) {
		info.jited_prog_len = 0;
		info.xlated_prog_len = 0;
		info.nr_jited_ksyms = 0;
		info.nr_jited_func_lens = 0;
		info.nr_func_info = 0;
		info.nr_line_info = 0;
		info.nr_jited_line_info = 0;
		goto done;
	}