// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 28/36



static const struct kvm_device_ops *kvm_device_ops_table[KVM_DEV_TYPE_MAX] = {
#ifdef CONFIG_KVM_MPIC
	[KVM_DEV_TYPE_FSL_MPIC_20]	= &kvm_mpic_ops,
	[KVM_DEV_TYPE_FSL_MPIC_42]	= &kvm_mpic_ops,
#endif
};

int kvm_register_device_ops(const struct kvm_device_ops *ops, u32 type)
{
	if (type >= ARRAY_SIZE(kvm_device_ops_table))
		return -ENOSPC;

	if (kvm_device_ops_table[type] != NULL)
		return -EEXIST;

	kvm_device_ops_table[type] = ops;
	return 0;
}

void kvm_unregister_device_ops(u32 type)
{
	if (kvm_device_ops_table[type] != NULL)
		kvm_device_ops_table[type] = NULL;
}

static int kvm_ioctl_create_device(struct kvm *kvm,
				   struct kvm_create_device *cd)
{
	const struct kvm_device_ops *ops;
	struct kvm_device *dev;
	bool test = cd->flags & KVM_CREATE_DEVICE_TEST;
	int type;
	int ret;

	if (cd->type >= ARRAY_SIZE(kvm_device_ops_table))
		return -ENODEV;

	type = array_index_nospec(cd->type, ARRAY_SIZE(kvm_device_ops_table));
	ops = kvm_device_ops_table[type];
	if (ops == NULL)
		return -ENODEV;

	if (test)
		return 0;

	dev = kzalloc_obj(*dev, GFP_KERNEL_ACCOUNT);
	if (!dev)
		return -ENOMEM;

	dev->ops = ops;
	dev->kvm = kvm;

	mutex_lock(&kvm->lock);
	ret = ops->create(dev, type);
	if (ret < 0) {
		mutex_unlock(&kvm->lock);
		kfree(dev);
		return ret;
	}
	list_add_rcu(&dev->vm_node, &kvm->devices);
	mutex_unlock(&kvm->lock);

	if (ops->init)
		ops->init(dev);

	kvm_get_kvm(kvm);
	ret = anon_inode_getfd(ops->name, &kvm_device_fops, dev, O_RDWR | O_CLOEXEC);
	if (ret < 0) {
		kvm_put_kvm_no_destroy(kvm);
		mutex_lock(&kvm->lock);
		list_del_rcu(&dev->vm_node);
		synchronize_rcu();
		if (ops->release)
			ops->release(dev);
		mutex_unlock(&kvm->lock);
		if (ops->destroy)
			ops->destroy(dev);
		return ret;
	}

	cd->fd = ret;
	return 0;
}

