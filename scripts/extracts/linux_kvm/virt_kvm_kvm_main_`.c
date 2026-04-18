// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 32/36



	/*
	 * Some flavors of hardware virtualization need to be disabled before
	 * transferring control to firmware (to perform shutdown/reboot), e.g.
	 * on x86, virtualization can block INIT interrupts, which are used by
	 * firmware to pull APs back under firmware control.  Note, this path
	 * is used for both shutdown and reboot scenarios, i.e. neither name is
	 * 100% comprehensive.
	 */
	pr_info("kvm: exiting hardware virtualization\n");
	on_each_cpu(kvm_disable_virtualization_cpu, NULL, 1);
}

static int kvm_suspend(void *data)
{
	/*
	 * Secondary CPUs and CPU hotplug are disabled across the suspend/resume
	 * callbacks, i.e. no need to acquire kvm_usage_lock to ensure the usage
	 * count is stable.  Assert that kvm_usage_lock is not held to ensure
	 * the system isn't suspended while KVM is enabling hardware.  Hardware
	 * enabling can be preempted, but the task cannot be frozen until it has
	 * dropped all locks (userspace tasks are frozen via a fake signal).
	 */
	lockdep_assert_not_held(&kvm_usage_lock);
	lockdep_assert_irqs_disabled();

	kvm_disable_virtualization_cpu(NULL);
	return 0;
}

static void kvm_resume(void *data)
{
	lockdep_assert_not_held(&kvm_usage_lock);
	lockdep_assert_irqs_disabled();

	WARN_ON_ONCE(kvm_enable_virtualization_cpu());
}

static const struct syscore_ops kvm_syscore_ops = {
	.suspend = kvm_suspend,
	.resume = kvm_resume,
	.shutdown = kvm_shutdown,
};

static struct syscore kvm_syscore = {
	.ops = &kvm_syscore_ops,
};

static int kvm_enable_virtualization(void)
{
	int r;

	guard(mutex)(&kvm_usage_lock);

	if (kvm_usage_count++)
		return 0;

	kvm_arch_enable_virtualization();

	r = cpuhp_setup_state(CPUHP_AP_KVM_ONLINE, "kvm/cpu:online",
			      kvm_online_cpu, kvm_offline_cpu);
	if (r)
		goto err_cpuhp;

	register_syscore(&kvm_syscore);

	/*
	 * Undo virtualization enabling and bail if the system is going down.
	 * If userspace initiated a forced reboot, e.g. reboot -f, then it's
	 * possible for an in-flight operation to enable virtualization after
	 * syscore_shutdown() is called, i.e. without kvm_shutdown() being
	 * invoked.  Note, this relies on system_state being set _before_
	 * kvm_shutdown(), e.g. to ensure either kvm_shutdown() is invoked
	 * or this CPU observes the impending shutdown.  Which is why KVM uses
	 * a syscore ops hook instead of registering a dedicated reboot
	 * notifier (the latter runs before system_state is updated).
	 */
	if (system_state == SYSTEM_HALT || system_state == SYSTEM_POWER_OFF ||
	    system_state == SYSTEM_RESTART) {
		r = -EBUSY;
		goto err_rebooting;
	}

	return 0;

err_rebooting:
	unregister_syscore(&kvm_syscore);
	cpuhp_remove_state(CPUHP_AP_KVM_ONLINE);
err_cpuhp:
	kvm_arch_disable_virtualization();
	--kvm_usage_count;
	return r;
}

static void kvm_disable_virtualization(void)
{
	guard(mutex)(&kvm_usage_lock);

	if (--kvm_usage_count)
		return;

	unregister_syscore(&kvm_syscore);
	cpuhp_remove_state(CPUHP_AP_KVM_ONLINE);
	kvm_arch_disable_virtualization();
}

static int kvm_init_virtualization(void)
{
	if (enable_virt_at_load)
		return kvm_enable_virtualization();

	return 0;
}

static void kvm_uninit_virtualization(void)
{
	if (enable_virt_at_load)
		kvm_disable_virtualization();
}
#else /* CONFIG_KVM_GENERIC_HARDWARE_ENABLING */
static int kvm_enable_virtualization(void)
{
	return 0;
}
static void kvm_disable_virtualization(void)
{

}
static int kvm_init_virtualization(void)
{
	return 0;
}

static void kvm_uninit_virtualization(void)
{

}
#endif /* CONFIG_KVM_GENERIC_HARDWARE_ENABLING */

static void kvm_iodevice_destructor(struct kvm_io_device *dev)
{
	if (dev->ops->destructor)
		dev->ops->destructor(dev);
}

static void kvm_io_bus_destroy(struct kvm_io_bus *bus)
{
	int i;

	for (i = 0; i < bus->dev_count; i++) {
		struct kvm_io_device *pos = bus->range[i].dev;

		kvm_iodevice_destructor(pos);
	}
	kfree(bus);
}

static inline int kvm_io_bus_cmp(const struct kvm_io_range *r1,
				 const struct kvm_io_range *r2)
{
	gpa_t addr1 = r1->addr;
	gpa_t addr2 = r2->addr;

	if (addr1 < addr2)
		return -1;

	/* If r2->len == 0, match the exact address.  If r2->len != 0,
	 * accept any overlapping write.  Any order is acceptable for
	 * overlapping ranges, because kvm_io_bus_get_first_dev ensures
	 * we process all of them.
	 */
	if (r2->len) {
		addr1 += r1->len;
		addr2 += r2->len;
	}

	if (addr1 > addr2)
		return 1;

	return 0;
}

static int kvm_io_bus_sort_cmp(const void *p1, const void *p2)
{
	return kvm_io_bus_cmp(p1, p2);
}

static int kvm_io_bus_get_first_dev(struct kvm_io_bus *bus,
			     gpa_t addr, int len)
{
	struct kvm_io_range *range, key;
	int off;

	key = (struct kvm_io_range) {
		.addr = addr,
		.len = len,
	};

	range = bsearch(&key, bus->range, bus->dev_count,
			sizeof(struct kvm_io_range), kvm_io_bus_sort_cmp);
	if (range == NULL)
		return -ENOENT;

	off = range - bus->range;

	while (off > 0 && kvm_io_bus_cmp(&key, &bus->range[off-1]) == 0)
		off--;

	return off;
}