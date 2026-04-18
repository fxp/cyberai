// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 4/16



	case POWER_SUPPLY_PROP_STATUS:
		if (bat->status != HID_BATTERY_REPORTED &&
		    !bat->avoid_query) {
			value = hidinput_query_battery_capacity(bat);
			if (value < 0)
				return value;

			bat->capacity = value;
			bat->status = HID_BATTERY_QUERIED;
		}

		if (bat->status == HID_BATTERY_UNKNOWN)
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		else
			val->intval = bat->charge_status;
		break;

	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct hid_battery *hidinput_find_battery(struct hid_device *dev,
						 int report_id)
{
	struct hid_battery *bat;

	list_for_each_entry(bat, &dev->batteries, list) {
		if (bat->report_id == report_id)
			return bat;
	}
	return NULL;
}

static int hidinput_setup_battery(struct hid_device *dev, unsigned report_type,
				  struct hid_field *field, bool is_percentage)
{
	struct hid_battery *bat;
	struct power_supply_desc *psy_desc;
	struct power_supply_config psy_cfg = { 0 };
	unsigned quirks;
	s32 min, max;
	int error;

	/* Check if battery for this report ID already exists */
	if (hidinput_find_battery(dev, field->report->id))
		return 0;

	quirks = find_battery_quirk(dev);

	hid_dbg(dev, "device %x:%x:%x %d quirks %d report_id %d\n",
		dev->bus, dev->vendor, dev->product, dev->version, quirks,
		field->report->id);

	if (quirks & HID_BATTERY_QUIRK_IGNORE)
		return 0;

	bat = devm_kzalloc(&dev->dev, sizeof(*bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	psy_desc = devm_kzalloc(&dev->dev, sizeof(*psy_desc), GFP_KERNEL);
	if (!psy_desc) {
		error = -ENOMEM;
		goto err_free_bat;
	}

	psy_desc->name = devm_kasprintf(&dev->dev, GFP_KERNEL,
					"hid-%s-battery-%d",
					strlen(dev->uniq) ?
						dev->uniq : dev_name(&dev->dev),
					field->report->id);
	if (!psy_desc->name) {
		error = -ENOMEM;
		goto err_free_desc;
	}

	psy_desc->type = POWER_SUPPLY_TYPE_BATTERY;
	psy_desc->properties = hidinput_battery_props;
	psy_desc->num_properties = ARRAY_SIZE(hidinput_battery_props);
	psy_desc->use_for_apm = 0;
	psy_desc->get_property = hidinput_get_battery_property;

	min = field->logical_minimum;
	max = field->logical_maximum;

	if (is_percentage || (quirks & HID_BATTERY_QUIRK_PERCENT)) {
		min = 0;
		max = 100;
	}

	if (quirks & HID_BATTERY_QUIRK_FEATURE)
		report_type = HID_FEATURE_REPORT;

	bat->dev = dev;
	bat->min = min;
	bat->max = max;
	bat->report_type = report_type;
	bat->report_id = field->report->id;
	bat->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	bat->status = HID_BATTERY_UNKNOWN;

	/*
	 * Stylus is normally not connected to the device and thus we
	 * can't query the device and get meaningful battery strength.
	 * We have to wait for the device to report it on its own.
	 */
	bat->avoid_query = report_type == HID_INPUT_REPORT &&
			   field->physical == HID_DG_STYLUS;

	if (quirks & HID_BATTERY_QUIRK_AVOID_QUERY)
		bat->avoid_query = true;

	bat->present = (quirks & HID_BATTERY_QUIRK_DYNAMIC) ? false : true;

	psy_cfg.drv_data = bat;
	bat->ps = devm_power_supply_register(&dev->dev, psy_desc, &psy_cfg);
	if (IS_ERR(bat->ps)) {
		error = PTR_ERR(bat->ps);
		hid_warn(dev, "can't register power supply: %d\n", error);
		goto err_free_name;
	}

	power_supply_powers(bat->ps, &dev->dev);
	list_add_tail(&bat->list, &dev->batteries);
	return 0;

err_free_name:
	devm_kfree(&dev->dev, psy_desc->name);
err_free_desc:
	devm_kfree(&dev->dev, psy_desc);
err_free_bat:
	devm_kfree(&dev->dev, bat);
	return error;
}

static bool hidinput_update_battery_charge_status(struct hid_battery *bat,
						  unsigned int usage, int value)
{
	switch (usage) {
	case HID_BAT_CHARGING:
		bat->charge_status = value ?
				     POWER_SUPPLY_STATUS_CHARGING :
				     POWER_SUPPLY_STATUS_DISCHARGING;
		return true;
	}

	return false;
}

static void hidinput_update_battery(struct hid_device *dev, int report_id,
				    unsigned int usage, int value)
{
	struct hid_battery *bat;
	int capacity;

	bat = hidinput_find_battery(dev, report_id);
	if (!bat)
		return;

	if (hidinput_update_battery_charge_status(bat, usage, value)) {
		bat->present = true;
		power_supply_changed(bat->ps);
		return;
	}

	if ((usage & HID_USAGE_PAGE) == HID_UP_DIGITIZER && value == 0)
		return;

	if (value < bat->min || value > bat->max)
		return;

	capacity = hidinput_scale_battery_capacity(bat, value);

	if (bat->status != HID_BATTERY_REPORTED ||
	    capacity != bat->capacity ||
	    ktime_after(ktime_get_coarse(), bat->ratelimit_time)) {
		bat->present = true;
		bat->capacity = capacity;
		bat->status = HID_BATTERY_REPORTED;
		bat->ratelimit_time =
			ktime_add_ms(ktime_get_coarse(), 30 * 1000);
		power_supply_changed(bat->ps);
	}
}
#else  /* !CONFIG_HID_BATTERY_STRENGTH */
static int hidinput_setup_battery(struct hid_device *dev, unsigned report_type,
				  struct hid_field *field, bool is_percentage)
{
	return 0;
}