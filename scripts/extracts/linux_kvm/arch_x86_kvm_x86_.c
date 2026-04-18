// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 75/86



int kvm_arch_vcpu_ioctl_get_fpu(struct kvm_vcpu *vcpu, struct kvm_fpu *fpu)
{
	struct fxregs_state *fxsave;

	if (fpstate_is_confidential(&vcpu->arch.guest_fpu))
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;

	vcpu_load(vcpu);

	fxsave = &vcpu->arch.guest_fpu.fpstate->regs.fxsave;
	memcpy(fpu->fpr, fxsave->st_space, 128);
	fpu->fcw = fxsave->cwd;
	fpu->fsw = fxsave->swd;
	fpu->ftwx = fxsave->twd;
	fpu->last_opcode = fxsave->fop;
	fpu->last_ip = fxsave->rip;
	fpu->last_dp = fxsave->rdp;
	memcpy(fpu->xmm, fxsave->xmm_space, sizeof(fxsave->xmm_space));

	vcpu_put(vcpu);
	return 0;
}

int kvm_arch_vcpu_ioctl_set_fpu(struct kvm_vcpu *vcpu, struct kvm_fpu *fpu)
{
	struct fxregs_state *fxsave;

	if (fpstate_is_confidential(&vcpu->arch.guest_fpu))
		return vcpu->kvm->arch.has_protected_state ? -EINVAL : 0;

	vcpu_load(vcpu);

	fxsave = &vcpu->arch.guest_fpu.fpstate->regs.fxsave;

	memcpy(fxsave->st_space, fpu->fpr, 128);
	fxsave->cwd = fpu->fcw;
	fxsave->swd = fpu->fsw;
	fxsave->twd = fpu->ftwx;
	fxsave->fop = fpu->last_opcode;
	fxsave->rip = fpu->last_ip;
	fxsave->rdp = fpu->last_dp;
	memcpy(fxsave->xmm_space, fpu->xmm, sizeof(fxsave->xmm_space));

	vcpu_put(vcpu);
	return 0;
}

static void store_regs(struct kvm_vcpu *vcpu)
{
	BUILD_BUG_ON(sizeof(struct kvm_sync_regs) > SYNC_REGS_SIZE_BYTES);

	if (vcpu->run->kvm_valid_regs & KVM_SYNC_X86_REGS)
		__get_regs(vcpu, &vcpu->run->s.regs.regs);

	if (vcpu->run->kvm_valid_regs & KVM_SYNC_X86_SREGS)
		__get_sregs(vcpu, &vcpu->run->s.regs.sregs);

	if (vcpu->run->kvm_valid_regs & KVM_SYNC_X86_EVENTS)
		kvm_vcpu_ioctl_x86_get_vcpu_events(
				vcpu, &vcpu->run->s.regs.events);
}

static int sync_regs(struct kvm_vcpu *vcpu)
{
	if (vcpu->run->kvm_dirty_regs & KVM_SYNC_X86_REGS) {
		__set_regs(vcpu, &vcpu->run->s.regs.regs);
		vcpu->run->kvm_dirty_regs &= ~KVM_SYNC_X86_REGS;
	}

	if (vcpu->run->kvm_dirty_regs & KVM_SYNC_X86_SREGS) {
		struct kvm_sregs sregs = vcpu->run->s.regs.sregs;

		if (__set_sregs(vcpu, &sregs))
			return -EINVAL;

		vcpu->run->kvm_dirty_regs &= ~KVM_SYNC_X86_SREGS;
	}

	if (vcpu->run->kvm_dirty_regs & KVM_SYNC_X86_EVENTS) {
		struct kvm_vcpu_events events = vcpu->run->s.regs.events;

		if (kvm_vcpu_ioctl_x86_set_vcpu_events(vcpu, &events))
			return -EINVAL;

		vcpu->run->kvm_dirty_regs &= ~KVM_SYNC_X86_EVENTS;
	}

	return 0;
}

#define PERF_MEDIATED_PMU_MSG \
	"Failed to enable mediated vPMU, try disabling system wide perf events and nmi_watchdog.\n"

int kvm_arch_vcpu_precreate(struct kvm *kvm, unsigned int id)
{
	int r;

	if (kvm_check_tsc_unstable() && kvm->created_vcpus)
		pr_warn_once("SMP vm created on host with unstable TSC; "
			     "guest TSC will not be reliable\n");

	if (!kvm->arch.max_vcpu_ids)
		kvm->arch.max_vcpu_ids = KVM_MAX_VCPU_IDS;

	if (id >= kvm->arch.max_vcpu_ids)
		return -EINVAL;

	/*
	 * Note, any actions done by .vcpu_create() must be idempotent with
	 * respect to creating multiple vCPUs, and therefore are not undone if
	 * creating a vCPU fails (including failure during pre-create).
	 */
	r = kvm_x86_call(vcpu_precreate)(kvm);
	if (r)
		return r;

	if (enable_mediated_pmu && kvm->arch.enable_pmu &&
	    !kvm->arch.created_mediated_pmu) {
		if (irqchip_in_kernel(kvm)) {
			r = perf_create_mediated_pmu();
			if (r) {
				pr_warn_ratelimited(PERF_MEDIATED_PMU_MSG);
				return r;
			}
			kvm->arch.created_mediated_pmu = true;
		} else {
			kvm->arch.enable_pmu = false;
		}
	}
	return 0;
}

int kvm_arch_vcpu_create(struct kvm_vcpu *vcpu)
{
	struct page *page;
	int r;

	vcpu->arch.last_vmentry_cpu = -1;
	vcpu->arch.regs_avail = ~0;
	vcpu->arch.regs_dirty = ~0;

	kvm_gpc_init(&vcpu->arch.pv_time, vcpu->kvm);

	if (!irqchip_in_kernel(vcpu->kvm) || kvm_vcpu_is_reset_bsp(vcpu))
		kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);
	else
		kvm_set_mp_state(vcpu, KVM_MP_STATE_UNINITIALIZED);

	r = kvm_mmu_create(vcpu);
	if (r < 0)
		return r;

	r = kvm_create_lapic(vcpu);
	if (r < 0)
		goto fail_mmu_destroy;

	r = -ENOMEM;

	page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!page)
		goto fail_free_lapic;
	vcpu->arch.pio_data = page_address(page);

	vcpu->arch.mce_banks = kcalloc(KVM_MAX_MCE_BANKS * 4, sizeof(u64),
				       GFP_KERNEL_ACCOUNT);
	vcpu->arch.mci_ctl2_banks = kcalloc(KVM_MAX_MCE_BANKS, sizeof(u64),
					    GFP_KERNEL_ACCOUNT);
	if (!vcpu->arch.mce_banks || !vcpu->arch.mci_ctl2_banks)
		goto fail_free_mce_banks;
	vcpu->arch.mcg_cap = KVM_MAX_MCE_BANKS;

	if (!zalloc_cpumask_var(&vcpu->arch.wbinvd_dirty_mask,
				GFP_KERNEL_ACCOUNT))
		goto fail_free_mce_banks;

	if (!alloc_emulate_ctxt(vcpu))
		goto free_wbinvd_dirty_mask;

	if (!fpu_alloc_guest_fpstate(&vcpu->arch.guest_fpu)) {
		pr_err("failed to allocate vcpu's fpu\n");
		goto free_emulate_ctxt;
	}

	kvm_async_pf_hash_reset(vcpu);