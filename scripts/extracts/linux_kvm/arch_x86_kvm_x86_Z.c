// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 58/86



	smp_call_function_single(cpu, tsc_khz_changed, freq, 1);

	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list) {
		kvm_for_each_vcpu(i, vcpu, kvm) {
			if (vcpu->cpu != cpu)
				continue;
			kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
			if (vcpu->cpu != raw_smp_processor_id())
				send_ipi = 1;
		}
	}
	mutex_unlock(&kvm_lock);

	if (freq->old < freq->new && send_ipi) {
		/*
		 * We upscale the frequency.  Must make the guest
		 * doesn't see old kvmclock values while running with
		 * the new frequency, otherwise we risk the guest sees
		 * time go backwards.
		 *
		 * In case we update the frequency for another cpu
		 * (which might be in guest context) send an interrupt
		 * to kick the cpu out of guest context.  Next time
		 * guest context is entered kvmclock will be updated,
		 * so the guest will not see stale values.
		 */
		smp_call_function_single(cpu, tsc_khz_changed, freq, 1);
	}
}

static int kvmclock_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
				     void *data)
{
	struct cpufreq_freqs *freq = data;
	int cpu;

	if (val == CPUFREQ_PRECHANGE && freq->old > freq->new)
		return 0;
	if (val == CPUFREQ_POSTCHANGE && freq->old < freq->new)
		return 0;

	for_each_cpu(cpu, freq->policy->cpus)
		__kvmclock_cpufreq_notifier(freq, cpu);

	return 0;
}

static struct notifier_block kvmclock_cpufreq_notifier_block = {
	.notifier_call  = kvmclock_cpufreq_notifier
};

static int kvmclock_cpu_online(unsigned int cpu)
{
	tsc_khz_changed(NULL);
	return 0;
}

static void kvm_timer_init(void)
{
	if (!boot_cpu_has(X86_FEATURE_CONSTANT_TSC)) {
		max_tsc_khz = tsc_khz;

		if (IS_ENABLED(CONFIG_CPU_FREQ)) {
			struct cpufreq_policy *policy;
			int cpu;

			cpu = get_cpu();
			policy = cpufreq_cpu_get(cpu);
			if (policy) {
				if (policy->cpuinfo.max_freq)
					max_tsc_khz = policy->cpuinfo.max_freq;
				cpufreq_cpu_put(policy);
			}
			put_cpu();
		}
		cpufreq_register_notifier(&kvmclock_cpufreq_notifier_block,
					  CPUFREQ_TRANSITION_NOTIFIER);

		cpuhp_setup_state(CPUHP_AP_X86_KVM_CLK_ONLINE, "x86/kvm/clk:online",
				  kvmclock_cpu_online, kvmclock_cpu_down_prep);
	}
}

#ifdef CONFIG_X86_64
static void pvclock_gtod_update_fn(struct work_struct *work)
{
	struct kvm *kvm;
	struct kvm_vcpu *vcpu;
	unsigned long i;

	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list)
		kvm_for_each_vcpu(i, vcpu, kvm)
			kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);
	atomic_set(&kvm_guest_has_master_clock, 0);
	mutex_unlock(&kvm_lock);
}

static DECLARE_WORK(pvclock_gtod_work, pvclock_gtod_update_fn);

/*
 * Indirection to move queue_work() out of the tk_core.seq write held
 * region to prevent possible deadlocks against time accessors which
 * are invoked with work related locks held.
 */
static void pvclock_irq_work_fn(struct irq_work *w)
{
	queue_work(system_long_wq, &pvclock_gtod_work);
}

static DEFINE_IRQ_WORK(pvclock_irq_work, pvclock_irq_work_fn);

/*
 * Notification about pvclock gtod data update.
 */
static int pvclock_gtod_notify(struct notifier_block *nb, unsigned long unused,
			       void *priv)
{
	struct pvclock_gtod_data *gtod = &pvclock_gtod_data;
	struct timekeeper *tk = priv;

	update_pvclock_gtod(tk);

	/*
	 * Disable master clock if host does not trust, or does not use,
	 * TSC based clocksource. Delegate queue_work() to irq_work as
	 * this is invoked with tk_core.seq write held.
	 */
	if (!gtod_is_based_on_tsc(gtod->clock.vclock_mode) &&
	    atomic_read(&kvm_guest_has_master_clock) != 0)
		irq_work_queue(&pvclock_irq_work);
	return 0;
}

static struct notifier_block pvclock_gtod_notifier = {
	.notifier_call = pvclock_gtod_notify,
};
#endif

void kvm_setup_xss_caps(void)
{
	if (!kvm_cpu_cap_has(X86_FEATURE_XSAVES))
		kvm_caps.supported_xss = 0;

	if (!kvm_cpu_cap_has(X86_FEATURE_SHSTK) &&
	    !kvm_cpu_cap_has(X86_FEATURE_IBT))
		kvm_caps.supported_xss &= ~XFEATURE_MASK_CET_ALL;

	if ((kvm_caps.supported_xss & XFEATURE_MASK_CET_ALL) != XFEATURE_MASK_CET_ALL) {
		kvm_cpu_cap_clear(X86_FEATURE_SHSTK);
		kvm_cpu_cap_clear(X86_FEATURE_IBT);
		kvm_caps.supported_xss &= ~XFEATURE_MASK_CET_ALL;
	}
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_setup_xss_caps);

static void kvm_setup_efer_caps(void)
{
	if (kvm_cpu_cap_has(X86_FEATURE_NX))
		kvm_enable_efer_bits(EFER_NX);

	if (kvm_cpu_cap_has(X86_FEATURE_FXSR_OPT))
		kvm_enable_efer_bits(EFER_FFXSR);

	if (kvm_cpu_cap_has(X86_FEATURE_AUTOIBRS))
		kvm_enable_efer_bits(EFER_AUTOIBRS);
}

static inline void kvm_ops_update(struct kvm_x86_init_ops *ops)
{
	memcpy(&kvm_x86_ops, ops->runtime_ops, sizeof(kvm_x86_ops));

#define __KVM_X86_OP(func) \
	static_call_update(kvm_x86_##func, kvm_x86_ops.func);
#define KVM_X86_OP(func) \
	WARN_ON(!kvm_x86_ops.func); __KVM_X86_OP(func)
#define KVM_X86_OP_OPTIONAL __KVM_X86_OP
#define KVM_X86_OP_OPTIONAL_RET0(func) \
	static_call_update(kvm_x86_##func, (void *)kvm_x86_ops.func ? : \
					   (void *)__static_call_return0);
#include <asm/kvm-x86-ops.h>
#undef __KVM_X86_OP