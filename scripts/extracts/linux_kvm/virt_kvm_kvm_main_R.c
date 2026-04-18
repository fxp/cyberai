// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/virt/kvm/kvm_main.c
// Segment 18/36



/*
 * The slow path to get the pfn of the specified host virtual address,
 * 1 indicates success, -errno is returned if error is detected.
 */
static int hva_to_pfn_slow(struct kvm_follow_pfn *kfp, kvm_pfn_t *pfn)
{
	/*
	 * When a VCPU accesses a page that is not mapped into the secondary
	 * MMU, we lookup the page using GUP to map it, so the guest VCPU can
	 * make progress. We always want to honor NUMA hinting faults in that
	 * case, because GUP usage corresponds to memory accesses from the VCPU.
	 * Otherwise, we'd not trigger NUMA hinting faults once a page is
	 * mapped into the secondary MMU and gets accessed by a VCPU.
	 *
	 * Note that get_user_page_fast_only() and FOLL_WRITE for now
	 * implicitly honor NUMA hinting faults and don't need this flag.
	 */
	unsigned int flags = FOLL_HWPOISON | FOLL_HONOR_NUMA_FAULT | kfp->flags;
	struct page *page, *wpage;
	int npages;

	if (kfp->pin)
		npages = pin_user_pages_unlocked(kfp->hva, 1, &page, flags);
	else
		npages = get_user_pages_unlocked(kfp->hva, 1, &page, flags);
	if (npages != 1)
		return npages;

	/*
	 * Pinning is mutually exclusive with opportunistically mapping a read
	 * fault as writable, as KVM should never pin pages when mapping memory
	 * into the guest (pinning is only for direct accesses from KVM).
	 */
	if (WARN_ON_ONCE(kfp->map_writable && kfp->pin))
		goto out;

	/* map read fault as writable if possible */
	if (!(flags & FOLL_WRITE) && kfp->map_writable &&
	    get_user_page_fast_only(kfp->hva, FOLL_WRITE, &wpage)) {
		put_page(page);
		page = wpage;
		flags |= FOLL_WRITE;
	}

out:
	*pfn = kvm_resolve_pfn(kfp, page, NULL, flags & FOLL_WRITE);
	return npages;
}

static bool vma_is_valid(struct vm_area_struct *vma, bool write_fault)
{
	if (unlikely(!(vma->vm_flags & VM_READ)))
		return false;

	if (write_fault && (unlikely(!(vma->vm_flags & VM_WRITE))))
		return false;

	return true;
}

static int hva_to_pfn_remapped(struct vm_area_struct *vma,
			       struct kvm_follow_pfn *kfp, kvm_pfn_t *p_pfn)
{
	struct follow_pfnmap_args args = { .vma = vma, .address = kfp->hva };
	bool write_fault = kfp->flags & FOLL_WRITE;
	int r;

	/*
	 * Remapped memory cannot be pinned in any meaningful sense.  Bail if
	 * the caller wants to pin the page, i.e. access the page outside of
	 * MMU notifier protection, and unsafe umappings are disallowed.
	 */
	if (kfp->pin && !allow_unsafe_mappings)
		return -EINVAL;

	r = follow_pfnmap_start(&args);
	if (r) {
		/*
		 * get_user_pages fails for VM_IO and VM_PFNMAP vmas and does
		 * not call the fault handler, so do it here.
		 */
		bool unlocked = false;
		r = fixup_user_fault(current->mm, kfp->hva,
				     (write_fault ? FAULT_FLAG_WRITE : 0),
				     &unlocked);
		if (unlocked)
			return -EAGAIN;
		if (r)
			return r;

		r = follow_pfnmap_start(&args);
		if (r)
			return r;
	}

	if (write_fault && !args.writable) {
		*p_pfn = KVM_PFN_ERR_RO_FAULT;
		goto out;
	}

	*p_pfn = kvm_resolve_pfn(kfp, NULL, &args, args.writable);
out:
	follow_pfnmap_end(&args);
	return r;
}

kvm_pfn_t hva_to_pfn(struct kvm_follow_pfn *kfp)
{
	struct vm_area_struct *vma;
	kvm_pfn_t pfn;
	int npages, r;

	might_sleep();

	if (WARN_ON_ONCE(!kfp->refcounted_page))
		return KVM_PFN_ERR_FAULT;

	if (hva_to_pfn_fast(kfp, &pfn))
		return pfn;

	npages = hva_to_pfn_slow(kfp, &pfn);
	if (npages == 1)
		return pfn;
	if (npages == -EINTR || npages == -EAGAIN)
		return KVM_PFN_ERR_SIGPENDING;
	if (npages == -EHWPOISON)
		return KVM_PFN_ERR_HWPOISON;

	mmap_read_lock(current->mm);
retry:
	vma = vma_lookup(current->mm, kfp->hva);

	if (vma == NULL)
		pfn = KVM_PFN_ERR_FAULT;
	else if (vma->vm_flags & (VM_IO | VM_PFNMAP)) {
		r = hva_to_pfn_remapped(vma, kfp, &pfn);
		if (r == -EAGAIN)
			goto retry;
		if (r < 0)
			pfn = KVM_PFN_ERR_FAULT;
	} else {
		if ((kfp->flags & FOLL_NOWAIT) &&
		    vma_is_valid(vma, kfp->flags & FOLL_WRITE))
			pfn = KVM_PFN_ERR_NEEDS_IO;
		else
			pfn = KVM_PFN_ERR_FAULT;
	}
	mmap_read_unlock(current->mm);
	return pfn;
}

static kvm_pfn_t kvm_follow_pfn(struct kvm_follow_pfn *kfp)
{
	kfp->hva = __gfn_to_hva_many(kfp->slot, kfp->gfn, NULL,
				     kfp->flags & FOLL_WRITE);

	if (kfp->hva == KVM_HVA_ERR_RO_BAD)
		return KVM_PFN_ERR_RO_FAULT;

	if (kvm_is_error_hva(kfp->hva))
		return KVM_PFN_NOSLOT;

	if (memslot_is_readonly(kfp->slot) && kfp->map_writable) {
		*kfp->map_writable = false;
		kfp->map_writable = NULL;
	}

	return hva_to_pfn(kfp);
}

kvm_pfn_t __kvm_faultin_pfn(const struct kvm_memory_slot *slot, gfn_t gfn,
			    unsigned int foll, bool *writable,
			    struct page **refcounted_page)
{
	struct kvm_follow_pfn kfp = {
		.slot = slot,
		.gfn = gfn,
		.flags = foll,
		.map_writable = writable,
		.refcounted_page = refcounted_page,
	};

	if (WARN_ON_ONCE(!writable || !refcounted_page))
		return KVM_PFN_ERR_FAULT;

	*writable = false;
	*refcounted_page = NULL;

	return kvm_follow_pfn(&kfp);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_faultin_pfn);