// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 13/17



	if (add_uevent_var(env, "INTERFACE=%d/%d/%d",
		   alt->desc.bInterfaceClass,
		   alt->desc.bInterfaceSubClass,
		   alt->desc.bInterfaceProtocol))
		return -ENOMEM;

	if (add_uevent_var(env,
		   "MODALIAS=usb:"
		   "v%04Xp%04Xd%04Xdc%02Xdsc%02Xdp%02Xic%02Xisc%02Xip%02Xin%02X",
		   le16_to_cpu(usb_dev->descriptor.idVendor),
		   le16_to_cpu(usb_dev->descriptor.idProduct),
		   le16_to_cpu(usb_dev->descriptor.bcdDevice),
		   usb_dev->descriptor.bDeviceClass,
		   usb_dev->descriptor.bDeviceSubClass,
		   usb_dev->descriptor.bDeviceProtocol,
		   alt->desc.bInterfaceClass,
		   alt->desc.bInterfaceSubClass,
		   alt->desc.bInterfaceProtocol,
		   alt->desc.bInterfaceNumber))
		return -ENOMEM;

	return 0;
}

const struct device_type usb_if_device_type = {
	.name =		"usb_interface",
	.release =	usb_release_interface,
	.uevent =	usb_if_uevent,
};

static struct usb_interface_assoc_descriptor *find_iad(struct usb_device *dev,
						struct usb_host_config *config,
						u8 inum)
{
	struct usb_interface_assoc_descriptor *retval = NULL;
	struct usb_interface_assoc_descriptor *intf_assoc;
	int first_intf;
	int last_intf;
	int i;

	for (i = 0; (i < USB_MAXIADS && config->intf_assoc[i]); i++) {
		intf_assoc = config->intf_assoc[i];
		if (intf_assoc->bInterfaceCount == 0)
			continue;

		first_intf = intf_assoc->bFirstInterface;
		last_intf = first_intf + (intf_assoc->bInterfaceCount - 1);
		if (inum >= first_intf && inum <= last_intf) {
			if (!retval)
				retval = intf_assoc;
			else
				dev_err(&dev->dev, "Interface #%d referenced"
					" by multiple IADs\n", inum);
		}
	}

	return retval;
}


/*
 * Internal function to queue a device reset
 * See usb_queue_reset_device() for more details
 */
static void __usb_queue_reset_device(struct work_struct *ws)
{
	int rc;
	struct usb_interface *iface =
		container_of(ws, struct usb_interface, reset_ws);
	struct usb_device *udev = interface_to_usbdev(iface);

	rc = usb_lock_device_for_reset(udev, iface);
	if (rc >= 0) {
		usb_reset_device(udev);
		usb_unlock_device(udev);
	}
	usb_put_intf(iface);	/* Undo _get_ in usb_queue_reset_device() */
}

/*
 * Internal function to set the wireless_status sysfs attribute
 * See usb_set_wireless_status() for more details
 */
static void __usb_wireless_status_intf(struct work_struct *ws)
{
	struct usb_interface *iface =
		container_of(ws, struct usb_interface, wireless_status_work);

	device_lock(iface->dev.parent);
	if (iface->sysfs_files_created)
		usb_update_wireless_status_attr(iface);
	device_unlock(iface->dev.parent);
	usb_put_intf(iface);	/* Undo _get_ in usb_set_wireless_status() */
}

/**
 * usb_set_wireless_status - sets the wireless_status struct member
 * @iface: the interface to modify
 * @status: the new wireless status
 *
 * Set the wireless_status struct member to the new value, and emit
 * sysfs changes as necessary.
 *
 * Returns: 0 on success, -EALREADY if already set.
 */
int usb_set_wireless_status(struct usb_interface *iface,
		enum usb_wireless_status status)
{
	if (iface->wireless_status == status)
		return -EALREADY;

	usb_get_intf(iface);
	iface->wireless_status = status;
	schedule_work(&iface->wireless_status_work);

	return 0;
}
EXPORT_SYMBOL_GPL(usb_set_wireless_status);