// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 9/16



		case 0x181: map_key_clear(KEY_BUTTONCONFIG);	break;
		case 0x182: map_key_clear(KEY_BOOKMARKS);	break;
		case 0x183: map_key_clear(KEY_CONFIG);		break;
		case 0x184: map_key_clear(KEY_WORDPROCESSOR);	break;
		case 0x185: map_key_clear(KEY_EDITOR);		break;
		case 0x186: map_key_clear(KEY_SPREADSHEET);	break;
		case 0x187: map_key_clear(KEY_GRAPHICSEDITOR);	break;
		case 0x188: map_key_clear(KEY_PRESENTATION);	break;
		case 0x189: map_key_clear(KEY_DATABASE);	break;
		case 0x18a: map_key_clear(KEY_MAIL);		break;
		case 0x18b: map_key_clear(KEY_NEWS);		break;
		case 0x18c: map_key_clear(KEY_VOICEMAIL);	break;
		case 0x18d: map_key_clear(KEY_ADDRESSBOOK);	break;
		case 0x18e: map_key_clear(KEY_CALENDAR);	break;
		case 0x18f: map_key_clear(KEY_TASKMANAGER);	break;
		case 0x190: map_key_clear(KEY_JOURNAL);		break;
		case 0x191: map_key_clear(KEY_FINANCE);		break;
		case 0x192: map_key_clear(KEY_CALC);		break;
		case 0x193: map_key_clear(KEY_PLAYER);		break;
		case 0x194: map_key_clear(KEY_FILE);		break;
		case 0x196: map_key_clear(KEY_WWW);		break;
		case 0x199: map_key_clear(KEY_CHAT);		break;
		case 0x19c: map_key_clear(KEY_LOGOFF);		break;
		case 0x19e: map_key_clear(KEY_COFFEE);		break;
		case 0x19f: map_key_clear(KEY_CONTROLPANEL);		break;
		case 0x1a2: map_key_clear(KEY_APPSELECT);		break;
		case 0x1a3: map_key_clear(KEY_NEXT);		break;
		case 0x1a4: map_key_clear(KEY_PREVIOUS);	break;
		case 0x1a6: map_key_clear(KEY_HELP);		break;
		case 0x1a7: map_key_clear(KEY_DOCUMENTS);	break;
		case 0x1ab: map_key_clear(KEY_SPELLCHECK);	break;
		case 0x1ae: map_key_clear(KEY_KEYBOARD);	break;
		case 0x1b1: map_key_clear(KEY_SCREENSAVER);		break;
		case 0x1b4: map_key_clear(KEY_FILE);		break;
		case 0x1b6: map_key_clear(KEY_IMAGES);		break;
		case 0x1b7: map_key_clear(KEY_AUDIO);		break;
		case 0x1b8: map_key_clear(KEY_VIDEO);		break;
		case 0x1bc: map_key_clear(KEY_MESSENGER);	break;
		case 0x1bd: map_key_clear(KEY_INFO);		break;
		case 0x1cb: map_key_clear(KEY_ASSISTANT);	break;
		case 0x1cc: map_key_clear(KEY_ACTION_ON_SELECTION);	break;
		case 0x1cd: map_key_clear(KEY_CONTEXTUAL_INSERT);	break;
		case 0x1ce: map_key_clear(KEY_CONTEXTUAL_QUERY);	break;
		case 0x201: map_key_clear(KEY_NEW);		break;
		case 0x202: map_key_clear(KEY_OPEN);		break;
		case 0x203: map_key_clear(KEY_CLOSE);		break;
		case 0x204: map_key_clear(KEY_EXIT);		break;
		case 0x207: map_key_clear(KEY_SAVE);		break;
		case 0x208: map_key_clear(KEY_PRINT);		break;
		case 0x209: map_key_clear(KEY_PROPS);		break;
		case 0x21a: map_key_clear(KEY_UNDO);		break;
		case 0x21b: map_key_clear(KEY_COPY);		break;
		case 0x21c: map_key_clear(KEY_CUT);		break;
		case 0x21d: map_key_clear(KEY_PASTE);		break;
		case 0x21f: map_key_clear(KEY_FIND);		break;
		case 0x221: map_key_clear(KEY_SEARCH);		break;
		case 0x222: map_key_clear(KEY_GOTO);		break;
		case 0x223: map_key_clear(KEY_HOMEPAGE);	break;
		case 0x224: map_key_clear(KEY_BACK);		break;
		case 0x225: map_key_clear(KEY_FORWARD);		break;
		case 0x226: map_key_clear(KEY_STOP);		break;
		case 0x227: map_key_clear(KEY_REFRESH);		break;
		case 0x22a: map_key_clear(KEY_BOOKMARKS);	break;
		case 0x22d: map_key_clear(KEY_ZOOMIN);		break;
		case 0x22e: map_key_clear(KEY_ZOOMOUT);		break;
		case 0x22f: map_key_clear(KEY_ZOOMRESET);	break;
		case 0x232: map_key_clear(KEY_FULL_SCREEN);	break;
		case 0x233: map_key_clear(KEY_SCROLLUP);	break;
		case 0x234: map_key_clear(KEY_SCROLLDOWN);	break;
		case 0x238: /* AC Pan */
			set_bit(REL_HWHEEL, input->relbit);
			map_rel(REL_HWHEEL_HI_RES);
			break;
		case 0x23d: map_key_clear(KEY_EDIT);		break;
		case 0x25f: map_key_clear(KEY_CANCEL);		break;
		case 0x269: map_key_clear(KEY_INSERT);		break;
		case 0x26a: map_key_clear(KEY_DELETE);		break;
		case 0x279: map_key_clear(KEY_REDO);		break;

		case 0x289: map_key_clear(KEY_REPLY);		break;
		case 0x28b: map_key_clear(KEY_FORWARDMAIL);	break;
		case 0x28c: map_key_clear(KEY_SEND);		break;

		case 0x29d: map_key_clear(KEY_KBD_LAYOUT_NEXT);	break;

		case 0x2a2: map_key_clear(KEY_ALL_APPLICATIONS);	break;

		case 0x2c7: map_key_clear(KEY_KBDINPUTASSIST_PREV);		break;
		case 0x2c8: map_key_clear(KEY_KBDINPUTASSIST_NEXT);		break;
		case 0x2c9: map_key_clear(KEY_KBDINPUTASSIST_PREVGROUP);		break;
		case 0x2ca: map_key_clear(KEY_KBDINPUTASSIST_NEXTGROUP);		break;
		case 0x2cb: map_key_clear(KEY_KBDINPUTASSIST_ACCEPT);	break;
		case 0x2cc: map_key_clear(KEY_KBDINPUTASSIST_CANCEL);	break;

		case 0x29f: map_key_clear(KEY_SCALE);		break;

		default: map_key_clear(KEY_UNKNOWN);
		}
		break;

	case HID_UP_GENDEVCTRLS:
		switch (usage->hid) {
		case HID_DC_BATTERYSTRENGTH:
			hidinput_setup_battery(device, HID_INPUT_REPORT, field, false);
			usage->type = EV_PWR;
			return;
		}
		goto unknown;