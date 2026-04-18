// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 6/20



void bpf_prog_pack_free(void *ptr, u32 size)
{
	struct bpf_prog_pack *pack = NULL, *tmp;
	unsigned int nbits;
	unsigned long pos;

	mutex_lock(&pack_mutex);
	if (size > BPF_PROG_PACK_SIZE) {
		bpf_jit_free_exec(ptr);
		goto out;
	}

	list_for_each_entry(tmp, &pack_list, list) {
		if (ptr >= tmp->ptr && (tmp->ptr + BPF_PROG_PACK_SIZE) > ptr) {
			pack = tmp;
			break;
		}
	}

	if (WARN_ONCE(!pack, "bpf_prog_pack bug\n"))
		goto out;

	nbits = BPF_PROG_SIZE_TO_NBITS(size);
	pos = ((unsigned long)ptr - (unsigned long)pack->ptr) >> BPF_PROG_CHUNK_SHIFT;

	WARN_ONCE(bpf_arch_text_invalidate(ptr, size),
		  "bpf_prog_pack bug: missing bpf_arch_text_invalidate?\n");

	bitmap_clear(pack->bitmap, pos, nbits);
	if (bitmap_find_next_zero_area(pack->bitmap, BPF_PROG_CHUNK_COUNT, 0,
				       BPF_PROG_CHUNK_COUNT, 0) == 0) {
		list_del(&pack->list);
		bpf_jit_free_exec(pack->ptr);
		kfree(pack);
	}
out:
	mutex_unlock(&pack_mutex);
}

static atomic_long_t bpf_jit_current;

/* Can be overridden by an arch's JIT compiler if it has a custom,
 * dedicated BPF backend memory area, or if neither of the two
 * below apply.
 */
u64 __weak bpf_jit_alloc_exec_limit(void)
{
#if defined(MODULES_VADDR)
	return MODULES_END - MODULES_VADDR;
#else
	return VMALLOC_END - VMALLOC_START;
#endif
}

static int __init bpf_jit_charge_init(void)
{
	/* Only used as heuristic here to derive limit. */
	bpf_jit_limit_max = bpf_jit_alloc_exec_limit();
	bpf_jit_limit = min_t(u64, round_up(bpf_jit_limit_max >> 1,
					    PAGE_SIZE), LONG_MAX);
	return 0;
}
pure_initcall(bpf_jit_charge_init);

int bpf_jit_charge_modmem(u32 size)
{
	if (atomic_long_add_return(size, &bpf_jit_current) > READ_ONCE(bpf_jit_limit)) {
		if (!bpf_capable()) {
			atomic_long_sub(size, &bpf_jit_current);
			return -EPERM;
		}
	}

	return 0;
}

void bpf_jit_uncharge_modmem(u32 size)
{
	atomic_long_sub(size, &bpf_jit_current);
}

void *__weak bpf_jit_alloc_exec(unsigned long size)
{
	return execmem_alloc(EXECMEM_BPF, size);
}

void __weak bpf_jit_free_exec(void *addr)
{
	execmem_free(addr);
}

struct bpf_binary_header *
bpf_jit_binary_alloc(unsigned int proglen, u8 **image_ptr,
		     unsigned int alignment,
		     bpf_jit_fill_hole_t bpf_fill_ill_insns)
{
	struct bpf_binary_header *hdr;
	u32 size, hole, start;

	WARN_ON_ONCE(!is_power_of_2(alignment) ||
		     alignment > BPF_IMAGE_ALIGNMENT);

	/* Most of BPF filters are really small, but if some of them
	 * fill a page, allow at least 128 extra bytes to insert a
	 * random section of illegal instructions.
	 */
	size = round_up(proglen + sizeof(*hdr) + 128, PAGE_SIZE);

	if (bpf_jit_charge_modmem(size))
		return NULL;
	hdr = bpf_jit_alloc_exec(size);
	if (!hdr) {
		bpf_jit_uncharge_modmem(size);
		return NULL;
	}

	/* Fill space with illegal/arch-dep instructions. */
	bpf_fill_ill_insns(hdr, size);

	hdr->size = size;
	hole = min_t(unsigned int, size - (proglen + sizeof(*hdr)),
		     PAGE_SIZE - sizeof(*hdr));
	start = get_random_u32_below(hole) & ~(alignment - 1);

	/* Leave a random number of instructions before BPF code. */
	*image_ptr = &hdr->image[start];

	return hdr;
}

void bpf_jit_binary_free(struct bpf_binary_header *hdr)
{
	u32 size = hdr->size;

	bpf_jit_free_exec(hdr);
	bpf_jit_uncharge_modmem(size);
}

/* Allocate jit binary from bpf_prog_pack allocator.
 * Since the allocated memory is RO+X, the JIT engine cannot write directly
 * to the memory. To solve this problem, a RW buffer is also allocated at
 * as the same time. The JIT engine should calculate offsets based on the
 * RO memory address, but write JITed program to the RW buffer. Once the
 * JIT engine finishes, it calls bpf_jit_binary_pack_finalize, which copies
 * the JITed program to the RO memory.
 */
struct bpf_binary_header *
bpf_jit_binary_pack_alloc(unsigned int proglen, u8 **image_ptr,
			  unsigned int alignment,
			  struct bpf_binary_header **rw_header,
			  u8 **rw_image,
			  bpf_jit_fill_hole_t bpf_fill_ill_insns)
{
	struct bpf_binary_header *ro_header;
	u32 size, hole, start;

	WARN_ON_ONCE(!is_power_of_2(alignment) ||
		     alignment > BPF_IMAGE_ALIGNMENT);

	/* add 16 bytes for a random section of illegal instructions */
	size = round_up(proglen + sizeof(*ro_header) + 16, BPF_PROG_CHUNK_SIZE);

	if (bpf_jit_charge_modmem(size))
		return NULL;
	ro_header = bpf_prog_pack_alloc(size, bpf_fill_ill_insns);
	if (!ro_header) {
		bpf_jit_uncharge_modmem(size);
		return NULL;
	}

	*rw_header = kvmalloc(size, GFP_KERNEL);
	if (!*rw_header) {
		bpf_prog_pack_free(ro_header, size);
		bpf_jit_uncharge_modmem(size);
		return NULL;
	}

	/* Fill space with illegal/arch-dep instructions. */
	bpf_fill_ill_insns(*rw_header, size);
	(*rw_header)->size = size;

	hole = min_t(unsigned int, size - (proglen + sizeof(*ro_header)),
		     BPF_PROG_CHUNK_SIZE - sizeof(*ro_header));
	start = get_random_u32_below(hole) & ~(alignment - 1);

	*image_ptr = &ro_header->image[start];
	*rw_image = &(*rw_header)->image[start];

	return ro_header;
}