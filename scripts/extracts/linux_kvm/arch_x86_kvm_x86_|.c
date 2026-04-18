// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 60/86



	if (kvm_caps.has_tsc_control) {
		/*
		 * Make sure the user can only configure tsc_khz values that
		 * fit into a signed integer.
		 * A min value is not calculated because it will always
		 * be 1 on all machines.
		 */
		u64 max = min(0x7fffffffULL,
			      __scale_tsc(kvm_caps.max_tsc_scaling_ratio, tsc_khz));
		kvm_caps.max_guest_tsc_khz = max;
	}
	kvm_caps.default_tsc_scaling_ratio = 1ULL << kvm_caps.tsc_scaling_ratio_frac_bits;
	kvm_init_msr_lists();
	return 0;

out_unwind_ops:
	kvm_x86_ops.enable_virtualization_cpu = NULL;
	kvm_x86_call(hardware_unsetup)();
out_mmu_exit:
	kvm_destroy_user_return_msrs();
	kvm_mmu_vendor_module_exit();
out_free_x86_emulator_cache:
	kmem_cache_destroy(x86_emulator_cache);
	return r;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_x86_vendor_init);

void kvm_x86_vendor_exit(void)
{
	kvm_unregister_perf_callbacks();

#ifdef CONFIG_X86_64
	if (hypervisor_is_type(X86_HYPER_MS_HYPERV))
		clear_hv_tscchange_cb();
#endif
	kvm_lapic_exit();

	if (!boot_cpu_has(X86_FEATURE_CONSTANT_TSC)) {
		cpufreq_unregister_notifier(&kvmclock_cpufreq_notifier_block,
					    CPUFREQ_TRANSITION_NOTIFIER);
		cpuhp_remove_state_nocalls(CPUHP_AP_X86_KVM_CLK_ONLINE);
	}
#ifdef CONFIG_X86_64
	pvclock_gtod_unregister_notifier(&pvclock_gtod_notifier);
	irq_work_sync(&pvclock_irq_work);
	cancel_work_sync(&pvclock_gtod_work);
#endif
	kvm_x86_call(hardware_unsetup)();
	kvm_destroy_user_return_msrs();
	kvm_mmu_vendor_module_exit();
	kmem_cache_destroy(x86_emulator_cache);
#ifdef CONFIG_KVM_XEN
	static_key_deferred_flush(&kvm_xen_enabled);
	WARN_ON(static_branch_unlikely(&kvm_xen_enabled.key));
#endif
	mutex_lock(&vendor_module_lock);
	kvm_x86_ops.enable_virtualization_cpu = NULL;
	mutex_unlock(&vendor_module_lock);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_x86_vendor_exit);

#ifdef CONFIG_X86_64
static int kvm_pv_clock_pairing(struct kvm_vcpu *vcpu, gpa_t paddr,
			        unsigned long clock_type)
{
	struct kvm_clock_pairing clock_pairing;
	struct timespec64 ts;
	u64 cycle;
	int ret;

	if (clock_type != KVM_CLOCK_PAIRING_WALLCLOCK)
		return -KVM_EOPNOTSUPP;

	/*
	 * When tsc is in permanent catchup mode guests won't be able to use
	 * pvclock_read_retry loop to get consistent view of pvclock
	 */
	if (vcpu->arch.tsc_always_catchup)
		return -KVM_EOPNOTSUPP;

	if (!kvm_get_walltime_and_clockread(&ts, &cycle))
		return -KVM_EOPNOTSUPP;

	clock_pairing.sec = ts.tv_sec;
	clock_pairing.nsec = ts.tv_nsec;
	clock_pairing.tsc = kvm_read_l1_tsc(vcpu, cycle);
	clock_pairing.flags = 0;
	memset(&clock_pairing.pad, 0, sizeof(clock_pairing.pad));

	ret = 0;
	if (kvm_write_guest(vcpu->kvm, paddr, &clock_pairing,
			    sizeof(struct kvm_clock_pairing)))
		ret = -KVM_EFAULT;

	return ret;
}
#endif

/*
 * kvm_pv_kick_cpu_op:  Kick a vcpu.
 *
 * @apicid - apicid of vcpu to be kicked.
 */
static void kvm_pv_kick_cpu_op(struct kvm *kvm, int apicid)
{
	/*
	 * All other fields are unused for APIC_DM_REMRD, but may be consumed by
	 * common code, e.g. for tracing. Defer initialization to the compiler.
	 */
	struct kvm_lapic_irq lapic_irq = {
		.delivery_mode = APIC_DM_REMRD,
		.dest_mode = APIC_DEST_PHYSICAL,
		.shorthand = APIC_DEST_NOSHORT,
		.dest_id = apicid,
	};

	kvm_irq_delivery_to_apic(kvm, NULL, &lapic_irq);
}

bool kvm_apicv_activated(struct kvm *kvm)
{
	return (READ_ONCE(kvm->arch.apicv_inhibit_reasons) == 0);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_apicv_activated);

bool kvm_vcpu_apicv_activated(struct kvm_vcpu *vcpu)
{
	ulong vm_reasons = READ_ONCE(vcpu->kvm->arch.apicv_inhibit_reasons);
	ulong vcpu_reasons =
			kvm_x86_call(vcpu_get_apicv_inhibit_reasons)(vcpu);

	return (vm_reasons | vcpu_reasons) == 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_apicv_activated);

static void set_or_clear_apicv_inhibit(unsigned long *inhibits,
				       enum kvm_apicv_inhibit reason, bool set)
{
	const struct trace_print_flags apicv_inhibits[] = { APICV_INHIBIT_REASONS };

	BUILD_BUG_ON(ARRAY_SIZE(apicv_inhibits) != NR_APICV_INHIBIT_REASONS);

	if (set)
		__set_bit(reason, inhibits);
	else
		__clear_bit(reason, inhibits);

	trace_kvm_apicv_inhibit_changed(reason, set, *inhibits);
}

static void kvm_apicv_init(struct kvm *kvm)
{
	enum kvm_apicv_inhibit reason = enable_apicv ? APICV_INHIBIT_REASON_ABSENT :
						       APICV_INHIBIT_REASON_DISABLED;

	set_or_clear_apicv_inhibit(&kvm->arch.apicv_inhibit_reasons, reason, true);

	init_rwsem(&kvm->arch.apicv_update_lock);
}

static void kvm_sched_yield(struct kvm_vcpu *vcpu, unsigned long dest_id)
{
	struct kvm_vcpu *target = NULL;
	struct kvm_apic_map *map;

	vcpu->stat.directed_yield_attempted++;

	if (single_task_running())
		goto no_yield;

	rcu_read_lock();
	map = rcu_dereference(vcpu->kvm->arch.apic_map);

	if (likely(map) && dest_id <= map->max_apic_id) {
		dest_id = array_index_nospec(dest_id, map->max_apic_id + 1);
		if (map->phys_map[dest_id])
			target = map->phys_map[dest_id]->vcpu;
	}

	rcu_read_unlock();

	if (!target || !READ_ONCE(target->ready))
		goto no_yield;