// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 2/17



static struct kvm_vcpu *get_vcpu_by_vpidx(struct kvm *kvm, u32 vpidx)
{
	struct kvm_vcpu *vcpu = NULL;
	unsigned long i;

	if (vpidx >= KVM_MAX_VCPUS)
		return NULL;

	vcpu = kvm_get_vcpu(kvm, vpidx);
	if (vcpu && kvm_hv_get_vpindex(vcpu) == vpidx)
		return vcpu;
	kvm_for_each_vcpu(i, vcpu, kvm)
		if (kvm_hv_get_vpindex(vcpu) == vpidx)
			return vcpu;
	return NULL;
}

static struct kvm_vcpu_hv_synic *synic_get(struct kvm *kvm, u32 vpidx)
{
	struct kvm_vcpu *vcpu;
	struct kvm_vcpu_hv_synic *synic;

	vcpu = get_vcpu_by_vpidx(kvm, vpidx);
	if (!vcpu || !to_hv_vcpu(vcpu))
		return NULL;
	synic = to_hv_synic(vcpu);
	return (synic->active) ? synic : NULL;
}

static void kvm_hv_notify_acked_sint(struct kvm_vcpu *vcpu, u32 sint)
{
	struct kvm *kvm = vcpu->kvm;
	struct kvm_vcpu_hv_synic *synic = to_hv_synic(vcpu);
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	struct kvm_vcpu_hv_stimer *stimer;
	int gsi, idx;

	trace_kvm_hv_notify_acked_sint(vcpu->vcpu_id, sint);

	/* Try to deliver pending Hyper-V SynIC timers messages */
	for (idx = 0; idx < ARRAY_SIZE(hv_vcpu->stimer); idx++) {
		stimer = &hv_vcpu->stimer[idx];
		if (stimer->msg_pending && stimer->config.enable &&
		    !stimer->config.direct_mode &&
		    stimer->config.sintx == sint)
			stimer_mark_pending(stimer, false);
	}

	idx = srcu_read_lock(&kvm->irq_srcu);
	gsi = atomic_read(&synic->sint_to_gsi[sint]);
	if (gsi != -1)
		kvm_notify_acked_gsi(kvm, gsi);
	srcu_read_unlock(&kvm->irq_srcu, idx);
}

static void synic_exit(struct kvm_vcpu_hv_synic *synic, u32 msr)
{
	struct kvm_vcpu *vcpu = hv_synic_to_vcpu(synic);
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);

	hv_vcpu->exit.type = KVM_EXIT_HYPERV_SYNIC;
	hv_vcpu->exit.u.synic.msr = msr;
	hv_vcpu->exit.u.synic.control = synic->control;
	hv_vcpu->exit.u.synic.evt_page = synic->evt_page;
	hv_vcpu->exit.u.synic.msg_page = synic->msg_page;

	kvm_make_request(KVM_REQ_HV_EXIT, vcpu);
}

static int synic_set_msr(struct kvm_vcpu_hv_synic *synic,
			 u32 msr, u64 data, bool host)
{
	struct kvm_vcpu *vcpu = hv_synic_to_vcpu(synic);
	int ret;

	if (!synic->active && (!host || data))
		return 1;

	trace_kvm_hv_synic_set_msr(vcpu->vcpu_id, msr, data, host);

	ret = 0;
	switch (msr) {
	case HV_X64_MSR_SCONTROL:
		synic->control = data;
		if (!host)
			synic_exit(synic, msr);
		break;
	case HV_X64_MSR_SVERSION:
		if (!host) {
			ret = 1;
			break;
		}
		synic->version = data;
		break;
	case HV_X64_MSR_SIEFP:
		if ((data & HV_SYNIC_SIEFP_ENABLE) && !host &&
		    !synic->dont_zero_synic_pages)
			if (kvm_clear_guest(vcpu->kvm,
					    data & PAGE_MASK, PAGE_SIZE)) {
				ret = 1;
				break;
			}
		synic->evt_page = data;
		if (!host)
			synic_exit(synic, msr);
		break;
	case HV_X64_MSR_SIMP:
		if ((data & HV_SYNIC_SIMP_ENABLE) && !host &&
		    !synic->dont_zero_synic_pages)
			if (kvm_clear_guest(vcpu->kvm,
					    data & PAGE_MASK, PAGE_SIZE)) {
				ret = 1;
				break;
			}
		synic->msg_page = data;
		if (!host)
			synic_exit(synic, msr);
		break;
	case HV_X64_MSR_EOM: {
		int i;

		if (!synic->active)
			break;

		for (i = 0; i < ARRAY_SIZE(synic->sint); i++)
			kvm_hv_notify_acked_sint(vcpu, i);
		break;
	}
	case HV_X64_MSR_SINT0 ... HV_X64_MSR_SINT15:
		ret = synic_set_sint(synic, msr - HV_X64_MSR_SINT0, data, host);
		break;
	default:
		ret = 1;
		break;
	}
	return ret;
}

static bool kvm_hv_is_syndbg_enabled(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);

	return hv_vcpu->cpuid_cache.syndbg_cap_eax &
		HV_X64_SYNDBG_CAP_ALLOW_KERNEL_DEBUGGING;
}

static int kvm_hv_syndbg_complete_userspace(struct kvm_vcpu *vcpu)
{
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);

	if (vcpu->run->hyperv.u.syndbg.msr == HV_X64_MSR_SYNDBG_CONTROL)
		hv->hv_syndbg.control.status =
			vcpu->run->hyperv.u.syndbg.status;
	return 1;
}

static void syndbg_exit(struct kvm_vcpu *vcpu, u32 msr)
{
	struct kvm_hv_syndbg *syndbg = to_hv_syndbg(vcpu);
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);

	hv_vcpu->exit.type = KVM_EXIT_HYPERV_SYNDBG;
	hv_vcpu->exit.u.syndbg.msr = msr;
	hv_vcpu->exit.u.syndbg.control = syndbg->control.control;
	hv_vcpu->exit.u.syndbg.send_page = syndbg->control.send_page;
	hv_vcpu->exit.u.syndbg.recv_page = syndbg->control.recv_page;
	hv_vcpu->exit.u.syndbg.pending_page = syndbg->control.pending_page;
	vcpu->arch.complete_userspace_io =
			kvm_hv_syndbg_complete_userspace;

	kvm_make_request(KVM_REQ_HV_EXIT, vcpu);
}

static int syndbg_set_msr(struct kvm_vcpu *vcpu, u32 msr, u64 data, bool host)
{
	struct kvm_hv_syndbg *syndbg = to_hv_syndbg(vcpu);

	if (!kvm_hv_is_syndbg_enabled(vcpu) && !host)
		return 1;