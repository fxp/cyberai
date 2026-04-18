// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 40/132



BTF_TYPE_SAFE_TRUSTED(struct linux_binprm) {
	struct file *file;
};

BTF_TYPE_SAFE_TRUSTED(struct file) {
	struct inode *f_inode;
};

BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct dentry) {
	struct inode *d_inode;
};

BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct socket) {
	struct sock *sk;
};

BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct vm_area_struct) {
	struct mm_struct *vm_mm;
	struct file *vm_file;
};

static bool type_is_rcu(struct bpf_verifier_env *env,
			struct bpf_reg_state *reg,
			const char *field_name, u32 btf_id)
{
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU(struct task_struct));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU(struct cgroup));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU(struct css_set));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU(struct cgroup_subsys_state));

	return btf_nested_type_is_trusted(&env->log, reg, field_name, btf_id, "__safe_rcu");
}

static bool type_is_rcu_or_null(struct bpf_verifier_env *env,
				struct bpf_reg_state *reg,
				const char *field_name, u32 btf_id)
{
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU_OR_NULL(struct mm_struct));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU_OR_NULL(struct sk_buff));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_RCU_OR_NULL(struct request_sock));

	return btf_nested_type_is_trusted(&env->log, reg, field_name, btf_id, "__safe_rcu_or_null");
}

static bool type_is_trusted(struct bpf_verifier_env *env,
			    struct bpf_reg_state *reg,
			    const char *field_name, u32 btf_id)
{
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED(struct bpf_iter_meta));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED(struct bpf_iter__task));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED(struct linux_binprm));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED(struct file));

	return btf_nested_type_is_trusted(&env->log, reg, field_name, btf_id, "__safe_trusted");
}

static bool type_is_trusted_or_null(struct bpf_verifier_env *env,
				    struct bpf_reg_state *reg,
				    const char *field_name, u32 btf_id)
{
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct socket));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct dentry));
	BTF_TYPE_EMIT(BTF_TYPE_SAFE_TRUSTED_OR_NULL(struct vm_area_struct));

	return btf_nested_type_is_trusted(&env->log, reg, field_name, btf_id,
					  "__safe_trusted_or_null");
}

static int check_ptr_to_btf_access(struct bpf_verifier_env *env,
				   struct bpf_reg_state *regs,
				   int regno, int off, int size,
				   enum bpf_access_type atype,
				   int value_regno)
{
	struct bpf_reg_state *reg = regs + regno;
	const struct btf_type *t = btf_type_by_id(reg->btf, reg->btf_id);
	const char *tname = btf_name_by_offset(reg->btf, t->name_off);
	const char *field_name = NULL;
	enum bpf_type_flag flag = 0;
	u32 btf_id = 0;
	int ret;

	if (!env->allow_ptr_leaks) {
		verbose(env,
			"'struct %s' access is allowed only to CAP_PERFMON and CAP_SYS_ADMIN\n",
			tname);
		return -EPERM;
	}
	if (!env->prog->gpl_compatible && btf_is_kernel(reg->btf)) {
		verbose(env,
			"Cannot access kernel 'struct %s' from non-GPL compatible program\n",
			tname);
		return -EINVAL;
	}

	if (!tnum_is_const(reg->var_off)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
		verbose(env,
			"R%d is ptr_%s invalid variable offset: off=%d, var_off=%s\n",
			regno, tname, off, tn_buf);
		return -EACCES;
	}

	off += reg->var_off.value;

	if (off < 0) {
		verbose(env,
			"R%d is ptr_%s invalid negative access: off=%d\n",
			regno, tname, off);
		return -EACCES;
	}

	if (reg->type & MEM_USER) {
		verbose(env,
			"R%d is ptr_%s access user memory: off=%d\n",
			regno, tname, off);
		return -EACCES;
	}

	if (reg->type & MEM_PERCPU) {
		verbose(env,
			"R%d is ptr_%s access percpu memory: off=%d\n",
			regno, tname, off);
		return -EACCES;
	}

	if (env->ops->btf_struct_access && !type_is_alloc(reg->type) && atype == BPF_WRITE) {
		if (!btf_is_kernel(reg->btf)) {
			verifier_bug(env, "reg->btf must be kernel btf");
			return -EFAULT;
		}
		ret = env->ops->btf_struct_access(&env->log, reg, off, size);
	} else {
		/* Writes are permitted with default btf_struct_access for
		 * program allocated objects (which always have ref_obj_id > 0),
		 * but not for untrusted PTR_TO_BTF_ID | MEM_ALLOC.
		 */
		if (atype != BPF_READ && !type_is_ptr_alloc_obj(reg->type)) {
			verbose(env, "only read is supported\n");
			return -EACCES;
		}

		if (type_is_alloc(reg->type) && !type_is_non_owning_ref(reg->type) &&
		    !(reg->type & MEM_RCU) && !reg->ref_obj_id) {
			verifier_bug(env, "ref_obj_id for allocated object must be non-zero");
			return -EFAULT;
		}

		ret = btf_struct_access(&env->log, reg, off, size, atype, &btf_id, &flag, &field_name);
	}

	if (ret < 0)
		return ret;

	if (ret != PTR_TO_BTF_ID) {
		/* just mark; */

	} else if (type_flag(reg->type) & PTR_UNTRUSTED) {
		/* If this is an untrusted pointer, all pointers formed by walking it
		 * also inherit the untrusted flag.
		 */
		flag = PTR_UNTRUSTED;