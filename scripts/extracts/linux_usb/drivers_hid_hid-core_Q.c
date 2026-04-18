// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 17/17



	if (hdev->driver == hdrv &&
	    !hdrv->match(hdev, hid_ignore_special_drivers) &&
	    !test_and_set_bit(ffs(HID_STAT_REPROBED), &hdev->status))
		return device_reprobe(dev);

	return 0;
}

static int __hid_bus_driver_added(struct device_driver *drv, void *data)
{
	struct hid_driver *hdrv = to_hid_driver(drv);

	if (hdrv->match) {
		bus_for_each_dev(&hid_bus_type, NULL, hdrv,
				 __hid_bus_reprobe_drivers);
	}

	return 0;
}

static int __bus_removed_driver(struct device_driver *drv, void *data)
{
	return bus_rescan_devices(&hid_bus_type);
}

int __hid_register_driver(struct hid_driver *hdrv, struct module *owner,
		const char *mod_name)
{
	int ret;

	hdrv->driver.name = hdrv->name;
	hdrv->driver.bus = &hid_bus_type;
	hdrv->driver.owner = owner;
	hdrv->driver.mod_name = mod_name;

	INIT_LIST_HEAD(&hdrv->dyn_list);
	spin_lock_init(&hdrv->dyn_lock);

	ret = driver_register(&hdrv->driver);

	if (ret == 0)
		bus_for_each_drv(&hid_bus_type, NULL, NULL,
				 __hid_bus_driver_added);

	return ret;
}
EXPORT_SYMBOL_GPL(__hid_register_driver);

void hid_unregister_driver(struct hid_driver *hdrv)
{
	driver_unregister(&hdrv->driver);
	hid_free_dynids(hdrv);

	bus_for_each_drv(&hid_bus_type, NULL, hdrv, __bus_removed_driver);
}
EXPORT_SYMBOL_GPL(hid_unregister_driver);

int hid_check_keys_pressed(struct hid_device *hid)
{
	struct hid_input *hidinput;
	int i;

	if (!(hid->claimed & HID_CLAIMED_INPUT))
		return 0;

	list_for_each_entry(hidinput, &hid->inputs, list) {
		for (i = 0; i < BITS_TO_LONGS(KEY_MAX); i++)
			if (hidinput->input->key[i])
				return 1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(hid_check_keys_pressed);

#ifdef CONFIG_HID_BPF
static const struct hid_ops __hid_ops = {
	.hid_get_report = hid_get_report,
	.hid_hw_raw_request = __hid_hw_raw_request,
	.hid_hw_output_report = __hid_hw_output_report,
	.hid_input_report = __hid_input_report,
	.owner = THIS_MODULE,
	.bus_type = &hid_bus_type,
};
#endif

static int __init hid_init(void)
{
	int ret;

	ret = bus_register(&hid_bus_type);
	if (ret) {
		pr_err("can't register hid bus\n");
		goto err;
	}

#ifdef CONFIG_HID_BPF
	hid_ops = &__hid_ops;
#endif

	ret = hidraw_init();
	if (ret)
		goto err_bus;

	hid_debug_init();

	return 0;
err_bus:
	bus_unregister(&hid_bus_type);
err:
	return ret;
}

static void __exit hid_exit(void)
{
#ifdef CONFIG_HID_BPF
	hid_ops = NULL;
#endif
	hid_debug_exit();
	hidraw_exit();
	bus_unregister(&hid_bus_type);
	hid_quirks_exit(HID_BUS_ANY);
}

module_init(hid_init);
module_exit(hid_exit);

MODULE_AUTHOR("Andreas Gal");
MODULE_AUTHOR("Vojtech Pavlik");
MODULE_AUTHOR("Jiri Kosina");
MODULE_DESCRIPTION("HID support for Linux");
MODULE_LICENSE("GPL");
