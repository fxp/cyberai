// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 7/20



/* Copy JITed text from rw_header to its final location, the ro_header. */
int bpf_jit_binary_pack_finalize(struct bpf_binary_header *ro_header,
				 struct bpf_binary_header *rw_header)
{
	void *ptr;

	ptr = bpf_arch_text_copy(ro_header, rw_header, rw_header->size);

	kvfree(rw_header);

	if (IS_ERR(ptr)) {
		bpf_prog_pack_free(ro_header, ro_header->size);
		return PTR_ERR(ptr);
	}
	return 0;
}

/* bpf_jit_binary_pack_free is called in two different scenarios:
 *   1) when the program is freed after;
 *   2) when the JIT engine fails (before bpf_jit_binary_pack_finalize).
 * For case 2), we need to free both the RO memory and the RW buffer.
 *
 * bpf_jit_binary_pack_free requires proper ro_header->size. However,
 * bpf_jit_binary_pack_alloc does not set it. Therefore, ro_header->size
 * must be set with either bpf_jit_binary_pack_finalize (normal path) or
 * bpf_arch_text_copy (when jit fails).
 */
void bpf_jit_binary_pack_free(struct bpf_binary_header *ro_header,
			      struct bpf_binary_header *rw_header)
{
	u32 size = ro_header->size;

	bpf_prog_pack_free(ro_header, size);
	kvfree(rw_header);
	bpf_jit_uncharge_modmem(size);
}

struct bpf_binary_header *
bpf_jit_binary_pack_hdr(const struct bpf_prog *fp)
{
	unsigned long real_start = (unsigned long)fp->bpf_func;
	unsigned long addr;

	addr = real_start & BPF_PROG_CHUNK_MASK;
	return (void *)addr;
}

static inline struct bpf_binary_header *
bpf_jit_binary_hdr(const struct bpf_prog *fp)
{
	unsigned long real_start = (unsigned long)fp->bpf_func;
	unsigned long addr;

	addr = real_start & PAGE_MASK;
	return (void *)addr;
}

/* This symbol is only overridden by archs that have different
 * requirements than the usual eBPF JITs, f.e. when they only
 * implement cBPF JIT, do not set images read-only, etc.
 */
void __weak bpf_jit_free(struct bpf_prog *fp)
{
	if (fp->jited) {
		struct bpf_binary_header *hdr = bpf_jit_binary_hdr(fp);

		bpf_jit_binary_free(hdr);
		WARN_ON_ONCE(!bpf_prog_kallsyms_verify_off(fp));
	}

	bpf_prog_unlock_free(fp);
}

int bpf_jit_get_func_addr(const struct bpf_prog *prog,
			  const struct bpf_insn *insn, bool extra_pass,
			  u64 *func_addr, bool *func_addr_fixed)
{
	s16 off = insn->off;
	s32 imm = insn->imm;
	u8 *addr;
	int err;

	*func_addr_fixed = insn->src_reg != BPF_PSEUDO_CALL;
	if (!*func_addr_fixed) {
		/* Place-holder address till the last pass has collected
		 * all addresses for JITed subprograms in which case we
		 * can pick them up from prog->aux.
		 */
		if (!extra_pass)
			addr = NULL;
		else if (prog->aux->func &&
			 off >= 0 && off < prog->aux->real_func_cnt)
			addr = (u8 *)prog->aux->func[off]->bpf_func;
		else
			return -EINVAL;
	} else if (insn->src_reg == BPF_PSEUDO_KFUNC_CALL &&
		   bpf_jit_supports_far_kfunc_call()) {
		err = bpf_get_kfunc_addr(prog, insn->imm, insn->off, &addr);
		if (err)
			return err;
	} else {
		/* Address of a BPF helper call. Since part of the core
		 * kernel, it's always at a fixed location. __bpf_call_base
		 * and the helper with imm relative to it are both in core
		 * kernel.
		 */
		addr = (u8 *)__bpf_call_base + imm;
	}

	*func_addr = (unsigned long)addr;
	return 0;
}

const char *bpf_jit_get_prog_name(struct bpf_prog *prog)
{
	if (prog->aux->ksym.prog)
		return prog->aux->ksym.name;
	return prog->aux->name;
}

static int bpf_jit_blind_insn(const struct bpf_insn *from,
			      const struct bpf_insn *aux,
			      struct bpf_insn *to_buff,
			      bool emit_zext)
{
	struct bpf_insn *to = to_buff;
	u32 imm_rnd = get_random_u32();
	s16 off;

	BUILD_BUG_ON(BPF_REG_AX  + 1 != MAX_BPF_JIT_REG);
	BUILD_BUG_ON(MAX_BPF_REG + 1 != MAX_BPF_JIT_REG);

	/* Constraints on AX register:
	 *
	 * AX register is inaccessible from user space. It is mapped in
	 * all JITs, and used here for constant blinding rewrites. It is
	 * typically "stateless" meaning its contents are only valid within
	 * the executed instruction, but not across several instructions.
	 * There are a few exceptions however which are further detailed
	 * below.
	 *
	 * Constant blinding is only used by JITs, not in the interpreter.
	 * The interpreter uses AX in some occasions as a local temporary
	 * register e.g. in DIV or MOD instructions.
	 *
	 * In restricted circumstances, the verifier can also use the AX
	 * register for rewrites as long as they do not interfere with
	 * the above cases!
	 */
	if (from->dst_reg == BPF_REG_AX || from->src_reg == BPF_REG_AX)
		goto out;

	if (from->imm == 0 &&
	    (from->code == (BPF_ALU   | BPF_MOV | BPF_K) ||
	     from->code == (BPF_ALU64 | BPF_MOV | BPF_K))) {
		*to++ = BPF_ALU64_REG(BPF_XOR, from->dst_reg, from->dst_reg);
		goto out;
	}