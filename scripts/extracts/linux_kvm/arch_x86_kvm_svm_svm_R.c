// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 18/36



		svm->vmcb01.ptr->save.g_pat = data;
		if (is_guest_mode(vcpu))
			nested_vmcb02_compute_g_pat(svm);
		vmcb_mark_dirty(svm->vmcb, VMCB_NPT);
		break;
	case MSR_IA32_SPEC_CTRL:
		if (!msr->host_initiated &&
		    !guest_has_spec_ctrl_msr(vcpu))
			return 1;

		if (kvm_spec_ctrl_test_value(data))
			return 1;

		if (boot_cpu_has(X86_FEATURE_V_SPEC_CTRL))
			svm->vmcb->save.spec_ctrl = data;
		else
			svm->spec_ctrl = data;
		if (!data)
			break;

		/*
		 * For non-nested:
		 * When it's written (to non-zero) for the first time, pass
		 * it through.
		 *
		 * For nested:
		 * The handling of the MSR bitmap for L2 guests is done in
		 * nested_svm_merge_msrpm().
		 * We update the L1 MSR bit as well since it will end up
		 * touching the MSR anyway now.
		 */
		svm_disable_intercept_for_msr(vcpu, MSR_IA32_SPEC_CTRL, MSR_TYPE_RW);
		break;
	case MSR_AMD64_VIRT_SPEC_CTRL:
		if (!msr->host_initiated &&
		    !guest_cpu_cap_has(vcpu, X86_FEATURE_VIRT_SSBD))
			return 1;

		if (data & ~SPEC_CTRL_SSBD)
			return 1;

		svm->virt_spec_ctrl = data;
		break;
	case MSR_STAR:
		svm->vmcb01.ptr->save.star = data;
		break;
#ifdef CONFIG_X86_64
	case MSR_LSTAR:
		svm->vmcb01.ptr->save.lstar = data;
		break;
	case MSR_CSTAR:
		svm->vmcb01.ptr->save.cstar = data;
		break;
	case MSR_GS_BASE:
		svm->vmcb01.ptr->save.gs.base = data;
		break;
	case MSR_FS_BASE:
		svm->vmcb01.ptr->save.fs.base = data;
		break;
	case MSR_KERNEL_GS_BASE:
		svm->vmcb01.ptr->save.kernel_gs_base = data;
		break;
	case MSR_SYSCALL_MASK:
		svm->vmcb01.ptr->save.sfmask = data;
		break;
#endif
	case MSR_IA32_SYSENTER_CS:
		svm->vmcb01.ptr->save.sysenter_cs = data;
		break;
	case MSR_IA32_SYSENTER_EIP:
		svm->vmcb01.ptr->save.sysenter_eip = (u32)data;
		/*
		 * We only intercept the MSR_IA32_SYSENTER_{EIP|ESP} msrs
		 * when we spoof an Intel vendor ID (for cross vendor migration).
		 * In this case we use this intercept to track the high
		 * 32 bit part of these msrs to support Intel's
		 * implementation of SYSENTER/SYSEXIT.
		 */
		svm->sysenter_eip_hi = guest_cpuid_is_intel_compatible(vcpu) ? (data >> 32) : 0;
		break;
	case MSR_IA32_SYSENTER_ESP:
		svm->vmcb01.ptr->save.sysenter_esp = (u32)data;
		svm->sysenter_esp_hi = guest_cpuid_is_intel_compatible(vcpu) ? (data >> 32) : 0;
		break;
	case MSR_IA32_S_CET:
		svm->vmcb->save.s_cet = data;
		vmcb_mark_dirty(svm->vmcb01.ptr, VMCB_CET);
		break;
	case MSR_IA32_INT_SSP_TAB:
		svm->vmcb->save.isst_addr = data;
		vmcb_mark_dirty(svm->vmcb01.ptr, VMCB_CET);
		break;
	case MSR_KVM_INTERNAL_GUEST_SSP:
		svm->vmcb->save.ssp = data;
		vmcb_mark_dirty(svm->vmcb01.ptr, VMCB_CET);
		break;
	case MSR_TSC_AUX:
		/*
		 * TSC_AUX is always virtualized for SEV-ES guests when the
		 * feature is available. The user return MSR support is not
		 * required in this case because TSC_AUX is restored on #VMEXIT
		 * from the host save area.
		 */
		if (boot_cpu_has(X86_FEATURE_V_TSC_AUX) && is_sev_es_guest(vcpu))
			break;

		/*
		 * TSC_AUX is usually changed only during boot and never read
		 * directly.  Intercept TSC_AUX and switch it via user return.
		 */
		preempt_disable();
		ret = kvm_set_user_return_msr(tsc_aux_uret_slot, data, -1ull);
		preempt_enable();
		if (ret)
			break;

		svm->tsc_aux = data;
		break;
	case MSR_IA32_DEBUGCTLMSR:
		if (!lbrv) {
			kvm_pr_unimpl_wrmsr(vcpu, ecx, data);
			break;
		}

		/*
		 * Suppress BTF as KVM doesn't virtualize BTF, but there's no
		 * way to communicate lack of support to the guest.
		 */
		if (data & DEBUGCTLMSR_BTF) {
			kvm_pr_unimpl_wrmsr(vcpu, MSR_IA32_DEBUGCTLMSR, data);
			data &= ~DEBUGCTLMSR_BTF;
		}

		if (data & DEBUGCTL_RESERVED_BITS)
			return 1;

		if (svm->vmcb->save.dbgctl == data)
			break;

		svm->vmcb->save.dbgctl = data;
		vmcb_mark_dirty(svm->vmcb, VMCB_LBR);
		svm_update_lbrv(vcpu);
		break;
	case MSR_IA32_LASTBRANCHFROMIP:
	case MSR_IA32_LASTBRANCHTOIP:
	case MSR_IA32_LASTINTFROMIP:
	case MSR_IA32_LASTINTTOIP:
		if (!lbrv)
			return KVM_MSR_RET_UNSUPPORTED;
		if (!msr->host_initiated)
			return 1;
		*svm_vmcb_lbr(svm, ecx) = data;
		vmcb_mark_dirty(svm->vmcb, VMCB_LBR);
		break;
	case MSR_VM_HSAVE_PA:
		/*
		 * Old kernels did not validate the value written to
		 * MSR_VM_HSAVE_PA.  Allow KVM_SET_MSR to set an invalid
		 * value to allow live migrating buggy or malicious guests
		 * originating from those kernels.
		 */
		if (!msr->host_initiated && !page_address_valid(vcpu, data))
			return 1;

		svm->nested.hsave_msr = data & PAGE_MASK;
		break;
	case MSR_VM_CR:
		return svm_set_vm_cr(vcpu, data);
	case MSR_VM_IGNNE:
		kvm_pr_unimpl_wrmsr(vcpu, ecx, data);
		break;
	case MSR_AMD64_DE_CFG: {
		u64 supported_de_cfg;

		if (svm_get_feature_msr(ecx, &supported_de_cfg))
			return 1;

		if (data & ~supported_de_cfg)
			return 1;

		svm->msr_decfg = data;
		break;
	}
	default:
		return kvm_set_msr_common(vcpu, msr);
	}
	return ret;
}