// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/kernel/bpf/core.c
// Segment 1/20

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Linux Socket Filter - Kernel level socket filtering
 *
 * Based on the design of the Berkeley Packet Filter. The new
 * internal format has been designed by PLUMgrid:
 *
 *	Copyright (c) 2011 - 2014 PLUMgrid, http://plumgrid.com
 *
 * Authors:
 *
 *	Jay Schulist <jschlst@samba.org>
 *	Alexei Starovoitov <ast@plumgrid.com>
 *	Daniel Borkmann <dborkman@redhat.com>
 *
 * Andi Kleen - Fix a few bad bugs and races.
 * Kris Katterjohn - Added many additional checks in bpf_check_classic()
 */

#include <uapi/linux/btf.h>
#include <linux/filter.h>
#include <linux/skbuff.h>
#include <linux/vmalloc.h>
#include <linux/prandom.h>
#include <linux/bpf.h>
#include <linux/btf.h>
#include <linux/hex.h>
#include <linux/objtool.h>
#include <linux/overflow.h>
#include <linux/rbtree_latch.h>
#include <linux/kallsyms.h>
#include <linux/rcupdate.h>
#include <linux/perf_event.h>
#include <linux/extable.h>
#include <linux/log2.h>
#include <linux/bpf_verifier.h>
#include <linux/nodemask.h>
#include <linux/nospec.h>
#include <linux/bpf_mem_alloc.h>
#include <linux/memcontrol.h>
#include <linux/execmem.h>
#include <crypto/sha2.h>

#include <asm/barrier.h>
#include <linux/unaligned.h>

/* Registers */
#define BPF_R0	regs[BPF_REG_0]
#define BPF_R1	regs[BPF_REG_1]
#define BPF_R2	regs[BPF_REG_2]
#define BPF_R3	regs[BPF_REG_3]
#define BPF_R4	regs[BPF_REG_4]
#define BPF_R5	regs[BPF_REG_5]
#define BPF_R6	regs[BPF_REG_6]
#define BPF_R7	regs[BPF_REG_7]
#define BPF_R8	regs[BPF_REG_8]
#define BPF_R9	regs[BPF_REG_9]
#define BPF_R10	regs[BPF_REG_10]

/* Named registers */
#define DST	regs[insn->dst_reg]
#define SRC	regs[insn->src_reg]
#define FP	regs[BPF_REG_FP]
#define AX	regs[BPF_REG_AX]
#define ARG1	regs[BPF_REG_ARG1]
#define CTX	regs[BPF_REG_CTX]
#define OFF	insn->off
#define IMM	insn->imm

struct bpf_mem_alloc bpf_global_ma;
bool bpf_global_ma_set;

/* No hurry in this branch
 *
 * Exported for the bpf jit load helper.
 */
void *bpf_internal_load_pointer_neg_helper(const struct sk_buff *skb, int k, unsigned int size)
{
	u8 *ptr = NULL;

	if (k >= SKF_NET_OFF) {
		ptr = skb_network_header(skb) + k - SKF_NET_OFF;
	} else if (k >= SKF_LL_OFF) {
		if (unlikely(!skb_mac_header_was_set(skb)))
			return NULL;
		ptr = skb_mac_header(skb) + k - SKF_LL_OFF;
	}
	if (ptr >= skb->head && ptr + size <= skb_tail_pointer(skb))
		return ptr;

	return NULL;
}

/* tell bpf programs that include vmlinux.h kernel's PAGE_SIZE */
enum page_size_enum {
	__PAGE_SIZE = PAGE_SIZE
};

struct bpf_prog *bpf_prog_alloc_no_stats(unsigned int size, gfp_t gfp_extra_flags)
{
	gfp_t gfp_flags = bpf_memcg_flags(GFP_KERNEL | __GFP_ZERO | gfp_extra_flags);
	struct bpf_prog_aux *aux;
	struct bpf_prog *fp;

	size = round_up(size, __PAGE_SIZE);
	fp = __vmalloc(size, gfp_flags);
	if (fp == NULL)
		return NULL;

	aux = kzalloc_obj(*aux, bpf_memcg_flags(GFP_KERNEL | gfp_extra_flags));
	if (aux == NULL) {
		vfree(fp);
		return NULL;
	}
	fp->active = __alloc_percpu_gfp(sizeof(u8[BPF_NR_CONTEXTS]), 4,
					bpf_memcg_flags(GFP_KERNEL | gfp_extra_flags));
	if (!fp->active) {
		vfree(fp);
		kfree(aux);
		return NULL;
	}

	fp->pages = size / PAGE_SIZE;
	fp->aux = aux;
	fp->aux->main_prog_aux = aux;
	fp->aux->prog = fp;
	fp->jit_requested = ebpf_jit_enabled();
	fp->blinding_requested = bpf_jit_blinding_enabled(fp);
#ifdef CONFIG_CGROUP_BPF
	aux->cgroup_atype = CGROUP_BPF_ATTACH_TYPE_INVALID;
#endif

	INIT_LIST_HEAD_RCU(&fp->aux->ksym.lnode);
#ifdef CONFIG_FINEIBT
	INIT_LIST_HEAD_RCU(&fp->aux->ksym_prefix.lnode);
#endif
	mutex_init(&fp->aux->used_maps_mutex);
	mutex_init(&fp->aux->ext_mutex);
	mutex_init(&fp->aux->dst_mutex);
	mutex_init(&fp->aux->st_ops_assoc_mutex);

#ifdef CONFIG_BPF_SYSCALL
	bpf_prog_stream_init(fp);
#endif

	return fp;
}

struct bpf_prog *bpf_prog_alloc(unsigned int size, gfp_t gfp_extra_flags)
{
	gfp_t gfp_flags = bpf_memcg_flags(GFP_KERNEL | __GFP_ZERO | gfp_extra_flags);
	struct bpf_prog *prog;
	int cpu;

	prog = bpf_prog_alloc_no_stats(size, gfp_extra_flags);
	if (!prog)
		return NULL;

	prog->stats = alloc_percpu_gfp(struct bpf_prog_stats, gfp_flags);
	if (!prog->stats) {
		free_percpu(prog->active);
		kfree(prog->aux);
		vfree(prog);
		return NULL;
	}

	for_each_possible_cpu(cpu) {
		struct bpf_prog_stats *pstats;

		pstats = per_cpu_ptr(prog->stats, cpu);
		u64_stats_init(&pstats->syncp);
	}
	return prog;
}
EXPORT_SYMBOL_GPL(bpf_prog_alloc);

int bpf_prog_alloc_jited_linfo(struct bpf_prog *prog)
{
	if (!prog->aux->nr_linfo || !prog->jit_requested)
		return 0;

	prog->aux->jited_linfo = kvzalloc_objs(*prog->aux->jited_linfo,
					       prog->aux->nr_linfo,
					       bpf_memcg_flags(GFP_KERNEL | __GFP_NOWARN));
	if (!prog->aux->jited_linfo)
		return -ENOMEM;

	return 0;
}

void bpf_prog_jit_attempt_done(struct bpf_prog *prog)
{
	if (prog->aux->jited_linfo &&
	    (!prog->jited || !prog->aux->jited_linfo[0])) {
		kvfree(prog->aux->jited_linfo);
		prog->aux->jited_linfo = NULL;
	}