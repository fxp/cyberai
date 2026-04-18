// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 16/17



static ssize_t modalias_show(struct device *dev, struct device_attribute *a,
			     char *buf)
{
	struct hid_device *hdev = container_of(dev, struct hid_device, dev);

	return sysfs_emit(buf, "hid:b%04Xg%04Xv%08Xp%08X\n",
			 hdev->bus, hdev->group, hdev->vendor, hdev->product);
}
static DEVICE_ATTR_RO(modalias);

static struct attribute *hid_dev_attrs[] = {
	&dev_attr_modalias.attr,
	NULL,
};
static const struct bin_attribute *hid_dev_bin_attrs[] = {
	&bin_attr_report_descriptor,
	NULL
};
static const struct attribute_group hid_dev_group = {
	.attrs = hid_dev_attrs,
	.bin_attrs = hid_dev_bin_attrs,
};
__ATTRIBUTE_GROUPS(hid_dev);

static int hid_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct hid_device *hdev = to_hid_device(dev);

	if (add_uevent_var(env, "HID_ID=%04X:%08X:%08X",
			hdev->bus, hdev->vendor, hdev->product))
		return -ENOMEM;

	if (add_uevent_var(env, "HID_NAME=%s", hdev->name))
		return -ENOMEM;

	if (add_uevent_var(env, "HID_PHYS=%s", hdev->phys))
		return -ENOMEM;

	if (add_uevent_var(env, "HID_UNIQ=%s", hdev->uniq))
		return -ENOMEM;

	if (add_uevent_var(env, "MODALIAS=hid:b%04Xg%04Xv%08Xp%08X",
			   hdev->bus, hdev->group, hdev->vendor, hdev->product))
		return -ENOMEM;
	if (hdev->firmware_version) {
		if (add_uevent_var(env, "HID_FIRMWARE_VERSION=0x%04llX",
				   hdev->firmware_version))
			return -ENOMEM;
	}

	return 0;
}

const struct bus_type hid_bus_type = {
	.name		= "hid",
	.dev_groups	= hid_dev_groups,
	.drv_groups	= hid_drv_groups,
	.match		= hid_bus_match,
	.probe		= hid_device_probe,
	.remove		= hid_device_remove,
	.uevent		= hid_uevent,
};
EXPORT_SYMBOL(hid_bus_type);

int hid_add_device(struct hid_device *hdev)
{
	static atomic_t id = ATOMIC_INIT(0);
	int ret;

	if (WARN_ON(hdev->status & HID_STAT_ADDED))
		return -EBUSY;

	hdev->quirks = hid_lookup_quirk(hdev);

	/* we need to kill them here, otherwise they will stay allocated to
	 * wait for coming driver */
	if (hid_ignore(hdev))
		return -ENODEV;

	/*
	 * Check for the mandatory transport channel.
	 */
	 if (!hdev->ll_driver->raw_request) {
		hid_err(hdev, "transport driver missing .raw_request()\n");
		return -EINVAL;
	 }

	/*
	 * Read the device report descriptor once and use as template
	 * for the driver-specific modifications.
	 */
	ret = hdev->ll_driver->parse(hdev);
	if (ret)
		return ret;
	if (!hdev->dev_rdesc)
		return -ENODEV;

	/*
	 * Scan generic devices for group information
	 */
	hid_set_group(hdev);

	hdev->id = atomic_inc_return(&id);

	/* XXX hack, any other cleaner solution after the driver core
	 * is converted to allow more than 20 bytes as the device name? */
	dev_set_name(&hdev->dev, "%04X:%04X:%04X.%04X", hdev->bus,
		     hdev->vendor, hdev->product, hdev->id);

	hid_debug_register(hdev, dev_name(&hdev->dev));
	ret = device_add(&hdev->dev);
	if (!ret)
		hdev->status |= HID_STAT_ADDED;
	else
		hid_debug_unregister(hdev);

	return ret;
}
EXPORT_SYMBOL_GPL(hid_add_device);

/**
 * hid_allocate_device - allocate new hid device descriptor
 *
 * Allocate and initialize hid device, so that hid_destroy_device might be
 * used to free it.
 *
 * New hid_device pointer is returned on success, otherwise ERR_PTR encoded
 * error value.
 */
struct hid_device *hid_allocate_device(void)
{
	struct hid_device *hdev;
	int ret = -ENOMEM;

	hdev = kzalloc_obj(*hdev);
	if (hdev == NULL)
		return ERR_PTR(ret);

	device_initialize(&hdev->dev);
	hdev->dev.release = hid_device_release;
	hdev->dev.bus = &hid_bus_type;
	device_enable_async_suspend(&hdev->dev);

	hid_close_report(hdev);

	init_waitqueue_head(&hdev->debug_wait);
	INIT_LIST_HEAD(&hdev->debug_list);
	spin_lock_init(&hdev->debug_list_lock);
	sema_init(&hdev->driver_input_lock, 1);
	mutex_init(&hdev->ll_open_lock);
	kref_init(&hdev->ref);

#ifdef CONFIG_HID_BATTERY_STRENGTH
	INIT_LIST_HEAD(&hdev->batteries);
#endif

	ret = hid_bpf_device_init(hdev);
	if (ret)
		goto out_err;

	return hdev;

out_err:
	hid_destroy_device(hdev);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(hid_allocate_device);

static void hid_remove_device(struct hid_device *hdev)
{
	if (hdev->status & HID_STAT_ADDED) {
		device_del(&hdev->dev);
		hid_debug_unregister(hdev);
		hdev->status &= ~HID_STAT_ADDED;
	}
	hid_free_bpf_rdesc(hdev);
	kfree(hdev->dev_rdesc);
	hdev->dev_rdesc = NULL;
	hdev->dev_rsize = 0;
	hdev->bpf_rsize = 0;
}

/**
 * hid_destroy_device - free previously allocated device
 *
 * @hdev: hid device
 *
 * If you allocate hid_device through hid_allocate_device, you should ever
 * free by this function.
 */
void hid_destroy_device(struct hid_device *hdev)
{
	hid_bpf_destroy_device(hdev);
	hid_remove_device(hdev);
	put_device(&hdev->dev);
}
EXPORT_SYMBOL_GPL(hid_destroy_device);


static int __hid_bus_reprobe_drivers(struct device *dev, void *data)
{
	struct hid_driver *hdrv = data;
	struct hid_device *hdev = to_hid_device(dev);