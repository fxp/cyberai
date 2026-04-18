// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 22/36



		if (kvm_vcpu_check_block(vcpu) < 0)
			break;

		waited = true;
		schedule();
	}

	preempt_disable();
	finish_rcuwait(wait);
	kvm_arch_vcpu_unblocking(vcpu);
	preempt_enable();

	vcpu->stat.generic.blocking = 0;

	return waited;
}

static inline void update_halt_poll_stats(struct kvm_vcpu *vcpu, ktime_t start,
					  ktime_t end, bool success)
{
	struct kvm_vcpu_stat_generic *stats = &vcpu->stat.generic;
	u64 poll_ns = ktime_to_ns(ktime_sub(end, start));

	++vcpu->stat.generic.halt_attempted_poll;

	if (success) {
		++vcpu->stat.generic.halt_successful_poll;

		if (!vcpu_valid_wakeup(vcpu))
			++vcpu->stat.generic.halt_poll_invalid;

		stats->halt_poll_success_ns += poll_ns;
		KVM_STATS_LOG_HIST_UPDATE(stats->halt_poll_success_hist, poll_ns);
	} else {
		stats->halt_poll_fail_ns += poll_ns;
		KVM_STATS_LOG_HIST_UPDATE(stats->halt_poll_fail_hist, poll_ns);
	}
}

static unsigned int kvm_vcpu_max_halt_poll_ns(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;

	if (kvm->override_halt_poll_ns) {
		/*
		 * Ensure kvm->max_halt_poll_ns is not read before
		 * kvm->override_halt_poll_ns.
		 *
		 * Pairs with the smp_wmb() when enabling KVM_CAP_HALT_POLL.
		 */
		smp_rmb();
		return READ_ONCE(kvm->max_halt_poll_ns);
	}

	return READ_ONCE(halt_poll_ns);
}

/*
 * Emulate a vCPU halt condition, e.g. HLT on x86, WFI on arm, etc...  If halt
 * polling is enabled, busy wait for a short time before blocking to avoid the
 * expensive block+unblock sequence if a wake event arrives soon after the vCPU
 * is halted.
 */
void kvm_vcpu_halt(struct kvm_vcpu *vcpu)
{
	unsigned int max_halt_poll_ns = kvm_vcpu_max_halt_poll_ns(vcpu);
	bool halt_poll_allowed = !kvm_arch_no_poll(vcpu);
	ktime_t start, cur, poll_end;
	bool waited = false;
	bool do_halt_poll;
	u64 halt_ns;

	if (vcpu->halt_poll_ns > max_halt_poll_ns)
		vcpu->halt_poll_ns = max_halt_poll_ns;

	do_halt_poll = halt_poll_allowed && vcpu->halt_poll_ns;

	start = cur = poll_end = ktime_get();
	if (do_halt_poll) {
		ktime_t stop = ktime_add_ns(start, vcpu->halt_poll_ns);

		do {
			if (kvm_vcpu_check_block(vcpu) < 0)
				goto out;
			cpu_relax();
			poll_end = cur = ktime_get();
		} while (kvm_vcpu_can_poll(cur, stop));
	}

	waited = kvm_vcpu_block(vcpu);

	cur = ktime_get();
	if (waited) {
		vcpu->stat.generic.halt_wait_ns +=
			ktime_to_ns(cur) - ktime_to_ns(poll_end);
		KVM_STATS_LOG_HIST_UPDATE(vcpu->stat.generic.halt_wait_hist,
				ktime_to_ns(cur) - ktime_to_ns(poll_end));
	}
out:
	/* The total time the vCPU was "halted", including polling time. */
	halt_ns = ktime_to_ns(cur) - ktime_to_ns(start);

	/*
	 * Note, halt-polling is considered successful so long as the vCPU was
	 * never actually scheduled out, i.e. even if the wake event arrived
	 * after of the halt-polling loop itself, but before the full wait.
	 */
	if (do_halt_poll)
		update_halt_poll_stats(vcpu, start, poll_end, !waited);

	if (halt_poll_allowed) {
		/* Recompute the max halt poll time in case it changed. */
		max_halt_poll_ns = kvm_vcpu_max_halt_poll_ns(vcpu);

		if (!vcpu_valid_wakeup(vcpu)) {
			shrink_halt_poll_ns(vcpu);
		} else if (max_halt_poll_ns) {
			if (halt_ns <= vcpu->halt_poll_ns)
				;
			/* we had a long block, shrink polling */
			else if (vcpu->halt_poll_ns &&
				 halt_ns > max_halt_poll_ns)
				shrink_halt_poll_ns(vcpu);
			/* we had a short halt and our poll time is too small */
			else if (vcpu->halt_poll_ns < max_halt_poll_ns &&
				 halt_ns < max_halt_poll_ns)
				grow_halt_poll_ns(vcpu);
		} else {
			vcpu->halt_poll_ns = 0;
		}
	}

	trace_kvm_vcpu_wakeup(halt_ns, waited, vcpu_valid_wakeup(vcpu));
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_halt);

bool kvm_vcpu_wake_up(struct kvm_vcpu *vcpu)
{
	if (__kvm_vcpu_wake_up(vcpu)) {
		WRITE_ONCE(vcpu->ready, true);
		++vcpu->stat.generic.halt_wakeup;
		return true;
	}

	return false;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_wake_up);

#ifndef CONFIG_S390
/*
 * Kick a sleeping VCPU, or a guest VCPU in guest mode, into host kernel mode.
 */
void __kvm_vcpu_kick(struct kvm_vcpu *vcpu, bool wait)
{
	int me, cpu;

	if (kvm_vcpu_wake_up(vcpu))
		return;

	me = get_cpu();
	/*
	 * The only state change done outside the vcpu mutex is IN_GUEST_MODE
	 * to EXITING_GUEST_MODE.  Therefore the moderately expensive "should
	 * kick" check does not need atomic operations if kvm_vcpu_kick is used
	 * within the vCPU thread itself.
	 */
	if (vcpu == __this_cpu_read(kvm_running_vcpu)) {
		if (vcpu->mode == IN_GUEST_MODE)
			WRITE_ONCE(vcpu->mode, EXITING_GUEST_MODE);
		goto out;
	}