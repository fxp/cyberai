// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 10/16



	case HID_UP_BATTERY:
		switch (usage->hid) {
		case HID_BAT_ABSOLUTESTATEOFCHARGE:
			hidinput_setup_battery(device, HID_INPUT_REPORT, field, true);
			usage->type = EV_PWR;
			return;
		case HID_BAT_CHARGING:
			usage->type = EV_PWR;
			return;
		}
		goto unknown;
	case HID_UP_CAMERA:
		switch (usage->hid & HID_USAGE) {
		case 0x020:
			map_key_clear(KEY_CAMERA_FOCUS);	break;
		case 0x021:
			map_key_clear(KEY_CAMERA);		break;
		default:
			goto ignore;
		}
		break;

	case HID_UP_HPVENDOR:	/* Reported on a Dutch layout HP5308 */
		set_bit(EV_REP, input->evbit);
		switch (usage->hid & HID_USAGE) {
		case 0x021: map_key_clear(KEY_PRINT);           break;
		case 0x070: map_key_clear(KEY_HP);		break;
		case 0x071: map_key_clear(KEY_CAMERA);		break;
		case 0x072: map_key_clear(KEY_SOUND);		break;
		case 0x073: map_key_clear(KEY_QUESTION);	break;
		case 0x080: map_key_clear(KEY_EMAIL);		break;
		case 0x081: map_key_clear(KEY_CHAT);		break;
		case 0x082: map_key_clear(KEY_SEARCH);		break;
		case 0x083: map_key_clear(KEY_CONNECT);	        break;
		case 0x084: map_key_clear(KEY_FINANCE);		break;
		case 0x085: map_key_clear(KEY_SPORT);		break;
		case 0x086: map_key_clear(KEY_SHOP);	        break;
		default:    goto ignore;
		}
		break;

	case HID_UP_HPVENDOR2:
		set_bit(EV_REP, input->evbit);
		switch (usage->hid & HID_USAGE) {
		case 0x001: map_key_clear(KEY_MICMUTE);		break;
		case 0x003: map_key_clear(KEY_BRIGHTNESSDOWN);	break;
		case 0x004: map_key_clear(KEY_BRIGHTNESSUP);	break;
		default:    goto ignore;
		}
		break;

	case HID_UP_MSVENDOR:
		goto ignore;

	case HID_UP_CUSTOM: /* Reported on Logitech and Apple USB keyboards */
		set_bit(EV_REP, input->evbit);
		goto ignore;

	case HID_UP_LOGIVENDOR:
		/* intentional fallback */
	case HID_UP_LOGIVENDOR2:
		/* intentional fallback */
	case HID_UP_LOGIVENDOR3:
		goto ignore;

	case HID_UP_PID:
		switch (usage->hid & HID_USAGE) {
		case 0xa4: map_key_clear(BTN_DEAD);	break;
		default: goto ignore;
		}
		break;

	default:
	unknown:
		if (field->report_size == 1) {
			if (field->report->type == HID_OUTPUT_REPORT) {
				map_led(LED_MISC);
				break;
			}
			map_key(BTN_MISC);
			break;
		}
		if (field->flags & HID_MAIN_ITEM_RELATIVE) {
			map_rel(REL_MISC);
			break;
		}
		map_abs(ABS_MISC);
		break;
	}

mapped:
	/* Mapping failed, bail out */
	if (!bit)
		return;

	if (device->driver->input_mapped &&
	    device->driver->input_mapped(device, hidinput, field, usage,
					 &bit, &max) < 0) {
		/*
		 * The driver indicated that no further generic handling
		 * of the usage is desired.
		 */
		return;
	}

	set_bit(usage->type, input->evbit);

	/*
	 * This part is *really* controversial:
	 * - HID aims at being generic so we should do our best to export
	 *   all incoming events
	 * - HID describes what events are, so there is no reason for ABS_X
	 *   to be mapped to ABS_Y
	 * - HID is using *_MISC+N as a default value, but nothing prevents
	 *   *_MISC+N to overwrite a legitimate even, which confuses userspace
	 *   (for instance ABS_MISC + 7 is ABS_MT_SLOT, which has a different
	 *   processing)
	 *
	 * If devices still want to use this (at their own risk), they will
	 * have to use the quirk HID_QUIRK_INCREMENT_USAGE_ON_DUPLICATE, but
	 * the default should be a reliable mapping.
	 */
	while (usage->code <= max && test_and_set_bit(usage->code, bit)) {
		if (device->quirks & HID_QUIRK_INCREMENT_USAGE_ON_DUPLICATE) {
			usage->code = find_next_zero_bit(bit,
							 max + 1,
							 usage->code);
		} else {
			device->status |= HID_STAT_DUP_DETECTED;
			goto ignore;
		}
	}

	if (usage->code > max)
		goto ignore;

	if (usage->type == EV_ABS) {

		int a = field->logical_minimum;
		int b = field->logical_maximum;

		if ((device->quirks & HID_QUIRK_BADPAD) && (usage->code == ABS_X || usage->code == ABS_Y)) {
			a = field->logical_minimum = 0;
			b = field->logical_maximum = 255;
		}

		if (field->application == HID_GD_GAMEPAD || field->application == HID_GD_JOYSTICK)
			input_set_abs_params(input, usage->code, a, b, (b - a) >> 8, (b - a) >> 4);
		else	input_set_abs_params(input, usage->code, a, b, 0, 0);

		input_abs_set_res(input, usage->code,
				  hidinput_calc_abs_res(field, usage->code));

		/* use a larger default input buffer for MT devices */
		if (usage->code == ABS_MT_POSITION_X && input->hint_events_per_packet == 0)
			input_set_events_per_packet(input, 60);
	}

	if (usage->type == EV_ABS &&
	    (usage->hat_min < usage->hat_max || usage->hat_dir)) {
		int i;
		for (i = usage->code; i < usage->code + 2 && i <= max; i++) {
			input_set_abs_params(input, i, -1, 1, 0, 0);
			set_bit(i, input->absbit);
		}
		if (usage->hat_dir && !field->dpad)
			field->dpad = usage->code;
	}