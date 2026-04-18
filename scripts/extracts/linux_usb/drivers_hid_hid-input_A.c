// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 1/16

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006-2010 Jiri Kosina
 *
 *  HID to Linux Input mapping
 */

/*
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/hid.h>
#include <linux/hid-debug.h>

#include "hid-ids.h"

#define unk	KEY_UNKNOWN

static const unsigned char hid_keyboard[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
	191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,unk,unk,unk,121,unk, 89, 93,124, 92, 94, 95,unk,unk,unk,
	122,123, 90, 91, 85,unk,unk,unk,unk,unk,unk,unk,111,unk,unk,unk,
	unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,
	unk,unk,unk,unk,unk,unk,179,180,unk,unk,unk,unk,unk,unk,unk,unk,
	unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,unk,
	unk,unk,unk,unk,unk,unk,unk,unk,111,unk,unk,unk,unk,unk,unk,unk,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140,unk,unk,unk,unk
};

static const struct {
	__s32 x;
	__s32 y;
}  hid_hat_to_axis[] = {{ 0, 0}, { 0,-1}, { 1,-1}, { 1, 0}, { 1, 1}, { 0, 1}, {-1, 1}, {-1, 0}, {-1,-1}};

struct usage_priority {
	__u32 usage;			/* the HID usage associated */
	bool global;			/* we assume all usages to be slotted,
					 * unless global
					 */
	unsigned int slot_overwrite;	/* for globals: allows to set the usage
					 * before or after the slots
					 */
};

/*
 * hid-input will convert this list into priorities:
 * the first element will have the highest priority
 * (the length of the following array) and the last
 * element the lowest (1).
 *
 * hid-input will then shift the priority by 8 bits to leave some space
 * in case drivers want to interleave other fields.
 *
 * To accommodate slotted devices, the slot priority is
 * defined in the next 8 bits (defined by 0xff - slot).
 *
 * If drivers want to add fields before those, hid-input will
 * leave out the first 8 bits of the priority value.
 *
 * This still leaves us 65535 individual priority values.
 */
static const struct usage_priority hidinput_usages_priorities[] = {
	{ /* Eraser (eraser touching) must always come before tipswitch */
	  .usage = HID_DG_ERASER,
	},
	{ /* Invert must always come before In Range */
	  .usage = HID_DG_INVERT,
	},
	{ /* Is the tip of the tool touching? */
	  .usage = HID_DG_TIPSWITCH,
	},
	{ /* Tip Pressure might emulate tip switch */
	  .usage = HID_DG_TIPPRESSURE,
	},
	{ /* In Range needs to come after the other tool states */
	  .usage = HID_DG_INRANGE,
	},
};

#define map_abs(c)	hid_map_usage(hidinput, usage, &bit, &max, EV_ABS, (c))
#define map_rel(c)	hid_map_usage(hidinput, usage, &bit, &max, EV_REL, (c))
#define map_key(c)	hid_map_usage(hidinput, usage, &bit, &max, EV_KEY, (c))
#define map_led(c)	hid_map_usage(hidinput, usage, &bit, &max, EV_LED, (c))
#define map_msc(c)	hid_map_usage(hidinput, usage, &bit, &max, EV_MSC, (c))

#define map_abs_clear(c)	hid_map_usage_clear(hidinput, usage, &bit, \
		&max, EV_ABS, (c))
#define map_key_clear(c)	hid_map_usage_clear(hidinput, usage, &bit, \
		&max, EV_KEY, (c))

static bool match_scancode(struct hid_usage *usage,
			   unsigned int cur_idx, unsigned int scancode)
{
	return (usage->hid & (HID_USAGE_PAGE | HID_USAGE)) == scancode;
}

static bool match_keycode(struct hid_usage *usage,
			  unsigned int cur_idx, unsigned int keycode)
{
	/*
	 * We should exclude unmapped usages when doing lookup by keycode.
	 */
	return (usage->type == EV_KEY && usage->code == keycode);
}

static bool match_index(struct hid_usage *usage,
			unsigned int cur_idx, unsigned int idx)
{
	return cur_idx == idx;
}

typedef bool (*hid_usage_cmp_t)(struct hid_usage *usage,
				unsigned int cur_idx, unsigned int val);

static struct hid_usage *hidinput_find_key(struct hid_device *hid,
					   hid_usage_cmp_t match,
					   unsigned int value,
					   unsigned int *usage_idx)
{
	unsigned int i, j, k, cur_idx = 0;
	struct hid_report *report;
	struct hid_usage *usage;