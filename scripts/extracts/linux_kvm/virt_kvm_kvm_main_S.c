// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 19/36



int kvm_prefetch_pages(struct kvm_memory_slot *slot, gfn_t gfn,
		       struct page **pages, int nr_pages)
{
	unsigned long addr;
	gfn_t entry = 0;

	addr = gfn_to_hva_many(slot, gfn, &entry);
	if (kvm_is_error_hva(addr))
		return -1;

	if (entry < nr_pages)
		return 0;

	return get_user_pages_fast_only(addr, nr_pages, FOLL_WRITE, pages);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_prefetch_pages);

/*
 * Don't use this API unless you are absolutely, positively certain that KVM
 * needs to get a struct page, e.g. to pin the page for firmware DMA.
 *
 * FIXME: Users of this API likely need to FOLL_PIN the page, not just elevate
 *	  its refcount.
 */
struct page *__gfn_to_page(struct kvm *kvm, gfn_t gfn, bool write)
{
	struct page *refcounted_page = NULL;
	struct kvm_follow_pfn kfp = {
		.slot = gfn_to_memslot(kvm, gfn),
		.gfn = gfn,
		.flags = write ? FOLL_WRITE : 0,
		.refcounted_page = &refcounted_page,
	};

	(void)kvm_follow_pfn(&kfp);
	return refcounted_page;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__gfn_to_page);

int __kvm_vcpu_map(struct kvm_vcpu *vcpu, gfn_t gfn, struct kvm_host_map *map,
		   bool writable)
{
	struct kvm_follow_pfn kfp = {
		.slot = kvm_vcpu_gfn_to_memslot(vcpu, gfn),
		.gfn = gfn,
		.flags = writable ? FOLL_WRITE : 0,
		.refcounted_page = &map->pinned_page,
		.pin = true,
	};

	map->pinned_page = NULL;
	map->page = NULL;
	map->hva = NULL;
	map->gfn = gfn;
	map->writable = writable;

	map->pfn = kvm_follow_pfn(&kfp);
	if (is_error_noslot_pfn(map->pfn))
		return -EINVAL;

	if (pfn_valid(map->pfn)) {
		map->page = pfn_to_page(map->pfn);
		map->hva = kmap(map->page);
#ifdef CONFIG_HAS_IOMEM
	} else {
		map->hva = memremap(pfn_to_hpa(map->pfn), PAGE_SIZE, MEMREMAP_WB);
#endif
	}

	return map->hva ? 0 : -EFAULT;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_vcpu_map);

void kvm_vcpu_unmap(struct kvm_vcpu *vcpu, struct kvm_host_map *map)
{
	if (!map->hva)
		return;

	if (map->page)
		kunmap(map->page);
#ifdef CONFIG_HAS_IOMEM
	else
		memunmap(map->hva);
#endif

	if (map->writable)
		kvm_vcpu_mark_page_dirty(vcpu, map->gfn);

	if (map->pinned_page) {
		if (map->writable)
			kvm_set_page_dirty(map->pinned_page);
		kvm_set_page_accessed(map->pinned_page);
		unpin_user_page(map->pinned_page);
	}

	map->hva = NULL;
	map->page = NULL;
	map->pinned_page = NULL;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_unmap);

static int next_segment(unsigned long len, int offset)
{
	if (len > PAGE_SIZE - offset)
		return PAGE_SIZE - offset;
	else
		return len;
}

/* Copy @len bytes from guest memory at '(@gfn * PAGE_SIZE) + @offset' to @data */
static int __kvm_read_guest_page(struct kvm_memory_slot *slot, gfn_t gfn,
				 void *data, int offset, int len)
{
	int r;
	unsigned long addr;

	if (WARN_ON_ONCE(offset + len > PAGE_SIZE))
		return -EFAULT;

	addr = gfn_to_hva_memslot_prot(slot, gfn, NULL);
	if (kvm_is_error_hva(addr))
		return -EFAULT;
	r = __copy_from_user(data, (void __user *)addr + offset, len);
	if (r)
		return -EFAULT;
	return 0;
}

int kvm_read_guest_page(struct kvm *kvm, gfn_t gfn, void *data, int offset,
			int len)
{
	struct kvm_memory_slot *slot = gfn_to_memslot(kvm, gfn);

	return __kvm_read_guest_page(slot, gfn, data, offset, len);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_read_guest_page);

int kvm_vcpu_read_guest_page(struct kvm_vcpu *vcpu, gfn_t gfn, void *data,
			     int offset, int len)
{
	struct kvm_memory_slot *slot = kvm_vcpu_gfn_to_memslot(vcpu, gfn);

	return __kvm_read_guest_page(slot, gfn, data, offset, len);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_read_guest_page);

int kvm_read_guest(struct kvm *kvm, gpa_t gpa, void *data, unsigned long len)
{
	gfn_t gfn = gpa >> PAGE_SHIFT;
	int seg;
	int offset = offset_in_page(gpa);
	int ret;

	while ((seg = next_segment(len, offset)) != 0) {
		ret = kvm_read_guest_page(kvm, gfn, data, offset, seg);
		if (ret < 0)
			return ret;
		offset = 0;
		len -= seg;
		data += seg;
		++gfn;
	}
	return 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_read_guest);

int kvm_vcpu_read_guest(struct kvm_vcpu *vcpu, gpa_t gpa, void *data, unsigned long len)
{
	gfn_t gfn = gpa >> PAGE_SHIFT;
	int seg;
	int offset = offset_in_page(gpa);
	int ret;

	while ((seg = next_segment(len, offset)) != 0) {
		ret = kvm_vcpu_read_guest_page(vcpu, gfn, data, offset, seg);
		if (ret < 0)
			return ret;
		offset = 0;
		len -= seg;
		data += seg;
		++gfn;
	}
	return 0;
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_read_guest);

static int __kvm_read_guest_atomic(struct kvm_memory_slot *slot, gfn_t gfn,
			           void *data, int offset, unsigned long len)
{
	int r;
	unsigned long addr;

	if (WARN_ON_ONCE(offset + len > PAGE_SIZE))
		return -EFAULT;

	addr = gfn_to_hva_memslot_prot(slot, gfn, NULL);
	if (kvm_is_error_hva(addr))
		return -EFAULT;
	pagefault_disable();
	r = __copy_from_user_inatomic(data, (void __user *)addr + offset, len);
	pagefault_enable();
	if (r)
		return -EFAULT;
	return 0;
}