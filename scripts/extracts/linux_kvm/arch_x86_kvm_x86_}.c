// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 61/86



	/* Ignore requests to yield to self */
	if (vcpu == target)
		goto no_yield;

	if (kvm_vcpu_yield_to(target) <= 0)
		goto no_yield;

	vcpu->stat.directed_yield_successful++;

no_yield:
	return;
}

static int complete_hypercall_exit(struct kvm_vcpu *vcpu)
{
	u64 ret = vcpu->run->hypercall.ret;

	if (!is_64_bit_hypercall(vcpu))
		ret = (u32)ret;
	kvm_rax_write(vcpu, ret);
	return kvm_skip_emulated_instruction(vcpu);
}

int ____kvm_emulate_hypercall(struct kvm_vcpu *vcpu, int cpl,
			      int (*complete_hypercall)(struct kvm_vcpu *))
{
	unsigned long ret;
	unsigned long nr = kvm_rax_read(vcpu);
	unsigned long a0 = kvm_rbx_read(vcpu);
	unsigned long a1 = kvm_rcx_read(vcpu);
	unsigned long a2 = kvm_rdx_read(vcpu);
	unsigned long a3 = kvm_rsi_read(vcpu);
	int op_64_bit = is_64_bit_hypercall(vcpu);

	++vcpu->stat.hypercalls;

	trace_kvm_hypercall(nr, a0, a1, a2, a3);

	if (!op_64_bit) {
		nr &= 0xFFFFFFFF;
		a0 &= 0xFFFFFFFF;
		a1 &= 0xFFFFFFFF;
		a2 &= 0xFFFFFFFF;
		a3 &= 0xFFFFFFFF;
	}

	if (cpl) {
		ret = -KVM_EPERM;
		goto out;
	}

	ret = -KVM_ENOSYS;

	switch (nr) {
	case KVM_HC_VAPIC_POLL_IRQ:
		ret = 0;
		break;
	case KVM_HC_KICK_CPU:
		if (!guest_pv_has(vcpu, KVM_FEATURE_PV_UNHALT))
			break;

		kvm_pv_kick_cpu_op(vcpu->kvm, a1);
		kvm_sched_yield(vcpu, a1);
		ret = 0;
		break;
#ifdef CONFIG_X86_64
	case KVM_HC_CLOCK_PAIRING:
		ret = kvm_pv_clock_pairing(vcpu, a0, a1);
		break;
#endif
	case KVM_HC_SEND_IPI:
		if (!guest_pv_has(vcpu, KVM_FEATURE_PV_SEND_IPI))
			break;

		ret = kvm_pv_send_ipi(vcpu->kvm, a0, a1, a2, a3, op_64_bit);
		break;
	case KVM_HC_SCHED_YIELD:
		if (!guest_pv_has(vcpu, KVM_FEATURE_PV_SCHED_YIELD))
			break;

		kvm_sched_yield(vcpu, a0);
		ret = 0;
		break;
	case KVM_HC_MAP_GPA_RANGE: {
		u64 gpa = a0, npages = a1, attrs = a2;

		ret = -KVM_ENOSYS;
		if (!user_exit_on_hypercall(vcpu->kvm, KVM_HC_MAP_GPA_RANGE))
			break;

		if (!PAGE_ALIGNED(gpa) || !npages ||
		    gpa_to_gfn(gpa) + npages <= gpa_to_gfn(gpa)) {
			ret = -KVM_EINVAL;
			break;
		}

		vcpu->run->exit_reason        = KVM_EXIT_HYPERCALL;
		vcpu->run->hypercall.nr       = KVM_HC_MAP_GPA_RANGE;
		/*
		 * In principle this should have been -KVM_ENOSYS, but userspace (QEMU <=9.2)
		 * assumed that vcpu->run->hypercall.ret is never changed by KVM and thus that
		 * it was always zero on KVM_EXIT_HYPERCALL.  Since KVM is now overwriting
		 * vcpu->run->hypercall.ret, ensuring that it is zero to not break QEMU.
		 */
		vcpu->run->hypercall.ret = 0;
		vcpu->run->hypercall.args[0]  = gpa;
		vcpu->run->hypercall.args[1]  = npages;
		vcpu->run->hypercall.args[2]  = attrs;
		vcpu->run->hypercall.flags    = 0;
		if (op_64_bit)
			vcpu->run->hypercall.flags |= KVM_EXIT_HYPERCALL_LONG_MODE;

		WARN_ON_ONCE(vcpu->run->hypercall.flags & KVM_EXIT_HYPERCALL_MBZ);
		vcpu->arch.complete_userspace_io = complete_hypercall;
		return 0;
	}
	default:
		ret = -KVM_ENOSYS;
		break;
	}

out:
	vcpu->run->hypercall.ret = ret;
	return 1;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(____kvm_emulate_hypercall);

int kvm_emulate_hypercall(struct kvm_vcpu *vcpu)
{
	if (kvm_xen_hypercall_enabled(vcpu->kvm))
		return kvm_xen_hypercall(vcpu);

	if (kvm_hv_hypercall_enabled(vcpu))
		return kvm_hv_hypercall(vcpu);

	return __kvm_emulate_hypercall(vcpu, kvm_x86_call(get_cpl)(vcpu),
				       complete_hypercall_exit);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_hypercall);

static int emulator_fix_hypercall(struct x86_emulate_ctxt *ctxt)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	char instruction[3];
	unsigned long rip = kvm_rip_read(vcpu);

	/*
	 * If the quirk is disabled, synthesize a #UD and let the guest pick up
	 * the pieces.
	 */
	if (!kvm_check_has_quirk(vcpu->kvm, KVM_X86_QUIRK_FIX_HYPERCALL_INSN)) {
		ctxt->exception.error_code_valid = false;
		ctxt->exception.vector = UD_VECTOR;
		ctxt->have_exception = true;
		return X86EMUL_PROPAGATE_FAULT;
	}

	kvm_x86_call(patch_hypercall)(vcpu, instruction);

	return emulator_write_emulated(ctxt, rip, instruction, 3,
		&ctxt->exception);
}

static int dm_request_for_irq_injection(struct kvm_vcpu *vcpu)
{
	return vcpu->run->request_interrupt_window &&
		likely(!pic_in_kernel(vcpu->kvm));
}

/* Called within kvm->srcu read side.  */
static void post_kvm_run_save(struct kvm_vcpu *vcpu)
{
	struct kvm_run *kvm_run = vcpu->run;

	kvm_run->if_flag = kvm_x86_call(get_if_flag)(vcpu);
	kvm_run->cr8 = kvm_get_cr8(vcpu);
	kvm_run->apic_base = vcpu->arch.apic_base;

	kvm_run->ready_for_interrupt_injection =
		pic_in_kernel(vcpu->kvm) ||
		kvm_vcpu_ready_for_interrupt_injection(vcpu);

	if (is_smm(vcpu))
		kvm_run->flags |= KVM_RUN_X86_SMM;
	if (is_guest_mode(vcpu))
		kvm_run->flags |= KVM_RUN_X86_GUEST_MODE;
}

static void update_cr8_intercept(struct kvm_vcpu *vcpu)
{
	int max_irr, tpr;

	if (!kvm_x86_ops.update_cr8_intercept)
		return;

	if (!lapic_in_kernel(vcpu))
		return;

	if (vcpu->arch.apic->apicv_active)
		return;

	if (!vcpu->arch.apic->vapic_addr)
		max_irr = kvm_lapic_find_highest_irr(vcpu);
	else
		max_irr = -1;