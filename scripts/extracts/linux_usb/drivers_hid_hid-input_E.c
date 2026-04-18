// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 5/16



static void hidinput_update_battery(struct hid_device *dev, int report_id,
				    unsigned int usage, int value)
{
}
#endif	/* CONFIG_HID_BATTERY_STRENGTH */

static bool hidinput_field_in_collection(struct hid_device *device, struct hid_field *field,
					 unsigned int type, unsigned int usage)
{
	struct hid_collection *collection;

	collection = &device->collection[field->usage->collection_index];

	return collection->type == type && collection->usage == usage;
}

static void hidinput_configure_usage(struct hid_input *hidinput, struct hid_field *field,
				     struct hid_usage *usage, unsigned int usage_index)
{
	struct input_dev *input = hidinput->input;
	struct hid_device *device = input_get_drvdata(input);
	const struct usage_priority *usage_priority = NULL;
	int max = 0, code;
	unsigned int i = 0;
	unsigned long *bit = NULL;

	field->hidinput = hidinput;

	if (field->flags & HID_MAIN_ITEM_CONSTANT)
		goto ignore;

	/* Ignore if report count is out of bounds. */
	if (field->report_count < 1)
		goto ignore;

	/* only LED and HAPTIC usages are supported in output fields */
	if (field->report_type == HID_OUTPUT_REPORT &&
	    (usage->hid & HID_USAGE_PAGE) != HID_UP_LED &&
	    (usage->hid & HID_USAGE_PAGE) != HID_UP_HAPTIC) {
		goto ignore;
	}

	/* assign a priority based on the static list declared here */
	for (i = 0; i < ARRAY_SIZE(hidinput_usages_priorities); i++) {
		if (usage->hid == hidinput_usages_priorities[i].usage) {
			usage_priority = &hidinput_usages_priorities[i];

			field->usages_priorities[usage_index] =
				(ARRAY_SIZE(hidinput_usages_priorities) - i) << 8;
			break;
		}
	}

	/*
	 * For slotted devices, we need to also add the slot index
	 * in the priority.
	 */
	if (usage_priority && usage_priority->global)
		field->usages_priorities[usage_index] |=
			usage_priority->slot_overwrite;
	else
		field->usages_priorities[usage_index] |=
			(0xff - field->slot_idx) << 16;

	if (device->driver->input_mapping) {
		int ret = device->driver->input_mapping(device, hidinput, field,
				usage, &bit, &max);
		if (ret > 0)
			goto mapped;
		if (ret < 0)
			goto ignore;
	}

	switch (usage->hid & HID_USAGE_PAGE) {
	case HID_UP_UNDEFINED:
		goto ignore;

	case HID_UP_KEYBOARD:
		set_bit(EV_REP, input->evbit);

		if ((usage->hid & HID_USAGE) < 256) {
			if (!hid_keyboard[usage->hid & HID_USAGE]) goto ignore;
			map_key_clear(hid_keyboard[usage->hid & HID_USAGE]);
		} else
			map_key(KEY_UNKNOWN);

		break;

	case HID_UP_BUTTON:
		code = ((usage->hid - 1) & HID_USAGE);

		switch (field->application) {
		case HID_GD_MOUSE:
		case HID_GD_POINTER:  code += BTN_MOUSE; break;
		case HID_GD_JOYSTICK:
				if (code <= 0xf)
					code += BTN_JOYSTICK;
				else
					code += BTN_TRIGGER_HAPPY - 0x10;
				break;
		case HID_GD_GAMEPAD:
				if (code <= 0xf)
					code += BTN_GAMEPAD;
				else
					code += BTN_TRIGGER_HAPPY - 0x10;
				break;
		case HID_CP_CONSUMER_CONTROL:
				if (hidinput_field_in_collection(device, field,
								 HID_COLLECTION_NAMED_ARRAY,
								 HID_CP_PROGRAMMABLEBUTTONS)) {
					if (code <= 0x1d)
						code += KEY_MACRO1;
					else
						code += BTN_TRIGGER_HAPPY - 0x1e;
					break;
				}
				fallthrough;
		default:
			switch (field->physical) {
			case HID_GD_MOUSE:
			case HID_GD_POINTER:  code += BTN_MOUSE; break;
			case HID_GD_JOYSTICK: code += BTN_JOYSTICK; break;
			case HID_GD_GAMEPAD:  code += BTN_GAMEPAD; break;
			default:              code += BTN_MISC;
			}
		}

		map_key(code);
		break;

	case HID_UP_SIMULATION:
		switch (usage->hid & 0xffff) {
		case 0xba: map_abs(ABS_RUDDER);   break;
		case 0xbb: map_abs(ABS_THROTTLE); break;
		case 0xc4: map_abs(ABS_GAS);      break;
		case 0xc5: map_abs(ABS_BRAKE);    break;
		case 0xc8: map_abs(ABS_WHEEL);    break;
		default:   goto ignore;
		}
		break;

	case HID_UP_GENDESK:
		if ((usage->hid & 0xf0) == 0x80) {	/* SystemControl */
			switch (usage->hid & 0xf) {
			case 0x1: map_key_clear(KEY_POWER);  break;
			case 0x2: map_key_clear(KEY_SLEEP);  break;
			case 0x3: map_key_clear(KEY_WAKEUP); break;
			case 0x4: map_key_clear(KEY_CONTEXT_MENU); break;
			case 0x5: map_key_clear(KEY_MENU); break;
			case 0x6: map_key_clear(KEY_PROG1); break;
			case 0x7: map_key_clear(KEY_HELP); break;
			case 0x8: map_key_clear(KEY_EXIT); break;
			case 0x9: map_key_clear(KEY_SELECT); break;
			case 0xa: map_key_clear(KEY_RIGHT); break;
			case 0xb: map_key_clear(KEY_LEFT); break;
			case 0xc: map_key_clear(KEY_UP); break;
			case 0xd: map_key_clear(KEY_DOWN); break;
			case 0xe: map_key_clear(KEY_POWER2); break;
			case 0xf: map_key_clear(KEY_RESTART); break;
			default: goto unknown;
			}
			break;
		}