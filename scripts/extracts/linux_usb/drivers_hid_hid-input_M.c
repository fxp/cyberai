// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 13/16



	list_for_each_entry(report,
			    &hid->report_enum[HID_OUTPUT_REPORT].report_list,
			    list) {
		for (i = 0; i < report->maxfield; i++) {
			field = report->field[i];
			for (j = 0; j < field->maxusage; j++)
				if (field->usage[j].type == EV_LED &&
				    field->value[j])
					count += 1;
		}
	}
	return count;
}
EXPORT_SYMBOL_GPL(hidinput_count_leds);

static void hidinput_led_worker(struct work_struct *work)
{
	struct hid_device *hid = container_of(work, struct hid_device,
					      led_work);
	struct hid_field *field;
	struct hid_report *report;
	int ret;
	u32 len;

	field = hidinput_get_led_field(hid);
	if (!field)
		return;

	/*
	 * field->report is accessed unlocked regarding HID core. So there might
	 * be another incoming SET-LED request from user-space, which changes
	 * the LED state while we assemble our outgoing buffer. However, this
	 * doesn't matter as hid_output_report() correctly converts it into a
	 * boolean value no matter what information is currently set on the LED
	 * field (even garbage). So the remote device will always get a valid
	 * request.
	 * And in case we send a wrong value, a next led worker is spawned
	 * for every SET-LED request so the following worker will send the
	 * correct value, guaranteed!
	 */

	report = field->report;

	/* use custom SET_REPORT request if possible (asynchronous) */
	if (hid->ll_driver->request)
		return hid->ll_driver->request(hid, report, HID_REQ_SET_REPORT);

	/* fall back to generic raw-output-report */
	len = hid_report_len(report);
	u8 *buf __free(kfree) = hid_alloc_report_buf(report, GFP_KERNEL);
	if (!buf)
		return;

	hid_output_report(report, buf);
	/* synchronous output report */
	ret = hid_hw_output_report(hid, buf, len);
	if (ret == -ENOSYS)
		hid_hw_raw_request(hid, report->id, buf, len, HID_OUTPUT_REPORT,
				HID_REQ_SET_REPORT);
}

static int hidinput_input_event(struct input_dev *dev, unsigned int type,
				unsigned int code, int value)
{
	struct hid_device *hid = input_get_drvdata(dev);
	struct hid_field *field;
	int offset;

	if (type == EV_FF)
		return input_ff_event(dev, type, code, value);

	if (type != EV_LED)
		return -1;

	if ((offset = hidinput_find_field(hid, type, code, &field)) == -1) {
		hid_warn(dev, "event field not found\n");
		return -1;
	}

	hid_set_field(field, offset, value);

	schedule_work(&hid->led_work);
	return 0;
}

static int hidinput_open(struct input_dev *dev)
{
	struct hid_device *hid = input_get_drvdata(dev);

	return hid_hw_open(hid);
}

static void hidinput_close(struct input_dev *dev)
{
	struct hid_device *hid = input_get_drvdata(dev);

	hid_hw_close(hid);
}

static bool __hidinput_change_resolution_multipliers(struct hid_device *hid,
		struct hid_report *report, bool use_logical_max)
{
	struct hid_usage *usage;
	bool update_needed = false;
	bool get_report_completed = false;
	int i, j;

	if (report->maxfield == 0)
		return false;

	for (i = 0; i < report->maxfield; i++) {
		__s32 value = use_logical_max ?
			      report->field[i]->logical_maximum :
			      report->field[i]->logical_minimum;

		/* There is no good reason for a Resolution
		 * Multiplier to have a count other than 1.
		 * Ignore that case.
		 */
		if (report->field[i]->report_count != 1)
			continue;

		for (j = 0; j < report->field[i]->maxusage; j++) {
			usage = &report->field[i]->usage[j];

			if (usage->hid != HID_GD_RESOLUTION_MULTIPLIER)
				continue;

			/*
			 * If we have more than one feature within this
			 * report we need to fill in the bits from the
			 * others before we can overwrite the ones for the
			 * Resolution Multiplier.
			 *
			 * But if we're not allowed to read from the device,
			 * we just bail. Such a device should not exist
			 * anyway.
			 */
			if (!get_report_completed && report->maxfield > 1) {
				if (hid->quirks & HID_QUIRK_NO_INIT_REPORTS)
					return update_needed;

				hid_hw_request(hid, report, HID_REQ_GET_REPORT);
				hid_hw_wait(hid);
				get_report_completed = true;
			}

			report->field[i]->value[j] = value;
			update_needed = true;
		}
	}

	return update_needed;
}

static void hidinput_change_resolution_multipliers(struct hid_device *hid)
{
	struct hid_report_enum *rep_enum;
	struct hid_report *rep;
	int ret;

	rep_enum = &hid->report_enum[HID_FEATURE_REPORT];
	list_for_each_entry(rep, &rep_enum->report_list, list) {
		bool update_needed = __hidinput_change_resolution_multipliers(hid,
								     rep, true);

		if (update_needed) {
			ret = __hid_request(hid, rep, HID_REQ_SET_REPORT);
			if (ret) {
				__hidinput_change_resolution_multipliers(hid,
								    rep, false);
				return;
			}
		}
	}

	/* refresh our structs */
	hid_setup_resolution_multiplier(hid);
}

static void report_features(struct hid_device *hid)
{
	struct hid_driver *drv = hid->driver;
	struct hid_report_enum *rep_enum;
	struct hid_report *rep;
	struct hid_usage *usage;
	int i, j;