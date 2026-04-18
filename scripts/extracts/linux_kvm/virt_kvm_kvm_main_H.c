// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 8/36



	r = kvm_create_vm_debugfs(kvm, fdname);
	if (r)
		goto out_err_no_debugfs;

	mutex_lock(&kvm_lock);
	list_add(&kvm->vm_list, &vm_list);
	mutex_unlock(&kvm_lock);

	preempt_notifier_inc();
	kvm_init_pm_notifier(kvm);

	return kvm;

out_err_no_debugfs:
	kvm_coalesced_mmio_free(kvm);
out_no_coalesced_mmio:
	if (kvm->mmu_notifier.ops)
		mmu_notifier_unregister(&kvm->mmu_notifier, current->mm);
out_err_no_mmu_notifier:
	kvm_disable_virtualization();
out_err_no_disable:
	kvm_arch_destroy_vm(kvm);
out_err_no_arch_destroy_vm:
	WARN_ON_ONCE(!refcount_dec_and_test(&kvm->users_count));
	for (i = 0; i < KVM_NR_BUSES; i++)
		kfree(kvm_get_bus_for_destruction(kvm, i));
	kvm_free_irq_routing(kvm);
out_err_no_irq_routing:
	cleanup_srcu_struct(&kvm->irq_srcu);
out_err_no_irq_srcu:
	cleanup_srcu_struct(&kvm->srcu);
out_err_no_srcu:
	kvm_arch_free_vm(kvm);
	mmdrop(current->mm);
	return ERR_PTR(r);
}

static void kvm_destroy_devices(struct kvm *kvm)
{
	struct kvm_device *dev, *tmp;

	/*
	 * We do not need to take the kvm->lock here, because nobody else
	 * has a reference to the struct kvm at this point and therefore
	 * cannot access the devices list anyhow.
	 *
	 * The device list is generally managed as an rculist, but list_del()
	 * is used intentionally here. If a bug in KVM introduced a reader that
	 * was not backed by a reference on the kvm struct, the hope is that
	 * it'd consume the poisoned forward pointer instead of suffering a
	 * use-after-free, even though this cannot be guaranteed.
	 */
	list_for_each_entry_safe(dev, tmp, &kvm->devices, vm_node) {
		list_del(&dev->vm_node);
		dev->ops->destroy(dev);
	}
}

static void kvm_destroy_vm(struct kvm *kvm)
{
	int i;
	struct mm_struct *mm = kvm->mm;

	kvm_destroy_pm_notifier(kvm);
	kvm_uevent_notify_change(KVM_EVENT_DESTROY_VM, kvm);
	kvm_destroy_vm_debugfs(kvm);
	mutex_lock(&kvm_lock);
	list_del(&kvm->vm_list);
	mutex_unlock(&kvm_lock);
	kvm_arch_pre_destroy_vm(kvm);

	kvm_free_irq_routing(kvm);
	for (i = 0; i < KVM_NR_BUSES; i++) {
		struct kvm_io_bus *bus = kvm_get_bus_for_destruction(kvm, i);

		if (bus)
			kvm_io_bus_destroy(bus);
		kvm->buses[i] = NULL;
	}
	kvm_coalesced_mmio_free(kvm);
	mmu_notifier_unregister(&kvm->mmu_notifier, kvm->mm);
	/*
	 * At this point, pending calls to invalidate_range_start()
	 * have completed but no more MMU notifiers will run, so
	 * mn_active_invalidate_count may remain unbalanced.
	 * No threads can be waiting in kvm_swap_active_memslots() as the
	 * last reference on KVM has been dropped, but freeing
	 * memslots would deadlock without this manual intervention.
	 *
	 * If the count isn't unbalanced, i.e. KVM did NOT unregister its MMU
	 * notifier between a start() and end(), then there shouldn't be any
	 * in-progress invalidations.
	 */
	WARN_ON(rcuwait_active(&kvm->mn_memslots_update_rcuwait));
	if (kvm->mn_active_invalidate_count)
		kvm->mn_active_invalidate_count = 0;
	else
		WARN_ON(kvm->mmu_invalidate_in_progress);
	kvm_arch_destroy_vm(kvm);
	kvm_destroy_devices(kvm);
	for (i = 0; i < kvm_arch_nr_memslot_as_ids(kvm); i++) {
		kvm_free_memslots(kvm, &kvm->__memslots[i][0]);
		kvm_free_memslots(kvm, &kvm->__memslots[i][1]);
	}
	cleanup_srcu_struct(&kvm->irq_srcu);
	srcu_barrier(&kvm->srcu);
	cleanup_srcu_struct(&kvm->srcu);
#ifdef CONFIG_KVM_GENERIC_MEMORY_ATTRIBUTES
	xa_destroy(&kvm->mem_attr_array);
#endif
	kvm_arch_free_vm(kvm);
	preempt_notifier_dec();
	kvm_disable_virtualization();
	mmdrop(mm);
}

void kvm_get_kvm(struct kvm *kvm)
{
	refcount_inc(&kvm->users_count);
}
EXPORT_SYMBOL_GPL(kvm_get_kvm);

/*
 * Make sure the vm is not during destruction, which is a safe version of
 * kvm_get_kvm().  Return true if kvm referenced successfully, false otherwise.
 */
bool kvm_get_kvm_safe(struct kvm *kvm)
{
	return refcount_inc_not_zero(&kvm->users_count);
}
EXPORT_SYMBOL_GPL(kvm_get_kvm_safe);

void kvm_put_kvm(struct kvm *kvm)
{
	if (refcount_dec_and_test(&kvm->users_count))
		kvm_destroy_vm(kvm);
}
EXPORT_SYMBOL_GPL(kvm_put_kvm);

/*
 * Used to put a reference that was taken on behalf of an object associated
 * with a user-visible file descriptor, e.g. a vcpu or device, if installation
 * of the new file descriptor fails and the reference cannot be transferred to
 * its final owner.  In such cases, the caller is still actively using @kvm and
 * will fail miserably if the refcount unexpectedly hits zero.
 */
void kvm_put_kvm_no_destroy(struct kvm *kvm)
{
	WARN_ON(refcount_dec_and_test(&kvm->users_count));
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_put_kvm_no_destroy);

static int kvm_vm_release(struct inode *inode, struct file *filp)
{
	struct kvm *kvm = filp->private_data;

	kvm_irqfd_release(kvm);

	kvm_put_kvm(kvm);
	return 0;
}

int kvm_trylock_all_vcpus(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i, j;

	lockdep_assert_held(&kvm->lock);

	kvm_for_each_vcpu(i, vcpu, kvm)
		if (!mutex_trylock_nest_lock(&vcpu->mutex, &kvm->lock))
			goto out_unlock;
	return 0;