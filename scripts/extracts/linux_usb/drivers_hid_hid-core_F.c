// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 6/17



	/*
	 * The parsing is simpler than the one in hid_open_report() as we should
	 * be robust against hid errors. Those errors will be raised by
	 * hid_open_report() anyway.
	 */
	while ((start = fetch_item(start, end, &item)) != NULL)
		dispatch_type[item.type](parser, &item);

	/*
	 * Handle special flags set during scanning.
	 */
	if ((parser->scan_flags & HID_SCAN_FLAG_MT_WIN_8) &&
	    (hid->group == HID_GROUP_MULTITOUCH))
		hid->group = HID_GROUP_MULTITOUCH_WIN_8;

	/*
	 * Vendor specific handlings
	 */
	switch (hid->vendor) {
	case USB_VENDOR_ID_WACOM:
		hid->group = HID_GROUP_WACOM;
		break;
	case USB_VENDOR_ID_SYNAPTICS:
		if (hid->group == HID_GROUP_GENERIC)
			if ((parser->scan_flags & HID_SCAN_FLAG_VENDOR_SPECIFIC)
			    && (parser->scan_flags & HID_SCAN_FLAG_GD_POINTER))
				/*
				 * hid-rmi should take care of them,
				 * not hid-generic
				 */
				hid->group = HID_GROUP_RMI;
		break;
	}

	kfree(parser->collection_stack);
	return 0;
}

/**
 * hid_parse_report - parse device report
 *
 * @hid: hid device
 * @start: report start
 * @size: report size
 *
 * Allocate the device report as read by the bus driver. This function should
 * only be called from parse() in ll drivers.
 */
int hid_parse_report(struct hid_device *hid, const __u8 *start, unsigned size)
{
	hid->dev_rdesc = kmemdup(start, size, GFP_KERNEL);
	if (!hid->dev_rdesc)
		return -ENOMEM;
	hid->dev_rsize = size;
	return 0;
}
EXPORT_SYMBOL_GPL(hid_parse_report);

static const char * const hid_report_names[] = {
	"HID_INPUT_REPORT",
	"HID_OUTPUT_REPORT",
	"HID_FEATURE_REPORT",
};
/**
 * hid_validate_values - validate existing device report's value indexes
 *
 * @hid: hid device
 * @type: which report type to examine
 * @id: which report ID to examine (0 for first)
 * @field_index: which report field to examine
 * @report_counts: expected number of values
 *
 * Validate the number of values in a given field of a given report, after
 * parsing.
 */
struct hid_report *hid_validate_values(struct hid_device *hid,
				       enum hid_report_type type, unsigned int id,
				       unsigned int field_index,
				       unsigned int report_counts)
{
	struct hid_report *report;

	if (type > HID_FEATURE_REPORT) {
		hid_err(hid, "invalid HID report type %u\n", type);
		return NULL;
	}

	if (id >= HID_MAX_IDS) {
		hid_err(hid, "invalid HID report id %u\n", id);
		return NULL;
	}

	/*
	 * Explicitly not using hid_get_report() here since it depends on
	 * ->numbered being checked, which may not always be the case when
	 * drivers go to access report values.
	 */
	if (id == 0) {
		/*
		 * Validating on id 0 means we should examine the first
		 * report in the list.
		 */
		report = list_first_entry_or_null(
				&hid->report_enum[type].report_list,
				struct hid_report, list);
	} else {
		report = hid->report_enum[type].report_id_hash[id];
	}
	if (!report) {
		hid_err(hid, "missing %s %u\n", hid_report_names[type], id);
		return NULL;
	}
	if (report->maxfield <= field_index) {
		hid_err(hid, "not enough fields in %s %u\n",
			hid_report_names[type], id);
		return NULL;
	}
	if (report->field[field_index]->report_count < report_counts) {
		hid_err(hid, "not enough values in %s %u field %u\n",
			hid_report_names[type], id, field_index);
		return NULL;
	}
	return report;
}
EXPORT_SYMBOL_GPL(hid_validate_values);

static int hid_calculate_multiplier(struct hid_device *hid,
				     struct hid_field *multiplier)
{
	int m;
	__s32 v = *multiplier->value;
	__s32 lmin = multiplier->logical_minimum;
	__s32 lmax = multiplier->logical_maximum;
	__s32 pmin = multiplier->physical_minimum;
	__s32 pmax = multiplier->physical_maximum;

	/*
	 * "Because OS implementations will generally divide the control's
	 * reported count by the Effective Resolution Multiplier, designers
	 * should take care not to establish a potential Effective
	 * Resolution Multiplier of zero."
	 * HID Usage Table, v1.12, Section 4.3.1, p31
	 */
	if (lmax - lmin == 0)
		return 1;
	/*
	 * Handling the unit exponent is left as an exercise to whoever
	 * finds a device where that exponent is not 0.
	 */
	m = ((v - lmin)/(lmax - lmin) * (pmax - pmin) + pmin);
	if (unlikely(multiplier->unit_exponent != 0)) {
		hid_warn(hid,
			 "unsupported Resolution Multiplier unit exponent %d\n",
			 multiplier->unit_exponent);
	}

	/* There are no devices with an effective multiplier > 255 */
	if (unlikely(m == 0 || m > 255 || m < -255)) {
		hid_warn(hid, "unsupported Resolution Multiplier %d\n", m);
		m = 1;
	}

	return m;
}

static void hid_apply_multiplier_to_field(struct hid_device *hid,
					  struct hid_field *field,
					  struct hid_collection *multiplier_collection,
					  int effective_multiplier)
{
	struct hid_collection *collection;
	struct hid_usage *usage;
	int i;