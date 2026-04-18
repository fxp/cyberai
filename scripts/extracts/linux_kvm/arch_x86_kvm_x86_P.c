// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/x86.c
// Segment 48/86



	/*
	 * If the memory is not _known_ to be emulated MMIO, attempt to access
	 * guest memory.  If accessing guest memory fails, e.g. because there's
	 * no memslot, then handle the access as MMIO.  Note, treating the
	 * access as emulated MMIO is technically wrong if there is a memslot,
	 * i.e. if accessing host user memory failed, but this has been KVM's
	 * historical ABI for decades.
	 */
	if (!ret && ops->read_write_guest(vcpu, gpa, val, bytes))
		return X86EMUL_CONTINUE;

	/*
	 * Attempt to handle emulated MMIO within the kernel, e.g. for accesses
	 * to an in-kernel local or I/O APIC, or to an ioeventfd range attached
	 * to MMIO bus.  If the access isn't fully resolved, insert an MMIO
	 * fragment with the relevant details.
	 */
	handled = ops->read_write_mmio(vcpu, gpa, bytes, val);
	if (handled == bytes)
		return X86EMUL_CONTINUE;

	gpa += handled;
	bytes -= handled;
	val += handled;

	WARN_ON(vcpu->mmio_nr_fragments >= KVM_MAX_MMIO_FRAGMENTS);
	frag = &vcpu->mmio_fragments[vcpu->mmio_nr_fragments++];
	frag->gpa = gpa;
	if (write && bytes <= 8u) {
		frag->val = 0;
		frag->data = &frag->val;
		memcpy(&frag->val, val, bytes);
	} else {
		frag->data = val;
	}
	frag->len = bytes;

	/*
	 * Continue emulating, even though KVM needs to (eventually) do an MMIO
	 * exit to userspace.  If the access splits multiple pages, then KVM
	 * needs to exit to userspace only after emulating both parts of the
	 * access.
	 */
	return X86EMUL_CONTINUE;
}

static int emulator_read_write(struct x86_emulate_ctxt *ctxt,
			unsigned long addr,
			void *val, unsigned int bytes,
			struct x86_exception *exception,
			const struct read_write_emulator_ops *ops)
{
	struct kvm_vcpu *vcpu = emul_to_vcpu(ctxt);
	int rc;

	if (WARN_ON_ONCE((bytes > 8u || !ops->write) && object_is_on_stack(val)))
		return X86EMUL_UNHANDLEABLE;

	/*
	 * If the read was already completed via a userspace MMIO exit, there's
	 * nothing left to do except trace the MMIO read.  When completing MMIO
	 * reads, KVM re-emulates the instruction to propagate the value into
	 * the correct destination, e.g. into the correct register, but the
	 * value itself has already been copied to the read cache.
	 *
	 * Note!  This is *tightly* coupled to read_emulated() satisfying reads
	 * from the emulator's mem_read cache, so that the MMIO fragment data
	 * is copied to the correct chunk of the correct operand.
	 */
	if (!ops->write && vcpu->mmio_read_completed) {
		/*
		 * For simplicity, trace the entire MMIO read in one shot, even
		 * though the GPA might be incorrect if there are two fragments
		 * that aren't contiguous in the GPA space.
		 */
		trace_kvm_mmio(KVM_TRACE_MMIO_READ, bytes,
			       vcpu->mmio_fragments[0].gpa, val);
		vcpu->mmio_read_completed = 0;
		return X86EMUL_CONTINUE;
	}

	vcpu->mmio_nr_fragments = 0;

	/* Crossing a page boundary? */
	if (((addr + bytes - 1) ^ addr) & PAGE_MASK) {
		int now;

		now = -addr & ~PAGE_MASK;
		rc = emulator_read_write_onepage(addr, val, now, exception,
						 vcpu, ops);

		if (rc != X86EMUL_CONTINUE)
			return rc;
		addr += now;
		if (ctxt->mode != X86EMUL_MODE_PROT64)
			addr = (u32)addr;
		val += now;
		bytes -= now;
	}

	rc = emulator_read_write_onepage(addr, val, bytes, exception,
					 vcpu, ops);
	if (rc != X86EMUL_CONTINUE)
		return rc;

	if (!vcpu->mmio_nr_fragments)
		return X86EMUL_CONTINUE;

	vcpu->mmio_needed = 1;
	vcpu->mmio_cur_fragment = 0;
	vcpu->mmio_is_write = ops->write;

	kvm_prepare_emulated_mmio_exit(vcpu, &vcpu->mmio_fragments[0]);

	/*
	 * For MMIO reads, stop emulating and immediately exit to userspace, as
	 * KVM needs the value to correctly emulate the instruction.  For MMIO
	 * writes, continue emulating as the write to MMIO is a side effect for
	 * all intents and purposes.  KVM will still exit to userspace, but
	 * after completing emulation (see the check on vcpu->mmio_needed in
	 * x86_emulate_instruction()).
	 */
	return ops->write ? X86EMUL_CONTINUE : X86EMUL_IO_NEEDED;
}

static int emulator_read_emulated(struct x86_emulate_ctxt *ctxt,
				  unsigned long addr,
				  void *val,
				  unsigned int bytes,
				  struct x86_exception *exception)
{
	static const struct read_write_emulator_ops ops = {
		.read_write_guest = emulator_read_guest,
		.read_write_mmio = vcpu_mmio_read,
		.write = false,
	};

	return emulator_read_write(ctxt, addr, val, bytes, exception, &ops);
}

static int emulator_write_emulated(struct x86_emulate_ctxt *ctxt,
			    unsigned long addr,
			    const void *val,
			    unsigned int bytes,
			    struct x86_exception *exception)
{
	static const struct read_write_emulator_ops ops = {
		.read_write_guest = emulator_write_guest,
		.read_write_mmio = vcpu_mmio_write,
		.write = true,
	};

	return emulator_read_write(ctxt, addr, (void *)val, bytes, exception, &ops);
}

#define emulator_try_cmpxchg_user(t, ptr, old, new) \
	(__try_cmpxchg_user((t __user *)(ptr), (t *)(old), *(t *)(new), efault ## t))