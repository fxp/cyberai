// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 78/86



	/*
	 * Sometimes, even reliable TSCs go backwards.  This happens on
	 * platforms that reset TSC during suspend or hibernate actions, but
	 * maintain synchronization.  We must compensate.  Fortunately, we can
	 * detect that condition here, which happens early in CPU bringup,
	 * before any KVM threads can be running.  Unfortunately, we can't
	 * bring the TSCs fully up to date with real time, as we aren't yet far
	 * enough into CPU bringup that we know how much real time has actually
	 * elapsed; our helper function, ktime_get_boottime_ns() will be using boot
	 * variables that haven't been updated yet.
	 *
	 * So we simply find the maximum observed TSC above, then record the
	 * adjustment to TSC in each VCPU.  When the VCPU later gets loaded,
	 * the adjustment will be applied.  Note that we accumulate
	 * adjustments, in case multiple suspend cycles happen before some VCPU
	 * gets a chance to run again.  In the event that no KVM threads get a
	 * chance to run, we will miss the entire elapsed period, as we'll have
	 * reset last_host_tsc, so VCPUs will not have the TSC adjusted and may
	 * loose cycle time.  This isn't too big a deal, since the loss will be
	 * uniform across all VCPUs (not to mention the scenario is extremely
	 * unlikely). It is possible that a second hibernate recovery happens
	 * much faster than a first, causing the observed TSC here to be
	 * smaller; this would require additional padding adjustment, which is
	 * why we set last_host_tsc to the local tsc observed here.
	 *
	 * N.B. - this code below runs only on platforms with reliable TSC,
	 * as that is the only way backwards_tsc is set above.  Also note
	 * that this runs for ALL vcpus, which is not a bug; all VCPUs should
	 * have the same delta_cyc adjustment applied if backwards_tsc
	 * is detected.  Note further, this adjustment is only done once,
	 * as we reset last_host_tsc on all VCPUs to stop this from being
	 * called multiple times (one for each physical CPU bringup).
	 *
	 * Platforms with unreliable TSCs don't have to deal with this, they
	 * will be compensated by the logic in vcpu_load, which sets the TSC to
	 * catchup mode.  This will catchup all VCPUs to real time, but cannot
	 * guarantee that they stay in perfect synchronization.
	 */
	if (backwards_tsc) {
		u64 delta_cyc = max_tsc - local_tsc;
		list_for_each_entry(kvm, &vm_list, vm_list) {
			kvm->arch.backwards_tsc_observed = true;
			kvm_for_each_vcpu(i, vcpu, kvm) {
				vcpu->arch.tsc_offset_adjustment += delta_cyc;
				vcpu->arch.last_host_tsc = local_tsc;
				kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);
			}

			/*
			 * We have to disable TSC offset matching.. if you were
			 * booting a VM while issuing an S4 host suspend....
			 * you may have some problem.  Solving this issue is
			 * left as an exercise to the reader.
			 */
			kvm->arch.last_tsc_nsec = 0;
			kvm->arch.last_tsc_write = 0;
		}

	}
	return 0;
}

void kvm_arch_shutdown(void)
{
	/*
	 * Set virt_rebooting to indicate that KVM has asynchronously disabled
	 * hardware virtualization, i.e. that errors and/or exceptions on SVM
	 * and VMX instructions are expected and should be ignored.
	 */
	virt_rebooting = true;

	/*
	 * Ensure virt_rebooting is visible before IPIs are sent to other CPUs
	 * to disable virtualization.  Effectively pairs with the reception of
	 * the IPI (virt_rebooting is read in task/exception context, but only
	 * _needs_ to be read as %true after the IPI function callback disables
	 * virtualization).
	 */
	smp_wmb();
}

void kvm_arch_disable_virtualization_cpu(void)
{
	kvm_x86_call(disable_virtualization_cpu)();

	/*
	 * Leave the user-return notifiers as-is when disabling virtualization
	 * for reboot, i.e. when disabling via IPI function call, and instead
	 * pin kvm.ko (if it's a module) to defend against use-after-free (in
	 * the *very* unlikely scenario module unload is racing with reboot).
	 * On a forced reboot, tasks aren't frozen before shutdown, and so KVM
	 * could be actively modifying user-return MSR state when the IPI to
	 * disable virtualization arrives.  Handle the extreme edge case here
	 * instead of trying to account for it in the normal flows.
	 */
	if (in_task() || WARN_ON_ONCE(!virt_rebooting))
		drop_user_return_notifiers();
	else
		__module_get(THIS_MODULE);
}

bool kvm_vcpu_is_reset_bsp(struct kvm_vcpu *vcpu)
{
	return vcpu->kvm->arch.bsp_vcpu_id == vcpu->vcpu_id;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_is_reset_bsp);

bool kvm_vcpu_is_bsp(struct kvm_vcpu *vcpu)
{
	return (vcpu->arch.apic_base & MSR_IA32_APICBASE_BSP) != 0;
}

void kvm_arch_free_vm(struct kvm *kvm)
{
#if IS_ENABLED(CONFIG_HYPERV)
	kfree(kvm->arch.hv_pa_pg);
#endif
	__kvm_arch_free_vm(kvm);
}


int kvm_arch_init_vm(struct kvm *kvm, unsigned long type)
{
	int ret;
	unsigned long flags;

	if (!kvm_is_vm_type_supported(type))
		return -EINVAL;