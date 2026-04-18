// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 7/17



	tsc_ref->tsc_offset = hv_clock->system_time;
	do_div(tsc_ref->tsc_offset, 100);
	tsc_ref->tsc_offset -=
		mul_u64_u64_shr(hv_clock->tsc_timestamp, tsc_ref->tsc_scale, 64);
	return true;
}

/*
 * Don't touch TSC page values if the guest has opted for TSC emulation after
 * migration. KVM doesn't fully support reenlightenment notifications and TSC
 * access emulation and Hyper-V is known to expect the values in TSC page to
 * stay constant before TSC access emulation is disabled from guest side
 * (HV_X64_MSR_TSC_EMULATION_STATUS). KVM userspace is expected to preserve TSC
 * frequency and guest visible TSC value across migration (and prevent it when
 * TSC scaling is unsupported).
 */
static inline bool tsc_page_update_unsafe(struct kvm_hv *hv)
{
	return (hv->hv_tsc_page_status != HV_TSC_PAGE_GUEST_CHANGED) &&
		hv->hv_tsc_emulation_control;
}

void kvm_hv_setup_tsc_page(struct kvm *kvm,
			   struct pvclock_vcpu_time_info *hv_clock)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	u32 tsc_seq;
	u64 gfn;

	BUILD_BUG_ON(sizeof(tsc_seq) != sizeof(hv->tsc_ref.tsc_sequence));
	BUILD_BUG_ON(offsetof(struct ms_hyperv_tsc_page, tsc_sequence) != 0);

	guard(mutex)(&hv->hv_lock);

	if (hv->hv_tsc_page_status == HV_TSC_PAGE_BROKEN ||
	    hv->hv_tsc_page_status == HV_TSC_PAGE_SET ||
	    hv->hv_tsc_page_status == HV_TSC_PAGE_UNSET)
		return;

	if (!(hv->hv_tsc_page & HV_X64_MSR_TSC_REFERENCE_ENABLE))
		return;

	gfn = hv->hv_tsc_page >> HV_X64_MSR_TSC_REFERENCE_ADDRESS_SHIFT;
	/*
	 * Because the TSC parameters only vary when there is a
	 * change in the master clock, do not bother with caching.
	 */
	if (unlikely(kvm_read_guest(kvm, gfn_to_gpa(gfn),
				    &tsc_seq, sizeof(tsc_seq))))
		goto out_err;

	if (tsc_seq && tsc_page_update_unsafe(hv)) {
		if (kvm_read_guest(kvm, gfn_to_gpa(gfn), &hv->tsc_ref, sizeof(hv->tsc_ref)))
			goto out_err;

		hv->hv_tsc_page_status = HV_TSC_PAGE_SET;
		return;
	}

	/*
	 * While we're computing and writing the parameters, force the
	 * guest to use the time reference count MSR.
	 */
	hv->tsc_ref.tsc_sequence = 0;
	if (kvm_write_guest(kvm, gfn_to_gpa(gfn),
			    &hv->tsc_ref, sizeof(hv->tsc_ref.tsc_sequence)))
		goto out_err;

	if (!compute_tsc_page_parameters(hv_clock, &hv->tsc_ref))
		goto out_err;

	/* Ensure sequence is zero before writing the rest of the struct.  */
	smp_wmb();
	if (kvm_write_guest(kvm, gfn_to_gpa(gfn), &hv->tsc_ref, sizeof(hv->tsc_ref)))
		goto out_err;

	/*
	 * Now switch to the TSC page mechanism by writing the sequence.
	 */
	tsc_seq++;
	if (tsc_seq == 0xFFFFFFFF || tsc_seq == 0)
		tsc_seq = 1;

	/* Write the struct entirely before the non-zero sequence.  */
	smp_wmb();

	hv->tsc_ref.tsc_sequence = tsc_seq;
	if (kvm_write_guest(kvm, gfn_to_gpa(gfn),
			    &hv->tsc_ref, sizeof(hv->tsc_ref.tsc_sequence)))
		goto out_err;

	hv->hv_tsc_page_status = HV_TSC_PAGE_SET;
	return;

out_err:
	hv->hv_tsc_page_status = HV_TSC_PAGE_BROKEN;
}

void kvm_hv_request_tsc_page_update(struct kvm *kvm)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);

	mutex_lock(&hv->hv_lock);

	if (hv->hv_tsc_page_status == HV_TSC_PAGE_SET &&
	    !tsc_page_update_unsafe(hv))
		hv->hv_tsc_page_status = HV_TSC_PAGE_HOST_CHANGED;

	mutex_unlock(&hv->hv_lock);
}

static bool hv_check_msr_access(struct kvm_vcpu_hv *hv_vcpu, u32 msr)
{
	if (!hv_vcpu->enforce_cpuid)
		return true;