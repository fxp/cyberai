// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 14/17



		trace_kvm_hv_send_ipi(vector, sparse_banks[0]);
	} else {
		if (!hc->fast) {
			if (unlikely(kvm_read_guest(kvm, hc->ingpa, &send_ipi_ex,
						    sizeof(send_ipi_ex))))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
		} else {
			send_ipi_ex.vector = (u32)hc->ingpa;
			send_ipi_ex.vp_set.format = hc->outgpa;
			send_ipi_ex.vp_set.valid_bank_mask = sse128_lo(hc->xmm[0]);
		}

		trace_kvm_hv_send_ipi_ex(send_ipi_ex.vector,
					 send_ipi_ex.vp_set.format,
					 send_ipi_ex.vp_set.valid_bank_mask);

		vector = send_ipi_ex.vector;
		valid_bank_mask = send_ipi_ex.vp_set.valid_bank_mask;
		all_cpus = send_ipi_ex.vp_set.format == HV_GENERIC_SET_ALL;

		if (hc->var_cnt != hweight64(valid_bank_mask))
			return HV_STATUS_INVALID_HYPERCALL_INPUT;

		if (all_cpus)
			goto check_and_send_ipi;

		if (!hc->var_cnt)
			goto ret_success;

		if (!hc->fast)
			hc->data_offset = offsetof(struct hv_send_ipi_ex,
						   vp_set.bank_contents);
		else
			hc->consumed_xmm_halves = 1;

		if (kvm_get_sparse_vp_set(kvm, hc, sparse_banks))
			return HV_STATUS_INVALID_HYPERCALL_INPUT;
	}

check_and_send_ipi:
	if ((vector < HV_IPI_LOW_VECTOR) || (vector > HV_IPI_HIGH_VECTOR))
		return HV_STATUS_INVALID_HYPERCALL_INPUT;

	if (all_cpus)
		kvm_hv_send_ipi_to_many(kvm, vector, NULL, 0);
	else
		kvm_hv_send_ipi_to_many(kvm, vector, sparse_banks, valid_bank_mask);

ret_success:
	return HV_STATUS_SUCCESS;
}

void kvm_hv_set_cpuid(struct kvm_vcpu *vcpu, bool hyperv_enabled)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	struct kvm_cpuid_entry2 *entry;

	vcpu->arch.hyperv_enabled = hyperv_enabled;

	if (!hv_vcpu) {
		/*
		 * KVM should have already allocated kvm_vcpu_hv if Hyper-V is
		 * enabled in CPUID.
		 */
		WARN_ON_ONCE(vcpu->arch.hyperv_enabled);
		return;
	}

	memset(&hv_vcpu->cpuid_cache, 0, sizeof(hv_vcpu->cpuid_cache));

	if (!vcpu->arch.hyperv_enabled)
		return;

	entry = kvm_find_cpuid_entry(vcpu, HYPERV_CPUID_FEATURES);
	if (entry) {
		hv_vcpu->cpuid_cache.features_eax = entry->eax;
		hv_vcpu->cpuid_cache.features_ebx = entry->ebx;
		hv_vcpu->cpuid_cache.features_edx = entry->edx;
	}

	entry = kvm_find_cpuid_entry(vcpu, HYPERV_CPUID_ENLIGHTMENT_INFO);
	if (entry) {
		hv_vcpu->cpuid_cache.enlightenments_eax = entry->eax;
		hv_vcpu->cpuid_cache.enlightenments_ebx = entry->ebx;
	}

	entry = kvm_find_cpuid_entry(vcpu, HYPERV_CPUID_SYNDBG_PLATFORM_CAPABILITIES);
	if (entry)
		hv_vcpu->cpuid_cache.syndbg_cap_eax = entry->eax;

	entry = kvm_find_cpuid_entry(vcpu, HYPERV_CPUID_NESTED_FEATURES);
	if (entry) {
		hv_vcpu->cpuid_cache.nested_eax = entry->eax;
		hv_vcpu->cpuid_cache.nested_ebx = entry->ebx;
	}
}

int kvm_hv_set_enforce_cpuid(struct kvm_vcpu *vcpu, bool enforce)
{
	struct kvm_vcpu_hv *hv_vcpu;
	int ret = 0;

	if (!to_hv_vcpu(vcpu)) {
		if (enforce) {
			ret = kvm_hv_vcpu_init(vcpu);
			if (ret)
				return ret;
		} else {
			return 0;
		}
	}

	hv_vcpu = to_hv_vcpu(vcpu);
	hv_vcpu->enforce_cpuid = enforce;

	return ret;
}

static void kvm_hv_hypercall_set_result(struct kvm_vcpu *vcpu, u64 result)
{
	bool longmode;

	longmode = is_64_bit_hypercall(vcpu);
	if (longmode)
		kvm_rax_write(vcpu, result);
	else {
		kvm_rdx_write(vcpu, result >> 32);
		kvm_rax_write(vcpu, result & 0xffffffff);
	}
}

static int kvm_hv_hypercall_complete(struct kvm_vcpu *vcpu, u64 result)
{
	u32 tlb_lock_count = 0;
	int ret;

	if (hv_result_success(result) && is_guest_mode(vcpu) &&
	    kvm_hv_is_tlb_flush_hcall(vcpu) &&
	    kvm_read_guest(vcpu->kvm, to_hv_vcpu(vcpu)->nested.pa_page_gpa,
			   &tlb_lock_count, sizeof(tlb_lock_count)))
		result = HV_STATUS_INVALID_HYPERCALL_INPUT;

	trace_kvm_hv_hypercall_done(result);
	kvm_hv_hypercall_set_result(vcpu, result);
	++vcpu->stat.hypercalls;

	ret = kvm_skip_emulated_instruction(vcpu);

	if (tlb_lock_count)
		kvm_x86_ops.nested_ops->hv_inject_synthetic_vmexit_post_tlb_flush(vcpu);

	return ret;
}

static int kvm_hv_hypercall_complete_userspace(struct kvm_vcpu *vcpu)
{
	return kvm_hv_hypercall_complete(vcpu, vcpu->run->hyperv.u.hcall.result);
}

static u16 kvm_hvcall_signal_event(struct kvm_vcpu *vcpu, struct kvm_hv_hcall *hc)
{
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);
	struct eventfd_ctx *eventfd;

	if (unlikely(!hc->fast)) {
		int ret;
		gpa_t gpa = hc->ingpa;

		if ((gpa & (__alignof__(hc->ingpa) - 1)) ||
		    offset_in_page(gpa) + sizeof(hc->ingpa) > PAGE_SIZE)
			return HV_STATUS_INVALID_ALIGNMENT;

		ret = kvm_vcpu_read_guest(vcpu, gpa,
					  &hc->ingpa, sizeof(hc->ingpa));
		if (ret < 0)
			return HV_STATUS_INVALID_ALIGNMENT;
	}

	/*
	 * Per spec, bits 32-47 contain the extra "flag number".  However, we
	 * have no use for it, and in all known usecases it is zero, so just
	 * report lookup failure if it isn't.
	 */
	if (hc->ingpa & 0xffff00000000ULL)
		return HV_STATUS_INVALID_PORT_ID;
	/* remaining bits are reserved-zero */
	if (hc->ingpa & ~KVM_HYPERV_CONN_ID_MASK)
		return HV_STATUS_INVALID_HYPERCALL_INPUT;