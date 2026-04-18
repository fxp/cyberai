// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 3/16



	/* Apply negative unit exponent */
	for (; unit_exponent < 0; unit_exponent++) {
		prev = logical_extents;
		logical_extents *= 10;
		if (logical_extents < prev)
			return 0;
	}
	/* Apply positive unit exponent */
	for (; unit_exponent > 0; unit_exponent--) {
		prev = physical_extents;
		physical_extents *= 10;
		if (physical_extents < prev)
			return 0;
	}

	/* Calculate resolution */
	return DIV_ROUND_CLOSEST(logical_extents, physical_extents);
}
EXPORT_SYMBOL_GPL(hidinput_calc_abs_res);

#ifdef CONFIG_HID_BATTERY_STRENGTH
static enum power_supply_property hidinput_battery_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_SCOPE,
};

#define HID_BATTERY_QUIRK_PERCENT	(1 << 0) /* always reports percent */
#define HID_BATTERY_QUIRK_FEATURE	(1 << 1) /* ask for feature report */
#define HID_BATTERY_QUIRK_IGNORE	(1 << 2) /* completely ignore the battery */
#define HID_BATTERY_QUIRK_AVOID_QUERY	(1 << 3) /* do not query the battery */
#define HID_BATTERY_QUIRK_DYNAMIC	(1 << 4) /* report present only after life signs */

static const struct hid_device_id hid_battery_quirks[] = {
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
		USB_DEVICE_ID_APPLE_ALU_WIRELESS_2009_ISO),
	  HID_BATTERY_QUIRK_PERCENT | HID_BATTERY_QUIRK_FEATURE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
		USB_DEVICE_ID_APPLE_ALU_WIRELESS_2009_ANSI),
	  HID_BATTERY_QUIRK_PERCENT | HID_BATTERY_QUIRK_FEATURE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
		USB_DEVICE_ID_APPLE_ALU_WIRELESS_2011_ANSI),
	  HID_BATTERY_QUIRK_PERCENT | HID_BATTERY_QUIRK_FEATURE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
			       USB_DEVICE_ID_APPLE_ALU_WIRELESS_2011_ISO),
	  HID_BATTERY_QUIRK_PERCENT | HID_BATTERY_QUIRK_FEATURE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
		USB_DEVICE_ID_APPLE_ALU_WIRELESS_ANSI),
	  HID_BATTERY_QUIRK_PERCENT | HID_BATTERY_QUIRK_FEATURE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_APPLE,
		USB_DEVICE_ID_APPLE_MAGICTRACKPAD),
	  HID_BATTERY_QUIRK_IGNORE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_ELECOM,
		USB_DEVICE_ID_ELECOM_BM084),
	  HID_BATTERY_QUIRK_IGNORE },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SYMBOL,
		USB_DEVICE_ID_SYMBOL_SCANNER_3),
	  HID_BATTERY_QUIRK_IGNORE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_ASUSTEK,
		USB_DEVICE_ID_ASUSTEK_T100CHI_KEYBOARD),
	  HID_BATTERY_QUIRK_IGNORE },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_LOGITECH,
		USB_DEVICE_ID_LOGITECH_DINOVO_EDGE_KBD),
	  HID_BATTERY_QUIRK_IGNORE },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UGEE, USB_DEVICE_ID_UGEE_XPPEN_TABLET_DECO_L),
	  HID_BATTERY_QUIRK_AVOID_QUERY },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UGEE, USB_DEVICE_ID_UGEE_XPPEN_TABLET_DECO_PRO_MW),
	  HID_BATTERY_QUIRK_AVOID_QUERY },
	{ HID_USB_DEVICE(USB_VENDOR_ID_UGEE, USB_DEVICE_ID_UGEE_XPPEN_TABLET_DECO_PRO_SW),
	  HID_BATTERY_QUIRK_AVOID_QUERY },
	{ HID_I2C_DEVICE(USB_VENDOR_ID_ELAN, I2C_DEVICE_ID_CHROMEBOOK_TROGDOR_POMPOM),
	  HID_BATTERY_QUIRK_AVOID_QUERY },
	/*
	 * Elan HID touchscreens seem to all report a non present battery,
	 * set HID_BATTERY_QUIRK_IGNORE for all Elan I2C and USB HID devices.
	 */
	{ HID_I2C_DEVICE(USB_VENDOR_ID_ELAN, HID_ANY_ID), HID_BATTERY_QUIRK_DYNAMIC },
	{ HID_USB_DEVICE(USB_VENDOR_ID_ELAN, HID_ANY_ID), HID_BATTERY_QUIRK_DYNAMIC },
	{}
};

static unsigned find_battery_quirk(struct hid_device *hdev)
{
	unsigned quirks = 0;
	const struct hid_device_id *match;

	match = hid_match_id(hdev, hid_battery_quirks);
	if (match != NULL)
		quirks = match->driver_data;

	return quirks;
}

static int hidinput_scale_battery_capacity(struct hid_battery *bat,
					   int value)
{
	if (bat->min < bat->max &&
	    value >= bat->min && value <= bat->max)
		value = ((value - bat->min) * 100) /
			(bat->max - bat->min);

	return value;
}

static int hidinput_query_battery_capacity(struct hid_battery *bat)
{
	int ret;

	u8 *buf __free(kfree) = kmalloc(4, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = hid_hw_raw_request(bat->dev, bat->report_id, buf, 4,
				 bat->report_type, HID_REQ_GET_REPORT);
	if (ret < 2)
		return -ENODATA;

	return hidinput_scale_battery_capacity(bat, buf[1]);
}

static int hidinput_get_battery_property(struct power_supply *psy,
					 enum power_supply_property prop,
					 union power_supply_propval *val)
{
	struct hid_battery *bat = power_supply_get_drvdata(psy);
	struct hid_device *dev = bat->dev;
	int value;
	int ret = 0;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = bat->present;
		break;

	case POWER_SUPPLY_PROP_CAPACITY:
		if (bat->status != HID_BATTERY_REPORTED &&
		    !bat->avoid_query) {
			value = hidinput_query_battery_capacity(bat);
			if (value < 0)
				return value;
		} else  {
			value = bat->capacity;
		}

		val->intval = value;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = dev->name;
		break;