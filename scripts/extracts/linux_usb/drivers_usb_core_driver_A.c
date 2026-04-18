// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 1/14

// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/usb/core/driver.c - most of the driver model stuff for usb
 *
 * (C) Copyright 2005 Greg Kroah-Hartman <gregkh@suse.de>
 *
 * based on drivers/usb/usb.c which had the following copyrights:
 *	(C) Copyright Linus Torvalds 1999
 *	(C) Copyright Johannes Erdfelt 1999-2001
 *	(C) Copyright Andreas Gal 1999
 *	(C) Copyright Gregory P. Smith 1999
 *	(C) Copyright Deti Fliegl 1999 (new USB architecture)
 *	(C) Copyright Randy Dunlap 2000
 *	(C) Copyright David Brownell 2000-2004
 *	(C) Copyright Yggdrasil Computing, Inc. 2000
 *		(usb_device_id matching changes by Adam J. Richter)
 *	(C) Copyright Greg Kroah-Hartman 2002-2003
 *
 * Released under the GPLv2 only.
 *
 * NOTE! This is not actually a driver at all, rather this is
 * just a collection of helper routines that implement the
 * matching, probing, releasing, suspending and resuming for
 * real drivers.
 *
 */

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include <linux/usb/hcd.h>

#include "usb.h"


/*
 * Adds a new dynamic USBdevice ID to this driver,
 * and cause the driver to probe for all devices again.
 */
ssize_t usb_store_new_id(struct usb_dynids *dynids,
			 const struct usb_device_id *id_table,
			 struct device_driver *driver,
			 const char *buf, size_t count)
{
	struct usb_dynid *dynid;
	u32 idVendor = 0;
	u32 idProduct = 0;
	unsigned int bInterfaceClass = 0;
	u32 refVendor, refProduct;
	int fields = 0;
	int retval = 0;

	fields = sscanf(buf, "%x %x %x %x %x", &idVendor, &idProduct,
			&bInterfaceClass, &refVendor, &refProduct);
	if (fields < 2)
		return -EINVAL;

	dynid = kzalloc_obj(*dynid);
	if (!dynid)
		return -ENOMEM;

	INIT_LIST_HEAD(&dynid->node);
	dynid->id.idVendor = idVendor;
	dynid->id.idProduct = idProduct;
	dynid->id.match_flags = USB_DEVICE_ID_MATCH_DEVICE;
	if (fields > 2 && bInterfaceClass) {
		if (bInterfaceClass > 255) {
			retval = -EINVAL;
			goto fail;
		}

		dynid->id.bInterfaceClass = (u8)bInterfaceClass;
		dynid->id.match_flags |= USB_DEVICE_ID_MATCH_INT_CLASS;
	}

	if (fields > 4) {
		const struct usb_device_id *id = id_table;

		if (!id) {
			retval = -ENODEV;
			goto fail;
		}

		for (; id->match_flags; id++)
			if (id->idVendor == refVendor && id->idProduct == refProduct)
				break;

		if (id->match_flags) {
			dynid->id.driver_info = id->driver_info;
		} else {
			retval = -ENODEV;
			goto fail;
		}
	}

	mutex_lock(&usb_dynids_lock);
	list_add_tail(&dynid->node, &dynids->list);
	mutex_unlock(&usb_dynids_lock);

	retval = driver_attach(driver);

	if (retval)
		return retval;
	return count;

fail:
	kfree(dynid);
	return retval;
}
EXPORT_SYMBOL_GPL(usb_store_new_id);

ssize_t usb_show_dynids(struct usb_dynids *dynids, char *buf)
{
	struct usb_dynid *dynid;
	size_t count = 0;

	guard(mutex)(&usb_dynids_lock);
	list_for_each_entry(dynid, &dynids->list, node)
		if (dynid->id.bInterfaceClass != 0)
			count += sysfs_emit_at(buf, count, "%04x %04x %02x\n",
					   dynid->id.idVendor, dynid->id.idProduct,
					   dynid->id.bInterfaceClass);
		else
			count += sysfs_emit_at(buf, count, "%04x %04x\n",
					   dynid->id.idVendor, dynid->id.idProduct);
	return count;
}
EXPORT_SYMBOL_GPL(usb_show_dynids);

static ssize_t new_id_show(struct device_driver *driver, char *buf)
{
	struct usb_driver *usb_drv = to_usb_driver(driver);

	return usb_show_dynids(&usb_drv->dynids, buf);
}

static ssize_t new_id_store(struct device_driver *driver,
			    const char *buf, size_t count)
{
	struct usb_driver *usb_drv = to_usb_driver(driver);

	return usb_store_new_id(&usb_drv->dynids, usb_drv->id_table, driver, buf, count);
}
static DRIVER_ATTR_RW(new_id);

/*
 * Remove a USB device ID from this driver
 */
static ssize_t remove_id_store(struct device_driver *driver, const char *buf,
			       size_t count)
{
	struct usb_dynid *dynid, *n;
	struct usb_driver *usb_driver = to_usb_driver(driver);
	u32 idVendor;
	u32 idProduct;
	int fields;

	fields = sscanf(buf, "%x %x", &idVendor, &idProduct);
	if (fields < 2)
		return -EINVAL;

	guard(mutex)(&usb_dynids_lock);
	list_for_each_entry_safe(dynid, n, &usb_driver->dynids.list, node) {
		struct usb_device_id *id = &dynid->id;

		if ((id->idVendor == idVendor) &&
		    (id->idProduct == idProduct)) {
			list_del(&dynid->node);
			kfree(dynid);
			break;
		}
	}
	return count;
}

static ssize_t remove_id_show(struct device_driver *driver, char *buf)
{
	return new_id_show(driver, buf);
}
static DRIVER_ATTR_RW(remove_id);

static int usb_create_newid_files(struct usb_driver *usb_drv)
{
	int error = 0;

	if (usb_drv->no_dynamic_id)
		goto exit;

	if (usb_drv->probe != NULL) {
		error = driver_create_file(&usb_drv->driver,
					   &driver_attr_new_id);
		if (error == 0) {
			error = driver_create_file(&usb_drv->driver,
					&driver_attr_remove_id);
			if (error)
				driver_remove_file(&usb_drv->driver,
						&driver_attr_new_id);
		}
	}
exit:
	return error;
}