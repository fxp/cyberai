// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/btf.c
// Segment 37/55



static struct btf *btf_parse_base(struct btf_verifier_env *env, const char *name,
				  void *data, unsigned int data_size)
{
	struct btf *btf = NULL;
	int err;

	if (!IS_ENABLED(CONFIG_DEBUG_INFO_BTF))
		return ERR_PTR(-ENOENT);

	btf = kzalloc_obj(*btf, GFP_KERNEL | __GFP_NOWARN);
	if (!btf) {
		err = -ENOMEM;
		goto errout;
	}
	env->btf = btf;

	btf->data = data;
	btf->data_size = data_size;
	btf->kernel_btf = true;
	btf->named_start_id = 0;
	strscpy(btf->name, name);

	err = btf_parse_hdr(env);
	if (err)
		goto errout;

	btf->nohdr_data = btf->data + btf->hdr.hdr_len;

	err = btf_parse_str_sec(env);
	if (err)
		goto errout;

	err = btf_check_all_metas(env);
	if (err)
		goto errout;

	err = btf_check_type_tags(env, btf, 1);
	if (err)
		goto errout;

	btf_check_sorted(btf);
	refcount_set(&btf->refcnt, 1);

	return btf;

errout:
	if (btf) {
		kvfree(btf->types);
		kfree(btf);
	}
	return ERR_PTR(err);
}

struct btf *btf_parse_vmlinux(void)
{
	struct btf_verifier_env *env = NULL;
	struct bpf_verifier_log *log;
	struct btf *btf;
	int err;

	env = kzalloc_obj(*env, GFP_KERNEL | __GFP_NOWARN);
	if (!env)
		return ERR_PTR(-ENOMEM);

	log = &env->log;
	log->level = BPF_LOG_KERNEL;
	btf = btf_parse_base(env, "vmlinux", __start_BTF, __stop_BTF - __start_BTF);
	if (IS_ERR(btf))
		goto err_out;

	/* btf_parse_vmlinux() runs under bpf_verifier_lock */
	bpf_ctx_convert.t = btf_type_by_id(btf, bpf_ctx_convert_btf_id[0]);
	err = btf_alloc_id(btf);
	if (err) {
		btf_free(btf);
		btf = ERR_PTR(err);
	}
err_out:
	btf_verifier_env_free(env);
	return btf;
}

/* If .BTF_ids section was created with distilled base BTF, both base and
 * split BTF ids will need to be mapped to actual base/split ids for
 * BTF now that it has been relocated.
 */
static __u32 btf_relocate_id(const struct btf *btf, __u32 id)
{
	if (!btf->base_btf || !btf->base_id_map)
		return id;
	return btf->base_id_map[id];
}

#ifdef CONFIG_DEBUG_INFO_BTF_MODULES

static struct btf *btf_parse_module(const char *module_name, const void *data,
				    unsigned int data_size, void *base_data,
				    unsigned int base_data_size)
{
	struct btf *btf = NULL, *vmlinux_btf, *base_btf = NULL;
	struct btf_verifier_env *env = NULL;
	struct bpf_verifier_log *log;
	int err = 0;

	vmlinux_btf = bpf_get_btf_vmlinux();
	if (IS_ERR(vmlinux_btf))
		return vmlinux_btf;
	if (!vmlinux_btf)
		return ERR_PTR(-EINVAL);

	env = kzalloc_obj(*env, GFP_KERNEL | __GFP_NOWARN);
	if (!env)
		return ERR_PTR(-ENOMEM);

	log = &env->log;
	log->level = BPF_LOG_KERNEL;

	if (base_data) {
		base_btf = btf_parse_base(env, ".BTF.base", base_data, base_data_size);
		if (IS_ERR(base_btf)) {
			err = PTR_ERR(base_btf);
			goto errout;
		}
	} else {
		base_btf = vmlinux_btf;
	}

	btf = kzalloc_obj(*btf, GFP_KERNEL | __GFP_NOWARN);
	if (!btf) {
		err = -ENOMEM;
		goto errout;
	}
	env->btf = btf;

	btf->base_btf = base_btf;
	btf->start_id = base_btf->nr_types;
	btf->start_str_off = base_btf->hdr.str_len;
	btf->kernel_btf = true;
	btf->named_start_id = 0;
	strscpy(btf->name, module_name);

	btf->data = kvmemdup(data, data_size, GFP_KERNEL | __GFP_NOWARN);
	if (!btf->data) {
		err = -ENOMEM;
		goto errout;
	}
	btf->data_size = data_size;

	err = btf_parse_hdr(env);
	if (err)
		goto errout;

	btf->nohdr_data = btf->data + btf->hdr.hdr_len;

	err = btf_parse_str_sec(env);
	if (err)
		goto errout;

	err = btf_check_all_metas(env);
	if (err)
		goto errout;

	err = btf_check_type_tags(env, btf, btf_nr_types(base_btf));
	if (err)
		goto errout;

	if (base_btf != vmlinux_btf) {
		err = btf_relocate(btf, vmlinux_btf, &btf->base_id_map);
		if (err)
			goto errout;
		btf_free(base_btf);
		base_btf = vmlinux_btf;
	}

	btf_verifier_env_free(env);
	btf_check_sorted(btf);
	refcount_set(&btf->refcnt, 1);
	return btf;

errout:
	btf_verifier_env_free(env);
	if (!IS_ERR(base_btf) && base_btf != vmlinux_btf)
		btf_free(base_btf);
	if (btf) {
		kvfree(btf->data);
		kvfree(btf->types);
		kfree(btf);
	}
	return ERR_PTR(err);
}

#endif /* CONFIG_DEBUG_INFO_BTF_MODULES */

struct btf *bpf_prog_get_target_btf(const struct bpf_prog *prog)
{
	struct bpf_prog *tgt_prog = prog->aux->dst_prog;

	if (tgt_prog)
		return tgt_prog->aux->btf;
	else
		return prog->aux->attach_btf;
}

u32 btf_ctx_arg_idx(struct btf *btf, const struct btf_type *func_proto,
		    int off)
{
	const struct btf_param *args;
	const struct btf_type *t;
	u32 offset = 0, nr_args;
	int i;

	if (!func_proto)
		return off / 8;

	nr_args = btf_type_vlen(func_proto);
	args = (const struct btf_param *)(func_proto + 1);
	for (i = 0; i < nr_args; i++) {
		t = btf_type_skip_modifiers(btf, args[i].type, NULL);
		offset += btf_type_is_ptr(t) ? 8 : roundup(t->size, 8);
		if (off < offset)
			return i;
	}

	t = btf_type_skip_modifiers(btf, func_proto->type, NULL);
	offset += btf_type_is_ptr(t) ? 8 : roundup(t->size, 8);
	if (off < offset)
		return nr_args;

	return nr_args + 1;
}