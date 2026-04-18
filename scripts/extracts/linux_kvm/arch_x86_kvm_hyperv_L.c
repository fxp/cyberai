// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 12/17



static void hv_tlb_flush_enqueue(struct kvm_vcpu *vcpu,
				 struct kvm_vcpu_hv_tlb_flush_fifo *tlb_flush_fifo,
				 u64 *entries, int count)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	u64 flush_all_entry = KVM_HV_TLB_FLUSHALL_ENTRY;

	if (!hv_vcpu)
		return;

	spin_lock(&tlb_flush_fifo->write_lock);

	/*
	 * All entries should fit on the fifo leaving one free for 'flush all'
	 * entry in case another request comes in. In case there's not enough
	 * space, just put 'flush all' entry there.
	 */
	if (count && entries && count < kfifo_avail(&tlb_flush_fifo->entries)) {
		WARN_ON(kfifo_in(&tlb_flush_fifo->entries, entries, count) != count);
		goto out_unlock;
	}

	/*
	 * Note: full fifo always contains 'flush all' entry, no need to check the
	 * return value.
	 */
	kfifo_in(&tlb_flush_fifo->entries, &flush_all_entry, 1);

out_unlock:
	spin_unlock(&tlb_flush_fifo->write_lock);
}

int kvm_hv_vcpu_flush_tlb(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv_tlb_flush_fifo *tlb_flush_fifo;
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	u64 entries[KVM_HV_TLB_FLUSH_FIFO_SIZE];
	int i, j, count;
	gva_t gva;

	if (!tdp_enabled || !hv_vcpu)
		return -EINVAL;

	tlb_flush_fifo = kvm_hv_get_tlb_flush_fifo(vcpu, is_guest_mode(vcpu));

	count = kfifo_out(&tlb_flush_fifo->entries, entries, KVM_HV_TLB_FLUSH_FIFO_SIZE);

	for (i = 0; i < count; i++) {
		if (entries[i] == KVM_HV_TLB_FLUSHALL_ENTRY)
			goto out_flush_all;

		/*
		 * Lower 12 bits of 'address' encode the number of additional
		 * pages to flush.
		 */
		gva = entries[i] & PAGE_MASK;
		for (j = 0; j < (entries[i] & ~PAGE_MASK) + 1; j++) {
			if (is_noncanonical_invlpg_address(gva + j * PAGE_SIZE, vcpu))
				continue;

			kvm_x86_call(flush_tlb_gva)(vcpu, gva + j * PAGE_SIZE);
		}

		++vcpu->stat.tlb_flush;
	}
	return 0;

out_flush_all:
	kfifo_reset_out(&tlb_flush_fifo->entries);

	/* Fall back to full flush. */
	return -ENOSPC;
}

static u64 kvm_hv_flush_tlb(struct kvm_vcpu *vcpu, struct kvm_hv_hcall *hc)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	unsigned long *vcpu_mask = hv_vcpu->vcpu_mask;
	u64 *sparse_banks = hv_vcpu->sparse_banks;
	struct kvm *kvm = vcpu->kvm;
	struct hv_tlb_flush_ex flush_ex;
	struct hv_tlb_flush flush;
	struct kvm_vcpu_hv_tlb_flush_fifo *tlb_flush_fifo;
	/*
	 * Normally, there can be no more than 'KVM_HV_TLB_FLUSH_FIFO_SIZE'
	 * entries on the TLB flush fifo. The last entry, however, needs to be
	 * always left free for 'flush all' entry which gets placed when
	 * there is not enough space to put all the requested entries.
	 */
	u64 __tlb_flush_entries[KVM_HV_TLB_FLUSH_FIFO_SIZE - 1];
	u64 *tlb_flush_entries;
	u64 valid_bank_mask;
	struct kvm_vcpu *v;
	unsigned long i;
	bool all_cpus;

	/*
	 * The Hyper-V TLFS doesn't allow more than HV_MAX_SPARSE_VCPU_BANKS
	 * sparse banks. Fail the build if KVM's max allowed number of
	 * vCPUs (>4096) exceeds this limit.
	 */
	BUILD_BUG_ON(KVM_HV_MAX_SPARSE_VCPU_SET_BITS > HV_MAX_SPARSE_VCPU_BANKS);

	/*
	 * 'Slow' hypercall's first parameter is the address in guest's memory
	 * where hypercall parameters are placed. This is either a GPA or a
	 * nested GPA when KVM is handling the call from L2 ('direct' TLB
	 * flush).  Translate the address here so the memory can be uniformly
	 * read with kvm_read_guest().
	 */
	if (!hc->fast && is_guest_mode(vcpu)) {
		hc->ingpa = translate_nested_gpa(vcpu, hc->ingpa, 0, NULL);
		if (unlikely(hc->ingpa == INVALID_GPA))
			return HV_STATUS_INVALID_HYPERCALL_INPUT;
	}

	if (hc->code == HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST ||
	    hc->code == HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE) {
		if (hc->fast) {
			flush.address_space = hc->ingpa;
			flush.flags = hc->outgpa;
			flush.processor_mask = sse128_lo(hc->xmm[0]);
			hc->consumed_xmm_halves = 1;
		} else {
			if (unlikely(kvm_read_guest(kvm, hc->ingpa,
						    &flush, sizeof(flush))))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
			hc->data_offset = sizeof(flush);
		}

		trace_kvm_hv_flush_tlb(flush.processor_mask,
				       flush.address_space, flush.flags,
				       is_guest_mode(vcpu));

		valid_bank_mask = BIT_ULL(0);
		sparse_banks[0] = flush.processor_mask;

		/*
		 * Work around possible WS2012 bug: it sends hypercalls
		 * with processor_mask = 0x0 and HV_FLUSH_ALL_PROCESSORS clear,
		 * while also expecting us to flush something and crashing if
		 * we don't. Let's treat processor_mask == 0 same as
		 * HV_FLUSH_ALL_PROCESSORS.
		 */
		all_cpus = (flush.flags & HV_FLUSH_ALL_PROCESSORS) ||
			flush.processor_mask == 0;
	} else {
		if (hc->fast) {
			flush_ex.address_space = hc->ingpa;
			flush_ex.flags = hc->outgpa;
			memcpy(&flush_ex.hv_vp_set,
			       &hc->xmm[0], sizeof(hc->xmm[0]));
			hc->consumed_xmm_halves = 2;
		} else {
			if (unlikely(kvm_read_guest(kvm, hc->ingpa, &flush_ex,
						    sizeof(flush_ex))))
				return HV_STATUS_INVALID_HYPERCALL_INPUT;
			hc->data_offset = sizeof(flush_ex);
		}