// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 13/17



		trace_kvm_hv_flush_tlb_ex(flush_ex.hv_vp_set.valid_bank_mask,
					  flush_ex.hv_vp_set.format,
					  flush_ex.address_space,
					  flush_ex.flags, is_guest_mode(vcpu));

		valid_bank_mask = flush_ex.hv_vp_set.valid_bank_mask;
		all_cpus = flush_ex.hv_vp_set.format !=
			HV_GENERIC_SET_SPARSE_4K;

		if (hc->var_cnt != hweight64(valid_bank_mask))
			return HV_STATUS_INVALID_HYPERCALL_INPUT;

		if (!all_cpus) {
			if (!hc->var_cnt)
				goto ret_success;

			if (kvm_get_sparse_vp_set(kvm, hc, sparse_banks))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
		}

		/*
		 * Hyper-V TLFS doesn't explicitly forbid non-empty sparse vCPU
		 * banks (and, thus, non-zero 'var_cnt') for the 'all vCPUs'
		 * case (HV_GENERIC_SET_ALL).  Always adjust data_offset and
		 * consumed_xmm_halves to make sure TLB flush entries are read
		 * from the correct offset.
		 */
		if (hc->fast)
			hc->consumed_xmm_halves += hc->var_cnt;
		else
			hc->data_offset += hc->var_cnt * sizeof(sparse_banks[0]);
	}

	if (hc->code == HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE ||
	    hc->code == HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX ||
	    hc->rep_cnt > ARRAY_SIZE(__tlb_flush_entries)) {
		tlb_flush_entries = NULL;
	} else {
		if (kvm_hv_get_tlb_flush_entries(kvm, hc, __tlb_flush_entries))
			return HV_STATUS_INVALID_HYPERCALL_INPUT;
		tlb_flush_entries = __tlb_flush_entries;
	}

	/*
	 * vcpu->arch.cr3 may not be up-to-date for running vCPUs so we can't
	 * analyze it here, flush TLB regardless of the specified address space.
	 */
	if (all_cpus && !is_guest_mode(vcpu)) {
		kvm_for_each_vcpu(i, v, kvm) {
			tlb_flush_fifo = kvm_hv_get_tlb_flush_fifo(v, false);
			hv_tlb_flush_enqueue(v, tlb_flush_fifo,
					     tlb_flush_entries, hc->rep_cnt);
		}

		kvm_make_all_cpus_request(kvm, KVM_REQ_HV_TLB_FLUSH);
	} else if (!is_guest_mode(vcpu)) {
		sparse_set_to_vcpu_mask(kvm, sparse_banks, valid_bank_mask, vcpu_mask);

		for_each_set_bit(i, vcpu_mask, KVM_MAX_VCPUS) {
			v = kvm_get_vcpu(kvm, i);
			if (!v)
				continue;
			tlb_flush_fifo = kvm_hv_get_tlb_flush_fifo(v, false);
			hv_tlb_flush_enqueue(v, tlb_flush_fifo,
					     tlb_flush_entries, hc->rep_cnt);
		}

		kvm_make_vcpus_request_mask(kvm, KVM_REQ_HV_TLB_FLUSH, vcpu_mask);
	} else {
		struct kvm_vcpu_hv *hv_v;

		bitmap_zero(vcpu_mask, KVM_MAX_VCPUS);

		kvm_for_each_vcpu(i, v, kvm) {
			hv_v = to_hv_vcpu(v);

			/*
			 * The following check races with nested vCPUs entering/exiting
			 * and/or migrating between L1's vCPUs, however the only case when
			 * KVM *must* flush the TLB is when the target L2 vCPU keeps
			 * running on the same L1 vCPU from the moment of the request until
			 * kvm_hv_flush_tlb() returns. TLB is fully flushed in all other
			 * cases, e.g. when the target L2 vCPU migrates to a different L1
			 * vCPU or when the corresponding L1 vCPU temporary switches to a
			 * different L2 vCPU while the request is being processed.
			 */
			if (!hv_v || hv_v->nested.vm_id != hv_vcpu->nested.vm_id)
				continue;

			if (!all_cpus &&
			    !hv_is_vp_in_sparse_set(hv_v->nested.vp_id, valid_bank_mask,
						    sparse_banks))
				continue;

			__set_bit(i, vcpu_mask);
			tlb_flush_fifo = kvm_hv_get_tlb_flush_fifo(v, true);
			hv_tlb_flush_enqueue(v, tlb_flush_fifo,
					     tlb_flush_entries, hc->rep_cnt);
		}

		kvm_make_vcpus_request_mask(kvm, KVM_REQ_HV_TLB_FLUSH, vcpu_mask);
	}

ret_success:
	/* We always do full TLB flush, set 'Reps completed' = 'Rep Count' */
	return (u64)HV_STATUS_SUCCESS |
		((u64)hc->rep_cnt << HV_HYPERCALL_REP_COMP_OFFSET);
}

static void kvm_hv_send_ipi_to_many(struct kvm *kvm, u32 vector,
				    u64 *sparse_banks, u64 valid_bank_mask)
{
	struct kvm_lapic_irq irq = {
		.delivery_mode = APIC_DM_FIXED,
		.vector = vector
	};
	struct kvm_vcpu *vcpu;
	unsigned long i;

	kvm_for_each_vcpu(i, vcpu, kvm) {
		if (sparse_banks &&
		    !hv_is_vp_in_sparse_set(kvm_hv_get_vpindex(vcpu),
					    valid_bank_mask, sparse_banks))
			continue;

		/* We fail only when APIC is disabled */
		kvm_apic_set_irq(vcpu, &irq, NULL);
	}
}

static u64 kvm_hv_send_ipi(struct kvm_vcpu *vcpu, struct kvm_hv_hcall *hc)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	u64 *sparse_banks = hv_vcpu->sparse_banks;
	struct kvm *kvm = vcpu->kvm;
	struct hv_send_ipi_ex send_ipi_ex;
	struct hv_send_ipi send_ipi;
	u64 valid_bank_mask;
	u32 vector;
	bool all_cpus;

	if (!lapic_in_kernel(vcpu))
		return HV_STATUS_INVALID_HYPERCALL_INPUT;

	if (hc->code == HVCALL_SEND_IPI) {
		if (!hc->fast) {
			if (unlikely(kvm_read_guest(kvm, hc->ingpa, &send_ipi,
						    sizeof(send_ipi))))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
			sparse_banks[0] = send_ipi.cpu_mask;
			vector = send_ipi.vector;
		} else {
			/* 'reserved' part of hv_send_ipi should be 0 */
			if (unlikely(hc->ingpa >> 32 != 0))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
			sparse_banks[0] = hc->outgpa;
			vector = (u32)hc->ingpa;
		}
		all_cpus = false;
		valid_bank_mask = BIT_ULL(0);