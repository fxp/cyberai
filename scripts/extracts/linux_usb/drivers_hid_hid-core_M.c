// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 13/17



	if ((connect_mask & HID_CONNECT_HIDINPUT) && !hidinput_connect(hdev,
				connect_mask & HID_CONNECT_HIDINPUT_FORCE))
		hdev->claimed |= HID_CLAIMED_INPUT;

	if ((connect_mask & HID_CONNECT_HIDDEV) && hdev->hiddev_connect &&
			!hdev->hiddev_connect(hdev,
				connect_mask & HID_CONNECT_HIDDEV_FORCE))
		hdev->claimed |= HID_CLAIMED_HIDDEV;
	if ((connect_mask & HID_CONNECT_HIDRAW) && !hidraw_connect(hdev))
		hdev->claimed |= HID_CLAIMED_HIDRAW;

	if (connect_mask & HID_CONNECT_DRIVER)
		hdev->claimed |= HID_CLAIMED_DRIVER;

	/* Drivers with the ->raw_event callback set are not required to connect
	 * to any other listener. */
	if (!hdev->claimed && !hdev->driver->raw_event) {
		hid_err(hdev, "device has no listeners, quitting\n");
		return -ENODEV;
	}

	hid_process_ordering(hdev);

	if ((hdev->claimed & HID_CLAIMED_INPUT) &&
			(connect_mask & HID_CONNECT_FF) && hdev->ff_init)
		hdev->ff_init(hdev);

	len = 0;
	if (hdev->claimed & HID_CLAIMED_INPUT)
		len += sprintf(buf + len, "input");
	if (hdev->claimed & HID_CLAIMED_HIDDEV)
		len += sprintf(buf + len, "%shiddev%d", len ? "," : "",
				((struct hiddev *)hdev->hiddev)->minor);
	if (hdev->claimed & HID_CLAIMED_HIDRAW)
		len += sprintf(buf + len, "%shidraw%d", len ? "," : "",
				((struct hidraw *)hdev->hidraw)->minor);

	type = "Device";
	for (i = 0; i < hdev->maxcollection; i++) {
		struct hid_collection *col = &hdev->collection[i];
		if (col->type == HID_COLLECTION_APPLICATION &&
		   (col->usage & HID_USAGE_PAGE) == HID_UP_GENDESK &&
		   (col->usage & 0xffff) < ARRAY_SIZE(types)) {
			type = types[col->usage & 0xffff];
			break;
		}
	}

	switch (hdev->bus) {
	case BUS_USB:
		bus = "USB";
		break;
	case BUS_BLUETOOTH:
		bus = "BLUETOOTH";
		break;
	case BUS_I2C:
		bus = "I2C";
		break;
	case BUS_SDW:
		bus = "SOUNDWIRE";
		break;
	case BUS_VIRTUAL:
		bus = "VIRTUAL";
		break;
	case BUS_INTEL_ISHTP:
	case BUS_AMD_SFH:
		bus = "SENSOR HUB";
		break;
	default:
		bus = "<UNKNOWN>";
	}

	ret = device_create_file(&hdev->dev, &dev_attr_country);
	if (ret)
		hid_warn(hdev,
			 "can't create sysfs country code attribute err: %d\n", ret);

	hid_info(hdev, "%s: %s HID v%x.%02x %s [%s] on %s\n",
		 buf, bus, hdev->version >> 8, hdev->version & 0xff,
		 type, hdev->name, hdev->phys);

	return 0;
}
EXPORT_SYMBOL_GPL(hid_connect);

void hid_disconnect(struct hid_device *hdev)
{
	device_remove_file(&hdev->dev, &dev_attr_country);
	if (hdev->claimed & HID_CLAIMED_INPUT)
		hidinput_disconnect(hdev);
	if (hdev->claimed & HID_CLAIMED_HIDDEV)
		hdev->hiddev_disconnect(hdev);
	if (hdev->claimed & HID_CLAIMED_HIDRAW)
		hidraw_disconnect(hdev);
	hdev->claimed = 0;

	hid_bpf_disconnect_device(hdev);
}
EXPORT_SYMBOL_GPL(hid_disconnect);

/**
 * hid_hw_start - start underlying HW
 * @hdev: hid device
 * @connect_mask: which outputs to connect, see HID_CONNECT_*
 *
 * Call this in probe function *after* hid_parse. This will setup HW
 * buffers and start the device (if not defeirred to device open).
 * hid_hw_stop must be called if this was successful.
 */
int hid_hw_start(struct hid_device *hdev, unsigned int connect_mask)
{
	int error;

	error = hdev->ll_driver->start(hdev);
	if (error)
		return error;

	if (connect_mask) {
		error = hid_connect(hdev, connect_mask);
		if (error) {
			hdev->ll_driver->stop(hdev);
			return error;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(hid_hw_start);

/**
 * hid_hw_stop - stop underlying HW
 * @hdev: hid device
 *
 * This is usually called from remove function or from probe when something
 * failed and hid_hw_start was called already.
 */
void hid_hw_stop(struct hid_device *hdev)
{
	hid_disconnect(hdev);
	hdev->ll_driver->stop(hdev);
}
EXPORT_SYMBOL_GPL(hid_hw_stop);

/**
 * hid_hw_open - signal underlying HW to start delivering events
 * @hdev: hid device
 *
 * Tell underlying HW to start delivering events from the device.
 * This function should be called sometime after successful call
 * to hid_hw_start().
 */
int hid_hw_open(struct hid_device *hdev)
{
	int ret;

	ret = mutex_lock_killable(&hdev->ll_open_lock);
	if (ret)
		return ret;

	if (!hdev->ll_open_count++) {
		ret = hdev->ll_driver->open(hdev);
		if (ret)
			hdev->ll_open_count--;

		if (hdev->driver->on_hid_hw_open)
			hdev->driver->on_hid_hw_open(hdev);
	}

	mutex_unlock(&hdev->ll_open_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(hid_hw_open);

/**
 * hid_hw_close - signal underlaying HW to stop delivering events
 *
 * @hdev: hid device
 *
 * This function indicates that we are not interested in the events
 * from this device anymore. Delivery of events may or may not stop,
 * depending on the number of users still outstanding.
 */
void hid_hw_close(struct hid_device *hdev)
{
	mutex_lock(&hdev->ll_open_lock);
	if (!--hdev->ll_open_count) {
		hdev->ll_driver->close(hdev);

		if (hdev->driver->on_hid_hw_close)
			hdev->driver->on_hid_hw_close(hdev);
	}
	mutex_unlock(&hdev->ll_open_lock);
}
EXPORT_SYMBOL_GPL(hid_hw_close);