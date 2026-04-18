// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 19/20



/* Stub for JITs that only support cBPF. eBPF programs are interpreted.
 * It is encouraged to implement bpf_int_jit_compile() instead, so that
 * eBPF and implicitly also cBPF can get JITed!
 */
struct bpf_prog * __weak bpf_int_jit_compile(struct bpf_verifier_env *env, struct bpf_prog *prog)
{
	return prog;
}

/* Stub for JITs that support eBPF. All cBPF code gets transformed into
 * eBPF by the kernel and is later compiled by bpf_int_jit_compile().
 */
void __weak bpf_jit_compile(struct bpf_prog *prog)
{
}

bool __weak bpf_helper_changes_pkt_data(enum bpf_func_id func_id)
{
	return false;
}

/* Return TRUE if the JIT backend wants verifier to enable sub-register usage
 * analysis code and wants explicit zero extension inserted by verifier.
 * Otherwise, return FALSE.
 *
 * The verifier inserts an explicit zero extension after BPF_CMPXCHGs even if
 * you don't override this. JITs that don't want these extra insns can detect
 * them using insn_is_zext.
 */
bool __weak bpf_jit_needs_zext(void)
{
	return false;
}

/* By default, enable the verifier's mitigations against Spectre v1 and v4 for
 * all archs. The value returned must not change at runtime as there is
 * currently no support for reloading programs that were loaded without
 * mitigations.
 */
bool __weak bpf_jit_bypass_spec_v1(void)
{
	return false;
}

bool __weak bpf_jit_bypass_spec_v4(void)
{
	return false;
}

/* Return true if the JIT inlines the call to the helper corresponding to
 * the imm.
 *
 * The verifier will not patch the insn->imm for the call to the helper if
 * this returns true.
 */
bool __weak bpf_jit_inlines_helper_call(s32 imm)
{
	return false;
}

/* Return TRUE if the JIT backend supports mixing bpf2bpf and tailcalls. */
bool __weak bpf_jit_supports_subprog_tailcalls(void)
{
	return false;
}

bool __weak bpf_jit_supports_percpu_insn(void)
{
	return false;
}

bool __weak bpf_jit_supports_kfunc_call(void)
{
	return false;
}

bool __weak bpf_jit_supports_far_kfunc_call(void)
{
	return false;
}

bool __weak bpf_jit_supports_arena(void)
{
	return false;
}

bool __weak bpf_jit_supports_insn(struct bpf_insn *insn, bool in_arena)
{
	return false;
}

bool __weak bpf_jit_supports_fsession(void)
{
	return false;
}

u64 __weak bpf_arch_uaddress_limit(void)
{
#if defined(CONFIG_64BIT) && defined(CONFIG_ARCH_HAS_NON_OVERLAPPING_ADDRESS_SPACE)
	return TASK_SIZE;
#else
	return 0;
#endif
}

/* Return TRUE if the JIT backend satisfies the following two conditions:
 * 1) JIT backend supports atomic_xchg() on pointer-sized words.
 * 2) Under the specific arch, the implementation of xchg() is the same
 *    as atomic_xchg() on pointer-sized words.
 */
bool __weak bpf_jit_supports_ptr_xchg(void)
{
	return false;
}

/* To execute LD_ABS/LD_IND instructions __bpf_prog_run() may call
 * skb_copy_bits(), so provide a weak definition of it for NET-less config.
 */
int __weak skb_copy_bits(const struct sk_buff *skb, int offset, void *to,
			 int len)
{
	return -EFAULT;
}

int __weak bpf_arch_text_poke(void *ip, enum bpf_text_poke_type old_t,
			      enum bpf_text_poke_type new_t, void *old_addr,
			      void *new_addr)
{
	return -ENOTSUPP;
}

void * __weak bpf_arch_text_copy(void *dst, void *src, size_t len)
{
	return ERR_PTR(-ENOTSUPP);
}

int __weak bpf_arch_text_invalidate(void *dst, size_t len)
{
	return -ENOTSUPP;
}

bool __weak bpf_jit_supports_exceptions(void)
{
	return false;
}

bool __weak bpf_jit_supports_private_stack(void)
{
	return false;
}

void __weak arch_bpf_stack_walk(bool (*consume_fn)(void *cookie, u64 ip, u64 sp, u64 bp), void *cookie)
{
}

bool __weak bpf_jit_supports_timed_may_goto(void)
{
	return false;
}

u64 __weak arch_bpf_timed_may_goto(void)
{
	return 0;
}

static noinline void bpf_prog_report_may_goto_violation(void)
{
#ifdef CONFIG_BPF_SYSCALL
	struct bpf_stream_stage ss;
	struct bpf_prog *prog;

	prog = bpf_prog_find_from_stack();
	if (!prog)
		return;
	bpf_stream_stage(ss, prog, BPF_STDERR, ({
		bpf_stream_printk(ss, "ERROR: Timeout detected for may_goto instruction\n");
		bpf_stream_dump_stack(ss);
	}));
#endif
}

u64 bpf_check_timed_may_goto(struct bpf_timed_may_goto *p)
{
	u64 time = ktime_get_mono_fast_ns();

	/* Populate the timestamp for this stack frame, and refresh count. */
	if (!p->timestamp) {
		p->timestamp = time;
		return BPF_MAX_TIMED_LOOPS;
	}
	/* Check if we've exhausted our time slice, and zero count. */
	if (unlikely(time - p->timestamp >= (NSEC_PER_SEC / 4))) {
		bpf_prog_report_may_goto_violation();
		return 0;
	}
	/* Refresh the count for the stack frame. */
	return BPF_MAX_TIMED_LOOPS;
}

/* for configs without MMU or 32-bit */
__weak const struct bpf_map_ops arena_map_ops;
__weak u64 bpf_arena_get_user_vm_start(struct bpf_arena *arena)
{
	return 0;
}
__weak u64 bpf_arena_get_kern_vm_start(struct bpf_arena *arena)
{
	return 0;
}

#ifdef CONFIG_BPF_SYSCALL
static int __init bpf_global_ma_init(void)
{
	int ret;