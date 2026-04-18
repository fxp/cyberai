// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 15/16



static struct hid_input *hidinput_match_application(struct hid_report *report)
{
	struct hid_device *hid = report->device;
	struct hid_input *hidinput;

	list_for_each_entry(hidinput, &hid->inputs, list) {
		if (hidinput->application == report->application)
			return hidinput;

		/*
		 * Keep SystemControl and ConsumerControl applications together
		 * with the main keyboard, if present.
		 */
		if ((report->application == HID_GD_SYSTEM_CONTROL ||
		     report->application == HID_CP_CONSUMER_CONTROL) &&
		    hidinput->application == HID_GD_KEYBOARD) {
			return hidinput;
		}
	}

	return NULL;
}

static inline void hidinput_configure_usages(struct hid_input *hidinput,
					     struct hid_report *report)
{
	int i, j, k;
	int first_field_index = 0;
	int slot_collection_index = -1;
	int prev_collection_index = -1;
	unsigned int slot_idx = 0;
	struct hid_field *field;

	/*
	 * First tag all the fields that are part of a slot,
	 * a slot needs to have one Contact ID in the collection
	 */
	for (i = 0; i < report->maxfield; i++) {
		field = report->field[i];

		/* ignore fields without usage */
		if (field->maxusage < 1)
			continue;

		/*
		 * janitoring when collection_index changes
		 */
		if (prev_collection_index != field->usage->collection_index) {
			prev_collection_index = field->usage->collection_index;
			first_field_index = i;
		}

		/*
		 * if we already found a Contact ID in the collection,
		 * tag and continue to the next.
		 */
		if (slot_collection_index == field->usage->collection_index) {
			field->slot_idx = slot_idx;
			continue;
		}

		/* check if the current field has Contact ID */
		for (j = 0; j < field->maxusage; j++) {
			if (field->usage[j].hid == HID_DG_CONTACTID) {
				slot_collection_index = field->usage->collection_index;
				slot_idx++;

				/*
				 * mark all previous fields and this one in the
				 * current collection to be slotted.
				 */
				for (k = first_field_index; k <= i; k++)
					report->field[k]->slot_idx = slot_idx;
				break;
			}
		}
	}

	for (i = 0; i < report->maxfield; i++)
		for (j = 0; j < report->field[i]->maxusage; j++)
			hidinput_configure_usage(hidinput, report->field[i],
						 report->field[i]->usage + j,
						 j);
}

/*
 * Register the input device; print a message.
 * Configure the input layer interface
 * Read all reports and initialize the absolute field values.
 */

int hidinput_connect(struct hid_device *hid, unsigned int force)
{
	struct hid_driver *drv = hid->driver;
	struct hid_report *report;
	struct hid_input *next, *hidinput = NULL;
	unsigned int application;
	int i, k;

	INIT_LIST_HEAD(&hid->inputs);
	INIT_WORK(&hid->led_work, hidinput_led_worker);

	hid->status &= ~HID_STAT_DUP_DETECTED;

	if (!force) {
		for (i = 0; i < hid->maxcollection; i++) {
			struct hid_collection *col = &hid->collection[i];
			if (col->type == HID_COLLECTION_APPLICATION ||
					col->type == HID_COLLECTION_PHYSICAL)
				if (IS_INPUT_APPLICATION(col->usage))
					break;
		}

		if (i == hid->maxcollection)
			return -1;
	}

	report_features(hid);

	for (k = HID_INPUT_REPORT; k <= HID_OUTPUT_REPORT; k++) {
		if (k == HID_OUTPUT_REPORT &&
			hid->quirks & HID_QUIRK_SKIP_OUTPUT_REPORTS)
			continue;

		list_for_each_entry(report, &hid->report_enum[k].report_list, list) {

			if (!report->maxfield)
				continue;

			application = report->application;

			/*
			 * Find the previous hidinput report attached
			 * to this report id.
			 */
			if (hid->quirks & HID_QUIRK_MULTI_INPUT)
				hidinput = hidinput_match(report);
			else if (hid->maxapplication > 1 &&
				 (hid->quirks & HID_QUIRK_INPUT_PER_APP))
				hidinput = hidinput_match_application(report);

			if (!hidinput) {
				hidinput = hidinput_allocate(hid, application);
				if (!hidinput)
					goto out_unwind;
			}

			hidinput_configure_usages(hidinput, report);

			if (hid->quirks & HID_QUIRK_MULTI_INPUT)
				hidinput->report = report;

			list_add_tail(&report->hidinput_list,
				      &hidinput->reports);
		}
	}

	hidinput_change_resolution_multipliers(hid);

	list_for_each_entry_safe(hidinput, next, &hid->inputs, list) {
		if (drv->input_configured &&
		    drv->input_configured(hid, hidinput))
			goto out_unwind;

		if (!hidinput_has_been_populated(hidinput)) {
			/* no need to register an input device not populated */
			hidinput_cleanup_hidinput(hid, hidinput);
			continue;
		}

		if (input_register_device(hidinput->input))
			goto out_unwind;
		hidinput->registered = true;
	}

	if (list_empty(&hid->inputs)) {
		hid_dbg(hid, "No inputs registered, leaving\n");
		goto out_unwind;
	}

	if (hid->status & HID_STAT_DUP_DETECTED)
		hid_dbg(hid,
			"Some usages could not be mapped, please use HID_QUIRK_INCREMENT_USAGE_ON_DUPLICATE if this is legitimate.\n");

	return 0;

out_unwind:
	/* unwind the ones we already registered */
	hidinput_disconnect(hid);

	return -1;
}
EXPORT_SYMBOL_GPL(hidinput_connect);

void hidinput_disconnect(struct hid_device *hid)
{
	struct hid_input *hidinput, *next;