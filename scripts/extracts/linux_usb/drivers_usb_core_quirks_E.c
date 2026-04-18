// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/quirks.c
// Segment 5/5



	/* Logitech Optical Mouse M90/M100 */
	{ USB_DEVICE(0x046d, 0xc05a), .driver_info = USB_QUIRK_RESET_RESUME },

	{ }  /* terminating entry must be last */
};

/*
 * Entries for endpoints that should be ignored when parsing configuration
 * descriptors.
 *
 * Matched for devices with USB_QUIRK_ENDPOINT_IGNORE.
 */
static const struct usb_device_id usb_endpoint_ignore[] = {
	{ USB_DEVICE_INTERFACE_NUMBER(0x06f8, 0xb000, 5), .driver_info = 0x01 },
	{ USB_DEVICE_INTERFACE_NUMBER(0x06f8, 0xb000, 5), .driver_info = 0x81 },
	{ USB_DEVICE_INTERFACE_NUMBER(0x0926, 0x0202, 1), .driver_info = 0x85 },
	{ USB_DEVICE_INTERFACE_NUMBER(0x0926, 0x0208, 1), .driver_info = 0x85 },
	{ }
};

bool usb_endpoint_is_ignored(struct usb_device *udev,
			     struct usb_host_interface *intf,
			     struct usb_endpoint_descriptor *epd)
{
	const struct usb_device_id *id;
	unsigned int address;

	for (id = usb_endpoint_ignore; id->match_flags; ++id) {
		if (!usb_match_device(udev, id))
			continue;

		if (!usb_match_one_id_intf(udev, intf, id))
			continue;

		address = id->driver_info;
		if (address == epd->bEndpointAddress)
			return true;
	}

	return false;
}

static bool usb_match_any_interface(struct usb_device *udev,
				    const struct usb_device_id *id)
{
	unsigned int i;

	for (i = 0; i < udev->descriptor.bNumConfigurations; ++i) {
		struct usb_host_config *cfg = &udev->config[i];
		unsigned int j;

		for (j = 0; j < cfg->desc.bNumInterfaces; ++j) {
			struct usb_interface_cache *cache;
			struct usb_host_interface *intf;

			cache = cfg->intf_cache[j];
			if (cache->num_altsetting == 0)
				continue;

			intf = &cache->altsetting[0];
			if (usb_match_one_id_intf(udev, intf, id))
				return true;
		}
	}

	return false;
}

static int usb_amd_resume_quirk(struct usb_device *udev)
{
	struct usb_hcd *hcd;

	hcd = bus_to_hcd(udev->bus);
	/* The device should be attached directly to root hub */
	if (udev->level == 1 && hcd->amd_resume_bug == 1)
		return 1;

	return 0;
}

static u32 usb_detect_static_quirks(struct usb_device *udev,
				    const struct usb_device_id *id)
{
	u32 quirks = 0;

	for (; id->match_flags; id++) {
		if (!usb_match_device(udev, id))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO) &&
		    !usb_match_any_interface(udev, id))
			continue;

		quirks |= (u32)(id->driver_info);
	}

	return quirks;
}

static u32 usb_detect_dynamic_quirks(struct usb_device *udev)
{
	u16 vid = le16_to_cpu(udev->descriptor.idVendor);
	u16 pid = le16_to_cpu(udev->descriptor.idProduct);
	int i, flags = 0;

	mutex_lock(&quirk_mutex);

	for (i = 0; i < quirk_count; i++) {
		if (vid == quirk_list[i].vid && pid == quirk_list[i].pid) {
			flags = quirk_list[i].flags;
			break;
		}
	}

	mutex_unlock(&quirk_mutex);

	return flags;
}

/*
 * Detect any quirks the device has, and do any housekeeping for it if needed.
 */
void usb_detect_quirks(struct usb_device *udev)
{
	udev->quirks = usb_detect_static_quirks(udev, usb_quirk_list);

	/*
	 * Pixart-based mice would trigger remote wakeup issue on AMD
	 * Yangtze chipset, so set them as RESET_RESUME flag.
	 */
	if (usb_amd_resume_quirk(udev))
		udev->quirks |= usb_detect_static_quirks(udev,
				usb_amd_resume_quirk_list);

	udev->quirks ^= usb_detect_dynamic_quirks(udev);

	if (udev->quirks)
		dev_dbg(&udev->dev, "USB quirks for this device: 0x%x\n",
			udev->quirks);

#ifdef CONFIG_USB_DEFAULT_PERSIST
	if (!(udev->quirks & USB_QUIRK_RESET))
		udev->persist_enabled = 1;
#else
	/* Hubs are automatically enabled for USB-PERSIST */
	if (udev->descriptor.bDeviceClass == USB_CLASS_HUB)
		udev->persist_enabled = 1;
#endif	/* CONFIG_USB_DEFAULT_PERSIST */
}

void usb_detect_interface_quirks(struct usb_device *udev)
{
	u32 quirks;

	quirks = usb_detect_static_quirks(udev, usb_interface_quirk_list);
	if (quirks == 0)
		return;

	dev_dbg(&udev->dev, "USB interface quirks for this device: %x\n",
		quirks);
	udev->quirks |= quirks;
}

void usb_release_quirk_list(void)
{
	mutex_lock(&quirk_mutex);
	kfree(quirk_list);
	quirk_list = NULL;
	mutex_unlock(&quirk_mutex);
}
