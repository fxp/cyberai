// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 17/36



unsigned long kvm_host_page_size(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	struct vm_area_struct *vma;
	unsigned long addr, size;

	size = PAGE_SIZE;

	addr = kvm_vcpu_gfn_to_hva_prot(vcpu, gfn, NULL);
	if (kvm_is_error_hva(addr))
		return PAGE_SIZE;

	mmap_read_lock(current->mm);
	vma = find_vma(current->mm, addr);
	if (!vma)
		goto out;

	size = vma_kernel_pagesize(vma);

out:
	mmap_read_unlock(current->mm);

	return size;
}

static bool memslot_is_readonly(const struct kvm_memory_slot *slot)
{
	return slot->flags & KVM_MEM_READONLY;
}

static unsigned long __gfn_to_hva_many(const struct kvm_memory_slot *slot, gfn_t gfn,
				       gfn_t *nr_pages, bool write)
{
	if (!slot || slot->flags & KVM_MEMSLOT_INVALID)
		return KVM_HVA_ERR_BAD;

	if (memslot_is_readonly(slot) && write)
		return KVM_HVA_ERR_RO_BAD;

	if (nr_pages)
		*nr_pages = slot->npages - (gfn - slot->base_gfn);

	return __gfn_to_hva_memslot(slot, gfn);
}

static unsigned long gfn_to_hva_many(struct kvm_memory_slot *slot, gfn_t gfn,
				     gfn_t *nr_pages)
{
	return __gfn_to_hva_many(slot, gfn, nr_pages, true);
}

unsigned long gfn_to_hva_memslot(struct kvm_memory_slot *slot,
					gfn_t gfn)
{
	return gfn_to_hva_many(slot, gfn, NULL);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(gfn_to_hva_memslot);

unsigned long gfn_to_hva(struct kvm *kvm, gfn_t gfn)
{
	return gfn_to_hva_many(gfn_to_memslot(kvm, gfn), gfn, NULL);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(gfn_to_hva);

unsigned long kvm_vcpu_gfn_to_hva(struct kvm_vcpu *vcpu, gfn_t gfn)
{
	return gfn_to_hva_many(kvm_vcpu_gfn_to_memslot(vcpu, gfn), gfn, NULL);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_vcpu_gfn_to_hva);

/*
 * Return the hva of a @gfn and the R/W attribute if possible.
 *
 * @slot: the kvm_memory_slot which contains @gfn
 * @gfn: the gfn to be translated
 * @writable: used to return the read/write attribute of the @slot if the hva
 * is valid and @writable is not NULL
 */
unsigned long gfn_to_hva_memslot_prot(struct kvm_memory_slot *slot,
				      gfn_t gfn, bool *writable)
{
	unsigned long hva = __gfn_to_hva_many(slot, gfn, NULL, false);

	if (!kvm_is_error_hva(hva) && writable)
		*writable = !memslot_is_readonly(slot);

	return hva;
}

unsigned long gfn_to_hva_prot(struct kvm *kvm, gfn_t gfn, bool *writable)
{
	struct kvm_memory_slot *slot = gfn_to_memslot(kvm, gfn);

	return gfn_to_hva_memslot_prot(slot, gfn, writable);
}

unsigned long kvm_vcpu_gfn_to_hva_prot(struct kvm_vcpu *vcpu, gfn_t gfn, bool *writable)
{
	struct kvm_memory_slot *slot = kvm_vcpu_gfn_to_memslot(vcpu, gfn);

	return gfn_to_hva_memslot_prot(slot, gfn, writable);
}

static bool kvm_is_ad_tracked_page(struct page *page)
{
	/*
	 * Per page-flags.h, pages tagged PG_reserved "should in general not be
	 * touched (e.g. set dirty) except by its owner".
	 */
	return !PageReserved(page);
}

static void kvm_set_page_dirty(struct page *page)
{
	if (kvm_is_ad_tracked_page(page))
		SetPageDirty(page);
}

static void kvm_set_page_accessed(struct page *page)
{
	if (kvm_is_ad_tracked_page(page))
		mark_page_accessed(page);
}

void kvm_release_page_clean(struct page *page)
{
	if (!page)
		return;

	kvm_set_page_accessed(page);
	put_page(page);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_release_page_clean);

void kvm_release_page_dirty(struct page *page)
{
	if (!page)
		return;

	kvm_set_page_dirty(page);
	kvm_release_page_clean(page);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_release_page_dirty);

static kvm_pfn_t kvm_resolve_pfn(struct kvm_follow_pfn *kfp, struct page *page,
				 struct follow_pfnmap_args *map, bool writable)
{
	kvm_pfn_t pfn;

	WARN_ON_ONCE(!!page == !!map);

	if (kfp->map_writable)
		*kfp->map_writable = writable;

	if (map)
		pfn = map->pfn;
	else
		pfn = page_to_pfn(page);

	*kfp->refcounted_page = page;

	return pfn;
}

/*
 * The fast path to get the writable pfn which will be stored in @pfn,
 * true indicates success, otherwise false is returned.
 */
static bool hva_to_pfn_fast(struct kvm_follow_pfn *kfp, kvm_pfn_t *pfn)
{
	struct page *page;
	bool r;

	/*
	 * Try the fast-only path when the caller wants to pin/get the page for
	 * writing.  If the caller only wants to read the page, KVM must go
	 * down the full, slow path in order to avoid racing an operation that
	 * breaks Copy-on-Write (CoW), e.g. so that KVM doesn't end up pointing
	 * at the old, read-only page while mm/ points at a new, writable page.
	 */
	if (!((kfp->flags & FOLL_WRITE) || kfp->map_writable))
		return false;

	if (kfp->pin)
		r = pin_user_pages_fast(kfp->hva, 1, FOLL_WRITE, &page) == 1;
	else
		r = get_user_page_fast_only(kfp->hva, FOLL_WRITE, &page);

	if (r) {
		*pfn = kvm_resolve_pfn(kfp, page, NULL, true);
		return true;
	}

	return false;
}