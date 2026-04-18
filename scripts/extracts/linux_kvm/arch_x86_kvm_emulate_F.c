// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/emulate.c
// Segment 6/33



	if (efer & EFER_LMA) {
		if (cs.l) {
			/* Proper long mode */
			ctxt->mode = X86EMUL_MODE_PROT64;
		} else if (cs.d) {
			/* 32 bit compatibility mode*/
			ctxt->mode = X86EMUL_MODE_PROT32;
		} else {
			ctxt->mode = X86EMUL_MODE_PROT16;
		}
	} else {
		/* Legacy 32 bit / 16 bit mode */
		ctxt->mode = cs.d ? X86EMUL_MODE_PROT32 : X86EMUL_MODE_PROT16;
	}

	return X86EMUL_CONTINUE;
}

static inline int assign_eip_near(struct x86_emulate_ctxt *ctxt, ulong dst)
{
	return assign_eip(ctxt, dst);
}

static int assign_eip_far(struct x86_emulate_ctxt *ctxt, ulong dst)
{
	int rc = emulator_recalc_and_set_mode(ctxt);

	if (rc != X86EMUL_CONTINUE)
		return rc;

	return assign_eip(ctxt, dst);
}

static inline int jmp_rel(struct x86_emulate_ctxt *ctxt, int rel)
{
	return assign_eip_near(ctxt, ctxt->_eip + rel);
}

static int linear_read_system(struct x86_emulate_ctxt *ctxt, ulong linear,
			      void *data, unsigned size)
{
	return ctxt->ops->read_std(ctxt, linear, data, size, &ctxt->exception, true);
}

static int linear_write_system(struct x86_emulate_ctxt *ctxt,
			       ulong linear, void *data,
			       unsigned int size)
{
	return ctxt->ops->write_std(ctxt, linear, data, size, &ctxt->exception, true);
}

static int segmented_read_std(struct x86_emulate_ctxt *ctxt,
			      struct segmented_address addr,
			      void *data,
			      unsigned size)
{
	int rc;
	ulong linear;

	rc = linearize(ctxt, addr, size, false, &linear);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	return ctxt->ops->read_std(ctxt, linear, data, size, &ctxt->exception, false);
}

static int segmented_write_std(struct x86_emulate_ctxt *ctxt,
			       struct segmented_address addr,
			       void *data,
			       unsigned int size)
{
	int rc;
	ulong linear;

	rc = linearize(ctxt, addr, size, true, &linear);
	if (rc != X86EMUL_CONTINUE)
		return rc;
	return ctxt->ops->write_std(ctxt, linear, data, size, &ctxt->exception, false);
}

/*
 * Prefetch the remaining bytes of the instruction without crossing page
 * boundary if they are not in fetch_cache yet.
 */
static int __do_insn_fetch_bytes(struct x86_emulate_ctxt *ctxt, int op_size)
{
	int rc;
	unsigned size, max_size;
	unsigned long linear;
	int cur_size = ctxt->fetch.end - ctxt->fetch.data;
	struct segmented_address addr = { .seg = VCPU_SREG_CS,
					   .ea = ctxt->eip + cur_size };

	/*
	 * We do not know exactly how many bytes will be needed, and
	 * __linearize is expensive, so fetch as much as possible.  We
	 * just have to avoid going beyond the 15 byte limit, the end
	 * of the segment, or the end of the page.
	 *
	 * __linearize is called with size 0 so that it does not do any
	 * boundary check itself.  Instead, we use max_size to check
	 * against op_size.
	 */
	rc = __linearize(ctxt, addr, &max_size, 0, ctxt->mode, &linear,
			 X86EMUL_F_FETCH);
	if (unlikely(rc != X86EMUL_CONTINUE))
		return rc;

	size = min_t(unsigned, 15UL ^ cur_size, max_size);
	size = min_t(unsigned, size, PAGE_SIZE - offset_in_page(linear));

	/*
	 * One instruction can only straddle two pages,
	 * and one has been loaded at the beginning of
	 * x86_decode_insn.  So, if not enough bytes
	 * still, we must have hit the 15-byte boundary.
	 */
	if (unlikely(size < op_size))
		return emulate_gp(ctxt, 0);

	rc = ctxt->ops->fetch(ctxt, linear, ctxt->fetch.end,
			      size, &ctxt->exception);
	if (unlikely(rc != X86EMUL_CONTINUE))
		return rc;
	ctxt->fetch.end += size;
	return X86EMUL_CONTINUE;
}

static __always_inline int do_insn_fetch_bytes(struct x86_emulate_ctxt *ctxt,
					       unsigned size)
{
	unsigned done_size = ctxt->fetch.end - ctxt->fetch.ptr;

	if (unlikely(done_size < size))
		return __do_insn_fetch_bytes(ctxt, size - done_size);
	else
		return X86EMUL_CONTINUE;
}

/* Fetch next part of the instruction being emulated. */
#define insn_fetch(_type, _ctxt)					\
({	_type _x;							\
									\
	rc = do_insn_fetch_bytes(_ctxt, sizeof(_type));			\
	if (rc != X86EMUL_CONTINUE)					\
		goto done;						\
	ctxt->_eip += sizeof(_type);					\
	memcpy(&_x, ctxt->fetch.ptr, sizeof(_type));			\
	ctxt->fetch.ptr += sizeof(_type);				\
	_x;								\
})

#define insn_fetch_arr(_arr, _size, _ctxt)				\
({									\
	rc = do_insn_fetch_bytes(_ctxt, _size);				\
	if (rc != X86EMUL_CONTINUE)					\
		goto done;						\
	ctxt->_eip += (_size);						\
	memcpy(_arr, ctxt->fetch.ptr, _size);				\
	ctxt->fetch.ptr += (_size);					\
})

/*
 * Given the 'reg' portion of a ModRM byte, and a register block, return a
 * pointer into the block that addresses the relevant register.
 * @highbyte_regs specifies whether to decode AH,CH,DH,BH.
 */
static void *decode_register(struct x86_emulate_ctxt *ctxt, u8 modrm_reg,
			     int byteop)
{
	void *p;
	int highbyte_regs = (ctxt->rex_prefix == REX_NONE) && byteop;

	if (highbyte_regs && modrm_reg >= 4 && modrm_reg < 8)
		p = (unsigned char *)reg_rmw(ctxt, modrm_reg & 3) + 1;
	else
		p = reg_rmw(ctxt, modrm_reg);
	return p;
}