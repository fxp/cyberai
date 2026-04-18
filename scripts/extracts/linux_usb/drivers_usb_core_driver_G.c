// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 7/14



static int usb_device_match(struct device *dev, const struct device_driver *drv)
{
	/* devices and interfaces are handled separately */
	if (is_usb_device(dev)) {
		struct usb_device *udev;
		const struct usb_device_driver *udrv;

		/* interface drivers never match devices */
		if (!is_usb_device_driver(drv))
			return 0;

		udev = to_usb_device(dev);
		udrv = to_usb_device_driver(drv);

		/* If the device driver under consideration does not have a
		 * id_table or a match function, then let the driver's probe
		 * function decide.
		 */
		if (!udrv->id_table && !udrv->match)
			return 1;

		return usb_driver_applicable(udev, udrv);

	} else if (is_usb_interface(dev)) {
		struct usb_interface *intf;
		const struct usb_driver *usb_drv;
		const struct usb_device_id *id;

		/* device drivers never match interfaces */
		if (is_usb_device_driver(drv))
			return 0;

		intf = to_usb_interface(dev);
		usb_drv = to_usb_driver(drv);

		id = usb_match_id(intf, usb_drv->id_table);
		if (id)
			return 1;

		id = usb_match_dynamic_id(intf, usb_drv);
		if (id)
			return 1;
	}

	return 0;
}

static int usb_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct usb_device *usb_dev;

	if (is_usb_device(dev)) {
		usb_dev = to_usb_device(dev);
	} else if (is_usb_interface(dev)) {
		const struct usb_interface *intf = to_usb_interface(dev);

		usb_dev = interface_to_usbdev(intf);
	} else {
		return 0;
	}

	if (usb_dev->devnum < 0) {
		/* driver is often null here; dev_dbg() would oops */
		pr_debug("usb %s: already deleted?\n", dev_name(dev));
		return -ENODEV;
	}
	if (!usb_dev->bus) {
		pr_debug("usb %s: bus removed?\n", dev_name(dev));
		return -ENODEV;
	}

	/* per-device configurations are common */
	if (add_uevent_var(env, "PRODUCT=%x/%x/%x",
			   le16_to_cpu(usb_dev->descriptor.idVendor),
			   le16_to_cpu(usb_dev->descriptor.idProduct),
			   le16_to_cpu(usb_dev->descriptor.bcdDevice)))
		return -ENOMEM;

	/* class-based driver binding models */
	if (add_uevent_var(env, "TYPE=%d/%d/%d",
			   usb_dev->descriptor.bDeviceClass,
			   usb_dev->descriptor.bDeviceSubClass,
			   usb_dev->descriptor.bDeviceProtocol))
		return -ENOMEM;

	return 0;
}

static int __usb_bus_reprobe_drivers(struct device *dev, void *data)
{
	struct usb_device_driver *new_udriver = data;
	struct usb_device *udev;
	int ret;

	/* Don't reprobe if current driver isn't usb_generic_driver */
	if (dev->driver != &usb_generic_driver.driver)
		return 0;

	udev = to_usb_device(dev);
	if (!usb_driver_applicable(udev, new_udriver))
		return 0;

	ret = device_reprobe(dev);
	if (ret && ret != -EPROBE_DEFER)
		dev_err(dev, "Failed to reprobe device (error %d)\n", ret);

	return 0;
}

bool is_usb_device_driver(const struct device_driver *drv)
{
	return drv->probe == usb_probe_device;
}

/**
 * usb_register_device_driver - register a USB device (not interface) driver
 * @new_udriver: USB operations for the device driver
 * @owner: module owner of this driver.
 *
 * Registers a USB device driver with the USB core.  The list of
 * unattached devices will be rescanned whenever a new driver is
 * added, allowing the new driver to attach to any recognized devices.
 *
 * Return: A negative error code on failure and 0 on success.
 */
int usb_register_device_driver(struct usb_device_driver *new_udriver,
		struct module *owner)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_udriver->driver.name = new_udriver->name;
	new_udriver->driver.bus = &usb_bus_type;
	new_udriver->driver.probe = usb_probe_device;
	new_udriver->driver.remove = usb_unbind_device;
	new_udriver->driver.owner = owner;
	new_udriver->driver.dev_groups = new_udriver->dev_groups;

	retval = driver_register(&new_udriver->driver);

	if (!retval) {
		pr_info("%s: registered new device driver %s\n",
			usbcore_name, new_udriver->name);
		/*
		 * Check whether any device could be better served with
		 * this new driver
		 */
		bus_for_each_dev(&usb_bus_type, NULL, new_udriver,
				 __usb_bus_reprobe_drivers);
	} else {
		pr_err("%s: error %d registering device driver %s\n",
			usbcore_name, retval, new_udriver->name);
	}

	return retval;
}
EXPORT_SYMBOL_GPL(usb_register_device_driver);

/**
 * usb_deregister_device_driver - unregister a USB device (not interface) driver
 * @udriver: USB operations of the device driver to unregister
 * Context: must be able to sleep
 *
 * Unlinks the specified driver from the internal USB driver list.
 */
void usb_deregister_device_driver(struct usb_device_driver *udriver)
{
	pr_info("%s: deregistering device driver %s\n",
			usbcore_name, udriver->name);

	driver_unregister(&udriver->driver);
}
EXPORT_SYMBOL_GPL(usb_deregister_device_driver);