// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 36/86



static int kvm_vcpu_ioctl_x86_set_debugregs(struct kvm_vcpu *vcpu,
					    struct kvm_debugregs *dbgregs)
{
	unsigned int i;

	if (vcpu->kvm->arch.has_protected_state &&
	    vcpu->arch.guest_state_protected)
		return -EINVAL;

	if (dbgregs->flags)
		return -EINVAL;

	if (!kvm_dr6_valid(dbgregs->dr6))
		return -EINVAL;
	if (!kvm_dr7_valid(dbgregs->dr7))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(vcpu->arch.db); i++)
		vcpu->arch.db[i] = dbgregs->db[i];

	kvm_update_dr0123(vcpu);
	vcpu->arch.dr6 = dbgregs->dr6;
	vcpu->arch.dr7 = dbgregs->dr7;
	kvm_update_dr7(vcpu);

	return 0;
}


static int kvm_vcpu_ioctl_x86_get_xsave2(struct kvm_vcpu *vcpu,
					 u8 *state, unsigned int size)
{
	/*
	 * Only copy state for features that are enabled for the guest.  The
	 * state itself isn't problematic, but setting bits in the header for
	 * features that are supported in *this* host but not exposed to the
	 * guest can result in KVM_SET_XSAVE failing when live migrating to a
	 * compatible host without the features that are NOT exposed to the
	 * guest.
	 *
	 * FP+SSE can always be saved/restored via KVM_{G,S}ET_XSAVE, even if
	 * XSAVE/XCRO are not exposed to the guest, and even if XSAVE isn't
	 * supported by the host.
	 */
	u64 supported_xcr0 = vcpu->arch.guest_supported_xcr0 |
			     XFEATURE_MASK_FPSSE;

	if (fpstate_is_confidential(&vcpu->arch.guest_fpu))
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;

	fpu_copy_guest_fpstate_to_uabi(&vcpu->arch.guest_fpu, state, size,
				       supported_xcr0, vcpu->arch.pkru);
	return 0;
}

static int kvm_vcpu_ioctl_x86_get_xsave(struct kvm_vcpu *vcpu,
					struct kvm_xsave *guest_xsave)
{
	return kvm_vcpu_ioctl_x86_get_xsave2(vcpu, (void *)guest_xsave->region,
					     sizeof(guest_xsave->region));
}

static int kvm_vcpu_ioctl_x86_set_xsave(struct kvm_vcpu *vcpu,
					struct kvm_xsave *guest_xsave)
{
	union fpregs_state *xstate = (union fpregs_state *)guest_xsave->region;

	if (fpstate_is_confidential(&vcpu->arch.guest_fpu))
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;

	/*
	 * For backwards compatibility, do not expect disabled features to be in
	 * their initial state.  XSTATE_BV[i] must still be cleared whenever
	 * XFD[i]=1, or XRSTOR would cause a #NM.
	 */
	xstate->xsave.header.xfeatures &= ~vcpu->arch.guest_fpu.fpstate->xfd;

	return fpu_copy_uabi_to_guest_fpstate(&vcpu->arch.guest_fpu,
					      guest_xsave->region,
					      kvm_caps.supported_xcr0,
					      &vcpu->arch.pkru);
}

static int kvm_vcpu_ioctl_x86_get_xcrs(struct kvm_vcpu *vcpu,
				       struct kvm_xcrs *guest_xcrs)
{
	if (vcpu->kvm->arch.has_protected_state &&
	    vcpu->arch.guest_state_protected)
		return -EINVAL;

	if (!boot_cpu_has(X86_FEATURE_XSAVE)) {
		guest_xcrs->nr_xcrs = 0;
		return 0;
	}

	guest_xcrs->nr_xcrs = 1;
	guest_xcrs->flags = 0;
	guest_xcrs->xcrs[0].xcr = XCR_XFEATURE_ENABLED_MASK;
	guest_xcrs->xcrs[0].value = vcpu->arch.xcr0;
	return 0;
}

static int kvm_vcpu_ioctl_x86_set_xcrs(struct kvm_vcpu *vcpu,
				       struct kvm_xcrs *guest_xcrs)
{
	int i, r = 0;

	if (vcpu->kvm->arch.has_protected_state &&
	    vcpu->arch.guest_state_protected)
		return -EINVAL;

	if (!boot_cpu_has(X86_FEATURE_XSAVE))
		return -EINVAL;

	if (guest_xcrs->nr_xcrs > KVM_MAX_XCRS || guest_xcrs->flags)
		return -EINVAL;

	for (i = 0; i < guest_xcrs->nr_xcrs; i++)
		/* Only support XCR0 currently */
		if (guest_xcrs->xcrs[i].xcr == XCR_XFEATURE_ENABLED_MASK) {
			r = __kvm_set_xcr(vcpu, XCR_XFEATURE_ENABLED_MASK,
				guest_xcrs->xcrs[i].value);
			break;
		}
	if (r)
		r = -EINVAL;
	return r;
}

/*
 * kvm_set_guest_paused() indicates to the guest kernel that it has been
 * stopped by the hypervisor.  This function will be called from the host only.
 * EINVAL is returned when the host attempts to set the flag for a guest that
 * does not support pv clocks.
 */
static int kvm_set_guest_paused(struct kvm_vcpu *vcpu)
{
	if (!vcpu->arch.pv_time.active)
		return -EINVAL;
	vcpu->arch.pvclock_set_guest_stopped_request = true;
	kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
	return 0;
}

static int kvm_arch_tsc_has_attr(struct kvm_vcpu *vcpu,
				 struct kvm_device_attr *attr)
{
	int r;

	switch (attr->attr) {
	case KVM_VCPU_TSC_OFFSET:
		r = 0;
		break;
	default:
		r = -ENXIO;
	}

	return r;
}

static int kvm_arch_tsc_get_attr(struct kvm_vcpu *vcpu,
				 struct kvm_device_attr *attr)
{
	u64 __user *uaddr = u64_to_user_ptr(attr->addr);
	int r;

	switch (attr->attr) {
	case KVM_VCPU_TSC_OFFSET:
		r = -EFAULT;
		if (put_user(vcpu->arch.l1_tsc_offset, uaddr))
			break;
		r = 0;
		break;
	default:
		r = -ENXIO;
	}

	return r;
}

static int kvm_arch_tsc_set_attr(struct kvm_vcpu *vcpu,
				 struct kvm_device_attr *attr)
{
	u64 __user *uaddr = u64_to_user_ptr(attr->addr);
	struct kvm *kvm = vcpu->kvm;
	int r;

	switch (attr->attr) {
	case KVM_VCPU_TSC_OFFSET: {
		u64 offset, tsc, ns;
		unsigned long flags;
		bool matched;

		r = -EFAULT;
		if (get_user(offset, uaddr))
			break;