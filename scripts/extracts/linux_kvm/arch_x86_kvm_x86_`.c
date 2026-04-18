// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 32/86



		r = -EFAULT;
		if (copy_from_user(&msr_list, user_msr_list, sizeof(msr_list)))
			goto out;
		n = msr_list.nmsrs;
		msr_list.nmsrs = num_msr_based_features;
		if (copy_to_user(user_msr_list, &msr_list, sizeof(msr_list)))
			goto out;
		r = -E2BIG;
		if (n < msr_list.nmsrs)
			goto out;
		r = -EFAULT;
		if (copy_to_user(user_msr_list->indices, &msr_based_features,
				 num_msr_based_features * sizeof(u32)))
			goto out;
		r = 0;
		break;
	}
	case KVM_GET_MSRS:
		r = msr_io(NULL, argp, do_get_feature_msr, 1);
		break;
#ifdef CONFIG_KVM_HYPERV
	case KVM_GET_SUPPORTED_HV_CPUID:
		r = kvm_ioctl_get_supported_hv_cpuid(NULL, argp);
		break;
#endif
	case KVM_GET_DEVICE_ATTR: {
		struct kvm_device_attr attr;
		r = -EFAULT;
		if (copy_from_user(&attr, (void __user *)arg, sizeof(attr)))
			break;
		r = kvm_x86_dev_get_attr(&attr);
		break;
	}
	case KVM_HAS_DEVICE_ATTR: {
		struct kvm_device_attr attr;
		r = -EFAULT;
		if (copy_from_user(&attr, (void __user *)arg, sizeof(attr)))
			break;
		r = kvm_x86_dev_has_attr(&attr);
		break;
	}
	default:
		r = -EINVAL;
		break;
	}
out:
	return r;
}

static bool need_emulate_wbinvd(struct kvm_vcpu *vcpu)
{
	return kvm_arch_has_noncoherent_dma(vcpu->kvm);
}

static DEFINE_PER_CPU(struct kvm_vcpu *, last_vcpu);

void kvm_arch_vcpu_load(struct kvm_vcpu *vcpu, int cpu)
{
	struct kvm_pmu *pmu = vcpu_to_pmu(vcpu);

	kvm_request_l1tf_flush_l1d();

	if (vcpu->scheduled_out && pmu->version && pmu->event_count) {
		pmu->need_cleanup = true;
		kvm_make_request(KVM_REQ_PMU, vcpu);
	}

	/* Address WBINVD may be executed by guest */
	if (need_emulate_wbinvd(vcpu)) {
		if (kvm_x86_call(has_wbinvd_exit)())
			cpumask_set_cpu(cpu, vcpu->arch.wbinvd_dirty_mask);
		else if (vcpu->cpu != -1 && vcpu->cpu != cpu)
			wbinvd_on_cpu(vcpu->cpu);
	}

	kvm_x86_call(vcpu_load)(vcpu, cpu);

	if (vcpu != per_cpu(last_vcpu, cpu)) {
		/*
		 * Flush the branch predictor when switching vCPUs on the same
		 * physical CPU, as each vCPU needs its own branch prediction
		 * domain.  No IBPB is needed when switching between L1 and L2
		 * on the same vCPU unless IBRS is advertised to the vCPU; that
		 * is handled on the nested VM-Exit path.
		 */
		if (static_branch_likely(&switch_vcpu_ibpb))
			indirect_branch_prediction_barrier();
		per_cpu(last_vcpu, cpu) = vcpu;
	}

	/* Save host pkru register if supported */
	vcpu->arch.host_pkru = read_pkru();

	/* Apply any externally detected TSC adjustments (due to suspend) */
	if (unlikely(vcpu->arch.tsc_offset_adjustment)) {
		adjust_tsc_offset_host(vcpu, vcpu->arch.tsc_offset_adjustment);
		vcpu->arch.tsc_offset_adjustment = 0;
		kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
	}

	if (unlikely(vcpu->cpu != cpu) || kvm_check_tsc_unstable()) {
		s64 tsc_delta = !vcpu->arch.last_host_tsc ? 0 :
				rdtsc() - vcpu->arch.last_host_tsc;
		if (tsc_delta < 0)
			mark_tsc_unstable("KVM discovered backwards TSC");

		if (kvm_check_tsc_unstable()) {
			u64 offset = kvm_compute_l1_tsc_offset(vcpu,
						vcpu->arch.last_guest_tsc);
			kvm_vcpu_write_tsc_offset(vcpu, offset);
			if (!vcpu->arch.guest_tsc_protected)
				vcpu->arch.tsc_catchup = 1;
		}

		if (kvm_lapic_hv_timer_in_use(vcpu))
			kvm_lapic_restart_hv_timer(vcpu);

		/*
		 * On a host with synchronized TSC, there is no need to update
		 * kvmclock on vcpu->cpu migration
		 */
		if (!vcpu->kvm->arch.use_master_clock || vcpu->cpu == -1)
			kvm_make_request(KVM_REQ_GLOBAL_CLOCK_UPDATE, vcpu);
		if (vcpu->cpu != cpu)
			kvm_make_request(KVM_REQ_MIGRATE_TIMER, vcpu);
		vcpu->cpu = cpu;
	}

	kvm_make_request(KVM_REQ_STEAL_UPDATE, vcpu);
}

static void kvm_steal_time_set_preempted(struct kvm_vcpu *vcpu)
{
	struct gfn_to_hva_cache *ghc = &vcpu->arch.st.cache;
	struct kvm_steal_time __user *st;
	struct kvm_memslots *slots;
	static const u8 preempted = KVM_VCPU_PREEMPTED;
	gpa_t gpa = vcpu->arch.st.msr_val & KVM_STEAL_VALID_BITS;

	/*
	 * The vCPU can be marked preempted if and only if the VM-Exit was on
	 * an instruction boundary and will not trigger guest emulation of any
	 * kind (see vcpu_run).  Vendor specific code controls (conservatively)
	 * when this is true, for example allowing the vCPU to be marked
	 * preempted if and only if the VM-Exit was due to a host interrupt.
	 */
	if (!vcpu->arch.at_instruction_boundary) {
		vcpu->stat.preemption_other++;
		return;
	}

	vcpu->stat.preemption_reported++;
	if (!(vcpu->arch.st.msr_val & KVM_MSR_ENABLED))
		return;

	if (vcpu->arch.st.preempted)
		return;

	/* This happens on process exit */
	if (unlikely(current->mm != vcpu->kvm->mm))
		return;

	slots = kvm_memslots(vcpu->kvm);

	if (unlikely(slots->generation != ghc->generation ||
		     gpa != ghc->gpa ||
		     kvm_is_error_hva(ghc->hva) || !ghc->memslot))
		return;

	st = (struct kvm_steal_time __user *)ghc->hva;
	BUILD_BUG_ON(sizeof(st->preempted) != sizeof(preempted));

	if (!copy_to_user_nofault(&st->preempted, &preempted, sizeof(preempted)))
		vcpu->arch.st.preempted = KVM_VCPU_PREEMPTED;