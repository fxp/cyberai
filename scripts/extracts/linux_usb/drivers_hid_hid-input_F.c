// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 6/16



		if ((usage->hid & 0xf0) == 0x90) { /* SystemControl & D-pad */
			switch (usage->hid) {
			case HID_GD_UP:	   usage->hat_dir = 1; break;
			case HID_GD_DOWN:  usage->hat_dir = 5; break;
			case HID_GD_RIGHT: usage->hat_dir = 3; break;
			case HID_GD_LEFT:  usage->hat_dir = 7; break;
			case HID_GD_DO_NOT_DISTURB:
				map_key_clear(KEY_DO_NOT_DISTURB); break;
			default: goto unknown;
			}

			if (usage->hid <= HID_GD_LEFT) {
				if (field->dpad) {
					map_abs(field->dpad);
					goto ignore;
				}
				map_abs(ABS_HAT0X);
			}
			break;
		}

		if ((usage->hid & 0xf0) == 0xa0) {	/* SystemControl */
			switch (usage->hid & 0xf) {
			case 0x9: map_key_clear(KEY_MICMUTE); break;
			case 0xa: map_key_clear(KEY_ACCESSIBILITY); break;
			default: goto ignore;
			}
			break;
		}

		if ((usage->hid & 0xf0) == 0xb0) {	/* SC - Display */
			switch (usage->hid & 0xf) {
			case 0x05: map_key_clear(KEY_SWITCHVIDEOMODE); break;
			default: goto ignore;
			}
			break;
		}

		/*
		 * Some lazy vendors declare 255 usages for System Control,
		 * leading to the creation of ABS_X|Y axis and too many others.
		 * It wouldn't be a problem if joydev doesn't consider the
		 * device as a joystick then.
		 */
		if (field->application == HID_GD_SYSTEM_CONTROL)
			goto ignore;

		switch (usage->hid) {
		/* These usage IDs map directly to the usage codes. */
		case HID_GD_X: case HID_GD_Y:
		case HID_GD_RX: case HID_GD_RY: case HID_GD_RZ:
			if (field->flags & HID_MAIN_ITEM_RELATIVE)
				map_rel(usage->hid & 0xf);
			else
				map_abs_clear(usage->hid & 0xf);
			break;

		case HID_GD_Z:
			/* HID_GD_Z is mapped to ABS_DISTANCE for stylus/pen */
			if (field->flags & HID_MAIN_ITEM_RELATIVE) {
				map_rel(usage->hid & 0xf);
			} else {
				if (field->application == HID_DG_PEN ||
				    field->physical == HID_DG_PEN ||
				    field->logical == HID_DG_STYLUS ||
				    field->physical == HID_DG_STYLUS ||
				    field->application == HID_DG_DIGITIZER)
					map_abs_clear(ABS_DISTANCE);
				else
					map_abs_clear(usage->hid & 0xf);
			}
			break;

		case HID_GD_WHEEL:
			if (field->flags & HID_MAIN_ITEM_RELATIVE) {
				set_bit(REL_WHEEL, input->relbit);
				map_rel(REL_WHEEL_HI_RES);
			} else {
				map_abs(usage->hid & 0xf);
			}
			break;
		case HID_GD_SLIDER: case HID_GD_DIAL:
			if (field->flags & HID_MAIN_ITEM_RELATIVE)
				map_rel(usage->hid & 0xf);
			else
				map_abs(usage->hid & 0xf);
			break;

		case HID_GD_HATSWITCH:
			usage->hat_min = field->logical_minimum;
			usage->hat_max = field->logical_maximum;
			map_abs(ABS_HAT0X);
			break;

		case HID_GD_START:	map_key_clear(BTN_START);	break;
		case HID_GD_SELECT:	map_key_clear(BTN_SELECT);	break;

		case HID_GD_RFKILL_BTN:
			/* MS wireless radio ctl extension, also check CA */
			if (field->application == HID_GD_WIRELESS_RADIO_CTLS) {
				map_key_clear(KEY_RFKILL);
				/* We need to simulate the btn release */
				field->flags |= HID_MAIN_ITEM_RELATIVE;
				break;
			}
			goto unknown;

		default: goto unknown;
		}

		break;

	case HID_UP_LED:
		switch (usage->hid & 0xffff) {		      /* HID-Value:                   */
		case 0x01:  map_led (LED_NUML);     break;    /*   "Num Lock"                 */
		case 0x02:  map_led (LED_CAPSL);    break;    /*   "Caps Lock"                */
		case 0x03:  map_led (LED_SCROLLL);  break;    /*   "Scroll Lock"              */
		case 0x04:  map_led (LED_COMPOSE);  break;    /*   "Compose"                  */
		case 0x05:  map_led (LED_KANA);     break;    /*   "Kana"                     */
		case 0x27:  map_led (LED_SLEEP);    break;    /*   "Stand-By"                 */
		case 0x4c:  map_led (LED_SUSPEND);  break;    /*   "System Suspend"           */
		case 0x09:  map_led (LED_MUTE);     break;    /*   "Mute"                     */
		case 0x4b:  map_led (LED_MISC);     break;    /*   "Generic Indicator"        */
		case 0x19:  map_led (LED_MAIL);     break;    /*   "Message Waiting"          */
		case 0x4d:  map_led (LED_CHARGING); break;    /*   "External Power Connected" */

		default: goto ignore;
		}
		break;

	case HID_UP_DIGITIZER:
		if ((field->application & 0xff) == 0x01) /* Digitizer */
			__set_bit(INPUT_PROP_POINTER, input->propbit);
		else if ((field->application & 0xff) == 0x02) /* Pen */
			__set_bit(INPUT_PROP_DIRECT, input->propbit);

		switch (usage->hid & 0xff) {
		case 0x00: /* Undefined */
			goto ignore;

		case 0x30: /* TipPressure */
			if (!test_bit(BTN_TOUCH, input->keybit)) {
				device->quirks |= HID_QUIRK_NOTOUCH;
				set_bit(EV_KEY, input->evbit);
				set_bit(BTN_TOUCH, input->keybit);
			}
			map_abs_clear(ABS_PRESSURE);
			break;