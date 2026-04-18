// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 11/17



/*
 * Compute the size of a report.
 */
static size_t hid_compute_report_size(struct hid_report *report)
{
	if (report->size)
		return ((report->size - 1) >> 3) + 1;

	return 0;
}

/*
 * Create a report. 'data' has to be allocated using
 * hid_alloc_report_buf() so that it has proper size.
 */

void hid_output_report(struct hid_report *report, __u8 *data)
{
	unsigned n;

	if (report->id > 0)
		*data++ = report->id;

	memset(data, 0, hid_compute_report_size(report));
	for (n = 0; n < report->maxfield; n++)
		hid_output_field(report->device, report->field[n], data);
}
EXPORT_SYMBOL_GPL(hid_output_report);

/*
 * Allocator for buffer that is going to be passed to hid_output_report()
 */
u8 *hid_alloc_report_buf(struct hid_report *report, gfp_t flags)
{
	/*
	 * 7 extra bytes are necessary to achieve proper functionality
	 * of implement() working on 8 byte chunks
	 * 1 extra byte for the report ID if it is null (not used) so
	 * we can reserve that extra byte in the first position of the buffer
	 * when sending it to .raw_request()
	 */

	u32 len = hid_report_len(report) + 7 + (report->id == 0);

	return kzalloc(len, flags);
}
EXPORT_SYMBOL_GPL(hid_alloc_report_buf);

/*
 * Set a field value. The report this field belongs to has to be
 * created and transferred to the device, to set this value in the
 * device.
 */

int hid_set_field(struct hid_field *field, unsigned offset, __s32 value)
{
	unsigned size;

	if (!field)
		return -1;

	size = field->report_size;

	hid_dump_input(field->report->device, field->usage + offset, value);

	if (offset >= field->report_count) {
		hid_err(field->report->device, "offset (%d) exceeds report_count (%d)\n",
				offset, field->report_count);
		return -1;
	}
	if (field->logical_minimum < 0) {
		if (value != snto32(s32ton(value, size), size)) {
			hid_err(field->report->device, "value %d is out of range\n", value);
			return -1;
		}
	}
	field->value[offset] = value;
	return 0;
}
EXPORT_SYMBOL_GPL(hid_set_field);

struct hid_field *hid_find_field(struct hid_device *hdev, unsigned int report_type,
				 unsigned int application, unsigned int usage)
{
	struct list_head *report_list = &hdev->report_enum[report_type].report_list;
	struct hid_report *report;
	int i, j;

	list_for_each_entry(report, report_list, list) {
		if (report->application != application)
			continue;

		for (i = 0; i < report->maxfield; i++) {
			struct hid_field *field = report->field[i];

			for (j = 0; j < field->maxusage; j++) {
				if (field->usage[j].hid == usage)
					return field;
			}
		}
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(hid_find_field);

static struct hid_report *hid_get_report(struct hid_report_enum *report_enum,
		const u8 *data)
{
	struct hid_report *report;
	unsigned int n = 0;	/* Normally report number is 0 */

	/* Device uses numbered reports, data[0] is report number */
	if (report_enum->numbered)
		n = *data;

	report = report_enum->report_id_hash[n];
	if (report == NULL)
		dbg_hid("undefined report_id %u received\n", n);

	return report;
}

/*
 * Implement a generic .request() callback, using .raw_request()
 * DO NOT USE in hid drivers directly, but through hid_hw_request instead.
 */
int __hid_request(struct hid_device *hid, struct hid_report *report,
		enum hid_class_request reqtype)
{
	u8 *data_buf;
	int ret;
	u32 len;

	u8 *buf __free(kfree) = hid_alloc_report_buf(report, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	data_buf = buf;
	len = hid_report_len(report);

	if (report->id == 0) {
		/* reserve the first byte for the report ID */
		data_buf++;
		len++;
	}

	if (reqtype == HID_REQ_SET_REPORT)
		hid_output_report(report, data_buf);

	ret = hid_hw_raw_request(hid, report->id, buf, len, report->type, reqtype);
	if (ret < 0) {
		dbg_hid("unable to complete request: %d\n", ret);
		return ret;
	}

	if (reqtype == HID_REQ_GET_REPORT)
		hid_input_report(hid, report->type, buf, ret, 0);

	return 0;
}
EXPORT_SYMBOL_GPL(__hid_request);

int hid_report_raw_event(struct hid_device *hid, enum hid_report_type type, u8 *data, u32 size,
			 int interrupt)
{
	struct hid_report_enum *report_enum = hid->report_enum + type;
	struct hid_report *report;
	struct hid_driver *hdrv;
	int max_buffer_size = HID_MAX_BUFFER_SIZE;
	u32 rsize, csize = size;
	u8 *cdata = data;
	int ret = 0;

	report = hid_get_report(report_enum, data);
	if (!report)
		goto out;

	if (report_enum->numbered) {
		cdata++;
		csize--;
	}

	rsize = hid_compute_report_size(report);

	if (hid->ll_driver->max_buffer_size)
		max_buffer_size = hid->ll_driver->max_buffer_size;

	if (report_enum->numbered && rsize >= max_buffer_size)
		rsize = max_buffer_size - 1;
	else if (rsize > max_buffer_size)
		rsize = max_buffer_size;

	if (csize < rsize) {
		hid_warn_ratelimited(hid, "Event data for report %d was too short (%d vs %d)\n",
				     report->id, rsize, csize);
		ret = -EINVAL;
		goto out;
	}