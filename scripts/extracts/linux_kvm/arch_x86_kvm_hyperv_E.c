// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 5/17



	/*
	 * Strictly following the spec-mandated ordering would assume setting
	 * .msg_pending before checking .message_type.  However, this function
	 * is only called in vcpu context so the entire update is atomic from
	 * guest POV and thus the exact order here doesn't matter.
	 */
	r = kvm_vcpu_read_guest_page(vcpu, msg_page_gfn, &hv_hdr.message_type,
				     msg_off + offsetof(struct hv_message,
							header.message_type),
				     sizeof(hv_hdr.message_type));
	if (r < 0)
		return r;

	if (hv_hdr.message_type != HVMSG_NONE) {
		if (no_retry)
			return 0;

		hv_hdr.message_flags.msg_pending = 1;
		r = kvm_vcpu_write_guest_page(vcpu, msg_page_gfn,
					      &hv_hdr.message_flags,
					      msg_off +
					      offsetof(struct hv_message,
						       header.message_flags),
					      sizeof(hv_hdr.message_flags));
		if (r < 0)
			return r;
		return -EAGAIN;
	}

	r = kvm_vcpu_write_guest_page(vcpu, msg_page_gfn, src_msg, msg_off,
				      sizeof(src_msg->header) +
				      src_msg->header.payload_size);
	if (r < 0)
		return r;

	r = synic_set_irq(synic, sint);
	if (r < 0)
		return r;
	if (r == 0)
		return -EFAULT;
	return 0;
}

static int stimer_send_msg(struct kvm_vcpu_hv_stimer *stimer)
{
	struct kvm_vcpu *vcpu = hv_stimer_to_vcpu(stimer);
	struct hv_message *msg = &stimer->msg;
	struct hv_timer_message_payload *payload =
			(struct hv_timer_message_payload *)&msg->u.payload;

	/*
	 * To avoid piling up periodic ticks, don't retry message
	 * delivery for them (within "lazy" lost ticks policy).
	 */
	bool no_retry = stimer->config.periodic;

	payload->expiration_time = stimer->exp_time;
	payload->delivery_time = get_time_ref_counter(vcpu->kvm);
	return synic_deliver_msg(to_hv_synic(vcpu),
				 stimer->config.sintx, msg,
				 no_retry);
}

static int stimer_notify_direct(struct kvm_vcpu_hv_stimer *stimer)
{
	struct kvm_vcpu *vcpu = hv_stimer_to_vcpu(stimer);
	struct kvm_lapic_irq irq = {
		.delivery_mode = APIC_DM_FIXED,
		.vector = stimer->config.apic_vector
	};

	if (lapic_in_kernel(vcpu))
		return !kvm_apic_set_irq(vcpu, &irq, NULL);
	return 0;
}

static void stimer_expiration(struct kvm_vcpu_hv_stimer *stimer)
{
	int r, direct = stimer->config.direct_mode;

	stimer->msg_pending = true;
	if (!direct)
		r = stimer_send_msg(stimer);
	else
		r = stimer_notify_direct(stimer);
	trace_kvm_hv_stimer_expiration(hv_stimer_to_vcpu(stimer)->vcpu_id,
				       stimer->index, direct, r);
	if (!r) {
		stimer->msg_pending = false;
		if (!(stimer->config.periodic))
			stimer->config.enable = 0;
	}
}

void kvm_hv_process_stimers(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	struct kvm_vcpu_hv_stimer *stimer;
	u64 time_now, exp_time;
	int i;

	if (!hv_vcpu)
		return;

	for (i = 0; i < ARRAY_SIZE(hv_vcpu->stimer); i++)
		if (test_and_clear_bit(i, hv_vcpu->stimer_pending_bitmap)) {
			stimer = &hv_vcpu->stimer[i];
			if (stimer->config.enable) {
				exp_time = stimer->exp_time;

				if (exp_time) {
					time_now =
						get_time_ref_counter(vcpu->kvm);
					if (time_now >= exp_time)
						stimer_expiration(stimer);
				}

				if ((stimer->config.enable) &&
				    stimer->count) {
					if (!stimer->msg_pending)
						stimer_start(stimer);
				} else
					stimer_cleanup(stimer);
			}
		}
}

void kvm_hv_vcpu_uninit(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);
	int i;

	if (!hv_vcpu)
		return;

	for (i = 0; i < ARRAY_SIZE(hv_vcpu->stimer); i++)
		stimer_cleanup(&hv_vcpu->stimer[i]);

	kfree(hv_vcpu);
	vcpu->arch.hyperv = NULL;
}

bool kvm_hv_assist_page_enabled(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);

	if (!hv_vcpu)
		return false;

	if (!(hv_vcpu->hv_vapic & HV_X64_MSR_VP_ASSIST_PAGE_ENABLE))
		return false;
	return vcpu->arch.pv_eoi.msr_val & KVM_MSR_ENABLED;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_hv_assist_page_enabled);

int kvm_hv_get_assist_page(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_hv *hv_vcpu = to_hv_vcpu(vcpu);

	if (!hv_vcpu || !kvm_hv_assist_page_enabled(vcpu))
		return -EFAULT;

	return kvm_read_guest_cached(vcpu->kvm, &vcpu->arch.pv_eoi.data,
				     &hv_vcpu->vp_assist_page, sizeof(struct hv_vp_assist_page));
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_hv_get_assist_page);

static void stimer_prepare_msg(struct kvm_vcpu_hv_stimer *stimer)
{
	struct hv_message *msg = &stimer->msg;
	struct hv_timer_message_payload *payload =
			(struct hv_timer_message_payload *)&msg->u.payload;

	memset(&msg->header, 0, sizeof(msg->header));
	msg->header.message_type = HVMSG_TIMER_EXPIRED;
	msg->header.payload_size = sizeof(*payload);

	payload->timer_index = stimer->index;
	payload->expiration_time = 0;
	payload->delivery_time = 0;
}

static void stimer_init(struct kvm_vcpu_hv_stimer *stimer, int timer_index)
{
	memset(stimer, 0, sizeof(*stimer));
	stimer->index = timer_index;
	hrtimer_setup(&stimer->timer, stimer_timer_callback, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	stimer_prepare_msg(stimer);
}