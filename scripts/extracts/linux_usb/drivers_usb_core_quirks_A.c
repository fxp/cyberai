// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/quirks.c
// Segment 1/5

// SPDX-License-Identifier: GPL-2.0
/*
 * USB device quirk handling logic and table
 *
 * Copyright (c) 2007 Oliver Neukum
 * Copyright (c) 2007 Greg Kroah-Hartman <gregkh@suse.de>
 */

#include <linux/moduleparam.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include <linux/usb/hcd.h>
#include "usb.h"

struct quirk_entry {
	u16 vid;
	u16 pid;
	u32 flags;
};

static DEFINE_MUTEX(quirk_mutex);

static struct quirk_entry *quirk_list;
static unsigned int quirk_count;

static char quirks_param[128];

static int quirks_param_set(const char *value, const struct kernel_param *kp)
{
	char *val, *p, *field;
	u16 vid, pid;
	u32 flags;
	size_t i;
	int err;

	val = kstrdup(value, GFP_KERNEL);
	if (!val)
		return -ENOMEM;

	err = param_set_copystring(val, kp);
	if (err) {
		kfree(val);
		return err;
	}

	mutex_lock(&quirk_mutex);

	if (!*val) {
		quirk_count = 0;
		kfree(quirk_list);
		quirk_list = NULL;
		goto unlock;
	}

	for (quirk_count = 1, i = 0; val[i]; i++)
		if (val[i] == ',')
			quirk_count++;

	if (quirk_list) {
		kfree(quirk_list);
		quirk_list = NULL;
	}

	quirk_list = kzalloc_objs(struct quirk_entry, quirk_count);
	if (!quirk_list) {
		quirk_count = 0;
		mutex_unlock(&quirk_mutex);
		kfree(val);
		return -ENOMEM;
	}

	for (i = 0, p = val; p && *p;) {
		/* Each entry consists of VID:PID:flags */
		field = strsep(&p, ":");
		if (!field)
			break;

		if (kstrtou16(field, 16, &vid))
			break;

		field = strsep(&p, ":");
		if (!field)
			break;

		if (kstrtou16(field, 16, &pid))
			break;

		field = strsep(&p, ",");
		if (!field || !*field)
			break;

		/* Collect the flags */
		for (flags = 0; *field; field++) {
			switch (*field) {
			case 'a':
				flags |= USB_QUIRK_STRING_FETCH_255;
				break;
			case 'b':
				flags |= USB_QUIRK_RESET_RESUME;
				break;
			case 'c':
				flags |= USB_QUIRK_NO_SET_INTF;
				break;
			case 'd':
				flags |= USB_QUIRK_CONFIG_INTF_STRINGS;
				break;
			case 'e':
				flags |= USB_QUIRK_RESET;
				break;
			case 'f':
				flags |= USB_QUIRK_HONOR_BNUMINTERFACES;
				break;
			case 'g':
				flags |= USB_QUIRK_DELAY_INIT;
				break;
			case 'h':
				flags |= USB_QUIRK_LINEAR_UFRAME_INTR_BINTERVAL;
				break;
			case 'i':
				flags |= USB_QUIRK_DEVICE_QUALIFIER;
				break;
			case 'j':
				flags |= USB_QUIRK_IGNORE_REMOTE_WAKEUP;
				break;
			case 'k':
				flags |= USB_QUIRK_NO_LPM;
				break;
			case 'l':
				flags |= USB_QUIRK_LINEAR_FRAME_INTR_BINTERVAL;
				break;
			case 'm':
				flags |= USB_QUIRK_DISCONNECT_SUSPEND;
				break;
			case 'n':
				flags |= USB_QUIRK_DELAY_CTRL_MSG;
				break;
			case 'o':
				flags |= USB_QUIRK_HUB_SLOW_RESET;
				break;
			case 'p':
				flags |= USB_QUIRK_SHORT_SET_ADDRESS_REQ_TIMEOUT;
				break;
			case 'q':
				flags |= USB_QUIRK_FORCE_ONE_CONFIG;
			/* Ignore unrecognized flag characters */
			}
		}

		quirk_list[i++] = (struct quirk_entry)
			{ .vid = vid, .pid = pid, .flags = flags };
	}

	if (i < quirk_count)
		quirk_count = i;

unlock:
	mutex_unlock(&quirk_mutex);
	kfree(val);

	return 0;
}

static const struct kernel_param_ops quirks_param_ops = {
	.set = quirks_param_set,
	.get = param_get_string,
};

static struct kparam_string quirks_param_string = {
	.maxlen = sizeof(quirks_param),
	.string = quirks_param,
};

device_param_cb(quirks, &quirks_param_ops, &quirks_param_string, 0644);
MODULE_PARM_DESC(quirks, "Add/modify USB quirks by specifying quirks=vendorID:productID:quirks");

/* Lists of quirky USB devices, split in device quirks and interface quirks.
 * Device quirks are applied at the very beginning of the enumeration process,
 * right after reading the device descriptor. They can thus only match on device
 * information.
 *
 * Interface quirks are applied after reading all the configuration descriptors.
 * They can match on both device and interface information.
 *
 * Note that the DELAY_INIT and HONOR_BNUMINTERFACES quirks do not make sense as
 * interface quirks, as they only influence the enumeration process which is run
 * before processing the interface quirks.
 *
 * Please keep the lists ordered by:
 * 	1) Vendor ID
 * 	2) Product ID
 * 	3) Class ID
 */
static const struct usb_device_id usb_quirk_list[] = {
	/* CBM - Flash disk */
	{ USB_DEVICE(0x0204, 0x6025), .driver_info = USB_QUIRK_RESET_RESUME },

	/* WORLDE Controller KS49 or Prodipe MIDI 49C USB controller */
	{ USB_DEVICE(0x0218, 0x0201), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* WORLDE easy key (easykey.25) MIDI controller  */
	{ USB_DEVICE(0x0218, 0x0401), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* HP 5300/5370C scanner */
	{ USB_DEVICE(0x03f0, 0x0701), .driver_info =
			USB_QUIRK_STRING_FETCH_255 },

	/* HP v222w 16GB Mini USB Drive */
	{ USB_DEVICE(0x03f0, 0x3f40), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Huawei 4G LTE module ME906S  */
	{ USB_DEVICE(0x03f0, 0xa31d), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },

	/* Creative SB Audigy 2 NX */
	{ USB_DEVICE(0x041e, 0x3020), .driver_info = USB_QUIRK_RESET_RESUME },