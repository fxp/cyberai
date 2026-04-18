// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 36/132



	switch (reg->type) {
	case PTR_TO_PACKET:
	case PTR_TO_PACKET_META:
		/* Special case, because of NET_IP_ALIGN. Given metadata sits
		 * right in front, treat it the very same way.
		 */
		return check_pkt_ptr_alignment(env, reg, off, size, strict);
	case PTR_TO_FLOW_KEYS:
		pointer_desc = "flow keys ";
		break;
	case PTR_TO_MAP_KEY:
		pointer_desc = "key ";
		break;
	case PTR_TO_MAP_VALUE:
		pointer_desc = "value ";
		if (reg->map_ptr->map_type == BPF_MAP_TYPE_INSN_ARRAY)
			strict = true;
		break;
	case PTR_TO_CTX:
		pointer_desc = "context ";
		break;
	case PTR_TO_STACK:
		pointer_desc = "stack ";
		/* The stack spill tracking logic in check_stack_write_fixed_off()
		 * and check_stack_read_fixed_off() relies on stack accesses being
		 * aligned.
		 */
		strict = true;
		break;
	case PTR_TO_SOCKET:
		pointer_desc = "sock ";
		break;
	case PTR_TO_SOCK_COMMON:
		pointer_desc = "sock_common ";
		break;
	case PTR_TO_TCP_SOCK:
		pointer_desc = "tcp_sock ";
		break;
	case PTR_TO_XDP_SOCK:
		pointer_desc = "xdp_sock ";
		break;
	case PTR_TO_ARENA:
		return 0;
	default:
		break;
	}
	return check_generic_ptr_alignment(env, reg, pointer_desc, off, size,
					   strict);
}

static enum priv_stack_mode bpf_enable_priv_stack(struct bpf_prog *prog)
{
	if (!bpf_jit_supports_private_stack())
		return NO_PRIV_STACK;

	/* bpf_prog_check_recur() checks all prog types that use bpf trampoline
	 * while kprobe/tp/perf_event/raw_tp don't use trampoline hence checked
	 * explicitly.
	 */
	switch (prog->type) {
	case BPF_PROG_TYPE_KPROBE:
	case BPF_PROG_TYPE_TRACEPOINT:
	case BPF_PROG_TYPE_PERF_EVENT:
	case BPF_PROG_TYPE_RAW_TRACEPOINT:
		return PRIV_STACK_ADAPTIVE;
	case BPF_PROG_TYPE_TRACING:
	case BPF_PROG_TYPE_LSM:
	case BPF_PROG_TYPE_STRUCT_OPS:
		if (prog->aux->priv_stack_requested || bpf_prog_check_recur(prog))
			return PRIV_STACK_ADAPTIVE;
		fallthrough;
	default:
		break;
	}

	return NO_PRIV_STACK;
}

static int round_up_stack_depth(struct bpf_verifier_env *env, int stack_depth)
{
	if (env->prog->jit_requested)
		return round_up(stack_depth, 16);

	/* round up to 32-bytes, since this is granularity
	 * of interpreter stack size
	 */
	return round_up(max_t(u32, stack_depth, 1), 32);
}

/* temporary state used for call frame depth calculation */
struct bpf_subprog_call_depth_info {
	int ret_insn; /* caller instruction where we return to. */
	int caller; /* caller subprogram idx */
	int frame; /* # of consecutive static call stack frames on top of stack */
};

/* starting from main bpf function walk all instructions of the function
 * and recursively walk all callees that given function can call.
 * Ignore jump and exit insns.
 */
static int check_max_stack_depth_subprog(struct bpf_verifier_env *env, int idx,
					 struct bpf_subprog_call_depth_info *dinfo,
					 bool priv_stack_supported)
{
	struct bpf_subprog_info *subprog = env->subprog_info;
	struct bpf_insn *insn = env->prog->insnsi;
	int depth = 0, frame = 0, i, subprog_end, subprog_depth;
	bool tail_call_reachable = false;
	int total;
	int tmp;

	/* no caller idx */
	dinfo[idx].caller = -1;

	i = subprog[idx].start;
	if (!priv_stack_supported)
		subprog[idx].priv_stack_mode = NO_PRIV_STACK;
process_func:
	/* protect against potential stack overflow that might happen when
	 * bpf2bpf calls get combined with tailcalls. Limit the caller's stack
	 * depth for such case down to 256 so that the worst case scenario
	 * would result in 8k stack size (32 which is tailcall limit * 256 =
	 * 8k).
	 *
	 * To get the idea what might happen, see an example:
	 * func1 -> sub rsp, 128
	 *  subfunc1 -> sub rsp, 256
	 *  tailcall1 -> add rsp, 256
	 *   func2 -> sub rsp, 192 (total stack size = 128 + 192 = 320)
	 *   subfunc2 -> sub rsp, 64
	 *   subfunc22 -> sub rsp, 128
	 *   tailcall2 -> add rsp, 128
	 *    func3 -> sub rsp, 32 (total stack size 128 + 192 + 64 + 32 = 416)
	 *
	 * tailcall will unwind the current stack frame but it will not get rid
	 * of caller's stack as shown on the example above.
	 */
	if (idx && subprog[idx].has_tail_call && depth >= 256) {
		verbose(env,
			"tail_calls are not allowed when call stack of previous frames is %d bytes. Too large\n",
			depth);
		return -EACCES;
	}

	subprog_depth = round_up_stack_depth(env, subprog[idx].stack_depth);
	if (priv_stack_supported) {
		/* Request private stack support only if the subprog stack
		 * depth is no less than BPF_PRIV_STACK_MIN_SIZE. This is to
		 * avoid jit penalty if the stack usage is small.
		 */
		if (subprog[idx].priv_stack_mode == PRIV_STACK_UNKNOWN &&
		    subprog_depth >= BPF_PRIV_STACK_MIN_SIZE)
			subprog[idx].priv_stack_mode = PRIV_STACK_ADAPTIVE;
	}