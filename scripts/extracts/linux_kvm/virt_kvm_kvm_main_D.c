// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 36/36



void __kvm_register_perf_callbacks(unsigned int (*pt_intr_handler)(void),
				   void (*mediated_pmi_handler)(void))
{
	kvm_guest_cbs.handle_intel_pt_intr = pt_intr_handler;
	kvm_guest_cbs.handle_mediated_pmi = mediated_pmi_handler;

	perf_register_guest_info_callbacks(&kvm_guest_cbs);
}
void kvm_unregister_perf_callbacks(void)
{
	perf_unregister_guest_info_callbacks(&kvm_guest_cbs);
}
#endif

int kvm_init(unsigned vcpu_size, unsigned vcpu_align, struct module *module)
{
	int r;
	int cpu;

	/* A kmem cache lets us meet the alignment requirements of fx_save. */
	if (!vcpu_align)
		vcpu_align = __alignof__(struct kvm_vcpu);
	kvm_vcpu_cache =
		kmem_cache_create_usercopy("kvm_vcpu", vcpu_size, vcpu_align,
					   SLAB_ACCOUNT,
					   offsetof(struct kvm_vcpu, arch),
					   offsetofend(struct kvm_vcpu, stats_id)
					   - offsetof(struct kvm_vcpu, arch),
					   NULL);
	if (!kvm_vcpu_cache)
		return -ENOMEM;

	for_each_possible_cpu(cpu) {
		if (!alloc_cpumask_var_node(&per_cpu(cpu_kick_mask, cpu),
					    GFP_KERNEL, cpu_to_node(cpu))) {
			r = -ENOMEM;
			goto err_cpu_kick_mask;
		}
	}

	r = kvm_irqfd_init();
	if (r)
		goto err_irqfd;

	r = kvm_async_pf_init();
	if (r)
		goto err_async_pf;

	kvm_chardev_ops.owner = module;
	kvm_vm_fops.owner = module;
	kvm_vcpu_fops.owner = module;
	kvm_device_fops.owner = module;

	kvm_preempt_ops.sched_in = kvm_sched_in;
	kvm_preempt_ops.sched_out = kvm_sched_out;

	kvm_init_debug();

	r = kvm_vfio_ops_init();
	if (WARN_ON_ONCE(r))
		goto err_vfio;

	r = kvm_gmem_init(module);
	if (r)
		goto err_gmem;

	r = kvm_init_virtualization();
	if (r)
		goto err_virt;

	/*
	 * Registration _must_ be the very last thing done, as this exposes
	 * /dev/kvm to userspace, i.e. all infrastructure must be setup!
	 */
	r = misc_register(&kvm_dev);
	if (r) {
		pr_err("kvm: misc device register failed\n");
		goto err_register;
	}

	return 0;

err_register:
	kvm_uninit_virtualization();
err_virt:
	kvm_gmem_exit();
err_gmem:
	kvm_vfio_ops_exit();
err_vfio:
	kvm_async_pf_deinit();
err_async_pf:
	kvm_irqfd_exit();
err_irqfd:
err_cpu_kick_mask:
	for_each_possible_cpu(cpu)
		free_cpumask_var(per_cpu(cpu_kick_mask, cpu));
	kmem_cache_destroy(kvm_vcpu_cache);
	return r;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_init);

void kvm_exit(void)
{
	int cpu;

	/*
	 * Note, unregistering /dev/kvm doesn't strictly need to come first,
	 * fops_get(), a.k.a. try_module_get(), prevents acquiring references
	 * to KVM while the module is being stopped.
	 */
	misc_deregister(&kvm_dev);

	kvm_uninit_virtualization();

	debugfs_remove_recursive(kvm_debugfs_dir);
	for_each_possible_cpu(cpu)
		free_cpumask_var(per_cpu(cpu_kick_mask, cpu));
	kmem_cache_destroy(kvm_vcpu_cache);
	kvm_gmem_exit();
	kvm_vfio_ops_exit();
	kvm_async_pf_deinit();
	kvm_irqfd_exit();
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_exit);
