// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 1/17

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  HID support for Linux
 *
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2005 Vojtech Pavlik <vojtech@suse.cz>
 *  Copyright (c) 2005 Michael Haboustak <mike-@cinci.rr.com> for Concept2, Inc
 *  Copyright (c) 2006-2012 Jiri Kosina
 */

/*
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/unaligned.h>
#include <asm/byteorder.h>
#include <linux/input.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/semaphore.h>

#include <linux/hid.h>
#include <linux/hiddev.h>
#include <linux/hid-debug.h>
#include <linux/hidraw.h>

#include "hid-ids.h"

/*
 * Version Information
 */

#define DRIVER_DESC "HID core driver"

static int hid_ignore_special_drivers = 0;
module_param_named(ignore_special_drivers, hid_ignore_special_drivers, int, 0600);
MODULE_PARM_DESC(ignore_special_drivers, "Ignore any special drivers and handle all devices by generic driver");

/*
 * Convert a signed n-bit integer to signed 32-bit integer.
 */

static s32 snto32(__u32 value, unsigned int n)
{
	if (!value || !n)
		return 0;

	if (n > 32)
		n = 32;

	return sign_extend32(value, n - 1);
}

/*
 * Convert a signed 32-bit integer to a signed n-bit integer.
 */

static u32 s32ton(__s32 value, unsigned int n)
{
	s32 a;

	if (!value || !n)
		return 0;

	if (n > 32)
		n = 32;

	a = value >> (n - 1);
	if (a && a != -1)
		return value < 0 ? 1 << (n - 1) : (1 << (n - 1)) - 1;
	return value & ((1 << n) - 1);
}

/*
 * Register a new report for a device.
 */

struct hid_report *hid_register_report(struct hid_device *device,
				       enum hid_report_type type, unsigned int id,
				       unsigned int application)
{
	struct hid_report_enum *report_enum = device->report_enum + type;
	struct hid_report *report;

	if (id >= HID_MAX_IDS)
		return NULL;
	if (report_enum->report_id_hash[id])
		return report_enum->report_id_hash[id];

	report = kzalloc_obj(struct hid_report);
	if (!report)
		return NULL;

	if (id != 0)
		report_enum->numbered = 1;

	report->id = id;
	report->type = type;
	report->size = 0;
	report->device = device;
	report->application = application;
	report_enum->report_id_hash[id] = report;

	list_add_tail(&report->list, &report_enum->report_list);
	INIT_LIST_HEAD(&report->field_entry_list);

	return report;
}
EXPORT_SYMBOL_GPL(hid_register_report);

/*
 * Register a new field for this report.
 */

static struct hid_field *hid_register_field(struct hid_report *report, unsigned usages)
{
	struct hid_field *field;

	if (report->maxfield == HID_MAX_FIELDS) {
		hid_err(report->device, "too many fields in report\n");
		return NULL;
	}

	field = kvzalloc((sizeof(struct hid_field) +
			  usages * sizeof(struct hid_usage) +
			  3 * usages * sizeof(unsigned int)), GFP_KERNEL);
	if (!field)
		return NULL;

	field->index = report->maxfield++;
	report->field[field->index] = field;
	field->usage = (struct hid_usage *)(field + 1);
	field->value = (s32 *)(field->usage + usages);
	field->new_value = (s32 *)(field->value + usages);
	field->usages_priorities = (s32 *)(field->new_value + usages);
	field->report = report;

	return field;
}

/*
 * Open a collection. The type/usage is pushed on the stack.
 */

static int open_collection(struct hid_parser *parser, unsigned type)
{
	struct hid_collection *collection;
	unsigned usage;
	int collection_index;

	usage = parser->local.usage[0];

	if (parser->collection_stack_ptr == parser->collection_stack_size) {
		unsigned int *collection_stack;
		unsigned int new_size = parser->collection_stack_size +
					HID_COLLECTION_STACK_SIZE;

		collection_stack = krealloc(parser->collection_stack,
					    new_size * sizeof(unsigned int),
					    GFP_KERNEL);
		if (!collection_stack)
			return -ENOMEM;

		parser->collection_stack = collection_stack;
		parser->collection_stack_size = new_size;
	}

	if (parser->device->maxcollection == parser->device->collection_size) {
		collection = kmalloc(
				array3_size(sizeof(struct hid_collection),
					    parser->device->collection_size,
					    2),
				GFP_KERNEL);
		if (collection == NULL) {
			hid_err(parser->device, "failed to reallocate collection array\n");
			return -ENOMEM;
		}
		memcpy(collection, parser->device->collection,
			sizeof(struct hid_collection) *
			parser->device->collection_size);
		memset(collection + parser->device->collection_size, 0,
			sizeof(struct hid_collection) *
			parser->device->collection_size);
		kfree(parser->device->collection);
		parser->device->collection = collection;
		parser->device->collection_size *= 2;
	}

	parser->collection_stack[parser->collection_stack_ptr++] =
		parser->device->maxcollection;