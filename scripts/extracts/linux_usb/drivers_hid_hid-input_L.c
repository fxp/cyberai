// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 12/16



		if (report->tool_active) {
			/*
			 * if tool is not set but is marked as active,
			 * assume ours
			 */
			if (!report->tool)
				report->tool = usage->code;

			/* drivers may have changed the value behind our back, resend it */
			hid_report_set_tool(report, input, report->tool);
		} else {
			hid_report_release_tool(report, input, usage->code);
		}

		/* reset tool_active for the next event */
		report->tool_active = false;

		/* no further processing */
		return;

	case HID_DG_TIPSWITCH:
		report->tool_active |= !!value;

		/* if tool is set to RUBBER we should ignore the current value */
		if (report->tool == BTN_TOOL_RUBBER)
			return;

		break;

	case HID_DG_TIPPRESSURE:
		if (*quirks & HID_QUIRK_NOTOUCH) {
			int a = field->logical_minimum;
			int b = field->logical_maximum;

			if (value > a + ((b - a) >> 3)) {
				input_event(input, EV_KEY, BTN_TOUCH, 1);
				report->tool_active = true;
			}
		}
		break;

	case HID_UP_PID | 0x83UL: /* Simultaneous Effects Max */
		dbg_hid("Maximum Effects - %d\n",value);
		return;

	case HID_UP_PID | 0x7fUL:
		dbg_hid("PID Pool Report\n");
		return;
	}

	switch (usage->type) {
	case EV_KEY:
		if (usage->code == 0) /* Key 0 is "unassigned", not KEY_UNKNOWN */
			return;
		break;

	case EV_REL:
		if (usage->code == REL_WHEEL_HI_RES ||
		    usage->code == REL_HWHEEL_HI_RES) {
			hidinput_handle_scroll(usage, input, value);
			return;
		}
		break;

	case EV_ABS:
		if ((field->flags & HID_MAIN_ITEM_RELATIVE) &&
		    usage->code == ABS_VOLUME) {
			int count = abs(value);
			int direction = value > 0 ? KEY_VOLUMEUP : KEY_VOLUMEDOWN;
			int i;

			for (i = 0; i < count; i++) {
				input_event(input, EV_KEY, direction, 1);
				input_sync(input);
				input_event(input, EV_KEY, direction, 0);
				input_sync(input);
			}
			return;

		} else if (((*quirks & HID_QUIRK_X_INVERT) && usage->code == ABS_X) ||
			   ((*quirks & HID_QUIRK_Y_INVERT) && usage->code == ABS_Y))
			value = field->logical_maximum - value;
		break;
	}

	/*
	 * Ignore reports for absolute data if the data didn't change. This is
	 * not only an optimization but also fixes 'dead' key reports. Some
	 * RollOver implementations for localized keys (like BACKSLASH/PIPE; HID
	 * 0x31 and 0x32) report multiple keys, even though a localized keyboard
	 * can only have one of them physically available. The 'dead' keys
	 * report constant 0. As all map to the same keycode, they'd confuse
	 * the input layer. If we filter the 'dead' keys on the HID level, we
	 * skip the keycode translation and only forward real events.
	 */
	if (!(field->flags & (HID_MAIN_ITEM_RELATIVE |
	                      HID_MAIN_ITEM_BUFFERED_BYTE)) &&
			      (field->flags & HID_MAIN_ITEM_VARIABLE) &&
	    usage->usage_index < field->maxusage &&
	    value == field->value[usage->usage_index])
		return;

	/* report the usage code as scancode if the key status has changed */
	if (usage->type == EV_KEY &&
	    (!test_bit(usage->code, input->key)) == value)
		input_event(input, EV_MSC, MSC_SCAN, usage->hid);

	input_event(input, usage->type, usage->code, value);

	if ((field->flags & HID_MAIN_ITEM_RELATIVE) &&
	    usage->type == EV_KEY && value) {
		input_sync(input);
		input_event(input, usage->type, usage->code, 0);
	}
}

void hidinput_report_event(struct hid_device *hid, struct hid_report *report)
{
	struct hid_input *hidinput;

	if (hid->quirks & HID_QUIRK_NO_INPUT_SYNC)
		return;

	list_for_each_entry(hidinput, &hid->inputs, list)
		input_sync(hidinput->input);
}
EXPORT_SYMBOL_GPL(hidinput_report_event);

static int hidinput_find_field(struct hid_device *hid, unsigned int type,
			       unsigned int code, struct hid_field **field)
{
	struct hid_report *report;
	int i, j;

	list_for_each_entry(report, &hid->report_enum[HID_OUTPUT_REPORT].report_list, list) {
		for (i = 0; i < report->maxfield; i++) {
			*field = report->field[i];
			for (j = 0; j < (*field)->maxusage; j++)
				if ((*field)->usage[j].type == type && (*field)->usage[j].code == code)
					return j;
		}
	}
	return -1;
}

struct hid_field *hidinput_get_led_field(struct hid_device *hid)
{
	struct hid_report *report;
	struct hid_field *field;
	int i, j;

	list_for_each_entry(report,
			    &hid->report_enum[HID_OUTPUT_REPORT].report_list,
			    list) {
		for (i = 0; i < report->maxfield; i++) {
			field = report->field[i];
			for (j = 0; j < field->maxusage; j++)
				if (field->usage[j].type == EV_LED)
					return field;
		}
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(hidinput_get_led_field);

unsigned int hidinput_count_leds(struct hid_device *hid)
{
	struct hid_report *report;
	struct hid_field *field;
	int i, j;
	unsigned int count = 0;