// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 16/17



	switch (hc.code) {
	case HVCALL_NOTIFY_LONG_SPIN_WAIT:
		if (unlikely(hc.rep || hc.var_cnt)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		kvm_vcpu_on_spin(vcpu, true);
		break;
	case HVCALL_SIGNAL_EVENT:
		if (unlikely(hc.rep || hc.var_cnt)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		ret = kvm_hvcall_signal_event(vcpu, &hc);
		if (ret != HV_STATUS_INVALID_PORT_ID)
			break;
		fallthrough;	/* maybe userspace knows this conn_id */
	case HVCALL_POST_MESSAGE:
		/* don't bother userspace if it has no way to handle it */
		if (unlikely(hc.rep || hc.var_cnt || !to_hv_synic(vcpu)->active)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		goto hypercall_userspace_exit;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST:
		if (unlikely(hc.var_cnt)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		fallthrough;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST_EX:
		if (unlikely(!hc.rep_cnt || hc.rep_idx)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		ret = kvm_hv_flush_tlb(vcpu, &hc);
		break;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE:
		if (unlikely(hc.var_cnt)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		fallthrough;
	case HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX:
		if (unlikely(hc.rep)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		ret = kvm_hv_flush_tlb(vcpu, &hc);
		break;
	case HVCALL_SEND_IPI:
		if (unlikely(hc.var_cnt)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		fallthrough;
	case HVCALL_SEND_IPI_EX:
		if (unlikely(hc.rep)) {
			ret = HV_STATUS_INVALID_HYPERCALL_INPUT;
			break;
		}
		ret = kvm_hv_send_ipi(vcpu, &hc);
		break;
	case HVCALL_POST_DEBUG_DATA:
	case HVCALL_RETRIEVE_DEBUG_DATA:
		if (unlikely(hc.fast)) {
			ret = HV_STATUS_INVALID_PARAMETER;
			break;
		}
		fallthrough;
	case HVCALL_RESET_DEBUG_SESSION: {
		struct kvm_hv_syndbg *syndbg = to_hv_syndbg(vcpu);

		if (!kvm_hv_is_syndbg_enabled(vcpu)) {
			ret = HV_STATUS_INVALID_HYPERCALL_CODE;
			break;
		}

		if (!(syndbg->options & HV_X64_SYNDBG_OPTION_USE_HCALLS)) {
			ret = HV_STATUS_OPERATION_DENIED;
			break;
		}
		goto hypercall_userspace_exit;
	}
	case HV_EXT_CALL_QUERY_CAPABILITIES ... HV_EXT_CALL_MAX:
		if (unlikely(hc.fast)) {
			ret = HV_STATUS_INVALID_PARAMETER;
			break;
		}
		goto hypercall_userspace_exit;
	default:
		ret = HV_STATUS_INVALID_HYPERCALL_CODE;
		break;
	}

hypercall_complete:
	return kvm_hv_hypercall_complete(vcpu, ret);

hypercall_userspace_exit:
	vcpu->run->exit_reason = KVM_EXIT_HYPERV;
	vcpu->run->hyperv.type = KVM_EXIT_HYPERV_HCALL;
	vcpu->run->hyperv.u.hcall.input = hc.param;
	vcpu->run->hyperv.u.hcall.params[0] = hc.ingpa;
	vcpu->run->hyperv.u.hcall.params[1] = hc.outgpa;
	vcpu->arch.complete_userspace_io = kvm_hv_hypercall_complete_userspace;
	return 0;
}

void kvm_hv_init_vm(struct kvm *kvm)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);

	mutex_init(&hv->hv_lock);
	idr_init(&hv->conn_to_evt);
}

void kvm_hv_destroy_vm(struct kvm *kvm)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	struct eventfd_ctx *eventfd;
	int i;

	idr_for_each_entry(&hv->conn_to_evt, eventfd, i)
		eventfd_ctx_put(eventfd);
	idr_destroy(&hv->conn_to_evt);
}

static int kvm_hv_eventfd_assign(struct kvm *kvm, u32 conn_id, int fd)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	struct eventfd_ctx *eventfd;
	int ret;

	eventfd = eventfd_ctx_fdget(fd);
	if (IS_ERR(eventfd))
		return PTR_ERR(eventfd);

	mutex_lock(&hv->hv_lock);
	ret = idr_alloc(&hv->conn_to_evt, eventfd, conn_id, conn_id + 1,
			GFP_KERNEL_ACCOUNT);
	mutex_unlock(&hv->hv_lock);

	if (ret >= 0)
		return 0;

	if (ret == -ENOSPC)
		ret = -EEXIST;
	eventfd_ctx_put(eventfd);
	return ret;
}

static int kvm_hv_eventfd_deassign(struct kvm *kvm, u32 conn_id)
{
	struct kvm_hv *hv = to_kvm_hv(kvm);
	struct eventfd_ctx *eventfd;

	mutex_lock(&hv->hv_lock);
	eventfd = idr_remove(&hv->conn_to_evt, conn_id);
	mutex_unlock(&hv->hv_lock);

	if (!eventfd)
		return -ENOENT;

	synchronize_srcu(&kvm->srcu);
	eventfd_ctx_put(eventfd);
	return 0;
}

int kvm_vm_ioctl_hv_eventfd(struct kvm *kvm, struct kvm_hyperv_eventfd *args)
{
	if ((args->flags & ~KVM_HYPERV_EVENTFD_DEASSIGN) ||
	    (args->conn_id & ~KVM_HYPERV_CONN_ID_MASK))
		return -EINVAL;

	if (args->flags == KVM_HYPERV_EVENTFD_DEASSIGN)
		return kvm_hv_eventfd_deassign(kvm, args->conn_id);
	return kvm_hv_eventfd_assign(kvm, args->conn_id, args->fd);
}