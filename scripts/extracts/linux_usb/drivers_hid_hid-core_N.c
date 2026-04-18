// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 14/17



/**
 * hid_hw_request - send report request to device
 *
 * @hdev: hid device
 * @report: report to send
 * @reqtype: hid request type
 */
void hid_hw_request(struct hid_device *hdev,
		    struct hid_report *report, enum hid_class_request reqtype)
{
	if (hdev->ll_driver->request)
		return hdev->ll_driver->request(hdev, report, reqtype);

	__hid_request(hdev, report, reqtype);
}
EXPORT_SYMBOL_GPL(hid_hw_request);

int __hid_hw_raw_request(struct hid_device *hdev,
			 unsigned char reportnum, __u8 *buf,
			 size_t len, enum hid_report_type rtype,
			 enum hid_class_request reqtype,
			 u64 source, bool from_bpf)
{
	unsigned int max_buffer_size = HID_MAX_BUFFER_SIZE;
	int ret;

	if (hdev->ll_driver->max_buffer_size)
		max_buffer_size = hdev->ll_driver->max_buffer_size;

	if (len < 1 || len > max_buffer_size || !buf)
		return -EINVAL;

	ret = dispatch_hid_bpf_raw_requests(hdev, reportnum, buf, len, rtype,
					    reqtype, source, from_bpf);
	if (ret)
		return ret;

	return hdev->ll_driver->raw_request(hdev, reportnum, buf, len,
					    rtype, reqtype);
}

/**
 * hid_hw_raw_request - send report request to device
 *
 * @hdev: hid device
 * @reportnum: report ID
 * @buf: in/out data to transfer
 * @len: length of buf
 * @rtype: HID report type
 * @reqtype: HID_REQ_GET_REPORT or HID_REQ_SET_REPORT
 *
 * Return: count of data transferred, negative if error
 *
 * Same behavior as hid_hw_request, but with raw buffers instead.
 */
int hid_hw_raw_request(struct hid_device *hdev,
		       unsigned char reportnum, __u8 *buf,
		       size_t len, enum hid_report_type rtype, enum hid_class_request reqtype)
{
	return __hid_hw_raw_request(hdev, reportnum, buf, len, rtype, reqtype, 0, false);
}
EXPORT_SYMBOL_GPL(hid_hw_raw_request);

int __hid_hw_output_report(struct hid_device *hdev, __u8 *buf, size_t len, u64 source,
			   bool from_bpf)
{
	unsigned int max_buffer_size = HID_MAX_BUFFER_SIZE;
	int ret;

	if (hdev->ll_driver->max_buffer_size)
		max_buffer_size = hdev->ll_driver->max_buffer_size;

	if (len < 1 || len > max_buffer_size || !buf)
		return -EINVAL;

	ret = dispatch_hid_bpf_output_report(hdev, buf, len, source, from_bpf);
	if (ret)
		return ret;

	if (hdev->ll_driver->output_report)
		return hdev->ll_driver->output_report(hdev, buf, len);

	return -ENOSYS;
}

/**
 * hid_hw_output_report - send output report to device
 *
 * @hdev: hid device
 * @buf: raw data to transfer
 * @len: length of buf
 *
 * Return: count of data transferred, negative if error
 */
int hid_hw_output_report(struct hid_device *hdev, __u8 *buf, size_t len)
{
	return __hid_hw_output_report(hdev, buf, len, 0, false);
}
EXPORT_SYMBOL_GPL(hid_hw_output_report);

#ifdef CONFIG_PM
int hid_driver_suspend(struct hid_device *hdev, pm_message_t state)
{
	if (hdev->driver && hdev->driver->suspend)
		return hdev->driver->suspend(hdev, state);

	return 0;
}
EXPORT_SYMBOL_GPL(hid_driver_suspend);

int hid_driver_reset_resume(struct hid_device *hdev)
{
	if (hdev->driver && hdev->driver->reset_resume)
		return hdev->driver->reset_resume(hdev);

	return 0;
}
EXPORT_SYMBOL_GPL(hid_driver_reset_resume);

int hid_driver_resume(struct hid_device *hdev)
{
	if (hdev->driver && hdev->driver->resume)
		return hdev->driver->resume(hdev);

	return 0;
}
EXPORT_SYMBOL_GPL(hid_driver_resume);
#endif /* CONFIG_PM */

struct hid_dynid {
	struct list_head list;
	struct hid_device_id id;
};

/**
 * new_id_store - add a new HID device ID to this driver and re-probe devices
 * @drv: target device driver
 * @buf: buffer for scanning device ID data
 * @count: input size
 *
 * Adds a new dynamic hid device ID to this driver,
 * and causes the driver to probe for all devices again.
 */
static ssize_t new_id_store(struct device_driver *drv, const char *buf,
		size_t count)
{
	struct hid_driver *hdrv = to_hid_driver(drv);
	struct hid_dynid *dynid;
	__u32 bus, vendor, product;
	unsigned long driver_data = 0;
	int ret;

	ret = sscanf(buf, "%x %x %x %lx",
			&bus, &vendor, &product, &driver_data);
	if (ret < 3)
		return -EINVAL;

	dynid = kzalloc_obj(*dynid);
	if (!dynid)
		return -ENOMEM;

	dynid->id.bus = bus;
	dynid->id.group = HID_GROUP_ANY;
	dynid->id.vendor = vendor;
	dynid->id.product = product;
	dynid->id.driver_data = driver_data;

	spin_lock(&hdrv->dyn_lock);
	list_add_tail(&dynid->list, &hdrv->dyn_list);
	spin_unlock(&hdrv->dyn_lock);

	ret = driver_attach(&hdrv->driver);

	return ret ? : count;
}
static DRIVER_ATTR_WO(new_id);

static struct attribute *hid_drv_attrs[] = {
	&driver_attr_new_id.attr,
	NULL,
};
ATTRIBUTE_GROUPS(hid_drv);

static void hid_free_dynids(struct hid_driver *hdrv)
{
	struct hid_dynid *dynid, *n;

	spin_lock(&hdrv->dyn_lock);
	list_for_each_entry_safe(dynid, n, &hdrv->dyn_list, list) {
		list_del(&dynid->list);
		kfree(dynid);
	}
	spin_unlock(&hdrv->dyn_lock);
}

const struct hid_device_id *hid_match_device(struct hid_device *hdev,
					     struct hid_driver *hdrv)
{
	struct hid_dynid *dynid;