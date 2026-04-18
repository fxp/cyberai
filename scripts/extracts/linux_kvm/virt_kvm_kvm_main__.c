// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 31/36



	switch (ioctl) {
#ifdef CONFIG_KVM_GENERIC_DIRTYLOG_READ_PROTECT
	case KVM_CLEAR_DIRTY_LOG: {
		struct compat_kvm_clear_dirty_log compat_log;
		struct kvm_clear_dirty_log log;

		if (copy_from_user(&compat_log, (void __user *)arg,
				   sizeof(compat_log)))
			return -EFAULT;
		log.slot	 = compat_log.slot;
		log.num_pages	 = compat_log.num_pages;
		log.first_page	 = compat_log.first_page;
		log.padding2	 = compat_log.padding2;
		log.dirty_bitmap = compat_ptr(compat_log.dirty_bitmap);

		r = kvm_vm_ioctl_clear_dirty_log(kvm, &log);
		break;
	}
#endif
	case KVM_GET_DIRTY_LOG: {
		struct compat_kvm_dirty_log compat_log;
		struct kvm_dirty_log log;

		if (copy_from_user(&compat_log, (void __user *)arg,
				   sizeof(compat_log)))
			return -EFAULT;
		log.slot	 = compat_log.slot;
		log.padding1	 = compat_log.padding1;
		log.padding2	 = compat_log.padding2;
		log.dirty_bitmap = compat_ptr(compat_log.dirty_bitmap);

		r = kvm_vm_ioctl_get_dirty_log(kvm, &log);
		break;
	}
	default:
		r = kvm_vm_ioctl(filp, ioctl, arg);
	}
	return r;
}
#endif

static struct file_operations kvm_vm_fops = {
	.release        = kvm_vm_release,
	.unlocked_ioctl = kvm_vm_ioctl,
	.llseek		= noop_llseek,
	KVM_COMPAT(kvm_vm_compat_ioctl),
};

bool file_is_kvm(struct file *file)
{
	return file && file->f_op == &kvm_vm_fops;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(file_is_kvm);

static int kvm_dev_ioctl_create_vm(unsigned long type)
{
	char fdname[ITOA_MAX_LEN + 1];
	int r, fd;
	struct kvm *kvm;
	struct file *file;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return fd;

	snprintf(fdname, sizeof(fdname), "%d", fd);

	kvm = kvm_create_vm(type, fdname);
	if (IS_ERR(kvm)) {
		r = PTR_ERR(kvm);
		goto put_fd;
	}

	file = anon_inode_getfile("kvm-vm", &kvm_vm_fops, kvm, O_RDWR);
	if (IS_ERR(file)) {
		r = PTR_ERR(file);
		goto put_kvm;
	}

	/*
	 * Don't call kvm_put_kvm anymore at this point; file->f_op is
	 * already set, with ->release() being kvm_vm_release().  In error
	 * cases it will be called by the final fput(file) and will take
	 * care of doing kvm_put_kvm(kvm).
	 */
	kvm_uevent_notify_change(KVM_EVENT_CREATE_VM, kvm);

	fd_install(fd, file);
	return fd;

put_kvm:
	kvm_put_kvm(kvm);
put_fd:
	put_unused_fd(fd);
	return r;
}

static long kvm_dev_ioctl(struct file *filp,
			  unsigned int ioctl, unsigned long arg)
{
	int r = -EINVAL;

	switch (ioctl) {
	case KVM_GET_API_VERSION:
		if (arg)
			goto out;
		r = KVM_API_VERSION;
		break;
	case KVM_CREATE_VM:
		r = kvm_dev_ioctl_create_vm(arg);
		break;
	case KVM_CHECK_EXTENSION:
		r = kvm_vm_ioctl_check_extension_generic(NULL, arg);
		break;
	case KVM_GET_VCPU_MMAP_SIZE:
		if (arg)
			goto out;
		r = PAGE_SIZE;     /* struct kvm_run */
#ifdef CONFIG_X86
		r += PAGE_SIZE;    /* pio data page */
#endif
#ifdef CONFIG_KVM_MMIO
		r += PAGE_SIZE;    /* coalesced mmio ring page */
#endif
		break;
	default:
		return kvm_arch_dev_ioctl(filp, ioctl, arg);
	}
out:
	return r;
}

static struct file_operations kvm_chardev_ops = {
	.unlocked_ioctl = kvm_dev_ioctl,
	.llseek		= noop_llseek,
	KVM_COMPAT(kvm_dev_ioctl),
};

static struct miscdevice kvm_dev = {
	KVM_MINOR,
	"kvm",
	&kvm_chardev_ops,
};

#ifdef CONFIG_KVM_GENERIC_HARDWARE_ENABLING
bool __ro_after_init enable_virt_at_load = true;
module_param(enable_virt_at_load, bool, 0444);
EXPORT_SYMBOL_FOR_KVM_INTERNAL(enable_virt_at_load);

static DEFINE_PER_CPU(bool, virtualization_enabled);
static DEFINE_MUTEX(kvm_usage_lock);
static int kvm_usage_count;

__weak void kvm_arch_shutdown(void)
{

}

__weak void kvm_arch_enable_virtualization(void)
{

}

__weak void kvm_arch_disable_virtualization(void)
{

}

static int kvm_enable_virtualization_cpu(void)
{
	if (__this_cpu_read(virtualization_enabled))
		return 0;

	if (kvm_arch_enable_virtualization_cpu()) {
		pr_info("kvm: enabling virtualization on CPU%d failed\n",
			raw_smp_processor_id());
		return -EIO;
	}

	__this_cpu_write(virtualization_enabled, true);
	return 0;
}

static int kvm_online_cpu(unsigned int cpu)
{
	/*
	 * Abort the CPU online process if hardware virtualization cannot
	 * be enabled. Otherwise running VMs would encounter unrecoverable
	 * errors when scheduled to this CPU.
	 */
	return kvm_enable_virtualization_cpu();
}

static void kvm_disable_virtualization_cpu(void *ign)
{
	if (!__this_cpu_read(virtualization_enabled))
		return;

	kvm_arch_disable_virtualization_cpu();

	__this_cpu_write(virtualization_enabled, false);
}

static int kvm_offline_cpu(unsigned int cpu)
{
	kvm_disable_virtualization_cpu(NULL);
	return 0;
}

static void kvm_shutdown(void *data)
{
	kvm_arch_shutdown();