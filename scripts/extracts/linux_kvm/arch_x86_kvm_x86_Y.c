// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 57/86



	/*
	 * Workaround userspace that relies on old KVM behavior of %rip being
	 * incremented prior to exiting to userspace to handle "OUT 0x7e".
	 */
	if (port == 0x7e &&
	    kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_OUT_7E_INC_RIP)) {
		vcpu->arch.complete_userspace_io =
			complete_fast_pio_out_port_0x7e;
		kvm_skip_emulated_instruction(vcpu);
	} else {
		vcpu->arch.cui_linear_rip = kvm_get_linear_rip(vcpu);
		vcpu->arch.complete_userspace_io = complete_fast_pio_out;
	}
	return 0;
}

static int complete_fast_pio_in(struct kvm_vcpu *vcpu)
{
	unsigned long val;

	/* We should only ever be called with arch.pio.count equal to 1 */
	if (KVM_BUG_ON(vcpu->arch.pio.count != 1, vcpu->kvm))
		return -EIO;

	if (unlikely(!kvm_is_linear_rip(vcpu, vcpu->arch.cui_linear_rip))) {
		vcpu->arch.pio.count = 0;
		return 1;
	}

	/* For size less than 4 we merge, else we zero extend */
	val = (vcpu->arch.pio.size < 4) ? kvm_rax_read(vcpu) : 0;

	complete_emulator_pio_in(vcpu, &val);
	kvm_rax_write(vcpu, val);

	return kvm_skip_emulated_instruction(vcpu);
}

static int kvm_fast_pio_in(struct kvm_vcpu *vcpu, int size,
			   unsigned short port)
{
	unsigned long val;
	int ret;

	/* For size less than 4 we merge, else we zero extend */
	val = (size < 4) ? kvm_rax_read(vcpu) : 0;

	ret = emulator_pio_in(vcpu, size, port, &val, 1);
	if (ret) {
		kvm_rax_write(vcpu, val);
		return ret;
	}

	vcpu->arch.cui_linear_rip = kvm_get_linear_rip(vcpu);
	vcpu->arch.complete_userspace_io = complete_fast_pio_in;

	return 0;
}

int kvm_fast_pio(struct kvm_vcpu *vcpu, int size, unsigned short port, int in)
{
	int ret;

	if (in)
		ret = kvm_fast_pio_in(vcpu, size, port);
	else
		ret = kvm_fast_pio_out(vcpu, size, port);
	return ret && kvm_skip_emulated_instruction(vcpu);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_fast_pio);

static int kvmclock_cpu_down_prep(unsigned int cpu)
{
	__this_cpu_write(cpu_tsc_khz, 0);
	return 0;
}

static void tsc_khz_changed(void *data)
{
	struct cpufreq_freqs *freq = data;
	unsigned long khz;

	WARN_ON_ONCE(boot_cpu_has(X86_FEATURE_CONSTANT_TSC));

	if (data)
		khz = freq->new;
	else
		khz = cpufreq_quick_get(raw_smp_processor_id());
	if (!khz)
		khz = tsc_khz;
	__this_cpu_write(cpu_tsc_khz, khz);
}

#ifdef CONFIG_X86_64
static void kvm_hyperv_tsc_notifier(void)
{
	struct kvm *kvm;
	int cpu;

	mutex_lock(&kvm_lock);
	list_for_each_entry(kvm, &vm_list, vm_list)
		kvm_make_mclock_inprogress_request(kvm);

	/* no guest entries from this point */
	hyperv_stop_tsc_emulation();

	/* TSC frequency always matches when on Hyper-V */
	if (!boot_cpu_has(X86_FEATURE_CONSTANT_TSC)) {
		for_each_present_cpu(cpu)
			per_cpu(cpu_tsc_khz, cpu) = tsc_khz;
	}
	kvm_caps.max_guest_tsc_khz = tsc_khz;

	list_for_each_entry(kvm, &vm_list, vm_list) {
		__kvm_start_pvclock_update(kvm);
		pvclock_update_vm_gtod_copy(kvm);
		kvm_end_pvclock_update(kvm);
	}

	mutex_unlock(&kvm_lock);
}
#endif

static void __kvmclock_cpufreq_notifier(struct cpufreq_freqs *freq, int cpu)
{
	struct kvm *kvm;
	struct kvm_vcpu *vcpu;
	int send_ipi = 0;
	unsigned long i;

	/*
	 * We allow guests to temporarily run on slowing clocks,
	 * provided we notify them after, or to run on accelerating
	 * clocks, provided we notify them before.  Thus time never
	 * goes backwards.
	 *
	 * However, we have a problem.  We can't atomically update
	 * the frequency of a given CPU from this function; it is
	 * merely a notifier, which can be called from any CPU.
	 * Changing the TSC frequency at arbitrary points in time
	 * requires a recomputation of local variables related to
	 * the TSC for each VCPU.  We must flag these local variables
	 * to be updated and be sure the update takes place with the
	 * new frequency before any guests proceed.
	 *
	 * Unfortunately, the combination of hotplug CPU and frequency
	 * change creates an intractable locking scenario; the order
	 * of when these callouts happen is undefined with respect to
	 * CPU hotplug, and they can race with each other.  As such,
	 * merely setting per_cpu(cpu_tsc_khz) = X during a hotadd is
	 * undefined; you can actually have a CPU frequency change take
	 * place in between the computation of X and the setting of the
	 * variable.  To protect against this problem, all updates of
	 * the per_cpu tsc_khz variable are done in an interrupt
	 * protected IPI, and all callers wishing to update the value
	 * must wait for a synchronous IPI to complete (which is trivial
	 * if the caller is on the CPU already).  This establishes the
	 * necessary total order on variable updates.
	 *
	 * Note that because a guest time update may take place
	 * anytime after the setting of the VCPU's request bit, the
	 * correct TSC value must be set before the request.  However,
	 * to ensure the update actually makes it to any guest which
	 * starts running in hardware virtualization between the set
	 * and the acquisition of the spinlock, we must also ping the
	 * CPU after setting the request bit.
	 *
	 */