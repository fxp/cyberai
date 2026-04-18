// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 23/36



	/*
	 * Note, the vCPU could get migrated to a different pCPU at any point
	 * after kvm_arch_vcpu_should_kick(), which could result in sending an
	 * IPI to the previous pCPU.  But, that's ok because the purpose of the
	 * IPI is to force the vCPU to leave IN_GUEST_MODE, and migrating the
	 * vCPU also requires it to leave IN_GUEST_MODE.
	 */
	if (kvm_arch_vcpu_should_kick(vcpu)) {
		cpu = READ_ONCE(vcpu->cpu);
		if (cpu != me && (unsigned int)cpu < nr_cpu_ids && cpu_online(cpu)) {
			/*
			 * Use a reschedule IPI to kick the vCPU if the caller
			 * doesn't need to wait for a response, as KVM allows
			 * kicking vCPUs while IRQs are disabled, but using the
			 * SMP function call framework with IRQs disabled can
			 * deadlock due to taking cross-CPU locks.
			 */
			if (wait)
				smp_call_function_single(cpu, ack_kick, NULL, wait);
			else
				smp_send_reschedule(cpu);
		}
	}
out:
	put_cpu();
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_vcpu_kick);
#endif /* !CONFIG_S390 */

int kvm_vcpu_yield_to(struct kvm_vcpu *target)
{
	struct task_struct *task = NULL;
	int ret;

	if (!read_trylock(&target->pid_lock))
		return 0;

	if (target->pid)
		task = get_pid_task(target->pid, PIDTYPE_PID);

	read_unlock(&target->pid_lock);

	if (!task)
		return 0;
	ret = yield_to(task, 1);
	put_task_struct(task);

	return ret;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_yield_to);

/*
 * Helper that checks whether a VCPU is eligible for directed yield.
 * Most eligible candidate to yield is decided by following heuristics:
 *
 *  (a) VCPU which has not done pl-exit or cpu relax intercepted recently
 *  (preempted lock holder), indicated by @in_spin_loop.
 *  Set at the beginning and cleared at the end of interception/PLE handler.
 *
 *  (b) VCPU which has done pl-exit/ cpu relax intercepted but did not get
 *  chance last time (mostly it has become eligible now since we have probably
 *  yielded to lockholder in last iteration. This is done by toggling
 *  @dy_eligible each time a VCPU checked for eligibility.)
 *
 *  Yielding to a recently pl-exited/cpu relax intercepted VCPU before yielding
 *  to preempted lock-holder could result in wrong VCPU selection and CPU
 *  burning. Giving priority for a potential lock-holder increases lock
 *  progress.
 *
 *  Since algorithm is based on heuristics, accessing another VCPU data without
 *  locking does not harm. It may result in trying to yield to  same VCPU, fail
 *  and continue with next VCPU and so on.
 */
static bool kvm_vcpu_eligible_for_directed_yield(struct kvm_vcpu *vcpu)
{
#ifdef CONFIG_HAVE_KVM_CPU_RELAX_INTERCEPT
	bool eligible;

	eligible = !vcpu->spin_loop.in_spin_loop ||
		    vcpu->spin_loop.dy_eligible;

	if (vcpu->spin_loop.in_spin_loop)
		kvm_vcpu_set_dy_eligible(vcpu, !vcpu->spin_loop.dy_eligible);

	return eligible;
#else
	return true;
#endif
}

/*
 * Unlike kvm_arch_vcpu_runnable, this function is called outside
 * a vcpu_load/vcpu_put pair.  However, for most architectures
 * kvm_arch_vcpu_runnable does not require vcpu_load.
 */
bool __weak kvm_arch_dy_runnable(struct kvm_vcpu *vcpu)
{
	return kvm_arch_vcpu_runnable(vcpu);
}

static bool vcpu_dy_runnable(struct kvm_vcpu *vcpu)
{
	if (kvm_arch_dy_runnable(vcpu))
		return true;

#ifdef CONFIG_KVM_ASYNC_PF
	if (!list_empty_careful(&vcpu->async_pf.done))
		return true;
#endif

	return false;
}

/*
 * By default, simply query the target vCPU's current mode when checking if a
 * vCPU was preempted in kernel mode.  All architectures except x86 (or more
 * specifical, except VMX) allow querying whether or not a vCPU is in kernel
 * mode even if the vCPU is NOT loaded, i.e. using kvm_arch_vcpu_in_kernel()
 * directly for cross-vCPU checks is functionally correct and accurate.
 */
bool __weak kvm_arch_vcpu_preempted_in_kernel(struct kvm_vcpu *vcpu)
{
	return kvm_arch_vcpu_in_kernel(vcpu);
}

bool __weak kvm_arch_dy_has_pending_interrupt(struct kvm_vcpu *vcpu)
{
	return false;
}

void kvm_vcpu_on_spin(struct kvm_vcpu *me, bool yield_to_kernel_mode)
{
	int nr_vcpus, start, i, idx, yielded;
	struct kvm *kvm = me->kvm;
	struct kvm_vcpu *vcpu;
	int try = 3;

	nr_vcpus = atomic_read(&kvm->online_vcpus);
	if (nr_vcpus < 2)
		return;

	/* Pairs with the smp_wmb() in kvm_vm_ioctl_create_vcpu(). */
	smp_rmb();

	kvm_vcpu_set_in_spin_loop(me, true);