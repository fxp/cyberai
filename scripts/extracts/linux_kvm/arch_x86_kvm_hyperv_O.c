// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 15/17



	/* the eventfd is protected by vcpu->kvm->srcu, but conn_to_evt isn't */
	rcu_read_lock();
	eventfd = idr_find(&hv->conn_to_evt, hc->ingpa);
	rcu_read_unlock();
	if (!eventfd)
		return HV_STATUS_INVALID_PORT_ID;

	eventfd_signal(eventfd);
	return HV_STATUS_SUCCESS;
}

static bool is_xmm_fast_hypercall(struct kvm_hv_hcall *hc)
{
	switch (hc->code) {
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST:
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE:
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST_EX:
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX:
	case HVCALL_SEND_IPI_EX:
		return true;
	}

	return false;
}

static void kvm_hv_hypercall_read_xmm(struct kvm_hv_hcall *hc)
{
	int reg;

	kvm_fpu_get();
	for (reg = 0; reg < HV_HYPERCALL_MAX_XMM_REGISTERS; reg++)
		_kvm_read_sse_reg(reg, &hc->xmm[reg]);
	kvm_fpu_put();
}

static bool hv_check_hypercall_access(struct kvm_vcpu_hv *hv_vcpu, u16 code)
{
	if (!hv_vcpu->enforce_cpuid)
		return true;

	switch (code) {
	case HVCALL_NOTIFY_LONG_SPIN_WAIT:
		return hv_vcpu->cpuid_cache.enlightenments_ebx &&
			hv_vcpu->cpuid_cache.enlightenments_ebx != U32_MAX;
	case HVCALL_POST_MESSAGE:
		return hv_vcpu->cpuid_cache.features_ebx & HV_POST_MESSAGES;
	case HVCALL_SIGNAL_EVENT:
		return hv_vcpu->cpuid_cache.features_ebx & HV_SIGNAL_EVENTS;
	case HVCALL_POST_DEBUG_DATA:
	case HVCALL_RETRIEVE_DEBUG_DATA:
	case HVCALL_RESET_DEBUG_SESSION:
		/*
		 * Return 'true' when SynDBG is disabled so the resulting code
		 * will be HV_STATUS_INVALID_HYPERCALL_CODE.
		 */
		return !kvm_hv_is_syndbg_enabled(hv_vcpu->vcpu) ||
			hv_vcpu->cpuid_cache.features_ebx & HV_DEBUGGING;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST_EX:
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX:
		if (!(hv_vcpu->cpuid_cache.enlightenments_eax &
		      HV_X64_EX_PROCESSOR_MASKS_RECOMMENDED))
			return false;
		fallthrough;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST:
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE:
		return hv_vcpu->cpuid_cache.enlightenments_eax &
			HV_X64_REMOTE_TLB_FLUSH_RECOMMENDED;
	case HVCALL_SEND_IPI_EX:
		if (!(hv_vcpu->cpuid_cache.enlightenments_eax &
		      HV_X64_EX_PROCESSOR_MASKS_RECOMMENDED))
			return false;
		fallthrough;
	case HVCALL_SEND_IPI:
		return hv_vcpu->cpuid_cache.enlightenments_eax &
			HV_X64_CLUSTER_IPI_RECOMMENDED;
	case HV_EXT_CALL_QUERY_CAPABILITIES ... HV_EXT_CALL_MAX:
		return hv_vcpu->cpuid_cache.features_ebx &
			HV_ENABLE_EXTENDED_HYPERCALLS;
	default:
		break;
	}

	return true;
}

int kvm_hv_hypercall(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	struct kvm_hv_hcall hc;
	u64 ret = HV_STATUS_SUCCESS;

	/*
	 * hypercall generates UD from non zero cpl and real mode
	 * per HYPER-V spec
	 */
	if (kvm_x86_call(get_cpl)(vcpu) != 0 || !is_protmode(vcpu)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

#ifdef CONFIG_X86_64
	if (is_64_bit_hypercall(vcpu)) {
		hc.param = kvm_rcx_read(vcpu);
		hc.ingpa = kvm_rdx_read(vcpu);
		hc.outgpa = kvm_r8_read(vcpu);
	} else
#endif
	{
		hc.param = ((u64)kvm_rdx_read(vcpu) << 32) |
			    (kvm_rax_read(vcpu) & 0xffffffff);
		hc.ingpa = ((u64)kvm_rbx_read(vcpu) << 32) |
			    (kvm_rcx_read(vcpu) & 0xffffffff);
		hc.outgpa = ((u64)kvm_rdi_read(vcpu) << 32) |
			     (kvm_rsi_read(vcpu) & 0xffffffff);
	}

	hc.code = hc.param & 0xffff;
	hc.var_cnt = (hc.param & HV_HYPERCALL_VARHEAD_MASK) >> HV_HYPERCALL_VARHEAD_OFFSET;
	hc.fast = !!(hc.param & HV_HYPERCALL_FAST_BIT);
	hc.rep_cnt = (hc.param >> HV_HYPERCALL_REP_COMP_OFFSET) & 0xfff;
	hc.rep_idx = (hc.param >> HV_HYPERCALL_REP_START_OFFSET) & 0xfff;
	hc.rep = !!(hc.rep_cnt || hc.rep_idx);

	trace_kvm_hv_hypercall(hc.code, hc.fast, hc.var_cnt, hc.rep_cnt,
			       hc.rep_idx, hc.ingpa, hc.outgpa);

	if (unlikely(!hv_check_hypercall_access(hv_vcpu, hc.code))) {
		ret = HV_STATUS_ACCESS_DENIED;
		goto hypercall_complete;
	}

	if (unlikely(hc.param & HV_HYPERCALL_RSVD_MASK)) {
		ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
		goto hypercall_complete;
	}

	if (hc.fast && is_xmm_fast_hypercall(&hc)) {
		if (unlikely(hv_vcpu->enforce_cpuid &&
			     !(hv_vcpu->cpuid_cache.features_edx &
			       HV_X64_HYPERCALL_XMM_INPUT_AVAILABLE))) {
			kvm_queue_exception(vcpu, UD_VECTOR);
			return 1;
		}

		kvm_hv_hypercall_read_xmm(&hc);
	}