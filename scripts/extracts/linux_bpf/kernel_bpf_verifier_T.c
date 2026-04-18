// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/verifier.c
// Segment 52/132



/* process_iter_next_call() is called when verifier gets to iterator's next
 * "method" (e.g., bpf_iter_num_next() for numbers iterator) call. We'll refer
 * to it as just "iter_next()" in comments below.
 *
 * BPF verifier relies on a crucial contract for any iter_next()
 * implementation: it should *eventually* return NULL, and once that happens
 * it should keep returning NULL. That is, once iterator exhausts elements to
 * iterate, it should never reset or spuriously return new elements.
 *
 * With the assumption of such contract, process_iter_next_call() simulates
 * a fork in the verifier state to validate loop logic correctness and safety
 * without having to simulate infinite amount of iterations.
 *
 * In current state, we first assume that iter_next() returned NULL and
 * iterator state is set to DRAINED (BPF_ITER_STATE_DRAINED). In such
 * conditions we should not form an infinite loop and should eventually reach
 * exit.
 *
 * Besides that, we also fork current state and enqueue it for later
 * verification. In a forked state we keep iterator state as ACTIVE
 * (BPF_ITER_STATE_ACTIVE) and assume non-NULL return from iter_next(). We
 * also bump iteration depth to prevent erroneous infinite loop detection
 * later on (see iter_active_depths_differ() comment for details). In this
 * state we assume that we'll eventually loop back to another iter_next()
 * calls (it could be in exactly same location or in some other instruction,
 * it doesn't matter, we don't make any unnecessary assumptions about this,
 * everything revolves around iterator state in a stack slot, not which
 * instruction is calling iter_next()). When that happens, we either will come
 * to iter_next() with equivalent state and can conclude that next iteration
 * will proceed in exactly the same way as we just verified, so it's safe to
 * assume that loop converges. If not, we'll go on another iteration
 * simulation with a different input state, until all possible starting states
 * are validated or we reach maximum number of instructions limit.
 *
 * This way, we will either exhaustively discover all possible input states
 * that iterator loop can start with and eventually will converge, or we'll
 * effectively regress into bounded loop simulation logic and either reach
 * maximum number of instructions if loop is not provably convergent, or there
 * is some statically known limit on number of iterations (e.g., if there is
 * an explicit `if n > 100 then break;` statement somewhere in the loop).
 *
 * Iteration convergence logic in is_state_visited() relies on exact
 * states comparison, which ignores read and precision marks.
 * This is necessary because read and precision marks are not finalized
 * while in the loop. Exact comparison might preclude convergence for
 * simple programs like below:
 *
 *     i = 0;
 *     while(iter_next(&it))
 *       i++;
 *
 * At each iteration step i++ would produce a new distinct state and
 * eventually instruction processing limit would be reached.
 *
 * To avoid such behavior speculatively forget (widen) range for
 * imprecise scalar registers, if those registers were not precise at the
 * end of the previous iteration and do not match exactly.
 *
 * This is a conservative heuristic that allows to verify wide range of programs,
 * however it precludes verification of programs that conjure an
 * imprecise value on the first loop iteration and use it as precise on a second.
 * For example, the following safe program would fail to verify:
 *
 *     struct bpf_num_iter it;
 *     int arr[10];
 *     int i = 0, a = 0;
 *     bpf_iter_num_new(&it, 0, 10);
 *     while (bpf_iter_num_next(&it)) {
 *       if (a == 0) {
 *         a = 1;
 *         i = 7; // Because i changed verifier would forget
 *                // it's range on second loop entry.
 *       } else {
 *         arr[i] = 42; // This would fail to verify.
 *       }
 *     }
 *     bpf_iter_num_destroy(&it);
 */
static int process_iter_next_call(struct bpf_verifier_env *env, int insn_idx,
				  struct bpf_kfunc_call_arg_meta *meta)
{
	struct bpf_verifier_state *cur_st = env->cur_state, *queued_st, *prev_st;
	struct bpf_func_state *cur_fr = cur_st->frame[cur_st->curframe], *queued_fr;
	struct bpf_reg_state *cur_iter, *queued_iter;

	BTF_TYPE_EMIT(struct bpf_iter);

	cur_iter = get_iter_from_state(cur_st, meta);

	if (cur_iter->iter.state != BPF_ITER_STATE_ACTIVE &&
	    cur_iter->iter.state != BPF_ITER_STATE_DRAINED) {
		verifier_bug(env, "unexpected iterator state %d (%s)",
			     cur_iter->iter.state, iter_state_str(cur_iter->iter.state));
		return -EFAULT;
	}