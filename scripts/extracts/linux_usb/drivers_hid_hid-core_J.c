// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 10/17



		if (hid_array_value_is_valid(field, value[n]) &&
		    search(field->value, value[n], count))
			hid_process_event(hid,
					  field,
					  &field->usage[value[n] - min],
					  1,
					  interrupt);
	}

	memcpy(field->value, value, count * sizeof(__s32));
}

/*
 * Analyse a received report, and fetch the data from it. The field
 * content is stored for next report processing (we do differential
 * reporting to the layer).
 */
static void hid_process_report(struct hid_device *hid,
			       struct hid_report *report,
			       __u8 *data,
			       int interrupt)
{
	unsigned int a;
	struct hid_field_entry *entry;
	struct hid_field *field;

	/* first retrieve all incoming values in data */
	for (a = 0; a < report->maxfield; a++)
		hid_input_fetch_field(hid, report->field[a], data);

	if (!list_empty(&report->field_entry_list)) {
		/* INPUT_REPORT, we have a priority list of fields */
		list_for_each_entry(entry,
				    &report->field_entry_list,
				    list) {
			field = entry->field;

			if (field->flags & HID_MAIN_ITEM_VARIABLE)
				hid_process_event(hid,
						  field,
						  &field->usage[entry->index],
						  field->new_value[entry->index],
						  interrupt);
			else
				hid_input_array_field(hid, field, interrupt);
		}

		/* we need to do the memcpy at the end for var items */
		for (a = 0; a < report->maxfield; a++) {
			field = report->field[a];

			if (field->flags & HID_MAIN_ITEM_VARIABLE)
				memcpy(field->value, field->new_value,
				       field->report_count * sizeof(__s32));
		}
	} else {
		/* FEATURE_REPORT, regular processing */
		for (a = 0; a < report->maxfield; a++) {
			field = report->field[a];

			if (field->flags & HID_MAIN_ITEM_VARIABLE)
				hid_input_var_field(hid, field, interrupt);
			else
				hid_input_array_field(hid, field, interrupt);
		}
	}
}

/*
 * Insert a given usage_index in a field in the list
 * of processed usages in the report.
 *
 * The elements of lower priority score are processed
 * first.
 */
static void __hid_insert_field_entry(struct hid_device *hid,
				     struct hid_report *report,
				     struct hid_field_entry *entry,
				     struct hid_field *field,
				     unsigned int usage_index)
{
	struct hid_field_entry *next;

	entry->field = field;
	entry->index = usage_index;
	entry->priority = field->usages_priorities[usage_index];

	/* insert the element at the correct position */
	list_for_each_entry(next,
			    &report->field_entry_list,
			    list) {
		/*
		 * the priority of our element is strictly higher
		 * than the next one, insert it before
		 */
		if (entry->priority > next->priority) {
			list_add_tail(&entry->list, &next->list);
			return;
		}
	}

	/* lowest priority score: insert at the end */
	list_add_tail(&entry->list, &report->field_entry_list);
}

static void hid_report_process_ordering(struct hid_device *hid,
					struct hid_report *report)
{
	struct hid_field *field;
	struct hid_field_entry *entries;
	unsigned int a, u, usages;
	unsigned int count = 0;

	/* count the number of individual fields in the report */
	for (a = 0; a < report->maxfield; a++) {
		field = report->field[a];

		if (field->flags & HID_MAIN_ITEM_VARIABLE)
			count += field->report_count;
		else
			count++;
	}

	/* allocate the memory to process the fields */
	entries = kzalloc_objs(*entries, count);
	if (!entries)
		return;

	report->field_entries = entries;

	/*
	 * walk through all fields in the report and
	 * store them by priority order in report->field_entry_list
	 *
	 * - Var elements are individualized (field + usage_index)
	 * - Arrays are taken as one, we can not chose an order for them
	 */
	usages = 0;
	for (a = 0; a < report->maxfield; a++) {
		field = report->field[a];

		if (field->flags & HID_MAIN_ITEM_VARIABLE) {
			for (u = 0; u < field->report_count; u++) {
				__hid_insert_field_entry(hid, report,
							 &entries[usages],
							 field, u);
				usages++;
			}
		} else {
			__hid_insert_field_entry(hid, report, &entries[usages],
						 field, 0);
			usages++;
		}
	}
}

static void hid_process_ordering(struct hid_device *hid)
{
	struct hid_report *report;
	struct hid_report_enum *report_enum = &hid->report_enum[HID_INPUT_REPORT];

	list_for_each_entry(report, &report_enum->report_list, list)
		hid_report_process_ordering(hid, report);
}

/*
 * Output the field into the report.
 */

static void hid_output_field(const struct hid_device *hid,
			     struct hid_field *field, __u8 *data)
{
	unsigned count = field->report_count;
	unsigned offset = field->report_offset;
	unsigned size = field->report_size;
	unsigned n;

	for (n = 0; n < count; n++) {
		if (field->logical_minimum < 0)	/* signed values */
			implement(hid, data, offset + n * size, size,
				  s32ton(field->value[n], size));
		else				/* unsigned values */
			implement(hid, data, offset + n * size, size,
				  field->value[n]);
	}
}