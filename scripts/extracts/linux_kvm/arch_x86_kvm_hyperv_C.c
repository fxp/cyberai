// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 3/17



	trace_kvm_hv_syndbg_set_msr(vcpu->vcpu_id,
				    to_hv_vcpu(vcpu)->vp_index, msr, data);
	switch (msr) {
	case HV_X64_MSR_SYNDBG_CONTROL:
		syndbg->control.control = data;
		if (!host)
			syndbg_exit(vcpu, msr);
		break;
	case HV_X64_MSR_SYNDBG_STATUS:
		syndbg->control.status = data;
		break;
	case HV_X64_MSR_SYNDBG_SEND_BUFFER:
		syndbg->control.send_page = data;
		break;
	case HV_X64_MSR_SYNDBG_RECV_BUFFER:
		syndbg->control.recv_page = data;
		break;
	case HV_X64_MSR_SYNDBG_PENDING_BUFFER:
		syndbg->control.pending_page = data;
		if (!host)
			syndbg_exit(vcpu, msr);
		break;
	case HV_X64_MSR_SYNDBG_OPTIONS:
		syndbg->options = data;
		break;
	default:
		break;
	}

	return 0;
}

static int syndbg_get_msr(struct kvm_vcpu *vcpu, u32 msr, u64 *pdata, bool host)
{
	struct kvm_hv_syndbg *syndbg = to_hv_syndbg(vcpu);

	if (!kvm_hv_is_syndbg_enabled(vcpu) && !host)
		return 1;

	switch (msr) {
	case HV_X64_MSR_SYNDBG_CONTROL:
		*pdata = syndbg->control.control;
		break;
	case HV_X64_MSR_SYNDBG_STATUS:
		*pdata = syndbg->control.status;
		break;
	case HV_X64_MSR_SYNDBG_SEND_BUFFER:
		*pdata = syndbg->control.send_page;
		break;
	case HV_X64_MSR_SYNDBG_RECV_BUFFER:
		*pdata = syndbg->control.recv_page;
		break;
	case HV_X64_MSR_SYNDBG_PENDING_BUFFER:
		*pdata = syndbg->control.pending_page;
		break;
	case HV_X64_MSR_SYNDBG_OPTIONS:
		*pdata = syndbg->options;
		break;
	default:
		break;
	}

	trace_kvm_hv_syndbg_get_msr(vcpu->vcpu_id, kvm_hv_get_vpindex(vcpu), msr, *pdata);

	return 0;
}

static int synic_get_msr(struct kvm_vcpu_hv_synic *synic, u32 msr, u64 *pdata,
			 bool host)
{
	int ret;

	if (!synic->active && !host)
		return 1;

	ret = 0;
	switch (msr) {
	case HV_X64_MSR_SCONTROL:
		*pdata = synic->control;
		break;
	case HV_X64_MSR_SVERSION:
		*pdata = synic->version;
		break;
	case HV_X64_MSR_SIEFP:
		*pdata = synic->evt_page;
		break;
	case HV_X64_MSR_SIMP:
		*pdata = synic->msg_page;
		break;
	case HV_X64_MSR_EOM:
		*pdata = 0;
		break;
	case HV_X64_MSR_SINT0 ... HV_X64_MSR_SINT15:
		*pdata = atomic64_read(&synic->sint[msr - HV_X64_MSR_SINT0]);
		break;
	default:
		ret = 1;
		break;
	}
	return ret;
}

static int synic_set_irq(struct kvm_vcpu_hv_synic *synic, u32 sint)
{
	struct kvm_vcpu *vcpu = hv_synic_to_vcpu(synic);
	struct kvm_lapic_irq irq;
	int ret, vector;

	if (KVM_BUG_ON(!lapic_in_kernel(vcpu), vcpu->kvm))
		return -EINVAL;

	if (sint >= ARRAY_SIZE(synic->sint))
		return -EINVAL;

	vector = synic_get_sint_vector(synic_read_sint(synic, sint));
	if (vector < 0)
		return -ENOENT;

	memset(&irq, 0, sizeof(irq));
	irq.shorthand = APIC_DEST_SELF;
	irq.dest_mode = APIC_DEST_PHYSICAL;
	irq.delivery_mode = APIC_DM_FIXED;
	irq.vector = vector;
	irq.level = 1;

	ret = kvm_irq_delivery_to_apic(vcpu->kvm, vcpu->arch.apic, &irq);
	trace_kvm_hv_synic_set_irq(vcpu->vcpu_id, sint, irq.vector, ret);
	return ret;
}

int kvm_hv_synic_set_irq(struct kvm_kernel_irq_routing_entry *e, struct kvm *kvm,
			 int irq_source_id, int level, bool line_status)
{
	struct kvm_vcpu_hv_synic *synic;

	if (!level)
		return -1;

	synic = synic_get(kvm, e->hv_sint.vcpu);
	if (!synic)
		return -EINVAL;

	return synic_set_irq(synic, e->hv_sint.sint);
}

void kvm_hv_synic_send_eoi(struct kvm_vcpu *vcpu, int vector)
{
	struct kvm_vcpu_hv_synic *synic = to_hv_synic(vcpu);
	int i;

	trace_kvm_hv_synic_send_eoi(vcpu->vcpu_id, vector);

	for (i = 0; i < ARRAY_SIZE(synic->sint); i++)
		if (synic_get_sint_vector(synic_read_sint(synic, i)) == vector)
			kvm_hv_notify_acked_sint(vcpu, i);
}

static int kvm_hv_set_sint_gsi(struct kvm *kvm, u32 vpidx, u32 sint, int gsi)
{
	struct kvm_vcpu_hv_synic *synic;

	synic = synic_get(kvm, vpidx);
	if (!synic)
		return -EINVAL;

	if (sint >= ARRAY_SIZE(synic->sint_to_gsi))
		return -EINVAL;

	atomic_set(&synic->sint_to_gsi[sint], gsi);
	return 0;
}

void kvm_hv_irq_routing_update(struct kvm *kvm)
{
	struct kvm_irq_routing_table *irq_rt;
	struct kvm_kernel_irq_routing_entry *e;
	u32 gsi;

	irq_rt = srcu_dereference_check(kvm->irq_routing, &kvm->irq_srcu,
					lockdep_is_held(&kvm->irq_lock));

	for (gsi = 0; gsi < irq_rt->nr_rt_entries; gsi++) {
		hlist_for_each_entry(e, &irq_rt->map[gsi], link) {
			if (e->type == KVM_IRQ_ROUTING_HV_SINT)
				kvm_hv_set_sint_gsi(kvm, e->hv_sint.vcpu,
						    e->hv_sint.sint, gsi);
		}
	}
}

static void synic_init(struct kvm_vcpu_hv_synic *synic)
{
	int i;

	memset(synic, 0, sizeof(*synic));
	synic->version = HV_SYNIC_VERSION_1;
	for (i = 0; i < ARRAY_SIZE(synic->sint); i++) {
		atomic64_set(&synic->sint[i], HV_SYNIC_SINT_MASKED);
		atomic_set(&synic->sint_to_gsi[i], -1);
	}
}

static u64 get_time_ref_counter(struct kvm *kvm)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	struct kvm_vcpu *vcpu;
	u64 tsc;

	/*
	 * Fall back to get_kvmclock_ns() when TSC page hasn't been set up,
	 * is broken, disabled or being updated.
	 */
	if (hv->hv_tsc_page_status != HV_TSC_PAGE_SET)
		return div_u64(get_kvmclock_ns(kvm), 100);