// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 82/86



	if (!kvm->arch.n_requested_mmu_pages &&
	    (change == KVM_MR_CREATE || change == KVM_MR_DELETE)) {
		unsigned long nr_mmu_pages;

		nr_mmu_pages = kvm->nr_memslot_pages / KVM_MEMSLOT_PAGES_TO_MMU_PAGES_RATIO;
		nr_mmu_pages = max(nr_mmu_pages, KVM_MIN_ALLOC_MMU_PAGES);
		kvm_mmu_change_mmu_pages(kvm, nr_mmu_pages);
	}

	kvm_mmu_slot_apply_flags(kvm, old, new, change);

	/* Free the arrays associated with the old memslot. */
	if (change == KVM_MR_MOVE)
		kvm_arch_free_memslot(kvm, old);
}

bool kvm_arch_vcpu_in_kernel(struct kvm_vcpu *vcpu)
{
	WARN_ON_ONCE(!kvm_arch_pmi_in_guest(vcpu));

	if (vcpu->arch.guest_state_protected)
		return true;

	return kvm_x86_call(get_cpl)(vcpu) == 0;
}

unsigned long kvm_arch_vcpu_get_ip(struct kvm_vcpu *vcpu)
{
	WARN_ON_ONCE(!kvm_arch_pmi_in_guest(vcpu));

	if (vcpu->arch.guest_state_protected)
		return 0;

	return kvm_rip_read(vcpu);
}

int kvm_arch_vcpu_should_kick(struct kvm_vcpu *vcpu)
{
	return kvm_vcpu_exiting_guest_mode(vcpu) == IN_GUEST_MODE;
}

int kvm_arch_interrupt_allowed(struct kvm_vcpu *vcpu)
{
	return kvm_x86_call(interrupt_allowed)(vcpu, false);
}

unsigned long kvm_get_linear_rip(struct kvm_vcpu *vcpu)
{
	/* Can't read the RIP when guest state is protected, just return 0 */
	if (vcpu->arch.guest_state_protected)
		return 0;

	if (is_64_bit_mode(vcpu))
		return kvm_rip_read(vcpu);
	return (u32)(get_segment_base(vcpu, VCPU_SREG_CS) +
		     kvm_rip_read(vcpu));
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_get_linear_rip);

bool kvm_is_linear_rip(struct kvm_vcpu *vcpu, unsigned long linear_rip)
{
	return kvm_get_linear_rip(vcpu) == linear_rip;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_is_linear_rip);

unsigned long kvm_get_rflags(struct kvm_vcpu *vcpu)
{
	unsigned long rflags;

	rflags = kvm_x86_call(get_rflags)(vcpu);
	if (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)
		rflags &= ~X86_EFLAGS_TF;
	return rflags;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_get_rflags);

static void __kvm_set_rflags(struct kvm_vcpu *vcpu, unsigned long rflags)
{
	if (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP &&
	    kvm_is_linear_rip(vcpu, vcpu->arch.singlestep_rip))
		rflags |= X86_EFLAGS_TF;
	kvm_x86_call(set_rflags)(vcpu, rflags);
}

void kvm_set_rflags(struct kvm_vcpu *vcpu, unsigned long rflags)
{
	__kvm_set_rflags(vcpu, rflags);
	kvm_make_request(KVM_REQ_EVENT, vcpu);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_set_rflags);

static inline u32 kvm_async_pf_hash_fn(gfn_t gfn)
{
	BUILD_BUG_ON(!is_power_of_2(ASYNC_PF_PER_VCPU));

	return hash_32(gfn & 0xffffffff, order_base_2(ASYNC_PF_PER_VCPU));
}

static inline u32 kvm_async_pf_next_probe(u32 key)
{
	return (key + 1) & (ASYNC_PF_PER_VCPU - 1);
}

static void kvm_add_async_pf_gfn(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	u32 key = kvm_async_pf_hash_fn(gfn);

	while (vcpu->arch.apf.gfns[key] != ~0)
		key = kvm_async_pf_next_probe(key);

	vcpu->arch.apf.gfns[key] = gfn;
}

static u32 kvm_async_pf_gfn_slot(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	int i;
	u32 key = kvm_async_pf_hash_fn(gfn);

	for (i = 0; i < ASYNC_PF_PER_VCPU &&
		     (vcpu->arch.apf.gfns[key] != gfn &&
		      vcpu->arch.apf.gfns[key] != ~0); i++)
		key = kvm_async_pf_next_probe(key);

	return key;
}

bool kvm_find_async_pf_gfn(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	return vcpu->arch.apf.gfns[kvm_async_pf_gfn_slot(vcpu, gfn)] == gfn;
}

static void kvm_del_async_pf_gfn(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	u32 i, j, k;

	i = j = kvm_async_pf_gfn_slot(vcpu, gfn);

	if (WARN_ON_ONCE(vcpu->arch.apf.gfns[i] != gfn))
		return;

	while (true) {
		vcpu->arch.apf.gfns[i] = ~0;
		do {
			j = kvm_async_pf_next_probe(j);
			if (vcpu->arch.apf.gfns[j] == ~0)
				return;
			k = kvm_async_pf_hash_fn(vcpu->arch.apf.gfns[j]);
			/*
			 * k lies cyclically in ]i,j]
			 * |    i.k.j |
			 * |....j i.k.| or  |.k..j i...|
			 */
		} while ((i <= j) ? (i < k && k <= j) : (i < k || k <= j));
		vcpu->arch.apf.gfns[i] = vcpu->arch.apf.gfns[j];
		i = j;
	}
}

static inline int apf_put_user_notpresent(struct kvm_vcpu *vcpu)
{
	u32 reason = KVM_PV_REASON_PAGE_NOT_PRESENT;

	return kvm_write_guest_cached(vcpu->kvm, &vcpu->arch.apf.data, &reason,
				      sizeof(reason));
}

static inline int apf_put_user_ready(struct kvm_vcpu *vcpu, u32 token)
{
	unsigned int offset = offsetof(struct kvm_vcpu_pv_apf_data, token);

	return kvm_write_guest_offset_cached(vcpu->kvm, &vcpu->arch.apf.data,
					     &token, offset, sizeof(token));
}

static inline bool apf_pageready_slot_free(struct kvm_vcpu *vcpu)
{
	unsigned int offset = offsetof(struct kvm_vcpu_pv_apf_data, token);
	u32 val;

	if (kvm_read_guest_offset_cached(vcpu->kvm, &vcpu->arch.apf.data,
					 &val, offset, sizeof(val)))
		return false;

	return !val;
}

static bool kvm_can_deliver_async_pf(struct kvm_vcpu *vcpu)
{

	if (!kvm_pv_async_pf_enabled(vcpu))
		return false;

	if (!vcpu->arch.apf.send_always &&
	    (vcpu->arch.guest_state_protected || !kvm_x86_call(get_cpl)(vcpu)))
		return false;