// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 15/17



	spin_lock(&hdrv->dyn_lock);
	list_for_each_entry(dynid, &hdrv->dyn_list, list) {
		if (hid_match_one_id(hdev, &dynid->id)) {
			spin_unlock(&hdrv->dyn_lock);
			return &dynid->id;
		}
	}
	spin_unlock(&hdrv->dyn_lock);

	return hid_match_id(hdev, hdrv->id_table);
}
EXPORT_SYMBOL_GPL(hid_match_device);

static int hid_bus_match(struct device *dev, const struct device_driver *drv)
{
	struct hid_driver *hdrv = to_hid_driver(drv);
	struct hid_device *hdev = to_hid_device(dev);

	return hid_match_device(hdev, hdrv) != NULL;
}

/**
 * hid_compare_device_paths - check if both devices share the same path
 * @hdev_a: hid device
 * @hdev_b: hid device
 * @separator: char to use as separator
 *
 * Check if two devices share the same path up to the last occurrence of
 * the separator char. Both paths must exist (i.e., zero-length paths
 * don't match).
 */
bool hid_compare_device_paths(struct hid_device *hdev_a,
			      struct hid_device *hdev_b, char separator)
{
	int n1 = strrchr(hdev_a->phys, separator) - hdev_a->phys;
	int n2 = strrchr(hdev_b->phys, separator) - hdev_b->phys;

	if (n1 != n2 || n1 <= 0 || n2 <= 0)
		return false;

	return !strncmp(hdev_a->phys, hdev_b->phys, n1);
}
EXPORT_SYMBOL_GPL(hid_compare_device_paths);

static bool hid_check_device_match(struct hid_device *hdev,
				   struct hid_driver *hdrv,
				   const struct hid_device_id **id)
{
	*id = hid_match_device(hdev, hdrv);
	if (!*id)
		return false;

	if (hdrv->match)
		return hdrv->match(hdev, hid_ignore_special_drivers);

	/*
	 * hid-generic implements .match(), so we must be dealing with a
	 * different HID driver here, and can simply check if
	 * hid_ignore_special_drivers or HID_QUIRK_IGNORE_SPECIAL_DRIVER
	 * are set or not.
	 */
	return !hid_ignore_special_drivers && !(hdev->quirks & HID_QUIRK_IGNORE_SPECIAL_DRIVER);
}

static void hid_set_group(struct hid_device *hdev)
{
	int ret;

	if (hid_ignore_special_drivers) {
		hdev->group = HID_GROUP_GENERIC;
	} else if (!hdev->group &&
		   !(hdev->quirks & HID_QUIRK_HAVE_SPECIAL_DRIVER)) {
		ret = hid_scan_report(hdev);
		if (ret)
			hid_warn(hdev, "bad device descriptor (%d)\n", ret);
	}
}

static int __hid_device_probe(struct hid_device *hdev, struct hid_driver *hdrv)
{
	const struct hid_device_id *id;
	int ret;

	if (!hdev->bpf_rsize) {
		/* we keep a reference to the currently scanned report descriptor */
		const __u8  *original_rdesc = hdev->bpf_rdesc;

		if (!original_rdesc)
			original_rdesc = hdev->dev_rdesc;

		/* in case a bpf program gets detached, we need to free the old one */
		hid_free_bpf_rdesc(hdev);

		/* keep this around so we know we called it once */
		hdev->bpf_rsize = hdev->dev_rsize;

		/* call_hid_bpf_rdesc_fixup will always return a valid pointer */
		hdev->bpf_rdesc = call_hid_bpf_rdesc_fixup(hdev, hdev->dev_rdesc,
							   &hdev->bpf_rsize);

		/* the report descriptor changed, we need to re-scan it */
		if (original_rdesc != hdev->bpf_rdesc) {
			hdev->group = 0;
			hid_set_group(hdev);
		}
	}

	if (!hid_check_device_match(hdev, hdrv, &id))
		return -ENODEV;

	hdev->devres_group_id = devres_open_group(&hdev->dev, NULL, GFP_KERNEL);
	if (!hdev->devres_group_id)
		return -ENOMEM;

	/* reset the quirks that has been previously set */
	hdev->quirks = hid_lookup_quirk(hdev);
	hdev->driver = hdrv;

	if (hdrv->probe) {
		ret = hdrv->probe(hdev, id);
	} else { /* default probe */
		ret = hid_open_report(hdev);
		if (!ret)
			ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	}

	/*
	 * Note that we are not closing the devres group opened above so
	 * even resources that were attached to the device after probe is
	 * run are released when hid_device_remove() is executed. This is
	 * needed as some drivers would allocate additional resources,
	 * for example when updating firmware.
	 */

	if (ret) {
		devres_release_group(&hdev->dev, hdev->devres_group_id);
		hid_close_report(hdev);
		hdev->driver = NULL;
	}

	return ret;
}

static int hid_device_probe(struct device *dev)
{
	struct hid_device *hdev = to_hid_device(dev);
	struct hid_driver *hdrv = to_hid_driver(dev->driver);
	int ret = 0;

	if (down_interruptible(&hdev->driver_input_lock))
		return -EINTR;

	hdev->io_started = false;
	clear_bit(ffs(HID_STAT_REPROBED), &hdev->status);

	if (!hdev->driver)
		ret = __hid_device_probe(hdev, hdrv);

	if (!hdev->io_started)
		up(&hdev->driver_input_lock);

	return ret;
}

static void hid_device_remove(struct device *dev)
{
	struct hid_device *hdev = to_hid_device(dev);
	struct hid_driver *hdrv;

	down(&hdev->driver_input_lock);
	hdev->io_started = false;

	hdrv = hdev->driver;
	if (hdrv) {
		if (hdrv->remove)
			hdrv->remove(hdev);
		else /* default remove */
			hid_hw_stop(hdev);

		/* Release all devres resources allocated by the driver */
		devres_release_group(&hdev->dev, hdev->devres_group_id);

		hid_close_report(hdev);
		hdev->driver = NULL;
	}

	if (!hdev->io_started)
		up(&hdev->driver_input_lock);
}