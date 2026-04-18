// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 43/86



	/*
	 * This pairs with kvm_guest_time_update(): when masterclock is
	 * in use, we use master_kernel_ns + kvmclock_offset to set
	 * unsigned 'system_time' so if we use get_kvmclock_ns() (which
	 * is slightly ahead) here we risk going negative on unsigned
	 * 'system_time' when 'data.clock' is very small.
	 */
	if (data.flags & KVM_CLOCK_REALTIME) {
		u64 now_real_ns = ktime_get_real_ns();

		/*
		 * Avoid stepping the kvmclock backwards.
		 */
		if (now_real_ns > data.realtime)
			data.clock += now_real_ns - data.realtime;
	}

	if (ka->use_master_clock)
		now_raw_ns = ka->master_kernel_ns;
	else
		now_raw_ns = get_kvmclock_base_ns();
	ka->kvmclock_offset = data.clock - now_raw_ns;
	kvm_end_pvclock_update(kvm);
	return 0;
}

long kvm_arch_vcpu_unlocked_ioctl(struct file *filp, unsigned int ioctl,
				  unsigned long arg)
{
	struct kvm_vcpu *vcpu = filp->private_data;
	void __user *argp = (void __user *)arg;

	if (ioctl == KVM_MEMORY_ENCRYPT_OP &&
	    kvm_x86_ops.vcpu_mem_enc_unlocked_ioctl)
		return kvm_x86_call(vcpu_mem_enc_unlocked_ioctl)(vcpu, argp);

	return -ENOIOCTLCMD;
}

int kvm_arch_vm_ioctl(struct file *filp, unsigned int ioctl, unsigned long arg)
{
	struct kvm *kvm = filp->private_data;
	void __user *argp = (void __user *)arg;
	int r = -ENOTTY;

#ifdef CONFIG_KVM_IOAPIC
	/*
	 * This union makes it completely explicit to gcc-3.x
	 * that these three variables' stack usage should be
	 * combined, not added together.
	 */
	union {
		struct kvm_pit_state ps;
		struct kvm_pit_state2 ps2;
		struct kvm_pit_config pit_config;
	} u;
#endif

	switch (ioctl) {
	case KVM_SET_TSS_ADDR:
		r = kvm_vm_ioctl_set_tss_addr(kvm, arg);
		break;
	case KVM_SET_IDENTITY_MAP_ADDR: {
		u64 ident_addr;

		mutex_lock(&kvm->lock);
		r = -EINVAL;
		if (kvm->created_vcpus)
			goto set_identity_unlock;
		r = -EFAULT;
		if (copy_from_user(&ident_addr, argp, sizeof(ident_addr)))
			goto set_identity_unlock;
		r = kvm_vm_ioctl_set_identity_map_addr(kvm, ident_addr);
set_identity_unlock:
		mutex_unlock(&kvm->lock);
		break;
	}
	case KVM_SET_NR_MMU_PAGES:
		r = kvm_vm_ioctl_set_nr_mmu_pages(kvm, arg);
		break;
#ifdef CONFIG_KVM_IOAPIC
	case KVM_CREATE_IRQCHIP: {
		mutex_lock(&kvm->lock);

		r = -EEXIST;
		if (irqchip_in_kernel(kvm))
			goto create_irqchip_unlock;

		/*
		 * Disallow an in-kernel I/O APIC if the VM has protected EOIs,
		 * i.e. if KVM can't intercept EOIs and thus can't properly
		 * emulate level-triggered interrupts.
		 */
		r = -ENOTTY;
		if (kvm->arch.has_protected_eoi)
			goto create_irqchip_unlock;

		r = -EINVAL;
		if (kvm->created_vcpus)
			goto create_irqchip_unlock;

		r = kvm_pic_init(kvm);
		if (r)
			goto create_irqchip_unlock;

		r = kvm_ioapic_init(kvm);
		if (r) {
			kvm_pic_destroy(kvm);
			goto create_irqchip_unlock;
		}

		r = kvm_setup_default_ioapic_and_pic_routing(kvm);
		if (r) {
			kvm_ioapic_destroy(kvm);
			kvm_pic_destroy(kvm);
			goto create_irqchip_unlock;
		}
		/* Write kvm->irq_routing before enabling irqchip_in_kernel. */
		smp_wmb();
		kvm->arch.irqchip_mode = KVM_IRQCHIP_KERNEL;
		kvm_clear_apicv_inhibit(kvm, APICV_INHIBIT_REASON_ABSENT);
	create_irqchip_unlock:
		mutex_unlock(&kvm->lock);
		break;
	}
	case KVM_CREATE_PIT:
		u.pit_config.flags = KVM_PIT_SPEAKER_DUMMY;
		goto create_pit;
	case KVM_CREATE_PIT2:
		r = -EFAULT;
		if (copy_from_user(&u.pit_config, argp,
				   sizeof(struct kvm_pit_config)))
			goto out;
	create_pit:
		mutex_lock(&kvm->lock);
		r = -EEXIST;
		if (kvm->arch.vpit)
			goto create_pit_unlock;
		r = -ENOENT;
		if (!pic_in_kernel(kvm))
			goto create_pit_unlock;
		r = -ENOMEM;
		kvm->arch.vpit = kvm_create_pit(kvm, u.pit_config.flags);
		if (kvm->arch.vpit)
			r = 0;
	create_pit_unlock:
		mutex_unlock(&kvm->lock);
		break;
	case KVM_GET_IRQCHIP: {
		/* 0: PIC master, 1: PIC slave, 2: IOAPIC */
		struct kvm_irqchip *chip;

		chip = memdup_user(argp, sizeof(*chip));
		if (IS_ERR(chip)) {
			r = PTR_ERR(chip);
			goto out;
		}

		r = -ENXIO;
		if (!irqchip_full(kvm))
			goto get_irqchip_out;
		r = kvm_vm_ioctl_get_irqchip(kvm, chip);
		if (r)
			goto get_irqchip_out;
		r = -EFAULT;
		if (copy_to_user(argp, chip, sizeof(*chip)))
			goto get_irqchip_out;
		r = 0;
	get_irqchip_out:
		kfree(chip);
		break;
	}
	case KVM_SET_IRQCHIP: {
		/* 0: PIC master, 1: PIC slave, 2: IOAPIC */
		struct kvm_irqchip *chip;

		chip = memdup_user(argp, sizeof(*chip));
		if (IS_ERR(chip)) {
			r = PTR_ERR(chip);
			goto out;
		}