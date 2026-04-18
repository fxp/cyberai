// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/arch/x86/kvm/hyperv.c
// Segment 1/17

// SPDX-License-Identifier: GPL-2.0-only
/*
 * KVM Microsoft Hyper-V emulation
 *
 * derived from arch/x86/kvm/x86.c
 *
 * Copyright (C) 2006 Qumranet, Inc.
 * Copyright (C) 2008 Qumranet, Inc.
 * Copyright IBM Corporation, 2008
 * Copyright 2010 Red Hat, Inc. and/or its affiliates.
 * Copyright (C) 2015 Andrey Smetanin <asmetanin@virtuozzo.com>
 *
 * Authors:
 *   Avi Kivity   <avi@qumranet.com>
 *   Yaniv Kamay  <yaniv@qumranet.com>
 *   Amit Shah    <amit.shah@qumranet.com>
 *   Ben-Ami Yassour <benami@il.ibm.com>
 *   Andrey Smetanin <asmetanin@virtuozzo.com>
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "x86.h"
#include "lapic.h"
#include "ioapic.h"
#include "cpuid.h"
#include "hyperv.h"
#include "mmu.h"
#include "xen.h"

#include <linux/cpu.h>
#include <linux/kvm_host.h>
#include <linux/highmem.h>
#include <linux/sched/cputime.h>
#include <linux/spinlock.h>
#include <linux/eventfd.h>

#include <asm/apicdef.h>
#include <asm/mshyperv.h>
#include <trace/events/kvm.h>

#include "trace.h"
#include "irq.h"
#include "fpu.h"

#define KVM_HV_MAX_SPARSE_VCPU_SET_BITS DIV_ROUND_UP(KVM_MAX_VCPUS, HV_VCPUS_PER_SPARSE_BANK)

/*
 * As per Hyper-V TLFS, extended hypercalls start from 0x8001
 * (HvExtCallQueryCapabilities). Response of this hypercalls is a 64 bit value
 * where each bit tells which extended hypercall is available besides
 * HvExtCallQueryCapabilities.
 *
 * 0x8001 - First extended hypercall, HvExtCallQueryCapabilities, no bit
 * assigned.
 *
 * 0x8002 - Bit 0
 * 0x8003 - Bit 1
 * ..
 * 0x8041 - Bit 63
 *
 * Therefore, HV_EXT_CALL_MAX = 0x8001 + 64
 */
#define HV_EXT_CALL_MAX (HV_EXT_CALL_QUERY_CAPABILITIES + 64)

static void stimer_mark_pending(struct kvm_vcpu_hv_stimer *stimer,
				bool vcpu_kick);

static inline u64 synic_read_sint(struct kvm_vcpu_hv_synic *synic, int sint)
{
	return atomic64_read(&synic->sint[sint]);
}

static inline int synic_get_sint_vector(u64 sint_value)
{
	if (sint_value & HV_SYNIC_SINT_MASKED)
		return -1;
	return sint_value & HV_SYNIC_SINT_VECTOR_MASK;
}

static bool synic_has_vector_connected(struct kvm_vcpu_hv_synic *synic,
				      int vector)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(synic->sint); i++) {
		if (synic_get_sint_vector(synic_read_sint(synic, i)) == vector)
			return true;
	}
	return false;
}

static bool synic_has_vector_auto_eoi(struct kvm_vcpu_hv_synic *synic,
				     int vector)
{
	int i;
	u64 sint_value;

	for (i = 0; i < ARRAY_SIZE(synic->sint); i++) {
		sint_value = synic_read_sint(synic, i);
		if (synic_get_sint_vector(sint_value) == vector &&
		    sint_value & HV_SYNIC_SINT_AUTO_EOI)
			return true;
	}
	return false;
}

static void synic_update_vector(struct kvm_vcpu_hv_synic *synic,
				int vector)
{
	struct kvm_vcpu *vcpu = hv_synic_to_vcpu(synic);
	struct kvm_hv *hv = to_kvm_hv(vcpu->kvm);
	bool auto_eoi_old, auto_eoi_new;

	if (vector < HV_SYNIC_FIRST_VALID_VECTOR)
		return;

	if (synic_has_vector_connected(synic, vector))
		__set_bit(vector, synic->vec_bitmap);
	else
		__clear_bit(vector, synic->vec_bitmap);

	auto_eoi_old = !bitmap_empty(synic->auto_eoi_bitmap, 256);

	if (synic_has_vector_auto_eoi(synic, vector))
		__set_bit(vector, synic->auto_eoi_bitmap);
	else
		__clear_bit(vector, synic->auto_eoi_bitmap);

	auto_eoi_new = !bitmap_empty(synic->auto_eoi_bitmap, 256);

	if (auto_eoi_old == auto_eoi_new)
		return;

	if (!enable_apicv)
		return;

	down_write(&vcpu->kvm->arch.apicv_update_lock);

	if (auto_eoi_new)
		hv->synic_auto_eoi_used++;
	else
		hv->synic_auto_eoi_used--;

	/*
	 * Inhibit APICv if any vCPU is using SynIC's AutoEOI, which relies on
	 * the hypervisor to manually inject IRQs.
	 */
	__kvm_set_or_clear_apicv_inhibit(vcpu->kvm,
					 APICV_INHIBIT_REASON_HYPERV,
					 !!hv->synic_auto_eoi_used);

	up_write(&vcpu->kvm->arch.apicv_update_lock);
}

static int synic_set_sint(struct kvm_vcpu_hv_synic *synic, int sint,
			  u64 data, bool host)
{
	int vector, old_vector;
	bool masked;

	vector = data & HV_SYNIC_SINT_VECTOR_MASK;
	masked = data & HV_SYNIC_SINT_MASKED;

	/*
	 * Valid vectors are 16-255, however, nested Hyper-V attempts to write
	 * default '0x10000' value on boot and this should not #GP. We need to
	 * allow zero-initing the register from host as well.
	 */
	if (vector < HV_SYNIC_FIRST_VALID_VECTOR && !host && !masked)
		return 1;
	/*
	 * Guest may configure multiple SINTs to use the same vector, so
	 * we maintain a bitmap of vectors handled by synic, and a
	 * bitmap of vectors with auto-eoi behavior.  The bitmaps are
	 * updated here, and atomically queried on fast paths.
	 */
	old_vector = synic_read_sint(synic, sint) & HV_SYNIC_SINT_VECTOR_MASK;

	atomic64_set(&synic->sint[sint], data);

	synic_update_vector(synic, old_vector);

	synic_update_vector(synic, vector);

	/* Load SynIC vectors into EOI exit bitmap */
	kvm_make_request(KVM_REQ_SCAN_IOAPIC, hv_synic_to_vcpu(synic));
	return 0;
}