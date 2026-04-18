// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/svm/svm.c
// Segment 10/36



static void svm_set_vintr(struct vcpu_svm *svm)
{
	struct vmcb_control_area *control;

	/*
	 * The following fields are ignored when AVIC is enabled
	 */
	WARN_ON(kvm_vcpu_apicv_activated(&svm->vcpu));

	svm_set_intercept(svm, INTERCEPT_VINTR);

	/*
	 * Recalculating intercepts may have cleared the VINTR intercept.  If
	 * V_INTR_MASKING is enabled in vmcb12, then the effective RFLAGS.IF
	 * for L1 physical interrupts is L1's RFLAGS.IF at the time of VMRUN.
	 * Requesting an interrupt window if save.RFLAGS.IF=0 is pointless as
	 * interrupts will never be unblocked while L2 is running.
	 */
	if (!svm_is_intercept(svm, INTERCEPT_VINTR))
		return;

	/*
	 * This is just a dummy VINTR to actually cause a vmexit to happen.
	 * Actual injection of virtual interrupts happens through EVENTINJ.
	 */
	control = &svm->vmcb->control;
	control->int_vector = 0x0;
	control->int_ctl &= ~V_INTR_PRIO_MASK;
	control->int_ctl |= V_IRQ_MASK |
		((/*control->int_vector >> 4*/ 0xf) << V_INTR_PRIO_SHIFT);
	vmcb_mark_dirty(svm->vmcb, VMCB_INTR);
}

static void svm_clear_vintr(struct vcpu_svm *svm)
{
	svm_clr_intercept(svm, INTERCEPT_VINTR);

	/* Drop int_ctl fields related to VINTR injection.  */
	svm->vmcb->control.int_ctl &= ~V_IRQ_INJECTION_BITS_MASK;
	if (is_guest_mode(&svm->vcpu)) {
		svm->vmcb01.ptr->control.int_ctl &= ~V_IRQ_INJECTION_BITS_MASK;

		WARN_ON((svm->vmcb->control.int_ctl & V_TPR_MASK) !=
			(svm->nested.ctl.int_ctl & V_TPR_MASK));

		svm->vmcb->control.int_ctl |= svm->nested.ctl.int_ctl &
			V_IRQ_INJECTION_BITS_MASK;

		svm->vmcb->control.int_vector = svm->nested.ctl.int_vector;
	}

	vmcb_mark_dirty(svm->vmcb, VMCB_INTR);
}

static struct vmcb_seg *svm_seg(struct kvm_vcpu *vcpu, int seg)
{
	struct vmcb_save_area *save = &to_svm(vcpu)->vmcb->save;
	struct vmcb_save_area *save01 = &to_svm(vcpu)->vmcb01.ptr->save;

	switch (seg) {
	case VCPU_SREG_CS: return &save->cs;
	case VCPU_SREG_DS: return &save->ds;
	case VCPU_SREG_ES: return &save->es;
	case VCPU_SREG_FS: return &save01->fs;
	case VCPU_SREG_GS: return &save01->gs;
	case VCPU_SREG_SS: return &save->ss;
	case VCPU_SREG_TR: return &save01->tr;
	case VCPU_SREG_LDTR: return &save01->ldtr;
	}
	BUG();
	return NULL;
}

static u64 svm_get_segment_base(struct kvm_vcpu *vcpu, int seg)
{
	struct vmcb_seg *s = svm_seg(vcpu, seg);

	return s->base;
}

static void svm_get_segment(struct kvm_vcpu *vcpu,
			    struct kvm_segment *var, int seg)
{
	struct vmcb_seg *s = svm_seg(vcpu, seg);

	var->base = s->base;
	var->limit = s->limit;
	var->selector = s->selector;
	var->type = s->attrib & SVM_SELECTOR_TYPE_MASK;
	var->s = (s->attrib >> SVM_SELECTOR_S_SHIFT) & 1;
	var->dpl = (s->attrib >> SVM_SELECTOR_DPL_SHIFT) & 3;
	var->present = (s->attrib >> SVM_SELECTOR_P_SHIFT) & 1;
	var->avl = (s->attrib >> SVM_SELECTOR_AVL_SHIFT) & 1;
	var->l = (s->attrib >> SVM_SELECTOR_L_SHIFT) & 1;
	var->db = (s->attrib >> SVM_SELECTOR_DB_SHIFT) & 1;

	/*
	 * AMD CPUs circa 2014 track the G bit for all segments except CS.
	 * However, the SVM spec states that the G bit is not observed by the
	 * CPU, and some VMware virtual CPUs drop the G bit for all segments.
	 * So let's synthesize a legal G bit for all segments, this helps
	 * running KVM nested. It also helps cross-vendor migration, because
	 * Intel's vmentry has a check on the 'G' bit.
	 */
	var->g = s->limit > 0xfffff;

	/*
	 * AMD's VMCB does not have an explicit unusable field, so emulate it
	 * for cross vendor migration purposes by "not present"
	 */
	var->unusable = !var->present;

	switch (seg) {
	case VCPU_SREG_TR:
		/*
		 * Work around a bug where the busy flag in the tr selector
		 * isn't exposed
		 */
		var->type |= 0x2;
		break;
	case VCPU_SREG_DS:
	case VCPU_SREG_ES:
	case VCPU_SREG_FS:
	case VCPU_SREG_GS:
		/*
		 * The accessed bit must always be set in the segment
		 * descriptor cache, although it can be cleared in the
		 * descriptor, the cached bit always remains at 1. Since
		 * Intel has a check on this, set it here to support
		 * cross-vendor migration.
		 */
		if (!var->unusable)
			var->type |= 0x1;
		break;
	case VCPU_SREG_SS:
		/*
		 * On AMD CPUs sometimes the DB bit in the segment
		 * descriptor is left as 1, although the whole segment has
		 * been made unusable. Clear it here to pass an Intel VMX
		 * entry check when cross vendor migrating.
		 */
		if (var->unusable)
			var->db = 0;
		/* This is symmetric with svm_set_segment() */
		var->dpl = to_svm(vcpu)->vmcb->save.cpl;
		break;
	}
}

static int svm_get_cpl(struct kvm_vcpu *vcpu)
{
	struct vmcb_save_area *save = &to_svm(vcpu)->vmcb->save;

	return save->cpl;
}

static void svm_get_cs_db_l_bits(struct kvm_vcpu *vcpu, int *db, int *l)
{
	struct kvm_segment cs;

	svm_get_segment(vcpu, &cs, VCPU_SREG_CS);
	*db = cs.db;
	*l = cs.l;
}

static void svm_get_idt(struct kvm_vcpu *vcpu, struct desc_ptr *dt)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	dt->size = svm->vmcb->save.idtr.limit;
	dt->address = svm->vmcb->save.idtr.base;
}