// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 129/132



	verbose(env, "fd %d is not pointing to valid bpf_map or btf\n", fd);
	return PTR_ERR(map);
}

static int process_fd_array(struct bpf_verifier_env *env, union bpf_attr *attr, bpfptr_t uattr)
{
	size_t size = sizeof(int);
	int ret;
	int fd;
	u32 i;

	env->fd_array = make_bpfptr(attr->fd_array, uattr.is_kernel);

	/*
	 * The only difference between old (no fd_array_cnt is given) and new
	 * APIs is that in the latter case the fd_array is expected to be
	 * continuous and is scanned for map fds right away
	 */
	if (!attr->fd_array_cnt)
		return 0;

	/* Check for integer overflow */
	if (attr->fd_array_cnt >= (U32_MAX / size)) {
		verbose(env, "fd_array_cnt is too big (%u)\n", attr->fd_array_cnt);
		return -EINVAL;
	}

	for (i = 0; i < attr->fd_array_cnt; i++) {
		if (copy_from_bpfptr_offset(&fd, env->fd_array, i * size, size))
			return -EFAULT;

		ret = add_fd_from_fd_array(env, fd);
		if (ret)
			return ret;
	}

	return 0;
}

/* replace a generic kfunc with a specialized version if necessary */
static int specialize_kfunc(struct bpf_verifier_env *env, struct bpf_kfunc_desc *desc, int insn_idx)
{
	struct bpf_prog *prog = env->prog;
	bool seen_direct_write;
	void *xdp_kfunc;
	bool is_rdonly;
	u32 func_id = desc->func_id;
	u16 offset = desc->offset;
	unsigned long addr = desc->addr;

	if (offset) /* return if module BTF is used */
		return 0;

	if (bpf_dev_bound_kfunc_id(func_id)) {
		xdp_kfunc = bpf_dev_bound_resolve_kfunc(prog, func_id);
		if (xdp_kfunc)
			addr = (unsigned long)xdp_kfunc;
		/* fallback to default kfunc when not supported by netdev */
	} else if (func_id == special_kfunc_list[KF_bpf_dynptr_from_skb]) {
		seen_direct_write = env->seen_direct_write;
		is_rdonly = !may_access_direct_pkt_data(env, NULL, BPF_WRITE);

		if (is_rdonly)
			addr = (unsigned long)bpf_dynptr_from_skb_rdonly;

		/* restore env->seen_direct_write to its original value, since
		 * may_access_direct_pkt_data mutates it
		 */
		env->seen_direct_write = seen_direct_write;
	} else if (func_id == special_kfunc_list[KF_bpf_set_dentry_xattr]) {
		if (bpf_lsm_has_d_inode_locked(prog))
			addr = (unsigned long)bpf_set_dentry_xattr_locked;
	} else if (func_id == special_kfunc_list[KF_bpf_remove_dentry_xattr]) {
		if (bpf_lsm_has_d_inode_locked(prog))
			addr = (unsigned long)bpf_remove_dentry_xattr_locked;
	} else if (func_id == special_kfunc_list[KF_bpf_dynptr_from_file]) {
		if (!env->insn_aux_data[insn_idx].non_sleepable)
			addr = (unsigned long)bpf_dynptr_from_file_sleepable;
	} else if (func_id == special_kfunc_list[KF_bpf_arena_alloc_pages]) {
		if (env->insn_aux_data[insn_idx].non_sleepable)
			addr = (unsigned long)bpf_arena_alloc_pages_non_sleepable;
	} else if (func_id == special_kfunc_list[KF_bpf_arena_free_pages]) {
		if (env->insn_aux_data[insn_idx].non_sleepable)
			addr = (unsigned long)bpf_arena_free_pages_non_sleepable;
	}
	desc->addr = addr;
	return 0;
}

static void __fixup_collection_insert_kfunc(struct bpf_insn_aux_data *insn_aux,
					    u16 struct_meta_reg,
					    u16 node_offset_reg,
					    struct bpf_insn *insn,
					    struct bpf_insn *insn_buf,
					    int *cnt)
{
	struct btf_struct_meta *kptr_struct_meta = insn_aux->kptr_struct_meta;
	struct bpf_insn addr[2] = { BPF_LD_IMM64(struct_meta_reg, (long)kptr_struct_meta) };

	insn_buf[0] = addr[0];
	insn_buf[1] = addr[1];
	insn_buf[2] = BPF_MOV64_IMM(node_offset_reg, insn_aux->insert_off);
	insn_buf[3] = *insn;
	*cnt = 4;
}

int bpf_fixup_kfunc_call(struct bpf_verifier_env *env, struct bpf_insn *insn,
		     struct bpf_insn *insn_buf, int insn_idx, int *cnt)
{
	struct bpf_kfunc_desc *desc;
	int err;

	if (!insn->imm) {
		verbose(env, "invalid kernel function call not eliminated in verifier pass\n");
		return -EINVAL;
	}

	*cnt = 0;

	/* insn->imm has the btf func_id. Replace it with an offset relative to
	 * __bpf_call_base, unless the JIT needs to call functions that are
	 * further than 32 bits away (bpf_jit_supports_far_kfunc_call()).
	 */
	desc = find_kfunc_desc(env->prog, insn->imm, insn->off);
	if (!desc) {
		verifier_bug(env, "kernel function descriptor not found for func_id %u",
			     insn->imm);
		return -EFAULT;
	}

	err = specialize_kfunc(env, desc, insn_idx);
	if (err)
		return err;

	if (!bpf_jit_supports_far_kfunc_call())
		insn->imm = BPF_CALL_IMM(desc->addr);

	if (is_bpf_obj_new_kfunc(desc->func_id) || is_bpf_percpu_obj_new_kfunc(desc->func_id)) {
		struct btf_struct_meta *kptr_struct_meta = env->insn_aux_data[insn_idx].kptr_struct_meta;
		struct bpf_insn addr[2] = { BPF_LD_IMM64(BPF_REG_2, (long)kptr_struct_meta) };
		u64 obj_new_size = env->insn_aux_data[insn_idx].obj_new_size;

		if (is_bpf_percpu_obj_new_kfunc(desc->func_id) && kptr_struct_meta) {
			verifier_bug(env, "NULL kptr_struct_meta expected at insn_idx %d",
				     insn_idx);
			return -EFAULT;
		}