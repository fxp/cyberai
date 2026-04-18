// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 71/86



		/* FIXME: return into emulator if single-stepping.  */
		if (vcpu->mmio_is_write)
			return 1;
		vcpu->mmio_read_completed = 1;
		return complete_emulated_io(vcpu);
	}

	kvm_prepare_emulated_mmio_exit(vcpu, frag);
	vcpu->arch.complete_userspace_io = complete_emulated_mmio;
	return 0;
}

/* Swap (qemu) user FPU context for the guest FPU context. */
static void kvm_load_guest_fpu(struct kvm_vcpu *vcpu)
{
	if (KVM_BUG_ON(vcpu->arch.guest_fpu.fpstate->in_use, vcpu->kvm))
		return;

	/* Exclude PKRU, it's restored separately immediately after VM-Exit. */
	fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, true);
	trace_kvm_fpu(1);
}

/* When vcpu_run ends, restore user space FPU context. */
static void kvm_put_guest_fpu(struct kvm_vcpu *vcpu)
{
	if (KVM_BUG_ON(!vcpu->arch.guest_fpu.fpstate->in_use, vcpu->kvm))
		return;

	fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, false);
	++vcpu->stat.fpu_reload;
	trace_kvm_fpu(0);
}

static int kvm_x86_vcpu_pre_run(struct kvm_vcpu *vcpu)
{
	/*
	 * Userspace may have modified vCPU state, mark nested_run_pending as
	 * "untrusted" to avoid triggering false-positive WARNs.
	 */
	if (vcpu->arch.nested_run_pending == KVM_NESTED_RUN_PENDING)
		vcpu->arch.nested_run_pending = KVM_NESTED_RUN_PENDING_UNTRUSTED;

	/*
	 * SIPI_RECEIVED is obsolete; KVM leaves the vCPU in Wait-For-SIPI and
	 * tracks the pending SIPI separately.  SIPI_RECEIVED is still accepted
	 * by KVM_SET_VCPU_EVENTS for backwards compatibility, but should be
	 * converted to INIT_RECEIVED.
	 */
	if (WARN_ON_ONCE(vcpu->arch.mp_state == KVM_MP_STATE_SIPI_RECEIVED))
		return -EINVAL;

	/*
	 * Disallow running the vCPU if userspace forced it into an impossible
	 * MP_STATE, e.g. if the vCPU is in WFS but SIPI is blocked.
	 */
	if (vcpu->arch.mp_state == KVM_MP_STATE_INIT_RECEIVED &&
	    !kvm_apic_init_sipi_allowed(vcpu))
		return -EINVAL;

	return kvm_x86_call(vcpu_pre_run)(vcpu);
}

int kvm_arch_vcpu_ioctl_run(struct kvm_vcpu *vcpu)
{
	struct kvm_queued_exception *ex = &vcpu->arch.exception;
	struct kvm_run *kvm_run = vcpu->run;
	u64 sync_valid_fields;
	int r;

	r = kvm_mmu_post_init_vm(vcpu->kvm);
	if (r)
		return r;

	vcpu_load(vcpu);
	kvm_sigset_activate(vcpu);
	kvm_run->flags = 0;
	kvm_load_guest_fpu(vcpu);

	kvm_vcpu_srcu_read_lock(vcpu);
	if (unlikely(vcpu->arch.mp_state == KVM_MP_STATE_UNINITIALIZED)) {
		if (!vcpu->wants_to_run) {
			r = -EINTR;
			goto out;
		}

		/*
		 * Don't bother switching APIC timer emulation from the
		 * hypervisor timer to the software timer, the only way for the
		 * APIC timer to be active is if userspace stuffed vCPU state,
		 * i.e. put the vCPU into a nonsensical state.  Only an INIT
		 * will transition the vCPU out of UNINITIALIZED (without more
		 * state stuffing from userspace), which will reset the local
		 * APIC and thus cancel the timer or drop the IRQ (if the timer
		 * already expired).
		 */
		kvm_vcpu_srcu_read_unlock(vcpu);
		kvm_vcpu_block(vcpu);
		kvm_vcpu_srcu_read_lock(vcpu);

		if (kvm_apic_accept_events(vcpu) < 0) {
			r = 0;
			goto out;
		}
		r = -EAGAIN;
		if (signal_pending(current)) {
			r = -EINTR;
			kvm_run->exit_reason = KVM_EXIT_INTR;
			++vcpu->stat.signal_exits;
		}
		goto out;
	}

	sync_valid_fields = kvm_sync_valid_fields(vcpu->kvm);
	if ((kvm_run->kvm_valid_regs & ~sync_valid_fields) ||
	    (kvm_run->kvm_dirty_regs & ~sync_valid_fields)) {
		r = -EINVAL;
		goto out;
	}

	if (kvm_run->kvm_dirty_regs) {
		r = sync_regs(vcpu);
		if (r != 0)
			goto out;
	}

	/* re-sync apic's tpr */
	if (!lapic_in_kernel(vcpu)) {
		if (kvm_set_cr8(vcpu, kvm_run->cr8) != 0) {
			r = -EINVAL;
			goto out;
		}
	}

	/*
	 * If userspace set a pending exception and L2 is active, convert it to
	 * a pending VM-Exit if L1 wants to intercept the exception.
	 */
	if (vcpu->arch.exception_from_userspace && is_guest_mode(vcpu) &&
	    kvm_x86_ops.nested_ops->is_exception_vmexit(vcpu, ex->vector,
							ex->error_code)) {
		kvm_queue_exception_vmexit(vcpu, ex->vector,
					   ex->has_error_code, ex->error_code,
					   ex->has_payload, ex->payload);
		ex->injected = false;
		ex->pending = false;
	}
	vcpu->arch.exception_from_userspace = false;

	if (unlikely(vcpu->arch.complete_userspace_io)) {
		int (*cui)(struct kvm_vcpu *) = vcpu->arch.complete_userspace_io;
		vcpu->arch.complete_userspace_io = NULL;
		r = cui(vcpu);
		if (r <= 0)
			goto out;
	} else {
		WARN_ON_ONCE(vcpu->arch.pio.count);
		WARN_ON_ONCE(vcpu->mmio_needed);
	}

	if (!vcpu->wants_to_run) {
		r = -EINTR;
		goto out;
	}

	r = kvm_x86_vcpu_pre_run(vcpu);
	if (r <= 0)
		goto out;

	r = vcpu_run(vcpu);

out:
	kvm_put_guest_fpu(vcpu);
	if (kvm_run->kvm_valid_regs && likely(!vcpu->arch.guest_state_protected))
		store_regs(vcpu);
	post_kvm_run_save(vcpu);
	kvm_vcpu_srcu_read_unlock(vcpu);

	kvm_sigset_deactivate(vcpu);
	vcpu_put(vcpu);
	return r;
}