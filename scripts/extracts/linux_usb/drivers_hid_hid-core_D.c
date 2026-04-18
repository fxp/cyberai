// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 4/17



		if (parser->local.delimiter_branch > 1) {
			dbg_hid("alternative usage ignored\n");
			return 0;
		}

		parser->local.usage_minimum = data;
		return 0;

	case HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:

		if (parser->local.delimiter_branch > 1) {
			dbg_hid("alternative usage ignored\n");
			return 0;
		}

		count = data - parser->local.usage_minimum;
		if (count + parser->local.usage_index >= HID_MAX_USAGES) {
			/*
			 * We do not warn if the name is not set, we are
			 * actually pre-scanning the device.
			 */
			if (dev_name(&parser->device->dev))
				hid_warn(parser->device,
					 "ignoring exceeding usage max\n");
			data = HID_MAX_USAGES - parser->local.usage_index +
				parser->local.usage_minimum - 1;
			if (data <= 0) {
				hid_err(parser->device,
					"no more usage index available\n");
				return -1;
			}
		}

		for (n = parser->local.usage_minimum; n <= data; n++)
			if (hid_add_usage(parser, n, item->size)) {
				dbg_hid("hid_add_usage failed\n");
				return -1;
			}
		return 0;

	default:

		dbg_hid("unknown local item tag 0x%x\n", item->tag);
		return 0;
	}
	return 0;
}

/*
 * Concatenate Usage Pages into Usages where relevant:
 * As per specification, 6.2.2.8: "When the parser encounters a main item it
 * concatenates the last declared Usage Page with a Usage to form a complete
 * usage value."
 */

static void hid_concatenate_last_usage_page(struct hid_parser *parser)
{
	int i;
	unsigned int usage_page;
	unsigned int current_page;

	if (!parser->local.usage_index)
		return;

	usage_page = parser->global.usage_page;

	/*
	 * Concatenate usage page again only if last declared Usage Page
	 * has not been already used in previous usages concatenation
	 */
	for (i = parser->local.usage_index - 1; i >= 0; i--) {
		if (parser->local.usage_size[i] > 2)
			/* Ignore extended usages */
			continue;

		current_page = parser->local.usage[i] >> 16;
		if (current_page == usage_page)
			break;

		complete_usage(parser, i);
	}
}

/*
 * Process a main item.
 */

static int hid_parser_main(struct hid_parser *parser, struct hid_item *item)
{
	__u32 data;
	int ret;

	hid_concatenate_last_usage_page(parser);

	data = item_udata(item);

	switch (item->tag) {
	case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
		ret = open_collection(parser, data & 0xff);
		break;
	case HID_MAIN_ITEM_TAG_END_COLLECTION:
		ret = close_collection(parser);
		break;
	case HID_MAIN_ITEM_TAG_INPUT:
		ret = hid_add_field(parser, HID_INPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_OUTPUT:
		ret = hid_add_field(parser, HID_OUTPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_FEATURE:
		ret = hid_add_field(parser, HID_FEATURE_REPORT, data);
		break;
	default:
		if (item->tag >= HID_MAIN_ITEM_TAG_RESERVED_MIN &&
			item->tag <= HID_MAIN_ITEM_TAG_RESERVED_MAX)
			hid_warn_ratelimited(parser->device, "reserved main item tag 0x%x\n", item->tag);
		else
			hid_warn_ratelimited(parser->device, "unknown main item tag 0x%x\n", item->tag);
		ret = 0;
	}

	memset(&parser->local, 0, sizeof(parser->local));	/* Reset the local parser environment */

	return ret;
}

/*
 * Process a reserved item.
 */

static int hid_parser_reserved(struct hid_parser *parser, struct hid_item *item)
{
	dbg_hid("reserved item type, tag 0x%x\n", item->tag);
	return 0;
}

/*
 * Free a report and all registered fields. The field->usage and
 * field->value table's are allocated behind the field, so we need
 * only to free(field) itself.
 */

static void hid_free_report(struct hid_report *report)
{
	unsigned n;

	kfree(report->field_entries);

	for (n = 0; n < report->maxfield; n++)
		kvfree(report->field[n]);
	kfree(report);
}

/*
 * Close report. This function returns the device
 * state to the point prior to hid_open_report().
 */
static void hid_close_report(struct hid_device *device)
{
	unsigned i, j;

	for (i = 0; i < HID_REPORT_TYPES; i++) {
		struct hid_report_enum *report_enum = device->report_enum + i;

		for (j = 0; j < HID_MAX_IDS; j++) {
			struct hid_report *report = report_enum->report_id_hash[j];
			if (report)
				hid_free_report(report);
		}
		memset(report_enum, 0, sizeof(*report_enum));
		INIT_LIST_HEAD(&report_enum->report_list);
	}

	/*
	 * If the HID driver had a rdesc_fixup() callback, dev->rdesc
	 * will be allocated by hid-core and needs to be freed.
	 * Otherwise, it is either equal to dev_rdesc or bpf_rdesc, in
	 * which cases it'll be freed later on device removal or destroy.
	 */
	if (device->rdesc != device->dev_rdesc && device->rdesc != device->bpf_rdesc)
		kfree(device->rdesc);
	device->rdesc = NULL;
	device->rsize = 0;

	kfree(device->collection);
	device->collection = NULL;
	device->collection_size = 0;
	device->maxcollection = 0;
	device->maxapplication = 0;

	device->status &= ~HID_STAT_PARSED;
}

static inline void hid_free_bpf_rdesc(struct hid_device *hdev)
{
	/* bpf_rdesc is either equal to dev_rdesc or allocated by call_hid_bpf_rdesc_fixup() */
	if (hdev->bpf_rdesc != hdev->dev_rdesc)
		kfree(hdev->bpf_rdesc);
	hdev->bpf_rdesc = NULL;
}