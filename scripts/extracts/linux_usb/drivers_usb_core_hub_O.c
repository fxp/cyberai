// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 15/41



	/* mark the device as inactive, so any further urb submissions for
	 * this device (and any of its children) will fail immediately.
	 * this quiesces everything except pending urbs.
	 */
	usb_set_device_state(udev, USB_STATE_NOTATTACHED);
	dev_info(&udev->dev, "USB disconnect, device number %d\n",
			udev->devnum);

	/*
	 * Ensure that the pm runtime code knows that the USB device
	 * is in the process of being disconnected.
	 */
	pm_runtime_barrier(&udev->dev);

	usb_lock_device(udev);

	hub_disconnect_children(udev);

	/* deallocate hcd/hardware state ... nuking all pending urbs and
	 * cleaning up all state associated with the current configuration
	 * so that the hardware is now fully quiesced.
	 */
	dev_dbg(&udev->dev, "unregistering device\n");
	usb_disable_device(udev, 0);
	usb_hcd_synchronize_unlinks(udev);

	if (udev->parent) {
		port1 = udev->portnum;
		hub = usb_hub_to_struct_hub(udev->parent);
		port_dev = hub->ports[port1 - 1];

		sysfs_remove_link(&udev->dev.kobj, "port");
		sysfs_remove_link(&port_dev->dev.kobj, "device");

		/*
		 * As usb_port_runtime_resume() de-references udev, make
		 * sure no resumes occur during removal
		 */
		if (!test_and_set_bit(port1, hub->child_usage_bits))
			pm_runtime_get_sync(&port_dev->dev);

		typec_deattach(port_dev->connector, &udev->dev);
	}

	usb_remove_ep_devs(&udev->ep0);
	usb_unlock_device(udev);

	if (udev->usb4_link)
		device_link_del(udev->usb4_link);

	/* Unregister the device.  The device driver is responsible
	 * for de-configuring the device and invoking the remove-device
	 * notifier chain (used by usbfs and possibly others).
	 */
	device_del(&udev->dev);

	/* Free the device number and delete the parent's children[]
	 * (or root_hub) pointer.
	 */
	release_devnum(udev);

	/* Avoid races with recursively_mark_NOTATTACHED() */
	spin_lock_irq(&device_state_lock);
	*pdev = NULL;
	spin_unlock_irq(&device_state_lock);

	if (port_dev && test_and_clear_bit(port1, hub->child_usage_bits))
		pm_runtime_put(&port_dev->dev);

	hub_free_dev(udev);

	put_device(&udev->dev);
}

#ifdef CONFIG_USB_ANNOUNCE_NEW_DEVICES
static void show_string(struct usb_device *udev, char *id, char *string)
{
	if (!string)
		return;
	dev_info(&udev->dev, "%s: %s\n", id, string);
}

static void announce_device(struct usb_device *udev)
{
	u16 bcdDevice = le16_to_cpu(udev->descriptor.bcdDevice);

	dev_info(&udev->dev,
		"New USB device found, idVendor=%04x, idProduct=%04x, bcdDevice=%2x.%02x\n",
		le16_to_cpu(udev->descriptor.idVendor),
		le16_to_cpu(udev->descriptor.idProduct),
		bcdDevice >> 8, bcdDevice & 0xff);
	dev_info(&udev->dev,
		"New USB device strings: Mfr=%d, Product=%d, SerialNumber=%d\n",
		udev->descriptor.iManufacturer,
		udev->descriptor.iProduct,
		udev->descriptor.iSerialNumber);
	show_string(udev, "Product", udev->product);
	show_string(udev, "Manufacturer", udev->manufacturer);
	show_string(udev, "SerialNumber", udev->serial);
}
#else
static inline void announce_device(struct usb_device *udev) { }
#endif


/**
 * usb_enumerate_device_otg - FIXME (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * Finish enumeration for On-The-Go devices
 *
 * Return: 0 if successful. A negative error code otherwise.
 */
static int usb_enumerate_device_otg(struct usb_device *udev)
{
	int err = 0;

#ifdef	CONFIG_USB_OTG
	/*
	 * OTG-aware devices on OTG-capable root hubs may be able to use SRP,
	 * to wake us after we've powered off VBUS; and HNP, switching roles
	 * "host" to "peripheral".  The OTG descriptor helps figure this out.
	 */
	if (!udev->bus->is_b_host
			&& udev->config
			&& udev->parent == udev->bus->root_hub) {
		struct usb_otg_descriptor	*desc = NULL;
		struct usb_bus			*bus = udev->bus;
		unsigned			port1 = udev->portnum;

		/* descriptor may appear anywhere in config */
		err = __usb_get_extra_descriptor(udev->rawdescriptors[0],
				le16_to_cpu(udev->config[0].desc.wTotalLength),
				USB_DT_OTG, (void **) &desc, sizeof(*desc));
		if (err || !(desc->bmAttributes & USB_OTG_HNP))
			return 0;

		dev_info(&udev->dev, "Dual-Role OTG device on %sHNP port\n",
					(port1 == bus->otg_port) ? "" : "non-");