// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 11/17



int kvm_hv_set_msr_common(struct kvm_vcpu *vcpu, u32 msr, u64 data, bool host)
{
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);

	if (!host && !vcpu->arch.hyperv_enabled)
		return 1;

	if (kvm_hv_vcpu_init(vcpu))
		return 1;

	if (kvm_hv_msr_partition_wide(msr)) {
		int r;

		mutex_lock(&hv->hv_lock);
		r = kvm_hv_set_msr_pw(vcpu, msr, data, host);
		mutex_unlock(&hv->hv_lock);
		return r;
	} else
		return kvm_hv_set_msr(vcpu, msr, data, host);
}

int kvm_hv_get_msr_common(struct kvm_vcpu *vcpu, u32 msr, u64 *pdata, bool host)
{
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);

	if (!host && !vcpu->arch.hyperv_enabled)
		return 1;

	if (kvm_hv_vcpu_init(vcpu))
		return 1;

	if (kvm_hv_msr_partition_wide(msr)) {
		int r;

		mutex_lock(&hv->hv_lock);
		r = kvm_hv_get_msr_pw(vcpu, msr, pdata, host);
		mutex_unlock(&hv->hv_lock);
		return r;
	} else
		return kvm_hv_get_msr(vcpu, msr, pdata, host);
}

static void sparse_set_to_vcpu_mask(struct kvm *kvm, u64 *sparse_banks,
				    u64 valid_bank_mask, unsigned long *vcpu_mask)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	bool has_mismatch = atomic_read(&hv->num_mismatched_vp_indexes);
	u64 vp_bitmap[KVM_HV_MAX_SPARSE_VCPU_SET_BITS];
	struct kvm_vcpu *vcpu;
	int bank, sbank = 0;
	unsigned long i;
	u64 *bitmap;

	BUILD_BUG_ON(sizeof(vp_bitmap) >
		     sizeof(*vcpu_mask) * BITS_TO_LONGS(KVM_MAX_VCPUS));

	/*
	 * If vp_index == vcpu_idx for all vCPUs, fill vcpu_mask directly, else
	 * fill a temporary buffer and manually test each vCPU's VP index.
	 */
	if (likely(!has_mismatch))
		bitmap = (u64 *)vcpu_mask;
	else
		bitmap = vp_bitmap;

	/*
	 * Each set of 64 VPs is packed into sparse_banks, with valid_bank_mask
	 * having a '1' for each bank that exists in sparse_banks.  Sets must
	 * be in ascending order, i.e. bank0..bankN.
	 */
	memset(bitmap, 0, sizeof(vp_bitmap));
	for_each_set_bit(bank, (unsigned long *)&valid_bank_mask,
			 KVM_HV_MAX_SPARSE_VCPU_SET_BITS)
		bitmap[bank] = sparse_banks[sbank++];

	if (likely(!has_mismatch))
		return;

	bitmap_zero(vcpu_mask, KVM_MAX_VCPUS);
	kvm_for_each_vcpu(i, vcpu, kvm) {
		if (test_bit(kvm_hv_get_vpindex(vcpu), (unsigned long *)vp_bitmap))
			__set_bit(i, vcpu_mask);
	}
}

static bool hv_is_vp_in_sparse_set(u32 vp_id, u64 valid_bank_mask, u64 sparse_banks[])
{
	int valid_bit_nr = vp_id / HV_VCPUS_PER_SPARSE_BANK;
	unsigned long sbank;

	if (!test_bit(valid_bit_nr, (unsigned long *)&valid_bank_mask))
		return false;

	/*
	 * The index into the sparse bank is the number of preceding bits in
	 * the valid mask.  Optimize for VMs with <64 vCPUs by skipping the
	 * fancy math if there can't possibly be preceding bits.
	 */
	if (valid_bit_nr)
		sbank = hweight64(valid_bank_mask & GENMASK_ULL(valid_bit_nr - 1, 0));
	else
		sbank = 0;

	return test_bit(vp_id % HV_VCPUS_PER_SPARSE_BANK,
			(unsigned long *)&sparse_banks[sbank]);
}

struct kvm_hv_hcall {
	/* Hypercall input data */
	u64 param;
	u64 ingpa;
	u64 outgpa;
	u16 code;
	u16 var_cnt;
	u16 rep_cnt;
	u16 rep_idx;
	bool fast;
	bool rep;
	sse128_t xmm[HV_HYPERCALL_MAX_XMM_REGISTERS];

	/*
	 * Current read offset when KVM reads hypercall input data gradually,
	 * either offset in bytes from 'ingpa' for regular hypercalls or the
	 * number of already consumed 'XMM halves' for 'fast' hypercalls.
	 */
	union {
		gpa_t data_offset;
		int consumed_xmm_halves;
	};
};


static int kvm_hv_get_hc_data(struct kvm *kvm, struct kvm_hv_hcall *hc,
			      u16 orig_cnt, u16 cnt_cap, u64 *data)
{
	/*
	 * Preserve the original count when ignoring entries via a "cap", KVM
	 * still needs to validate the guest input (though the non-XMM path
	 * punts on the checks).
	 */
	u16 cnt = min(orig_cnt, cnt_cap);
	int i, j;

	if (hc->fast) {
		/*
		 * Each XMM holds two sparse banks, but do not count halves that
		 * have already been consumed for hypercall parameters.
		 */
		if (orig_cnt > 2 * HV_HYPERCALL_MAX_XMM_REGISTERS - hc->consumed_xmm_halves)
			return HV_STATUS_INVALID_HYPERCALL_INPUT;

		for (i = 0; i < cnt; i++) {
			j = i + hc->consumed_xmm_halves;
			if (j % 2)
				data[i] = sse128_hi(hc->xmm[j / 2]);
			else
				data[i] = sse128_lo(hc->xmm[j / 2]);
		}
		return 0;
	}

	return kvm_read_guest(kvm, hc->ingpa + hc->data_offset, data,
			      cnt * sizeof(*data));
}

static u64 kvm_get_sparse_vp_set(struct kvm *kvm, struct kvm_hv_hcall *hc,
				 u64 *sparse_banks)
{
	if (hc->var_cnt > HV_MAX_SPARSE_VCPU_BANKS)
		return -EINVAL;

	/* Cap var_cnt to ignore banks that cannot contain a legal VP index. */
	return kvm_hv_get_hc_data(kvm, hc, hc->var_cnt, KVM_HV_MAX_SPARSE_VCPU_SET_BITS,
				  sparse_banks);
}

static int kvm_hv_get_tlb_flush_entries(struct kvm *kvm, struct kvm_hv_hcall *hc, u64 entries[])
{
	return kvm_hv_get_hc_data(kvm, hc, hc->rep_cnt, hc->rep_cnt, entries);
}