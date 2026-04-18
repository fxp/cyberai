// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 24/36



	/*
	 * The current vCPU ("me") is spinning in kernel mode, i.e. is likely
	 * waiting for a resource to become available.  Attempt to yield to a
	 * vCPU that is runnable, but not currently running, e.g. because the
	 * vCPU was preempted by a higher priority task.  With luck, the vCPU
	 * that was preempted is holding a lock or some other resource that the
	 * current vCPU is waiting to acquire, and yielding to the other vCPU
	 * will allow it to make forward progress and release the lock (or kick
	 * the spinning vCPU, etc).
	 *
	 * Since KVM has no insight into what exactly the guest is doing,
	 * approximate a round-robin selection by iterating over all vCPUs,
	 * starting at the last boosted vCPU.  I.e. if N=kvm->last_boosted_vcpu,
	 * iterate over vCPU[N+1]..vCPU[N-1], wrapping as needed.
	 *
	 * Note, this is inherently racy, e.g. if multiple vCPUs are spinning,
	 * they may all try to yield to the same vCPU(s).  But as above, this
	 * is all best effort due to KVM's lack of visibility into the guest.
	 */
	start = READ_ONCE(kvm->last_boosted_vcpu) + 1;
	for (i = 0; i < nr_vcpus; i++) {
		idx = (start + i) % nr_vcpus;
		if (idx == me->vcpu_idx)
			continue;

		vcpu = xa_load(&kvm->vcpu_array, idx);
		if (!READ_ONCE(vcpu->ready))
			continue;
		if (kvm_vcpu_is_blocking(vcpu) && !vcpu_dy_runnable(vcpu))
			continue;

		/*
		 * Treat the target vCPU as being in-kernel if it has a pending
		 * interrupt, as the vCPU trying to yield may be spinning
		 * waiting on IPI delivery, i.e. the target vCPU is in-kernel
		 * for the purposes of directed yield.
		 */
		if (READ_ONCE(vcpu->preempted) && yield_to_kernel_mode &&
		    !kvm_arch_dy_has_pending_interrupt(vcpu) &&
		    !kvm_arch_vcpu_preempted_in_kernel(vcpu))
			continue;

		if (!kvm_vcpu_eligible_for_directed_yield(vcpu))
			continue;

		yielded = kvm_vcpu_yield_to(vcpu);
		if (yielded > 0) {
			WRITE_ONCE(kvm->last_boosted_vcpu, idx);
			break;
		} else if (yielded < 0 && !--try) {
			break;
		}
	}
	kvm_vcpu_set_in_spin_loop(me, false);

	/* Ensure vcpu is not eligible during next spinloop */
	kvm_vcpu_set_dy_eligible(me, false);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_on_spin);

static bool kvm_page_in_dirty_ring(struct kvm *kvm, unsigned long pgoff)
{
#ifdef CONFIG_HAVE_KVM_DIRTY_RING
	return (pgoff >= KVM_DIRTY_LOG_PAGE_OFFSET) &&
	    (pgoff < KVM_DIRTY_LOG_PAGE_OFFSET +
	     kvm->dirty_ring_size / PAGE_SIZE);
#else
	return false;
#endif
}

static vm_fault_t kvm_vcpu_fault(struct vm_fault *vmf)
{
	struct kvm_vcpu *vcpu = vmf->vma->vm_file->private_data;
	struct page *page;

	if (vmf->pgoff == 0)
		page = virt_to_page(vcpu->run);
#ifdef CONFIG_X86
	else if (vmf->pgoff == KVM_PIO_PAGE_OFFSET)
		page = virt_to_page(vcpu->arch.pio_data);
#endif
#ifdef CONFIG_KVM_MMIO
	else if (vmf->pgoff == KVM_COALESCED_MMIO_PAGE_OFFSET)
		page = virt_to_page(vcpu->kvm->coalesced_mmio_ring);
#endif
	else if (kvm_page_in_dirty_ring(vcpu->kvm, vmf->pgoff))
		page = kvm_dirty_ring_get_page(
		    &vcpu->dirty_ring,
		    vmf->pgoff - KVM_DIRTY_LOG_PAGE_OFFSET);
	else
		return kvm_arch_vcpu_fault(vcpu, vmf);
	get_page(page);
	vmf->page = page;
	return 0;
}

static const struct vm_operations_struct kvm_vcpu_vm_ops = {
	.fault = kvm_vcpu_fault,
};

static int kvm_vcpu_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct kvm_vcpu *vcpu = file->private_data;
	unsigned long pages = vma_pages(vma);

	if ((kvm_page_in_dirty_ring(vcpu->kvm, vma->vm_pgoff) ||
	     kvm_page_in_dirty_ring(vcpu->kvm, vma->vm_pgoff + pages - 1)) &&
	    ((vma->vm_flags & VM_EXEC) || !(vma->vm_flags & VM_SHARED)))
		return -EINVAL;

	vma->vm_ops = &kvm_vcpu_vm_ops;
	return 0;
}

static int kvm_vcpu_release(struct inode *inode, struct file *filp)
{
	struct kvm_vcpu *vcpu = filp->private_data;

	kvm_put_kvm(vcpu->kvm);
	return 0;
}

static struct file_operations kvm_vcpu_fops = {
	.release        = kvm_vcpu_release,
	.unlocked_ioctl = kvm_vcpu_ioctl,
	.mmap           = kvm_vcpu_mmap,
	.llseek		= noop_llseek,
	KVM_COMPAT(kvm_vcpu_compat_ioctl),
};

/*
 * Allocates an inode for the vcpu.
 */
static int create_vcpu_fd(struct kvm_vcpu *vcpu)
{
	char name[8 + 1 + ITOA_MAX_LEN + 1];

	snprintf(name, sizeof(name), "kvm-vcpu:%d", vcpu->vcpu_id);
	return anon_inode_getfd(name, &kvm_vcpu_fops, vcpu, O_RDWR | O_CLOEXEC);
}

#ifdef __KVM_HAVE_ARCH_VCPU_DEBUGFS
static int vcpu_get_pid(void *data, u64 *val)
{
	struct kvm_vcpu *vcpu = data;

	read_lock(&vcpu->pid_lock);
	*val = pid_nr(vcpu->pid);
	read_unlock(&vcpu->pid_lock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vcpu_get_pid_fops, vcpu_get_pid, NULL, "%llu\n");

static void kvm_create_vcpu_debugfs(struct kvm_vcpu *vcpu)
{
	struct dentry *debugfs_dentry;
	char dir_name[ITOA_MAX_LEN * 2];

	if (!debugfs_initialized())
		return;