static int kvm_vm_ioctl_check_extension_generic(struct kvm *kvm, long arg)
{
	switch (arg) {
	case KVM_CAP_SYNC_MMU:
	case KVM_CAP_USER_MEMORY:
	case KVM_CAP_USER_MEMORY2:
	case KVM_CAP_DESTROY_MEMORY_REGION_WORKS:
	case KVM_CAP_JOIN_MEMORY_REGIONS_WORKS:
	case KVM_CAP_INTERNAL_ERROR_DATA:
#ifdef CONFIG_HAVE_KVM_MSI
	case KVM_CAP_SIGNAL_MSI:
#endif
#ifdef CONFIG_HAVE_KVM_IRQCHIP
	case KVM_CAP_IRQFD:
#endif
	case KVM_CAP_IOEVENTFD_ANY_LENGTH:
	case KVM_CAP_CHECK_EXTENSION_VM:
	case KVM_CAP_ENABLE_CAP_VM:
	case KVM_CAP_HALT_POLL:
		return 1;
#ifdef CONFIG_KVM_MMIO
	case KVM_CAP_COALESCED_MMIO:
		return KVM_COALESCED_MMIO_PAGE_OFFSET;
	case KVM_CAP_COALESCED_PIO:
		return 1;
#endif
#ifdef CONFIG_KVM_GENERIC_DIRTYLOG_READ_PROTECT
	case KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2:
		return KVM_DIRTY_LOG_MANUAL_CAPS;
#endif
#ifdef CONFIG_HAVE_KVM_IRQ_ROUTING
	case KVM_CAP_IRQ_ROUTING:
		return KVM_MAX_IRQ_ROUTES;
#endif
#if KVM_MAX_NR_ADDRESS_SPACES > 1
	case KVM_CAP_MULTI_ADDRESS_SPACE:
		if (kvm)
			return kvm_arch_nr_memslot_as_ids(kvm);
		return KVM_MAX_NR_ADDRESS_SPACES;
#endif
	case KVM_CAP_NR_MEMSLOTS:
		return KVM_USER_MEM_SLOTS;
	case KVM_CAP_DIRTY_LOG_RING:
#ifdef CONFIG_HAVE_KVM_DIRTY_RING_TSO
		return KVM_DIRTY_RING_MAX_ENTRIES * sizeof(struct kvm_dirty_gfn);
#else
		return 0;
#endif
	case KVM_CAP_DIRTY_LOG_RING_ACQ_REL:
#ifdef CONFIG_HAVE_KVM_DIRTY_RING_ACQ_REL
		return KVM_DIRTY_RING_MAX_ENTRIES * sizeof(struct kvm_dirty_gfn);
#else
		return 0;
#endif
#ifdef CONFIG_NEED_KVM_DIRTY_RING_WITH_BITMAP
	case KVM_CAP_DIRTY_LOG_RING_WITH_BITMAP:
#endif
	case KVM_CAP_BINARY_STATS_FD:
	case KVM_CAP_SYSTEM_EVENT_DATA:
	case KVM_CAP_DEVICE_CTRL:
		return 1;
#ifdef CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES
	case KVM_CAP_MEMORY_ATTRIBUTES:
		return kvm_supported_mem_attributes(kvm);
#endif
#ifdef CONFIG_KVM_GUEST_MEMFD
	case KVM_CAP_GUEST_MEMFD:
		return 1;
	case KVM_CAP_GUEST_MEMFD_FLAGS:
		return kvm_gmem_get_supported_flags(kvm);
#endif
	default:
		break;
	}
	return kvm_vm_ioctl_check_extension(kvm, arg);
}

static int kvm_vm_ioctl_enable_dirty_log_ring(struct kvm *kvm, u32 size)
{
	int r;

	if (!KVM_DIRTY_LOG_PAGE_OFFSET)
		return -EINVAL;

	/* the size should be power of 2 */
	if (!size || (size & (size - 1)))
		return -EINVAL;

	/* Should be bigger to keep the reserved entries, or a page */
	if (size < kvm_dirty_ring_get_rsvd_entries(kvm) *
	    sizeof(struct kvm_dirty_gfn) || size < PAGE_SIZE)
		return -EINVAL;

	if (size > KVM_DIRTY_RING_MAX_ENTRIES *
	    sizeof(struct kvm_dirty_gfn))
		return -E2BIG;

	/* We only allow it to set once */
	if (kvm->dirty_ring_size)
		return -EINVAL;

	mutex_lock(&kvm->lock);

	if (kvm->created_vcpus) {
		/* We don't allow to change this value after vcpu created */
		r = -EINVAL;
	} else {
		kvm->dirty_ring_size = size;
		r = 0;
	}

	mutex_unlock(&kvm->lock);
	return r;
}

static int kvm_vm_ioctl_reset_dirty_pages(struct kvm *kvm)
{
	unsigned long i;
	struct kvm_vcpu *vcpu;
	int cleared = 0, r;

	if (!kvm->dirty_ring_size)
		return -EINVAL;

	mutex_lock(&kvm->slots_lock);

	kvm_for_each_vcpu(i, vcpu, kvm) {
		r = kvm_dirty_ring_reset(vcpu->kvm, &vcpu->dirty_ring, &cleared);
		if (r)
			break;
	}

	mutex_unlock(&kvm->slots_lock);