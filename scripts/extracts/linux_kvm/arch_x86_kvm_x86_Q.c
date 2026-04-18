// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 49/86



static int emulator_cmpxchg_emulated(struct x86_emulate_ctxt *ctxt,
				     unsigned long addr,
				     const void *old,
				     const void *new,
				     unsigned int bytes,
				     struct x86_exception *exception)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	u64 page_line_mask;
	unsigned long hva;
	gpa_t gpa;
	int r;

	/* guests cmpxchg8b have to be emulated atomically */
	if (bytes > 8 || (bytes & (bytes - 1)))
		goto emul_write;

	gpa = kvm_mmu_gva_to_gpa_write(vcpu, addr, NULL);

	if (gpa == INVALID_GPA ||
	    (gpa & PAGE_MASK) == APIC_DEFAULT_PHYS_BASE)
		goto emul_write;

	/*
	 * Emulate the atomic as a straight write to avoid #AC if SLD is
	 * enabled in the host and the access splits a cache line.
	 */
	if (boot_cpu_has(X86_FEATURE_SPLIT_LOCK_DETECT))
		page_line_mask = ~(cache_line_size() - 1);
	else
		page_line_mask = PAGE_MASK;

	if (((gpa + bytes - 1) & page_line_mask) != (gpa & page_line_mask))
		goto emul_write;

	hva = kvm_vcpu_gfn_to_hva(vcpu, gpa_to_gfn(gpa));
	if (kvm_is_error_hva(hva))
		goto emul_write;

	hva += offset_in_page(gpa);

	switch (bytes) {
	case 1:
		r = emulator_try_cmpxchg_user(u8, hva, old, new);
		break;
	case 2:
		r = emulator_try_cmpxchg_user(u16, hva, old, new);
		break;
	case 4:
		r = emulator_try_cmpxchg_user(u32, hva, old, new);
		break;
	case 8:
		r = emulator_try_cmpxchg_user(u64, hva, old, new);
		break;
	default:
		BUG();
	}

	if (r < 0)
		return X86EMUL_UNHANDLEABLE;

	/*
	 * Mark the page dirty _before_ checking whether or not the CMPXCHG was
	 * successful, as the old value is written back on failure.  Note, for
	 * live migration, this is unnecessarily conservative as CMPXCHG writes
	 * back the original value and the access is atomic, but KVM's ABI is
	 * that all writes are dirty logged, regardless of the value written.
	 */
	kvm_vcpu_mark_page_dirty(vcpu, gpa_to_gfn(gpa));

	if (r)
		return X86EMUL_CMPXCHG_FAILED;

	kvm_page_track_write(vcpu, gpa, new, bytes);

	return X86EMUL_CONTINUE;

emul_write:
	pr_warn_once("emulating exchange as write\n");

	return emulator_write_emulated(ctxt, addr, new, bytes, exception);
}

static int emulator_pio_in_out(struct kvm_vcpu *vcpu, int size,
			       unsigned short port, void *data,
			       unsigned int count, bool in)
{
	unsigned i;
	int r;

	WARN_ON_ONCE(vcpu->arch.pio.count);
	for (i = 0; i < count; i++) {
		if (in)
			r = kvm_io_bus_read(vcpu, KVM_PIO_BUS, port, size, data);
		else
			r = kvm_io_bus_write(vcpu, KVM_PIO_BUS, port, size, data);

		if (r) {
			if (i == 0)
				goto userspace_io;

			/*
			 * Userspace must have unregistered the device while PIO
			 * was running.  Drop writes / read as 0.
			 */
			if (in)
				memset(data, 0, size * (count - i));
			break;
		}

		data += size;
	}
	return 1;

userspace_io:
	vcpu->arch.pio.port = port;
	vcpu->arch.pio.in = in;
	vcpu->arch.pio.count = count;
	vcpu->arch.pio.size = size;

	if (in)
		memset(vcpu->arch.pio_data, 0, size * count);
	else
		memcpy(vcpu->arch.pio_data, data, size * count);

	vcpu->run->exit_reason = KVM_EXIT_IO;
	vcpu->run->io.direction = in ? KVM_EXIT_IO_IN : KVM_EXIT_IO_OUT;
	vcpu->run->io.size = size;
	vcpu->run->io.data_offset = KVM_PIO_PAGE_OFFSET * PAGE_SIZE;
	vcpu->run->io.count = count;
	vcpu->run->io.port = port;
	return 0;
}

static int emulator_pio_in(struct kvm_vcpu *vcpu, int size,
      			   unsigned short port, void *val, unsigned int count)
{
	int r = emulator_pio_in_out(vcpu, size, port, val, count, true);
	if (r)
		trace_kvm_pio(KVM_PIO_IN, port, size, count, val);

	return r;
}

static void complete_emulator_pio_in(struct kvm_vcpu *vcpu, void *val)
{
	int size = vcpu->arch.pio.size;
	unsigned int count = vcpu->arch.pio.count;
	memcpy(val, vcpu->arch.pio_data, size * count);
	trace_kvm_pio(KVM_PIO_IN, vcpu->arch.pio.port, size, count, vcpu->arch.pio_data);
	vcpu->arch.pio.count = 0;
}

static int emulator_pio_in_emulated(struct x86_emulate_ctxt *ctxt,
				    int size, unsigned short port, void *val,
				    unsigned int count)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	if (vcpu->arch.pio.count) {
		/*
		 * Complete a previous iteration that required userspace I/O.
		 * Note, @count isn't guaranteed to match pio.count as userspace
		 * can modify ECX before rerunning the vCPU.  Ignore any such
		 * shenanigans as KVM doesn't support modifying the rep count,
		 * and the emulator ensures @count doesn't overflow the buffer.
		 */
		complete_emulator_pio_in(vcpu, val);
		return 1;
	}

	return emulator_pio_in(vcpu, size, port, val, count);
}

static int emulator_pio_out(struct kvm_vcpu *vcpu, int size,
			    unsigned short port, const void *val,
			    unsigned int count)
{
	trace_kvm_pio(KVM_PIO_OUT, port, size, count, val);
	return emulator_pio_in_out(vcpu, size, port, (void *)val, count, false);
}