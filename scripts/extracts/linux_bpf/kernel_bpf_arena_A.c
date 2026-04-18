// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/arena.c
// Segment 1/6

// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2024 Meta Platforms, Inc. and affiliates. */
#include <linux/bpf.h>
#include <linux/btf.h>
#include <linux/cacheflush.h>
#include <linux/err.h>
#include <linux/irq_work.h>
#include "linux/filter.h"
#include <linux/llist.h>
#include <linux/btf_ids.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <asm/tlbflush.h>
#include "range_tree.h"

/*
 * bpf_arena is a sparsely populated shared memory region between bpf program and
 * user space process.
 *
 * For example on x86-64 the values could be:
 * user_vm_start 7f7d26200000     // picked by mmap()
 * kern_vm_start ffffc90001e69000 // picked by get_vm_area()
 * For user space all pointers within the arena are normal 8-byte addresses.
 * In this example 7f7d26200000 is the address of the first page (pgoff=0).
 * The bpf program will access it as: kern_vm_start + lower_32bit_of_user_ptr
 * (u32)7f7d26200000 -> 26200000
 * hence
 * ffffc90001e69000 + 26200000 == ffffc90028069000 is "pgoff=0" within 4Gb
 * kernel memory region.
 *
 * BPF JITs generate the following code to access arena:
 *   mov eax, eax  // eax has lower 32-bit of user pointer
 *   mov word ptr [rax + r12 + off], bx
 * where r12 == kern_vm_start and off is s16.
 * Hence allocate 4Gb + GUARD_SZ/2 on each side.
 *
 * Initially kernel vm_area and user vma are not populated.
 * User space can fault-in any address which will insert the page
 * into kernel and user vma.
 * bpf program can allocate a page via bpf_arena_alloc_pages() kfunc
 * which will insert it into kernel vm_area.
 * The later fault-in from user space will populate that page into user vma.
 */

/* number of bytes addressable by LDX/STX insn with 16-bit 'off' field */
#define GUARD_SZ round_up(1ull << sizeof_field(struct bpf_insn, off) * 8, PAGE_SIZE << 1)
#define KERN_VM_SZ (SZ_4G + GUARD_SZ)

static void arena_free_pages(struct bpf_arena *arena, long uaddr, long page_cnt, bool sleepable);

struct bpf_arena {
	struct bpf_map map;
	u64 user_vm_start;
	u64 user_vm_end;
	struct vm_struct *kern_vm;
	struct range_tree rt;
	/* protects rt */
	rqspinlock_t spinlock;
	struct list_head vma_list;
	/* protects vma_list */
	struct mutex lock;
	struct irq_work     free_irq;
	struct work_struct  free_work;
	struct llist_head   free_spans;
};

static void arena_free_worker(struct work_struct *work);
static void arena_free_irq(struct irq_work *iw);

struct arena_free_span {
	struct llist_node node;
	unsigned long uaddr;
	u32 page_cnt;
};

u64 bpf_arena_get_kern_vm_start(struct bpf_arena *arena)
{
	return arena ? (u64) (long) arena->kern_vm->addr + GUARD_SZ / 2 : 0;
}

u64 bpf_arena_get_user_vm_start(struct bpf_arena *arena)
{
	return arena ? arena->user_vm_start : 0;
}

static long arena_map_peek_elem(struct bpf_map *map, void *value)
{
	return -EOPNOTSUPP;
}

static long arena_map_push_elem(struct bpf_map *map, void *value, u64 flags)
{
	return -EOPNOTSUPP;
}

static long arena_map_pop_elem(struct bpf_map *map, void *value)
{
	return -EOPNOTSUPP;
}

static long arena_map_delete_elem(struct bpf_map *map, void *value)
{
	return -EOPNOTSUPP;
}

static int arena_map_get_next_key(struct bpf_map *map, void *key, void *next_key)
{
	return -EOPNOTSUPP;
}

static long compute_pgoff(struct bpf_arena *arena, long uaddr)
{
	return (u32)(uaddr - (u32)arena->user_vm_start) >> PAGE_SHIFT;
}

struct apply_range_data {
	struct page **pages;
	int i;
};

static int apply_range_set_cb(pte_t *pte, unsigned long addr, void *data)
{
	struct apply_range_data *d = data;
	struct page *page;

	if (!data)
		return 0;
	/* sanity check */
	if (unlikely(!pte_none(ptep_get(pte))))
		return -EBUSY;

	page = d->pages[d->i];
	/* paranoia, similar to vmap_pages_pte_range() */
	if (WARN_ON_ONCE(!pfn_valid(page_to_pfn(page))))
		return -EINVAL;

	set_pte_at(&init_mm, addr, pte, mk_pte(page, PAGE_KERNEL));
	d->i++;
	return 0;
}

static void flush_vmap_cache(unsigned long start, unsigned long size)
{
	flush_cache_vmap(start, start + size);
}

static int apply_range_clear_cb(pte_t *pte, unsigned long addr, void *free_pages)
{
	pte_t old_pte;
	struct page *page;

	/* sanity check */
	old_pte = ptep_get(pte);
	if (pte_none(old_pte) || !pte_present(old_pte))
		return 0; /* nothing to do */

	page = pte_page(old_pte);
	if (WARN_ON_ONCE(!page))
		return -EINVAL;

	pte_clear(&init_mm, addr, pte);

	/* Add page to the list so it is freed later */
	if (free_pages)
		__llist_add(&page->pcp_llist, free_pages);

	return 0;
}

static int populate_pgtable_except_pte(struct bpf_arena *arena)
{
	return apply_to_page_range(&init_mm, bpf_arena_get_kern_vm_start(arena),
				   KERN_VM_SZ - GUARD_SZ, apply_range_set_cb, NULL);
}

static struct bpf_map *arena_map_alloc(union bpf_attr *attr)
{
	struct vm_struct *kern_vm;
	int numa_node = bpf_map_attr_numa_node(attr);
	struct bpf_arena *arena;
	u64 vm_range;
	int err = -ENOMEM;

	if (!bpf_jit_supports_arena())
		return ERR_PTR(-EOPNOTSUPP);