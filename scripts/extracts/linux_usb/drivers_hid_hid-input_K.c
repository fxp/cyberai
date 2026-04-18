// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 11/16



	/* for those devices which produce Consumer volume usage as relative,
	 * we emulate pressing volumeup/volumedown appropriate number of times
	 * in hidinput_hid_event()
	 */
	if ((usage->type == EV_ABS) && (field->flags & HID_MAIN_ITEM_RELATIVE) &&
			(usage->code == ABS_VOLUME)) {
		set_bit(KEY_VOLUMEUP, input->keybit);
		set_bit(KEY_VOLUMEDOWN, input->keybit);
	}

	if (usage->type == EV_KEY) {
		set_bit(EV_MSC, input->evbit);
		set_bit(MSC_SCAN, input->mscbit);
	}

	return;

ignore:
	usage->type = 0;
	usage->code = 0;
}

static void hidinput_handle_scroll(struct hid_usage *usage,
				   struct input_dev *input,
				   __s32 value)
{
	int code;
	int hi_res, lo_res;

	if (value == 0)
		return;

	if (usage->code == REL_WHEEL_HI_RES)
		code = REL_WHEEL;
	else
		code = REL_HWHEEL;

	/*
	 * Windows reports one wheel click as value 120. Where a high-res
	 * scroll wheel is present, a fraction of 120 is reported instead.
	 * Our REL_WHEEL_HI_RES axis does the same because all HW must
	 * adhere to the 120 expectation.
	 */
	hi_res = value * 120/usage->resolution_multiplier;

	usage->wheel_accumulated += hi_res;
	lo_res = usage->wheel_accumulated/120;
	if (lo_res)
		usage->wheel_accumulated -= lo_res * 120;

	input_event(input, EV_REL, code, lo_res);
	input_event(input, EV_REL, usage->code, hi_res);
}

static void hid_report_release_tool(struct hid_report *report, struct input_dev *input,
				    unsigned int tool)
{
	/* if the given tool is not currently reported, ignore */
	if (!test_bit(tool, input->key))
		return;

	/*
	 * if the given tool was previously set, release it,
	 * release any TOUCH and send an EV_SYN
	 */
	input_event(input, EV_KEY, BTN_TOUCH, 0);
	input_event(input, EV_KEY, tool, 0);
	input_event(input, EV_SYN, SYN_REPORT, 0);

	report->tool = 0;
}

static void hid_report_set_tool(struct hid_report *report, struct input_dev *input,
				unsigned int new_tool)
{
	if (report->tool != new_tool)
		hid_report_release_tool(report, input, report->tool);

	input_event(input, EV_KEY, new_tool, 1);
	report->tool = new_tool;
}

void hidinput_hid_event(struct hid_device *hid, struct hid_field *field, struct hid_usage *usage, __s32 value)
{
	struct input_dev *input;
	struct hid_report *report = field->report;
	unsigned *quirks = &hid->quirks;

	if (!usage->type)
		return;

	if (usage->type == EV_PWR) {
		hidinput_update_battery(hid, report->id, usage->hid, value);
		return;
	}

	if (!field->hidinput)
		return;

	input = field->hidinput->input;

	if (usage->hat_min < usage->hat_max || usage->hat_dir) {
		int hat_dir = usage->hat_dir;
		if (!hat_dir)
			hat_dir = (value - usage->hat_min) * 8 / (usage->hat_max - usage->hat_min + 1) + 1;
		if (hat_dir < 0 || hat_dir > 8) hat_dir = 0;
		input_event(input, usage->type, usage->code    , hid_hat_to_axis[hat_dir].x);
		input_event(input, usage->type, usage->code + 1, hid_hat_to_axis[hat_dir].y);
		return;
	}

	/*
	 * Ignore out-of-range values as per HID specification,
	 * section 5.10 and 6.2.25, when NULL state bit is present.
	 * When it's not, clamp the value to match Microsoft's input
	 * driver as mentioned in "Required HID usages for digitizers":
	 * https://msdn.microsoft.com/en-us/library/windows/hardware/dn672278(v=vs.85).asp
	 *
	 * The logical_minimum < logical_maximum check is done so that we
	 * don't unintentionally discard values sent by devices which
	 * don't specify logical min and max.
	 */
	if ((field->flags & HID_MAIN_ITEM_VARIABLE) &&
	    field->logical_minimum < field->logical_maximum) {
		if (field->flags & HID_MAIN_ITEM_NULL_STATE &&
		    (value < field->logical_minimum ||
		     value > field->logical_maximum)) {
			dbg_hid("Ignoring out-of-range value %x\n", value);
			return;
		}
		value = clamp(value,
			      field->logical_minimum,
			      field->logical_maximum);
	}

	switch (usage->hid) {
	case HID_DG_ERASER:
		report->tool_active |= !!value;

		/*
		 * if eraser is set, we must enforce BTN_TOOL_RUBBER
		 * to accommodate for devices not following the spec.
		 */
		if (value)
			hid_report_set_tool(report, input, BTN_TOOL_RUBBER);
		else if (report->tool != BTN_TOOL_RUBBER)
			/* value is off, tool is not rubber, ignore */
			return;
		else if (*quirks & HID_QUIRK_NOINVERT &&
			 !test_bit(BTN_TOUCH, input->key)) {
			/*
			 * There is no invert to release the tool, let hid_input
			 * send BTN_TOUCH with scancode and release the tool after.
			 */
			hid_report_release_tool(report, input, BTN_TOOL_RUBBER);
			return;
		}

		/* let hid-input set BTN_TOUCH */
		break;

	case HID_DG_INVERT:
		report->tool_active |= !!value;

		/*
		 * If invert is set, we store BTN_TOOL_RUBBER.
		 */
		if (value)
			hid_report_set_tool(report, input, BTN_TOOL_RUBBER);
		else if (!report->tool_active)
			/* tool_active not set means Invert and Eraser are not set */
			hid_report_release_tool(report, input, BTN_TOOL_RUBBER);

		/* no further processing */
		return;

	case HID_DG_INRANGE:
		report->tool_active |= !!value;