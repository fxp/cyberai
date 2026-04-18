// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 9/20



void bpf_jit_prog_release_other(struct bpf_prog *fp, struct bpf_prog *fp_other)
{
	/* We have to repoint aux->prog to self, as we don't
	 * know whether fp here is the clone or the original.
	 */
	fp->aux->prog = fp;
	if (fp->aux->offload)
		fp->aux->offload->prog = fp;
	bpf_prog_clone_free(fp_other);
}

/*
 * Now this function is used only to blind the main prog and must be invoked only when
 * bpf_prog_need_blind() returns true.
 */
struct bpf_prog *bpf_jit_blind_constants(struct bpf_verifier_env *env, struct bpf_prog *prog)
{
	struct bpf_insn insn_buff[16], aux[2];
	struct bpf_prog *clone, *tmp;
	int insn_delta, insn_cnt;
	struct bpf_insn *insn;
	int i, rewritten;

	if (WARN_ON_ONCE(env && env->prog != prog))
		return ERR_PTR(-EINVAL);

	clone = bpf_prog_clone_create(prog, GFP_USER);
	if (!clone)
		return ERR_PTR(-ENOMEM);

	/* make sure bpf_patch_insn_data() patches the correct prog */
	if (env)
		env->prog = clone;

	insn_cnt = clone->len;
	insn = clone->insnsi;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (bpf_pseudo_func(insn)) {
			/* ld_imm64 with an address of bpf subprog is not
			 * a user controlled constant. Don't randomize it,
			 * since it will conflict with jit_subprogs() logic.
			 */
			insn++;
			i++;
			continue;
		}

		/* We temporarily need to hold the original ld64 insn
		 * so that we can still access the first part in the
		 * second blinding run.
		 */
		if (insn[0].code == (BPF_LD | BPF_IMM | BPF_DW) &&
		    insn[1].code == 0)
			memcpy(aux, insn, sizeof(aux));

		rewritten = bpf_jit_blind_insn(insn, aux, insn_buff,
						clone->aux->verifier_zext);
		if (!rewritten)
			continue;

		if (env)
			tmp = bpf_patch_insn_data(env, i, insn_buff, rewritten);
		else
			tmp = bpf_patch_insn_single(clone, i, insn_buff, rewritten);

		if (IS_ERR_OR_NULL(tmp)) {
			if (env)
				/* restore the original prog */
				env->prog = prog;
			/* Patching may have repointed aux->prog during
			 * realloc from the original one, so we need to
			 * fix it up here on error.
			 */
			bpf_jit_prog_release_other(prog, clone);
			return IS_ERR(tmp) ? tmp : ERR_PTR(-ENOMEM);
		}

		clone = tmp;
		insn_delta = rewritten - 1;

		if (env)
			env->prog = clone;

		/* Walk new program and skip insns we just inserted. */
		insn = clone->insnsi + i + insn_delta;
		insn_cnt += insn_delta;
		i        += insn_delta;
	}

	clone->blinded = 1;
	return clone;
}

bool bpf_insn_is_indirect_target(const struct bpf_verifier_env *env, const struct bpf_prog *prog,
				 int insn_idx)
{
	if (!env)
		return false;
	insn_idx += prog->aux->subprog_start;
	return env->insn_aux_data[insn_idx].indirect_target;
}
#endif /* CONFIG_BPF_JIT */

/* Base function for offset calculation. Needs to go into .text section,
 * therefore keeping it non-static as well; will also be used by JITs
 * anyway later on, so do not let the compiler omit it. This also needs
 * to go into kallsyms for correlation from e.g. bpftool, so naming
 * must not change.
 */
noinline u64 __bpf_call_base(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5)
{
	return 0;
}
EXPORT_SYMBOL_GPL(__bpf_call_base);