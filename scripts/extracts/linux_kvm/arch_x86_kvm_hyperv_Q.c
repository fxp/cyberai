// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 17/17



int kvm_get_hv_cpuid(struct kvm_vcpu *vcpu, struct kvm_cpuid2 *cpuid,
		     struct kvm_cpuid_entry2 __user *entries)
{
	uint16_t evmcs_ver = 0;
	struct kvm_cpuid_entry2 cpuid_entries[] = {
		{ .function = HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS },
		{ .function = HYPERV_CPUID_INTERFACE },
		{ .function = HYPERV_CPUID_VERSION },
		{ .function = HYPERV_CPUID_FEATURES },
		{ .function = HYPERV_CPUID_ENLIGHTMENT_INFO },
		{ .function = HYPERV_CPUID_IMPLEMENT_LIMITS },
		{ .function = HYPERV_CPUID_SYNDBG_VENDOR_AND_MAX_FUNCTIONS },
		{ .function = HYPERV_CPUID_SYNDBG_INTERFACE },
		{ .function = HYPERV_CPUID_SYNDBG_PLATFORM_CAPABILITIES	},
		{ .function = HYPERV_CPUID_NESTED_FEATURES },
	};
	int i, nent = ARRAY_SIZE(cpuid_entries);

	if (kvm_x86_ops.nested_ops->get_evmcs_version)
		evmcs_ver = kvm_x86_ops.nested_ops->get_evmcs_version(vcpu);

	if (cpuid->nent < nent)
		return -E2BIG;

	if (cpuid->nent > nent)
		cpuid->nent = nent;

	for (i = 0; i < nent; i++) {
		struct kvm_cpuid_entry2 *ent = &cpuid_entries[i];
		u32 signature[3];

		switch (ent->function) {
		case HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS:
			memcpy(signature, "Linux KVM Hv", 12);

			ent->eax = HYPERV_CPUID_SYNDBG_PLATFORM_CAPABILITIES;
			ent->ebx = signature[0];
			ent->ecx = signature[1];
			ent->edx = signature[2];
			break;

		case HYPERV_CPUID_INTERFACE:
			ent->eax = HYPERV_CPUID_SIGNATURE_EAX;
			break;

		case HYPERV_CPUID_VERSION:
			/*
			 * We implement some Hyper-V 2016 functions so let's use
			 * this version.
			 */
			ent->eax = 0x00003839;
			ent->ebx = 0x000A0000;
			break;

		case HYPERV_CPUID_FEATURES:
			ent->eax |= HV_MSR_VP_RUNTIME_AVAILABLE;
			ent->eax |= HV_MSR_TIME_REF_COUNT_AVAILABLE;
			ent->eax |= HV_MSR_SYNIC_AVAILABLE;
			ent->eax |= HV_MSR_SYNTIMER_AVAILABLE;
			ent->eax |= HV_MSR_APIC_ACCESS_AVAILABLE;
			ent->eax |= HV_MSR_HYPERCALL_AVAILABLE;
			ent->eax |= HV_MSR_VP_INDEX_AVAILABLE;
			ent->eax |= HV_MSR_RESET_AVAILABLE;
			ent->eax |= HV_MSR_REFERENCE_TSC_AVAILABLE;
			ent->eax |= HV_ACCESS_FREQUENCY_MSRS;
			ent->eax |= HV_ACCESS_REENLIGHTENMENT;
			ent->eax |= HV_ACCESS_TSC_INVARIANT;

			ent->ebx |= HV_POST_MESSAGES;
			ent->ebx |= HV_SIGNAL_EVENTS;
			ent->ebx |= HV_ENABLE_EXTENDED_HYPERCALLS;

			ent->edx |= HV_X64_HYPERCALL_XMM_INPUT_AVAILABLE;
			ent->edx |= HV_FEATURE_FREQUENCY_MSRS_AVAILABLE;
			ent->edx |= HV_FEATURE_GUEST_CRASH_MSR_AVAILABLE;

			ent->ebx |= HV_DEBUGGING;
			ent->edx |= HV_X64_GUEST_DEBUGGING_AVAILABLE;
			ent->edx |= HV_FEATURE_DEBUG_MSRS_AVAILABLE;
			ent->edx |= HV_FEATURE_EXT_GVA_RANGES_FLUSH;

			/*
			 * Direct Synthetic timers only make sense with in-kernel
			 * LAPIC
			 */
			if (!vcpu || lapic_in_kernel(vcpu))
				ent->edx |= HV_STIMER_DIRECT_MODE_AVAILABLE;

			break;

		case HYPERV_CPUID_ENLIGHTMENT_INFO:
			ent->eax |= HV_X64_REMOTE_TLB_FLUSH_RECOMMENDED;
			ent->eax |= HV_X64_APIC_ACCESS_RECOMMENDED;
			ent->eax |= HV_X64_RELAXED_TIMING_RECOMMENDED;
			if (!vcpu || lapic_in_kernel(vcpu))
				ent->eax |= HV_X64_CLUSTER_IPI_RECOMMENDED;
			ent->eax |= HV_X64_EX_PROCESSOR_MASKS_RECOMMENDED;
			if (evmcs_ver)
				ent->eax |= HV_X64_ENLIGHTENED_VMCS_RECOMMENDED;
			if (!cpu_smt_possible())
				ent->eax |= HV_X64_NO_NONARCH_CORESHARING;

			ent->eax |= HV_DEPRECATING_AEOI_RECOMMENDED;
			/*
			 * Default number of spinlock retry attempts, matches
			 * HyperV 2016.
			 */
			ent->ebx = 0x00000FFF;

			break;

		case HYPERV_CPUID_IMPLEMENT_LIMITS:
			/* Maximum number of virtual processors */
			ent->eax = KVM_MAX_VCPUS;
			/*
			 * Maximum number of logical processors, matches
			 * HyperV 2016.
			 */
			ent->ebx = 64;

			break;

		case HYPERV_CPUID_NESTED_FEATURES:
			ent->eax = evmcs_ver;
			ent->eax |= HV_X64_NESTED_DIRECT_FLUSH;
			ent->eax |= HV_X64_NESTED_MSR_BITMAP;
			ent->ebx |= HV_X64_NESTED_EVMCS1_PERF_GLOBAL_CTRL;
			break;

		case HYPERV_CPUID_SYNDBG_VENDOR_AND_MAX_FUNCTIONS:
			memcpy(signature, "Linux KVM Hv", 12);

			ent->eax = 0;
			ent->ebx = signature[0];
			ent->ecx = signature[1];
			ent->edx = signature[2];
			break;

		case HYPERV_CPUID_SYNDBG_INTERFACE:
			memcpy(signature, "VS#1\0\0\0\0\0\0\0\0", 12);
			ent->eax = signature[0];
			break;

		case HYPERV_CPUID_SYNDBG_PLATFORM_CAPABILITIES:
			ent->eax |= HV_X64_SYNDBG_CAP_ALLOW_KERNEL_DEBUGGING;
			break;

		default:
			break;
		}
	}

	if (copy_to_user(entries, cpuid_entries,
			 nent * sizeof(struct kvm_cpuid_entry2)))
		return -EFAULT;

	return 0;
}
