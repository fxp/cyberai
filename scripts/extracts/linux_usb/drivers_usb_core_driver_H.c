// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 8/14



/**
 * usb_register_driver - register a USB interface driver
 * @new_driver: USB operations for the interface driver
 * @owner: module owner of this driver.
 * @mod_name: module name string
 *
 * Registers a USB interface driver with the USB core.  The list of
 * unattached interfaces will be rescanned whenever a new driver is
 * added, allowing the new driver to attach to any recognized interfaces.
 *
 * Return: A negative error code on failure and 0 on success.
 *
 * NOTE: if you want your driver to use the USB major number, you must call
 * usb_register_dev() to enable that functionality.  This function no longer
 * takes care of that.
 */
int usb_register_driver(struct usb_driver *new_driver, struct module *owner,
			const char *mod_name)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_driver->driver.name = new_driver->name;
	new_driver->driver.bus = &usb_bus_type;
	new_driver->driver.probe = usb_probe_interface;
	new_driver->driver.remove = usb_unbind_interface;
	new_driver->driver.shutdown = usb_shutdown_interface;
	new_driver->driver.owner = owner;
	new_driver->driver.mod_name = mod_name;
	new_driver->driver.dev_groups = new_driver->dev_groups;
	INIT_LIST_HEAD(&new_driver->dynids.list);

	retval = driver_register(&new_driver->driver);
	if (retval)
		goto out;

	retval = usb_create_newid_files(new_driver);
	if (retval)
		goto out_newid;

	pr_info("%s: registered new interface driver %s\n",
			usbcore_name, new_driver->name);

	return 0;

out_newid:
	driver_unregister(&new_driver->driver);
out:
	pr_err("%s: error %d registering interface driver %s\n",
		usbcore_name, retval, new_driver->name);
	return retval;
}
EXPORT_SYMBOL_GPL(usb_register_driver);

/**
 * usb_deregister - unregister a USB interface driver
 * @driver: USB operations of the interface driver to unregister
 * Context: must be able to sleep
 *
 * Unlinks the specified driver from the internal USB driver list.
 *
 * NOTE: If you called usb_register_dev(), you still need to call
 * usb_deregister_dev() to clean up your driver's allocated minor numbers,
 * this * call will no longer do it for you.
 */
void usb_deregister(struct usb_driver *driver)
{
	pr_info("%s: deregistering interface driver %s\n",
			usbcore_name, driver->name);

	usb_remove_newid_files(driver);
	driver_unregister(&driver->driver);
	usb_free_dynids(driver);
}
EXPORT_SYMBOL_GPL(usb_deregister);

/* Forced unbinding of a USB interface driver, either because
 * it doesn't support pre_reset/post_reset/reset_resume or
 * because it doesn't support suspend/resume.
 *
 * The caller must hold @intf's device's lock, but not @intf's lock.
 */
void usb_forced_unbind_intf(struct usb_interface *intf)
{
	struct usb_driver *driver = to_usb_driver(intf->dev.driver);

	dev_dbg(&intf->dev, "forced unbind\n");
	usb_driver_release_interface(driver, intf);

	/* Mark the interface for later rebinding */
	intf->needs_binding = 1;
}

/*
 * Unbind drivers for @udev's marked interfaces.  These interfaces have
 * the needs_binding flag set, for example by usb_resume_interface().
 *
 * The caller must hold @udev's device lock.
 */
static void unbind_marked_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			if (intf->dev.driver && intf->needs_binding)
				usb_forced_unbind_intf(intf);
		}
	}
}

/* Delayed forced unbinding of a USB interface driver and scan
 * for rebinding.
 *
 * The caller must hold @intf's device's lock, but not @intf's lock.
 *
 * Note: Rebinds will be skipped if a system sleep transition is in
 * progress and the PM "complete" callback hasn't occurred yet.
 */
static void usb_rebind_intf(struct usb_interface *intf)
{
	int rc;

	/* Delayed unbind of an existing driver */
	if (intf->dev.driver)
		usb_forced_unbind_intf(intf);

	/* Try to rebind the interface */
	if (!intf->dev.power.is_prepared) {
		intf->needs_binding = 0;
		rc = device_attach(&intf->dev);
		if (rc < 0 && rc != -EPROBE_DEFER)
			dev_warn(&intf->dev, "rebind failed: %d\n", rc);
	}
}

/*
 * Rebind drivers to @udev's marked interfaces.  These interfaces have
 * the needs_binding flag set.
 *
 * The caller must hold @udev's device lock.
 */
static void rebind_marked_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			if (intf->needs_binding)
				usb_rebind_intf(intf);
		}
	}
}