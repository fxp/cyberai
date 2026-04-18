// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 7/16



		case 0x32: /* InRange */
			switch (field->physical) {
			case HID_DG_PUCK:
				map_key(BTN_TOOL_MOUSE);
				break;
			case HID_DG_FINGER:
				map_key(BTN_TOOL_FINGER);
				break;
			default:
				/*
				 * If the physical is not given,
				 * rely on the application.
				 */
				if (!field->physical) {
					switch (field->application) {
					case HID_DG_TOUCHSCREEN:
					case HID_DG_TOUCHPAD:
						map_key_clear(BTN_TOOL_FINGER);
						break;
					default:
						map_key_clear(BTN_TOOL_PEN);
					}
				} else {
					map_key(BTN_TOOL_PEN);
				}
				break;
			}
			break;

		case 0x3b: /* Battery Strength */
			hidinput_setup_battery(device, HID_INPUT_REPORT, field, false);
			usage->type = EV_PWR;
			return;

		case 0x3c: /* Invert */
			device->quirks &= ~HID_QUIRK_NOINVERT;
			map_key_clear(BTN_TOOL_RUBBER);
			break;

		case 0x3d: /* X Tilt */
			map_abs_clear(ABS_TILT_X);
			break;

		case 0x3e: /* Y Tilt */
			map_abs_clear(ABS_TILT_Y);
			break;

		case 0x33: /* Touch */
		case 0x42: /* TipSwitch */
		case 0x43: /* TipSwitch2 */
			device->quirks &= ~HID_QUIRK_NOTOUCH;
			map_key_clear(BTN_TOUCH);
			break;

		case 0x44: /* BarrelSwitch */
			map_key_clear(BTN_STYLUS);
			break;

		case 0x45: /* ERASER */
			/*
			 * This event is reported when eraser tip touches the surface.
			 * Actual eraser (BTN_TOOL_RUBBER) is set and released either
			 * by Invert if tool reports proximity or by Eraser directly.
			 */
			if (!test_bit(BTN_TOOL_RUBBER, input->keybit)) {
				device->quirks |= HID_QUIRK_NOINVERT;
				set_bit(BTN_TOOL_RUBBER, input->keybit);
			}
			map_key_clear(BTN_TOUCH);
			break;

		case 0x46: /* TabletPick */
		case 0x5a: /* SecondaryBarrelSwitch */
			map_key_clear(BTN_STYLUS2);
			break;

		case 0x5b: /* TransducerSerialNumber */
		case 0x6e: /* TransducerSerialNumber2 */
			map_msc(MSC_SERIAL);
			break;

		default:  goto unknown;
		}
		break;

	case HID_UP_TELEPHONY:
		switch (usage->hid & HID_USAGE) {
		case 0x2f: map_key_clear(KEY_MICMUTE);		break;
		case 0xb0: map_key_clear(KEY_NUMERIC_0);	break;
		case 0xb1: map_key_clear(KEY_NUMERIC_1);	break;
		case 0xb2: map_key_clear(KEY_NUMERIC_2);	break;
		case 0xb3: map_key_clear(KEY_NUMERIC_3);	break;
		case 0xb4: map_key_clear(KEY_NUMERIC_4);	break;
		case 0xb5: map_key_clear(KEY_NUMERIC_5);	break;
		case 0xb6: map_key_clear(KEY_NUMERIC_6);	break;
		case 0xb7: map_key_clear(KEY_NUMERIC_7);	break;
		case 0xb8: map_key_clear(KEY_NUMERIC_8);	break;
		case 0xb9: map_key_clear(KEY_NUMERIC_9);	break;
		case 0xba: map_key_clear(KEY_NUMERIC_STAR);	break;
		case 0xbb: map_key_clear(KEY_NUMERIC_POUND);	break;
		case 0xbc: map_key_clear(KEY_NUMERIC_A);	break;
		case 0xbd: map_key_clear(KEY_NUMERIC_B);	break;
		case 0xbe: map_key_clear(KEY_NUMERIC_C);	break;
		case 0xbf: map_key_clear(KEY_NUMERIC_D);	break;
		default: goto ignore;
		}
		break;

	case HID_UP_CONSUMER:	/* USB HUT v1.12, pages 75-84 */
		switch (usage->hid & HID_USAGE) {
		case 0x000: goto ignore;
		case 0x030: map_key_clear(KEY_POWER);		break;
		case 0x031: map_key_clear(KEY_RESTART);		break;
		case 0x032: map_key_clear(KEY_SLEEP);		break;
		case 0x034: map_key_clear(KEY_SLEEP);		break;
		case 0x035: map_key_clear(KEY_KBDILLUMTOGGLE);	break;
		case 0x036: map_key_clear(BTN_MISC);		break;

		case 0x040: map_key_clear(KEY_MENU);		break; /* Menu */
		case 0x041: map_key_clear(KEY_SELECT);		break; /* Menu Pick */
		case 0x042: map_key_clear(KEY_UP);		break; /* Menu Up */
		case 0x043: map_key_clear(KEY_DOWN);		break; /* Menu Down */
		case 0x044: map_key_clear(KEY_LEFT);		break; /* Menu Left */
		case 0x045: map_key_clear(KEY_RIGHT);		break; /* Menu Right */
		case 0x046: map_key_clear(KEY_ESC);		break; /* Menu Escape */
		case 0x047: map_key_clear(KEY_KPPLUS);		break; /* Menu Value Increase */
		case 0x048: map_key_clear(KEY_KPMINUS);		break; /* Menu Value Decrease */

		case 0x060: map_key_clear(KEY_INFO);		break; /* Data On Screen */
		case 0x061: map_key_clear(KEY_SUBTITLE);	break; /* Closed Caption */
		case 0x063: map_key_clear(KEY_VCR);		break; /* VCR/TV */
		case 0x065: map_key_clear(KEY_CAMERA);		break; /* Snapshot */
		case 0x069: map_key_clear(KEY_RED);		break;
		case 0x06a: map_key_clear(KEY_GREEN);		break;
		case 0x06b: map_key_clear(KEY_BLUE);		break;
		case 0x06c: map_key_clear(KEY_YELLOW);		break;
		case 0x06d: map_key_clear(KEY_ASPECT_RATIO);	break;

		case 0x06f: map_key_clear(KEY_BRIGHTNESSUP);		break;
		case 0x070: map_key_clear(KEY_BRIGHTNESSDOWN);		break;
		case 0x072: map_key_clear(KEY_BRIGHTNESS_TOGGLE);	break;
		case 0x073: map_key_clear(KEY_BRIGHTNESS_MIN);		break;
		case 0x074: map_key_clear(KEY_BRIGHTNESS_MAX);		break;
		case 0x075: map_key_clear(KEY_BRIGHTNESS_AUTO);		break;

		case 0x076: map_key_clear(KEY_CAMERA_ACCESS_ENABLE);	break;
		case 0x077: map_key_clear(KEY_CAMERA_ACCESS_DISABLE);	break;
		case 0x078: map_key_clear(KEY_CAMERA_ACCESS_TOGGLE);	break;