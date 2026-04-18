// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 33/41



	/*
	 * Some superspeed devices have finished the link training process
	 * and attached to a superspeed hub port, but the device descriptor
	 * got from those devices show they aren't superspeed devices. Warm
	 * reset the port attached by the devices can fix them.
	 */
	if ((udev->speed >= USB_SPEED_SUPER) &&
			(le16_to_cpu(udev->descriptor.bcdUSB) < 0x0300)) {
		dev_err(&udev->dev, "got a wrong device descriptor, warm reset device\n");
		hub_port_reset(hub, port1, udev, HUB_BH_RESET_TIME, true);
		retval = -EINVAL;
		goto fail;
	}

	usb_detect_quirks(udev);

	if (le16_to_cpu(udev->descriptor.bcdUSB) >= 0x0201) {
		retval = usb_get_bos_descriptor(udev);
		if (!retval) {
			udev->lpm_capable = usb_device_supports_lpm(udev);
			udev->lpm_disable_count = 1;
			usb_set_lpm_parameters(udev);
			usb_req_set_sel(udev);
		}
	}

	retval = 0;
	/* notify HCD that we have a device connected and addressed */
	if (hcd->driver->update_device)
		hcd->driver->update_device(hcd, udev);
	hub_set_initial_usb2_lpm_policy(udev);
fail:
	if (retval) {
		hub_port_disable(hub, port1, 0);
		update_devnum(udev, devnum);	/* for disconnect processing */
	}
	kfree(buf);
	return retval;
}

static void
check_highspeed(struct usb_hub *hub, struct usb_device *udev, int port1)
{
	struct usb_qualifier_descriptor	*qual;
	int				status;

	if (udev->quirks & USB_QUIRK_DEVICE_QUALIFIER)
		return;

	qual = kmalloc_obj(*qual);
	if (qual == NULL)
		return;

	status = usb_get_descriptor(udev, USB_DT_DEVICE_QUALIFIER, 0,
			qual, sizeof *qual);
	if (status == sizeof *qual) {
		dev_info(&udev->dev, "not running at top speed; "
			"connect to a high speed hub\n");
		/* hub LEDs are probably harder to miss than syslog */
		if (hub->has_indicators) {
			hub->indicator[port1-1] = INDICATOR_GREEN_BLINK;
			queue_delayed_work(system_power_efficient_wq,
					&hub->leds, 0);
		}
	}
	kfree(qual);
}

static unsigned
hub_power_remaining(struct usb_hub *hub)
{
	struct usb_device *hdev = hub->hdev;
	int remaining;
	int port1;

	if (!hub->limited_power)
		return 0;

	remaining = hdev->bus_mA - hub->descriptor->bHubContrCurrent;
	for (port1 = 1; port1 <= hdev->maxchild; ++port1) {
		struct usb_port *port_dev = hub->ports[port1 - 1];
		struct usb_device *udev = port_dev->child;
		unsigned unit_load;
		int delta;

		if (!udev)
			continue;
		if (hub_is_superspeed(udev))
			unit_load = 150;
		else
			unit_load = 100;

		/*
		 * Unconfigured devices may not use more than one unit load,
		 * or 8mA for OTG ports
		 */
		if (udev->actconfig)
			delta = usb_get_max_power(udev, udev->actconfig);
		else if (port1 != udev->bus->otg_port || hdev->parent)
			delta = unit_load;
		else
			delta = 8;
		if (delta > hub->mA_per_port)
			dev_warn(&port_dev->dev, "%dmA is over %umA budget!\n",
					delta, hub->mA_per_port);
		remaining -= delta;
	}
	if (remaining < 0) {
		dev_warn(hub->intfdev, "%dmA over power budget!\n",
			-remaining);
		remaining = 0;
	}
	return remaining;
}


static int descriptors_changed(struct usb_device *udev,
		struct usb_device_descriptor *new_device_descriptor,
		struct usb_host_bos *old_bos)
{
	int		changed = 0;
	unsigned	index;
	unsigned	serial_len = 0;
	unsigned	len;
	unsigned	old_length;
	int		length;
	char		*buf;

	if (memcmp(&udev->descriptor, new_device_descriptor,
			sizeof(*new_device_descriptor)) != 0)
		return 1;

	if ((old_bos && !udev->bos) || (!old_bos && udev->bos))
		return 1;
	if (udev->bos) {
		len = le16_to_cpu(udev->bos->desc->wTotalLength);
		if (len != le16_to_cpu(old_bos->desc->wTotalLength))
			return 1;
		if (memcmp(udev->bos->desc, old_bos->desc, len))
			return 1;
	}

	/* Since the idVendor, idProduct, and bcdDevice values in the
	 * device descriptor haven't changed, we will assume the
	 * Manufacturer and Product strings haven't changed either.
	 * But the SerialNumber string could be different (e.g., a
	 * different flash card of the same brand).
	 */
	if (udev->serial)
		serial_len = strlen(udev->serial) + 1;

	len = serial_len;
	for (index = 0; index < udev->descriptor.bNumConfigurations; index++) {
		old_length = le16_to_cpu(udev->config[index].desc.wTotalLength);
		len = max(len, old_length);
	}

	buf = kmalloc(len, GFP_NOIO);
	if (!buf)
		/* assume the worst */
		return 1;

	for (index = 0; index < udev->descriptor.bNumConfigurations; index++) {
		old_length = le16_to_cpu(udev->config[index].desc.wTotalLength);
		length = usb_get_descriptor(udev, USB_DT_CONFIG, index, buf,
				old_length);
		if (length != old_length) {
			dev_dbg(&udev->dev, "config index %d, error %d\n",
					index, length);
			changed = 1;
			break;
		}
		if (memcmp(buf, udev->rawdescriptors[index], old_length)
				!= 0) {
			dev_dbg(&udev->dev, "config index %d changed (#%d)\n",
				index,
				((struct usb_config_descriptor *) buf)->
					bConfigurationValue);
			changed = 1;
			break;
		}
	}