// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 26/36



	if (!PAGE_ALIGNED(range->gpa) ||
	    !PAGE_ALIGNED(range->size) ||
	    range->gpa + range->size <= range->gpa)
		return -EINVAL;

	vcpu_load(vcpu);
	idx = srcu_read_lock(&vcpu->kvm->srcu);

	full_size = range->size;
	do {
		if (signal_pending(current)) {
			r = -EINTR;
			break;
		}

		r = kvm_arch_vcpu_pre_fault_memory(vcpu, range);
		if (WARN_ON_ONCE(r == 0 || r == -EIO))
			break;

		if (r < 0)
			break;

		range->size -= r;
		range->gpa += r;
		cond_resched();
	} while (range->size);

	srcu_read_unlock(&vcpu->kvm->srcu, idx);
	vcpu_put(vcpu);

	/* Return success if at least one page was mapped successfully.  */
	return full_size == range->size ? r : 0;
}
#endif

static int kvm_wait_for_vcpu_online(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;

	/*
	 * In practice, this happy path will always be taken, as a well-behaved
	 * VMM will never invoke a vCPU ioctl() before KVM_CREATE_VCPU returns.
	 */
	if (likely(vcpu->vcpu_idx < atomic_read(&kvm->online_vcpus)))
		return 0;

	/*
	 * Acquire and release the vCPU's mutex to wait for vCPU creation to
	 * complete (kvm_vm_ioctl_create_vcpu() holds the mutex until the vCPU
	 * is fully online).
	 */
	if (mutex_lock_killable(&vcpu->mutex))
		return -EINTR;

	mutex_unlock(&vcpu->mutex);

	if (WARN_ON_ONCE(!kvm_get_vcpu(kvm, vcpu->vcpu_idx)))
		return -EIO;

	return 0;
}

static long kvm_vcpu_ioctl(struct file *filp,
			   unsigned int ioctl, unsigned long arg)
{
	struct kvm_vcpu *vcpu = filp->private_data;
	void __user *argp = (void __user *)arg;
	int r;
	struct kvm_fpu *fpu = NULL;
	struct kvm_sregs *kvm_sregs = NULL;

	if (vcpu->kvm->mm != current->mm || vcpu->kvm->vm_dead)
		return -EIO;

	if (unlikely(_IOC_TYPE(ioctl) != KVMIO))
		return -EINVAL;

	/*
	 * Wait for the vCPU to be online before handling the ioctl(), as KVM
	 * assumes the vCPU is reachable via vcpu_array, i.e. may dereference
	 * a NULL pointer if userspace invokes an ioctl() before KVM is ready.
	 */
	r = kvm_wait_for_vcpu_online(vcpu);
	if (r)
		return r;

	/*
	 * Let arch code handle select vCPU ioctls without holding vcpu->mutex,
	 * e.g. to support ioctls that can run asynchronous to vCPU execution.
	 */
	r = kvm_arch_vcpu_unlocked_ioctl(filp, ioctl, arg);
	if (r != -ENOIOCTLCMD)
		return r;

	if (mutex_lock_killable(&vcpu->mutex))
		return -EINTR;
	switch (ioctl) {
	case KVM_RUN: {
		struct pid *oldpid;
		r = -EINVAL;
		if (arg)
			goto out;

		/*
		 * Note, vcpu->pid is primarily protected by vcpu->mutex. The
		 * dedicated r/w lock allows other tasks, e.g. other vCPUs, to
		 * read vcpu->pid while this vCPU is in KVM_RUN, e.g. to yield
		 * directly to this vCPU
		 */
		oldpid = vcpu->pid;
		if (unlikely(oldpid != task_pid(current))) {
			/* The thread running this VCPU changed. */
			struct pid *newpid;

			r = kvm_arch_vcpu_run_pid_change(vcpu);
			if (r)
				break;

			newpid = get_task_pid(current, PIDTYPE_PID);
			write_lock(&vcpu->pid_lock);
			vcpu->pid = newpid;
			write_unlock(&vcpu->pid_lock);

			put_pid(oldpid);
		}
		vcpu->wants_to_run = !READ_ONCE(vcpu->run->immediate_exit__unsafe);
		r = kvm_arch_vcpu_ioctl_run(vcpu);
		vcpu->wants_to_run = false;

		/*
		 * FIXME: Remove this hack once all KVM architectures
		 * support the generic TIF bits, i.e. a dedicated TIF_RSEQ.
		 */
		rseq_virt_userspace_exit();

		trace_kvm_userspace_exit(vcpu->run->exit_reason, r);
		break;
	}
	case KVM_GET_REGS: {
		struct kvm_regs *kvm_regs;

		r = -ENOMEM;
		kvm_regs = kzalloc_obj(struct kvm_regs);
		if (!kvm_regs)
			goto out;
		r = kvm_arch_vcpu_ioctl_get_regs(vcpu, kvm_regs);
		if (r)
			goto out_free1;
		r = -EFAULT;
		if (copy_to_user(argp, kvm_regs, sizeof(struct kvm_regs)))
			goto out_free1;
		r = 0;
out_free1:
		kfree(kvm_regs);
		break;
	}
	case KVM_SET_REGS: {
		struct kvm_regs *kvm_regs;

		kvm_regs = memdup_user(argp, sizeof(*kvm_regs));
		if (IS_ERR(kvm_regs)) {
			r = PTR_ERR(kvm_regs);
			goto out;
		}
		r = kvm_arch_vcpu_ioctl_set_regs(vcpu, kvm_regs);
		kfree(kvm_regs);
		break;
	}
	case KVM_GET_SREGS: {
		kvm_sregs = kzalloc_obj(struct kvm_sregs);
		r = -ENOMEM;
		if (!kvm_sregs)
			goto out;
		r = kvm_arch_vcpu_ioctl_get_sregs(vcpu, kvm_sregs);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, kvm_sregs, sizeof(struct kvm_sregs)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_SREGS: {
		kvm_sregs = memdup_user(argp, sizeof(*kvm_sregs));
		if (IS_ERR(kvm_sregs)) {
			r = PTR_ERR(kvm_sregs);
			kvm_sregs = NULL;
			goto out;
		}
		r = kvm_arch_vcpu_ioctl_set_sregs(vcpu, kvm_sregs);
		break;
	}
	case KVM_GET_MP_STATE: {
		struct kvm_mp_state mp_state;

		r = kvm_arch_vcpu_ioctl_get_mpstate(vcpu, &mp_state);
		if (r)
			goto out;
		r = -EFAULT;
		if (copy_to_user(argp, &mp_state, sizeof(mp_state)))
			goto out;
		r = 0;
		break;
	}
	case KVM_SET_MP_STATE: {
		struct kvm_mp_state mp_state;