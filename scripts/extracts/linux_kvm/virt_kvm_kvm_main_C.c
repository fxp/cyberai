// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 35/36



	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_clear_stat_per_vcpu(kvm, offset);
	}
	mutex_unlock(&kvm_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vcpu_stat_fops, vcpu_stat_get, vcpu_stat_clear,
			"%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(vcpu_stat_readonly_fops, vcpu_stat_get, NULL, "%llu\n");

static void kvm_uevent_notify_change(unsigned int type, struct kvm *kvm)
{
	struct kobj_uevent_env *env;
	unsigned long long created, active;

	if (!kvm_dev.this_device || !kvm)
		return;

	mutex_lock(&kvm_lock);
	if (type == KVM_EVENT_CREATE_VM) {
		kvm_createvm_count++;
		kvm_active_vms++;
	} else if (type == KVM_EVENT_DESTROY_VM) {
		kvm_active_vms--;
	}
	created = kvm_createvm_count;
	active = kvm_active_vms;
	mutex_unlock(&kvm_lock);

	env = kzalloc_obj(*env);
	if (!env)
		return;

	add_uevent_var(env, "CREATED=%llu", created);
	add_uevent_var(env, "COUNT=%llu", active);

	if (type == KVM_EVENT_CREATE_VM) {
		add_uevent_var(env, "EVENT=create");
		kvm->userspace_pid = task_pid_nr(current);
	} else if (type == KVM_EVENT_DESTROY_VM) {
		add_uevent_var(env, "EVENT=destroy");
	}
	add_uevent_var(env, "PID=%d", kvm->userspace_pid);

	if (!IS_ERR(kvm->debugfs_dentry)) {
		char *tmp, *p = kmalloc(PATH_MAX, GFP_KERNEL);

		if (p) {
			tmp = dentry_path_raw(kvm->debugfs_dentry, p, PATH_MAX);
			if (!IS_ERR(tmp))
				add_uevent_var(env, "STATS_PATH=%s", tmp);
			kfree(p);
		}
	}
	/* no need for checks, since we are adding at most only 5 keys */
	env->envp[env->envp_idx++] = NULL;
	kobject_uevent_env(&kvm_dev.this_device->kobj, KOBJ_CHANGE, env->envp);
	kfree(env);
}

static void kvm_init_debug(void)
{
	const struct file_operations *fops;
	const struct kvm_stats_desc *pdesc;
	int i;

	kvm_debugfs_dir = debugfs_create_dir("kvm", NULL);

	for (i = 0; i < kvm_vm_stats_header.num_desc; ++i) {
		pdesc = &kvm_vm_stats_desc[i];
		if (kvm_stats_debugfs_mode(pdesc) & 0222)
			fops = &vm_stat_fops;
		else
			fops = &vm_stat_readonly_fops;
		debugfs_create_file(pdesc->name, kvm_stats_debugfs_mode(pdesc),
				kvm_debugfs_dir,
				(void *)(long)pdesc->offset, fops);
	}

	for (i = 0; i < kvm_vcpu_stats_header.num_desc; ++i) {
		pdesc = &kvm_vcpu_stats_desc[i];
		if (kvm_stats_debugfs_mode(pdesc) & 0222)
			fops = &vcpu_stat_fops;
		else
			fops = &vcpu_stat_readonly_fops;
		debugfs_create_file(pdesc->name, kvm_stats_debugfs_mode(pdesc),
				kvm_debugfs_dir,
				(void *)(long)pdesc->offset, fops);
	}
}

static inline
struct kvm_vcpu *preempt_notifier_to_vcpu(struct preempt_notifier *pn)
{
	return container_of(pn, struct kvm_vcpu, preempt_notifier);
}

static void kvm_sched_in(struct preempt_notifier *pn, int cpu)
{
	struct kvm_vcpu *vcpu = preempt_notifier_to_vcpu(pn);

	WRITE_ONCE(vcpu->preempted, false);
	WRITE_ONCE(vcpu->ready, false);

	__this_cpu_write(kvm_running_vcpu, vcpu);
	kvm_arch_vcpu_load(vcpu, cpu);

	WRITE_ONCE(vcpu->scheduled_out, false);
}

static void kvm_sched_out(struct preempt_notifier *pn,
			  struct task_struct *next)
{
	struct kvm_vcpu *vcpu = preempt_notifier_to_vcpu(pn);

	WRITE_ONCE(vcpu->scheduled_out, true);

	if (task_is_runnable(current) && vcpu->wants_to_run) {
		WRITE_ONCE(vcpu->preempted, true);
		WRITE_ONCE(vcpu->ready, true);
	}
	kvm_arch_vcpu_put(vcpu);
	__this_cpu_write(kvm_running_vcpu, NULL);
}

/**
 * kvm_get_running_vcpu - get the vcpu running on the current CPU.
 *
 * We can disable preemption locally around accessing the per-CPU variable,
 * and use the resolved vcpu pointer after enabling preemption again,
 * because even if the current thread is migrated to another CPU, reading
 * the per-CPU value later will give us the same value as we update the
 * per-CPU variable in the preempt notifier handlers.
 */
struct kvm_vcpu *kvm_get_running_vcpu(void)
{
	struct kvm_vcpu *vcpu;

	preempt_disable();
	vcpu = __this_cpu_read(kvm_running_vcpu);
	preempt_enable();

	return vcpu;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_get_running_vcpu);

/**
 * kvm_get_running_vcpus - get the per-CPU array of currently running vcpus.
 */
struct kvm_vcpu * __percpu *kvm_get_running_vcpus(void)
{
        return &kvm_running_vcpu;
}

#ifdef CONFIG_GUEST_PERF_EVENTS
static unsigned int kvm_guest_state(void)
{
	struct kvm_vcpu *vcpu = kvm_get_running_vcpu();
	unsigned int state;

	if (!kvm_arch_pmi_in_guest(vcpu))
		return 0;

	state = PERF_GUEST_ACTIVE;
	if (!kvm_arch_vcpu_in_kernel(vcpu))
		state |= PERF_GUEST_USER;

	return state;
}

static unsigned long kvm_guest_get_ip(void)
{
	struct kvm_vcpu *vcpu = kvm_get_running_vcpu();

	/* Retrieving the IP must be guarded by a call to kvm_guest_state(). */
	if (WARN_ON_ONCE(!kvm_arch_pmi_in_guest(vcpu)))
		return 0;

	return kvm_arch_vcpu_get_ip(vcpu);
}

static struct perf_guest_info_callbacks kvm_guest_cbs = {
	.state			= kvm_guest_state,
	.get_ip			= kvm_guest_get_ip,
	.handle_intel_pt_intr	= NULL,
	.handle_mediated_pmi	= NULL,
};