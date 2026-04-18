// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 34/36



	/*
	 * If NULL bus is installed, destroy the old bus, including all the
	 * attached devices. Otherwise, destroy the caller's device only.
	 */
	if (!new_bus) {
		pr_err("kvm: failed to shrink bus, removing it completely\n");
		kvm_io_bus_destroy(bus);
		return -ENOMEM;
	}

	kvm_iodevice_destructor(dev);
	kfree(bus);
	return 0;
}

struct kvm_io_device *kvm_io_bus_get_dev(struct kvm *kvm, enum kvm_bus bus_idx,
					 gpa_t addr)
{
	struct kvm_io_bus *bus;
	int dev_idx, srcu_idx;
	struct kvm_io_device *iodev = NULL;

	srcu_idx = srcu_read_lock(&kvm->srcu);

	bus = kvm_get_bus_srcu(kvm, bus_idx);
	if (!bus)
		goto out_unlock;

	dev_idx = kvm_io_bus_get_first_dev(bus, addr, 1);
	if (dev_idx < 0)
		goto out_unlock;

	iodev = bus->range[dev_idx].dev;

out_unlock:
	srcu_read_unlock(&kvm->srcu, srcu_idx);

	return iodev;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_io_bus_get_dev);

static int kvm_debugfs_open(struct inode *inode, struct file *file,
			   int (*get)(void *, u64 *), int (*set)(void *, u64),
			   const char *fmt)
{
	int ret;
	struct kvm_stat_data *stat_data = inode->i_private;

	/*
	 * The debugfs files are a reference to the kvm struct which
        * is still valid when kvm_destroy_vm is called.  kvm_get_kvm_safe
        * avoids the race between open and the removal of the debugfs directory.
	 */
	if (!kvm_get_kvm_safe(stat_data->kvm))
		return -ENOENT;

	ret = simple_attr_open(inode, file, get,
			       kvm_stats_debugfs_mode(stat_data->desc) & 0222
			       ? set : NULL, fmt);
	if (ret)
		kvm_put_kvm(stat_data->kvm);

	return ret;
}

static int kvm_debugfs_release(struct inode *inode, struct file *file)
{
	struct kvm_stat_data *stat_data = inode->i_private;

	simple_attr_release(inode, file);
	kvm_put_kvm(stat_data->kvm);

	return 0;
}

static int kvm_get_stat_per_vm(struct kvm *kvm, size_t offset, u64 *val)
{
	*val = *(u64 *)((void *)(&kvm->stat) + offset);

	return 0;
}

static int kvm_clear_stat_per_vm(struct kvm *kvm, size_t offset)
{
	*(u64 *)((void *)(&kvm->stat) + offset) = 0;

	return 0;
}

static int kvm_get_stat_per_vcpu(struct kvm *kvm, size_t offset, u64 *val)
{
	unsigned long i;
	struct kvm_vcpu *vcpu;

	*val = 0;

	kvm_for_each_vcpu(i, vcpu, kvm)
		*val += *(u64 *)((void *)(&vcpu->stat) + offset);

	return 0;
}

static int kvm_clear_stat_per_vcpu(struct kvm *kvm, size_t offset)
{
	unsigned long i;
	struct kvm_vcpu *vcpu;

	kvm_for_each_vcpu(i, vcpu, kvm)
		*(u64 *)((void *)(&vcpu->stat) + offset) = 0;

	return 0;
}

static int kvm_stat_data_get(void *data, u64 *val)
{
	int r = -EFAULT;
	struct kvm_stat_data *stat_data = data;

	switch (stat_data->kind) {
	case KVM_STAT_VM:
		r = kvm_get_stat_per_vm(stat_data->kvm,
					stat_data->desc->offset, val);
		break;
	case KVM_STAT_VCPU:
		r = kvm_get_stat_per_vcpu(stat_data->kvm,
					  stat_data->desc->offset, val);
		break;
	}

	return r;
}

static int kvm_stat_data_clear(void *data, u64 val)
{
	int r = -EFAULT;
	struct kvm_stat_data *stat_data = data;

	if (val)
		return -EINVAL;

	switch (stat_data->kind) {
	case KVM_STAT_VM:
		r = kvm_clear_stat_per_vm(stat_data->kvm,
					  stat_data->desc->offset);
		break;
	case KVM_STAT_VCPU:
		r = kvm_clear_stat_per_vcpu(stat_data->kvm,
					    stat_data->desc->offset);
		break;
	}

	return r;
}

static int kvm_stat_data_open(struct inode *inode, struct file *file)
{
	__simple_attr_check_format("%llu\n", 0ull);
	return kvm_debugfs_open(inode, file, kvm_stat_data_get,
				kvm_stat_data_clear, "%llu\n");
}

static const struct file_operations stat_fops_per_vm = {
	.owner = THIS_MODULE,
	.open = kvm_stat_data_open,
	.release = kvm_debugfs_release,
	.read = simple_attr_read,
	.write = simple_attr_write,
};

static int vm_stat_get(void *_offset, u64 *val)
{
	unsigned offset = (long)_offset;
	struct kvm *kvm;
	u64 tmp_val;

	*val = 0;
	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_get_stat_per_vm(kvm, offset, &tmp_val);
		*val += tmp_val;
	}
	mutex_unlock(&kvm_lock);
	return 0;
}

static int vm_stat_clear(void *_offset, u64 val)
{
	unsigned offset = (long)_offset;
	struct kvm *kvm;

	if (val)
		return -EINVAL;

	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_clear_stat_per_vm(kvm, offset);
	}
	mutex_unlock(&kvm_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vm_stat_fops, vm_stat_get, vm_stat_clear, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(vm_stat_readonly_fops, vm_stat_get, NULL, "%llu\n");

static int vcpu_stat_get(void *_offset, u64 *val)
{
	unsigned offset = (long)_offset;
	struct kvm *kvm;
	u64 tmp_val;

	*val = 0;
	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_get_stat_per_vcpu(kvm, offset, &tmp_val);
		*val += tmp_val;
	}
	mutex_unlock(&kvm_lock);
	return 0;
}

static int vcpu_stat_clear(void *_offset, u64 val)
{
	unsigned offset = (long)_offset;
	struct kvm *kvm;

	if (val)
		return -EINVAL;