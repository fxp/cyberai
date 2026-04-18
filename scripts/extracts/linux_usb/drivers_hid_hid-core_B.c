// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 2/17



	collection_index = parser->device->maxcollection++;
	collection = parser->device->collection + collection_index;
	collection->type = type;
	collection->usage = usage;
	collection->level = parser->collection_stack_ptr - 1;
	collection->parent_idx = (collection->level == 0) ? -1 :
		parser->collection_stack[collection->level - 1];

	if (type == HID_COLLECTION_APPLICATION)
		parser->device->maxapplication++;

	return 0;
}

/*
 * Close a collection.
 */

static int close_collection(struct hid_parser *parser)
{
	if (!parser->collection_stack_ptr) {
		hid_err(parser->device, "collection stack underflow\n");
		return -EINVAL;
	}
	parser->collection_stack_ptr--;
	return 0;
}

/*
 * Climb up the stack, search for the specified collection type
 * and return the usage.
 */

static unsigned hid_lookup_collection(struct hid_parser *parser, unsigned type)
{
	struct hid_collection *collection = parser->device->collection;
	int n;

	for (n = parser->collection_stack_ptr - 1; n >= 0; n--) {
		unsigned index = parser->collection_stack[n];
		if (collection[index].type == type)
			return collection[index].usage;
	}
	return 0; /* we know nothing about this usage type */
}

/*
 * Concatenate usage which defines 16 bits or less with the
 * currently defined usage page to form a 32 bit usage
 */

static void complete_usage(struct hid_parser *parser, unsigned int index)
{
	parser->local.usage[index] &= 0xFFFF;
	parser->local.usage[index] |=
		(parser->global.usage_page & 0xFFFF) << 16;
}

/*
 * Add a usage to the temporary parser table.
 */

static int hid_add_usage(struct hid_parser *parser, unsigned usage, u8 size)
{
	if (parser->local.usage_index >= HID_MAX_USAGES) {
		hid_err(parser->device, "usage index exceeded\n");
		return -1;
	}
	parser->local.usage[parser->local.usage_index] = usage;

	/*
	 * If Usage item only includes usage id, concatenate it with
	 * currently defined usage page
	 */
	if (size <= 2)
		complete_usage(parser, parser->local.usage_index);

	parser->local.usage_size[parser->local.usage_index] = size;
	parser->local.collection_index[parser->local.usage_index] =
		parser->collection_stack_ptr ?
		parser->collection_stack[parser->collection_stack_ptr - 1] : 0;
	parser->local.usage_index++;
	return 0;
}

/*
 * Register a new field for this report.
 */

static int hid_add_field(struct hid_parser *parser, unsigned report_type, unsigned flags)
{
	struct hid_report *report;
	struct hid_field *field;
	unsigned int max_buffer_size = HID_MAX_BUFFER_SIZE;
	unsigned int usages;
	unsigned int offset;
	unsigned int i;
	unsigned int application;

	application = hid_lookup_collection(parser, HID_COLLECTION_APPLICATION);

	report = hid_register_report(parser->device, report_type,
				     parser->global.report_id, application);
	if (!report) {
		hid_err(parser->device, "hid_register_report failed\n");
		return -1;
	}

	/* Handle both signed and unsigned cases properly */
	if ((parser->global.logical_minimum < 0 &&
		parser->global.logical_maximum <
		parser->global.logical_minimum) ||
		(parser->global.logical_minimum >= 0 &&
		(__u32)parser->global.logical_maximum <
		(__u32)parser->global.logical_minimum)) {
		dbg_hid("logical range invalid 0x%x 0x%x\n",
			parser->global.logical_minimum,
			parser->global.logical_maximum);
		return -1;
	}

	offset = report->size;
	report->size += parser->global.report_size * parser->global.report_count;

	if (parser->device->ll_driver->max_buffer_size)
		max_buffer_size = parser->device->ll_driver->max_buffer_size;

	/* Total size check: Allow for possible report index byte */
	if (report->size > (max_buffer_size - 1) << 3) {
		hid_err(parser->device, "report is too long\n");
		return -1;
	}

	if (!parser->local.usage_index) /* Ignore padding fields */
		return 0;

	usages = max_t(unsigned, parser->local.usage_index,
				 parser->global.report_count);

	field = hid_register_field(report, usages);
	if (!field)
		return 0;

	field->physical = hid_lookup_collection(parser, HID_COLLECTION_PHYSICAL);
	field->logical = hid_lookup_collection(parser, HID_COLLECTION_LOGICAL);
	field->application = application;

	for (i = 0; i < usages; i++) {
		unsigned j = i;
		/* Duplicate the last usage we parsed if we have excess values */
		if (i >= parser->local.usage_index)
			j = parser->local.usage_index - 1;
		field->usage[i].hid = parser->local.usage[j];
		field->usage[i].collection_index =
			parser->local.collection_index[j];
		field->usage[i].usage_index = i;
		field->usage[i].resolution_multiplier = 1;
	}