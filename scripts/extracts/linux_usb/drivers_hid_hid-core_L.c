// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 12/17



	if ((hid->claimed & HID_CLAIMED_HIDDEV) && hid->hiddev_report_event)
		hid->hiddev_report_event(hid, report);
	if (hid->claimed & HID_CLAIMED_HIDRAW) {
		ret = hidraw_report_event(hid, data, size);
		if (ret)
			goto out;
	}

	if (hid->claimed != HID_CLAIMED_HIDRAW && report->maxfield) {
		hid_process_report(hid, report, cdata, interrupt);
		hdrv = hid->driver;
		if (hdrv && hdrv->report)
			hdrv->report(hid, report);
	}

	if (hid->claimed & HID_CLAIMED_INPUT)
		hidinput_report_event(hid, report);
out:
	return ret;
}
EXPORT_SYMBOL_GPL(hid_report_raw_event);


static int __hid_input_report(struct hid_device *hid, enum hid_report_type type,
			      u8 *data, u32 size, int interrupt, u64 source, bool from_bpf,
			      bool lock_already_taken)
{
	struct hid_report_enum *report_enum;
	struct hid_driver *hdrv;
	struct hid_report *report;
	int ret = 0;

	if (!hid)
		return -ENODEV;

	ret = down_trylock(&hid->driver_input_lock);
	if (lock_already_taken && !ret) {
		up(&hid->driver_input_lock);
		return -EINVAL;
	} else if (!lock_already_taken && ret) {
		return -EBUSY;
	}

	if (!hid->driver) {
		ret = -ENODEV;
		goto unlock;
	}
	report_enum = hid->report_enum + type;
	hdrv = hid->driver;

	data = dispatch_hid_bpf_device_event(hid, type, data, &size, interrupt, source, from_bpf);
	if (IS_ERR(data)) {
		ret = PTR_ERR(data);
		goto unlock;
	}

	if (!size) {
		dbg_hid("empty report\n");
		ret = -1;
		goto unlock;
	}

	/* Avoid unnecessary overhead if debugfs is disabled */
	if (!list_empty(&hid->debug_list))
		hid_dump_report(hid, type, data, size);

	report = hid_get_report(report_enum, data);

	if (!report) {
		ret = -1;
		goto unlock;
	}

	if (hdrv && hdrv->raw_event && hid_match_report(hid, report)) {
		ret = hdrv->raw_event(hid, report, data, size);
		if (ret < 0)
			goto unlock;
	}

	ret = hid_report_raw_event(hid, type, data, size, interrupt);

unlock:
	if (!lock_already_taken)
		up(&hid->driver_input_lock);
	return ret;
}

/**
 * hid_input_report - report data from lower layer (usb, bt...)
 *
 * @hid: hid device
 * @type: HID report type (HID_*_REPORT)
 * @data: report contents
 * @size: size of data parameter
 * @interrupt: distinguish between interrupt and control transfers
 *
 * This is data entry for lower layers.
 */
int hid_input_report(struct hid_device *hid, enum hid_report_type type, u8 *data, u32 size,
		     int interrupt)
{
	return __hid_input_report(hid, type, data, size, interrupt, 0,
				  false, /* from_bpf */
				  false /* lock_already_taken */);
}
EXPORT_SYMBOL_GPL(hid_input_report);

bool hid_match_one_id(const struct hid_device *hdev,
		      const struct hid_device_id *id)
{
	return (id->bus == HID_BUS_ANY || id->bus == hdev->bus) &&
		(id->group == HID_GROUP_ANY || id->group == hdev->group) &&
		(id->vendor == HID_ANY_ID || id->vendor == hdev->vendor) &&
		(id->product == HID_ANY_ID || id->product == hdev->product);
}

const struct hid_device_id *hid_match_id(const struct hid_device *hdev,
		const struct hid_device_id *id)
{
	for (; id->bus; id++)
		if (hid_match_one_id(hdev, id))
			return id;

	return NULL;
}
EXPORT_SYMBOL_GPL(hid_match_id);

static const struct hid_device_id hid_hiddev_list[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_MGE, USB_DEVICE_ID_MGE_UPS) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_MGE, USB_DEVICE_ID_MGE_UPS1) },
	{ }
};

static bool hid_hiddev(struct hid_device *hdev)
{
	return !!hid_match_id(hdev, hid_hiddev_list);
}


static ssize_t
report_descriptor_read(struct file *filp, struct kobject *kobj,
		       const struct bin_attribute *attr,
		       char *buf, loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct hid_device *hdev = to_hid_device(dev);

	if (off >= hdev->rsize)
		return 0;

	if (off + count > hdev->rsize)
		count = hdev->rsize - off;

	memcpy(buf, hdev->rdesc + off, count);

	return count;
}

static ssize_t
country_show(struct device *dev, struct device_attribute *attr,
	     char *buf)
{
	struct hid_device *hdev = to_hid_device(dev);

	return sprintf(buf, "%02x\n", hdev->country & 0xff);
}

static const BIN_ATTR_RO(report_descriptor, HID_MAX_DESCRIPTOR_SIZE);

static const DEVICE_ATTR_RO(country);

int hid_connect(struct hid_device *hdev, unsigned int connect_mask)
{
	static const char *types[] = { "Device", "Pointer", "Mouse", "Device",
		"Joystick", "Gamepad", "Keyboard", "Keypad",
		"Multi-Axis Controller"
	};
	const char *type, *bus;
	char buf[64] = "";
	unsigned int i;
	int len;
	int ret;

	ret = hid_bpf_connect_device(hdev);
	if (ret)
		return ret;

	if (hdev->quirks & HID_QUIRK_HIDDEV_FORCE)
		connect_mask |= (HID_CONNECT_HIDDEV_FORCE | HID_CONNECT_HIDDEV);
	if (hdev->quirks & HID_QUIRK_HIDINPUT_FORCE)
		connect_mask |= HID_CONNECT_HIDINPUT_FORCE;
	if (hdev->bus != BUS_USB)
		connect_mask &= ~HID_CONNECT_HIDDEV;
	if (hid_hiddev(hdev))
		connect_mask |= HID_CONNECT_HIDDEV_FORCE;