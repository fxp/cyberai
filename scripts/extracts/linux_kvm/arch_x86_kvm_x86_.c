// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 85/86



		/*
		 * All done, as frag->data always points at the GHCB scratch
		 * area and VMGEXIT is trap-like (RIP is advanced by hardware).
		 */
		return 1;
	}

	// More MMIO is needed
	kvm_prepare_emulated_mmio_exit(vcpu, frag);
	vcpu->arch.complete_userspace_io = complete_sev_es_emulated_mmio;
	return 0;
}

int kvm_sev_es_mmio(struct kvm_vcpu *vcpu, bool is_write, gpa_t gpa,
		    unsigned int bytes, void *data)
{
	struct kvm_mmio_fragment *frag;
	int handled;

	if (!data || WARN_ON_ONCE(object_is_on_stack(data)))
		return -EINVAL;

	if (is_write)
		handled = vcpu_mmio_write(vcpu, gpa, bytes, data);
	else
		handled = vcpu_mmio_read(vcpu, gpa, bytes, data);
	if (handled == bytes)
		return 1;

	bytes -= handled;
	gpa += handled;
	data += handled;

	/*
	 * TODO: Determine whether or not userspace plays nice with MMIO
	 *       requests that split a page boundary.
	 */
	frag = vcpu->mmio_fragments;
	frag->len = bytes;
	frag->gpa = gpa;
	frag->data = data;

	vcpu->mmio_needed = 1;
	vcpu->mmio_cur_fragment = 0;
	vcpu->mmio_nr_fragments = 1;
	vcpu->mmio_is_write = is_write;

	kvm_prepare_emulated_mmio_exit(vcpu, frag);
	vcpu->arch.complete_userspace_io = complete_sev_es_emulated_mmio;
	return 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_sev_es_mmio);

static void advance_sev_es_emulated_pio(struct kvm_vcpu *vcpu, unsigned count, int size)
{
	vcpu->arch.sev_pio_count -= count;
	vcpu->arch.sev_pio_data += count * size;
}

static int kvm_sev_es_outs(struct kvm_vcpu *vcpu, unsigned int size,
			   unsigned int port);

static int complete_sev_es_emulated_outs(struct kvm_vcpu *vcpu)
{
	int size = vcpu->arch.pio.size;
	int port = vcpu->arch.pio.port;

	vcpu->arch.pio.count = 0;
	if (vcpu->arch.sev_pio_count)
		return kvm_sev_es_outs(vcpu, size, port);
	return 1;
}

static int kvm_sev_es_outs(struct kvm_vcpu *vcpu, unsigned int size,
			   unsigned int port)
{
	for (;;) {
		unsigned int count =
			min_t(unsigned int, PAGE_SIZE / size, vcpu->arch.sev_pio_count);
		int ret = emulator_pio_out(vcpu, size, port, vcpu->arch.sev_pio_data, count);

		/* memcpy done already by emulator_pio_out.  */
		advance_sev_es_emulated_pio(vcpu, count, size);
		if (!ret)
			break;

		/* Emulation done by the kernel.  */
		if (!vcpu->arch.sev_pio_count)
			return 1;
	}

	vcpu->arch.complete_userspace_io = complete_sev_es_emulated_outs;
	return 0;
}

static int kvm_sev_es_ins(struct kvm_vcpu *vcpu, unsigned int size,
			  unsigned int port);

static int complete_sev_es_emulated_ins(struct kvm_vcpu *vcpu)
{
	unsigned count = vcpu->arch.pio.count;
	int size = vcpu->arch.pio.size;
	int port = vcpu->arch.pio.port;

	complete_emulator_pio_in(vcpu, vcpu->arch.sev_pio_data);
	advance_sev_es_emulated_pio(vcpu, count, size);
	if (vcpu->arch.sev_pio_count)
		return kvm_sev_es_ins(vcpu, size, port);
	return 1;
}

static int kvm_sev_es_ins(struct kvm_vcpu *vcpu, unsigned int size,
			  unsigned int port)
{
	for (;;) {
		unsigned int count =
			min_t(unsigned int, PAGE_SIZE / size, vcpu->arch.sev_pio_count);
		if (!emulator_pio_in(vcpu, size, port, vcpu->arch.sev_pio_data, count))
			break;

		/* Emulation done by the kernel.  */
		advance_sev_es_emulated_pio(vcpu, count, size);
		if (!vcpu->arch.sev_pio_count)
			return 1;
	}

	vcpu->arch.complete_userspace_io = complete_sev_es_emulated_ins;
	return 0;
}

int kvm_sev_es_string_io(struct kvm_vcpu *vcpu, unsigned int size,
			 unsigned int port, void *data,  unsigned int count,
			 int in)
{
	vcpu->arch.sev_pio_data = data;
	vcpu->arch.sev_pio_count = count;
	return in ? kvm_sev_es_ins(vcpu, size, port)
		  : kvm_sev_es_outs(vcpu, size, port);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_sev_es_string_io);