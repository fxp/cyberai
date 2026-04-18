// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 5/17



/*
 * Free a device structure, all reports, and all fields.
 */

void hiddev_free(struct kref *ref)
{
	struct hid_device *hid = container_of(ref, struct hid_device, ref);

	hid_close_report(hid);
	hid_free_bpf_rdesc(hid);
	kfree(hid->dev_rdesc);
	kfree(hid);
}

static void hid_device_release(struct device *dev)
{
	struct hid_device *hid = to_hid_device(dev);

	kref_put(&hid->ref, hiddev_free);
}

/*
 * Fetch a report description item from the data stream. We support long
 * items, though they are not used yet.
 */

static const u8 *fetch_item(const __u8 *start, const __u8 *end, struct hid_item *item)
{
	u8 b;

	if ((end - start) <= 0)
		return NULL;

	b = *start++;

	item->type = (b >> 2) & 3;
	item->tag  = (b >> 4) & 15;

	if (item->tag == HID_ITEM_TAG_LONG) {

		item->format = HID_ITEM_FORMAT_LONG;

		if ((end - start) < 2)
			return NULL;

		item->size = *start++;
		item->tag  = *start++;

		if ((end - start) < item->size)
			return NULL;

		item->data.longdata = start;
		start += item->size;
		return start;
	}

	item->format = HID_ITEM_FORMAT_SHORT;
	item->size = BIT(b & 3) >> 1; /* 0, 1, 2, 3 -> 0, 1, 2, 4 */

	if (end - start < item->size)
		return NULL;

	switch (item->size) {
	case 0:
		break;

	case 1:
		item->data.u8 = *start;
		break;

	case 2:
		item->data.u16 = get_unaligned_le16(start);
		break;

	case 4:
		item->data.u32 = get_unaligned_le32(start);
		break;
	}

	return start + item->size;
}

static void hid_scan_input_usage(struct hid_parser *parser, u32 usage)
{
	struct hid_device *hid = parser->device;

	if (usage == HID_DG_CONTACTID)
		hid->group = HID_GROUP_MULTITOUCH;
}

static void hid_scan_feature_usage(struct hid_parser *parser, u32 usage)
{
	if (usage == 0xff0000c5 && parser->global.report_count == 256 &&
	    parser->global.report_size == 8)
		parser->scan_flags |= HID_SCAN_FLAG_MT_WIN_8;

	if (usage == 0xff0000c6 && parser->global.report_count == 1 &&
	    parser->global.report_size == 8)
		parser->scan_flags |= HID_SCAN_FLAG_MT_WIN_8;
}

static void hid_scan_collection(struct hid_parser *parser, unsigned type)
{
	struct hid_device *hid = parser->device;
	int i;

	if (((parser->global.usage_page << 16) == HID_UP_SENSOR) &&
	    (type == HID_COLLECTION_PHYSICAL ||
	     type == HID_COLLECTION_APPLICATION))
		hid->group = HID_GROUP_SENSOR_HUB;

	if (hid->vendor == USB_VENDOR_ID_MICROSOFT &&
	    hid->product == USB_DEVICE_ID_MS_POWER_COVER &&
	    hid->group == HID_GROUP_MULTITOUCH)
		hid->group = HID_GROUP_GENERIC;

	if ((parser->global.usage_page << 16) == HID_UP_GENDESK)
		for (i = 0; i < parser->local.usage_index; i++)
			if (parser->local.usage[i] == HID_GD_POINTER)
				parser->scan_flags |= HID_SCAN_FLAG_GD_POINTER;

	if ((parser->global.usage_page << 16) >= HID_UP_MSVENDOR)
		parser->scan_flags |= HID_SCAN_FLAG_VENDOR_SPECIFIC;

	if ((parser->global.usage_page << 16) == HID_UP_GOOGLEVENDOR)
		for (i = 0; i < parser->local.usage_index; i++)
			if (parser->local.usage[i] ==
					(HID_UP_GOOGLEVENDOR | 0x0001))
				parser->device->group =
					HID_GROUP_VIVALDI;
}

static int hid_scan_main(struct hid_parser *parser, struct hid_item *item)
{
	__u32 data;
	int i;

	hid_concatenate_last_usage_page(parser);

	data = item_udata(item);

	switch (item->tag) {
	case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
		hid_scan_collection(parser, data & 0xff);
		break;
	case HID_MAIN_ITEM_TAG_END_COLLECTION:
		break;
	case HID_MAIN_ITEM_TAG_INPUT:
		/* ignore constant inputs, they will be ignored by hid-input */
		if (data & HID_MAIN_ITEM_CONSTANT)
			break;
		for (i = 0; i < parser->local.usage_index; i++)
			hid_scan_input_usage(parser, parser->local.usage[i]);
		break;
	case HID_MAIN_ITEM_TAG_OUTPUT:
		break;
	case HID_MAIN_ITEM_TAG_FEATURE:
		for (i = 0; i < parser->local.usage_index; i++)
			hid_scan_feature_usage(parser, parser->local.usage[i]);
		break;
	}

	/* Reset the local parser environment */
	memset(&parser->local, 0, sizeof(parser->local));

	return 0;
}

/*
 * Scan a report descriptor before the device is added to the bus.
 * Sets device groups and other properties that determine what driver
 * to load.
 */
static int hid_scan_report(struct hid_device *hid)
{
	struct hid_item item;
	const __u8 *start = hid->dev_rdesc;
	const __u8 *end = start + hid->dev_rsize;
	static int (*dispatch_type[])(struct hid_parser *parser,
				      struct hid_item *item) = {
		hid_scan_main,
		hid_parser_global,
		hid_parser_local,
		hid_parser_reserved
	};

	struct hid_parser *parser __free(kvfree) = vzalloc(sizeof(*parser));
	if (!parser)
		return -ENOMEM;

	parser->device = hid;
	hid->group = HID_GROUP_GENERIC;

	/*
	 * In case we are re-scanning after a BPF has been loaded,
	 * we need to use the bpf report descriptor, not the original one.
	 */
	if (hid->bpf_rdesc && hid->bpf_rsize) {
		start = hid->bpf_rdesc;
		end = start + hid->bpf_rsize;
	}