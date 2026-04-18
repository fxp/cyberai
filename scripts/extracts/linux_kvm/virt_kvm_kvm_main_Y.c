// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 25/36



	snprintf(dir_name, sizeof(dir_name), "vcpu%d", vcpu->vcpu_id);
	debugfs_dentry = debugfs_create_dir(dir_name,
					    vcpu->kvm->debugfs_dentry);
	debugfs_create_file("pid", 0444, debugfs_dentry, vcpu,
			    &vcpu_get_pid_fops);

	kvm_arch_create_vcpu_debugfs(vcpu, debugfs_dentry);
}
#endif

/*
 * Creates some virtual cpus.  Good luck creating more than one.
 */
static int kvm_vm_ioctl_create_vcpu(struct kvm *kvm, unsigned long id)
{
	int r;
	struct kvm_vcpu *vcpu;
	struct page *page;

	/*
	 * KVM tracks vCPU IDs as 'int', be kind to userspace and reject
	 * too-large values instead of silently truncating.
	 *
	 * Ensure KVM_MAX_VCPU_IDS isn't pushed above INT_MAX without first
	 * changing the storage type (at the very least, IDs should be tracked
	 * as unsigned ints).
	 */
	BUILD_BUG_ON(KVM_MAX_VCPU_IDS > INT_MAX);
	if (id >= KVM_MAX_VCPU_IDS)
		return -EINVAL;

	mutex_lock(&kvm->lock);
	if (kvm->created_vcpus >= kvm->max_vcpus) {
		mutex_unlock(&kvm->lock);
		return -EINVAL;
	}

	r = kvm_arch_vcpu_precreate(kvm, id);
	if (r) {
		mutex_unlock(&kvm->lock);
		return r;
	}

	kvm->created_vcpus++;
	mutex_unlock(&kvm->lock);

	vcpu = kmem_cache_zalloc(kvm_vcpu_cache, GFP_KERNEL_ACCOUNT);
	if (!vcpu) {
		r = -ENOMEM;
		goto vcpu_decrement;
	}

	BUILD_BUG_ON(sizeof(struct kvm_run) > PAGE_SIZE);
	page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!page) {
		r = -ENOMEM;
		goto vcpu_free;
	}
	vcpu->run = page_address(page);

	kvm_vcpu_init(vcpu, kvm, id);

	r = kvm_arch_vcpu_create(vcpu);
	if (r)
		goto vcpu_free_run_page;

	if (kvm->dirty_ring_size) {
		r = kvm_dirty_ring_alloc(kvm, &vcpu->dirty_ring,
					 id, kvm->dirty_ring_size);
		if (r)
			goto arch_vcpu_destroy;
	}

	mutex_lock(&kvm->lock);

	if (kvm_get_vcpu_by_id(kvm, id)) {
		r = -EEXIST;
		goto unlock_vcpu_destroy;
	}

	vcpu->vcpu_idx = atomic_read(&kvm->online_vcpus);
	r = xa_insert(&kvm->vcpu_array, vcpu->vcpu_idx, vcpu, GFP_KERNEL_ACCOUNT);
	WARN_ON_ONCE(r == -EBUSY);
	if (r)
		goto unlock_vcpu_destroy;

	/*
	 * Now it's all set up, let userspace reach it.  Grab the vCPU's mutex
	 * so that userspace can't invoke vCPU ioctl()s until the vCPU is fully
	 * visible (per online_vcpus), e.g. so that KVM doesn't get tricked
	 * into a NULL-pointer dereference because KVM thinks the _current_
	 * vCPU doesn't exist.  As a bonus, taking vcpu->mutex ensures lockdep
	 * knows it's taken *inside* kvm->lock.
	 */
	mutex_lock(&vcpu->mutex);
	kvm_get_kvm(kvm);
	r = create_vcpu_fd(vcpu);
	if (r < 0)
		goto kvm_put_xa_erase;

	/*
	 * Pairs with smp_rmb() in kvm_get_vcpu.  Store the vcpu
	 * pointer before kvm->online_vcpu's incremented value.
	 */
	smp_wmb();
	atomic_inc(&kvm->online_vcpus);
	mutex_unlock(&vcpu->mutex);

	mutex_unlock(&kvm->lock);
	kvm_arch_vcpu_postcreate(vcpu);
	kvm_create_vcpu_debugfs(vcpu);
	return r;

kvm_put_xa_erase:
	mutex_unlock(&vcpu->mutex);
	kvm_put_kvm_no_destroy(kvm);
	xa_erase(&kvm->vcpu_array, vcpu->vcpu_idx);
unlock_vcpu_destroy:
	mutex_unlock(&kvm->lock);
	kvm_dirty_ring_free(&vcpu->dirty_ring);
arch_vcpu_destroy:
	kvm_arch_vcpu_destroy(vcpu);
vcpu_free_run_page:
	free_page((unsigned long)vcpu->run);
vcpu_free:
	kmem_cache_free(kvm_vcpu_cache, vcpu);
vcpu_decrement:
	mutex_lock(&kvm->lock);
	kvm->created_vcpus--;
	mutex_unlock(&kvm->lock);
	return r;
}

static int kvm_vcpu_ioctl_set_sigmask(struct kvm_vcpu *vcpu, sigset_t *sigset)
{
	if (sigset) {
		sigdelsetmask(sigset, sigmask(SIGKILL)|sigmask(SIGSTOP));
		vcpu->sigset_active = 1;
		vcpu->sigset = *sigset;
	} else
		vcpu->sigset_active = 0;
	return 0;
}

static ssize_t kvm_vcpu_stats_read(struct file *file, char __user *user_buffer,
			      size_t size, loff_t *offset)
{
	struct kvm_vcpu *vcpu = file->private_data;

	return kvm_stats_read(vcpu->stats_id, &kvm_vcpu_stats_header,
			&kvm_vcpu_stats_desc[0], &vcpu->stat,
			sizeof(vcpu->stat), user_buffer, size, offset);
}

static int kvm_vcpu_stats_release(struct inode *inode, struct file *file)
{
	struct kvm_vcpu *vcpu = file->private_data;

	kvm_put_kvm(vcpu->kvm);
	return 0;
}

static const struct file_operations kvm_vcpu_stats_fops = {
	.owner = THIS_MODULE,
	.read = kvm_vcpu_stats_read,
	.release = kvm_vcpu_stats_release,
	.llseek = noop_llseek,
};

static int kvm_vcpu_ioctl_get_stats_fd(struct kvm_vcpu *vcpu)
{
	int fd;
	struct file *file;
	char name[15 + ITOA_MAX_LEN + 1];

	snprintf(name, sizeof(name), "kvm-vcpu-stats:%d", vcpu->vcpu_id);

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return fd;

	file = anon_inode_getfile_fmode(name, &kvm_vcpu_stats_fops, vcpu,
					O_RDONLY, FMODE_PREAD);
	if (IS_ERR(file)) {
		put_unused_fd(fd);
		return PTR_ERR(file);
	}

	kvm_get_kvm(vcpu->kvm);
	fd_install(fd, file);

	return fd;
}

#ifdef CONFIG_KVM_GENERIC_PRE_FAULT_MEMORY
static int kvm_vcpu_pre_fault_memory(struct kvm_vcpu *vcpu,
				     struct kvm_pre_fault_memory *range)
{
	int idx;
	long r;
	u64 full_size;

	if (range->flags)
		return -EINVAL;