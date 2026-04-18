// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 2/14



static void usb_remove_newid_files(struct usb_driver *usb_drv)
{
	if (usb_drv->no_dynamic_id)
		return;

	if (usb_drv->probe != NULL) {
		driver_remove_file(&usb_drv->driver,
				&driver_attr_remove_id);
		driver_remove_file(&usb_drv->driver,
				   &driver_attr_new_id);
	}
}

static void usb_free_dynids(struct usb_driver *usb_drv)
{
	struct usb_dynid *dynid, *n;

	guard(mutex)(&usb_dynids_lock);
	list_for_each_entry_safe(dynid, n, &usb_drv->dynids.list, node) {
		list_del(&dynid->node);
		kfree(dynid);
	}
}

static const struct usb_device_id *usb_match_dynamic_id(struct usb_interface *intf,
							const struct usb_driver *drv)
{
	struct usb_dynid *dynid;

	guard(mutex)(&usb_dynids_lock);
	list_for_each_entry(dynid, &drv->dynids.list, node) {
		if (usb_match_one_id(intf, &dynid->id)) {
			return &dynid->id;
		}
	}
	return NULL;
}


/* called from driver core with dev locked */
static int usb_probe_device(struct device *dev)
{
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);
	struct usb_device *udev = to_usb_device(dev);
	int error = 0;

	dev_dbg(dev, "%s\n", __func__);

	/* TODO: Add real matching code */

	/* The device should always appear to be in use
	 * unless the driver supports autosuspend.
	 */
	if (!udriver->supports_autosuspend)
		error = usb_autoresume_device(udev);
	if (error)
		return error;

	if (udriver->generic_subclass)
		error = usb_generic_driver_probe(udev);
	if (error)
		return error;

	/* Probe the USB device with the driver in hand, but only
	 * defer to a generic driver in case the current USB
	 * device driver has an id_table or a match function; i.e.,
	 * when the device driver was explicitly matched against
	 * a device.
	 *
	 * If the device driver does not have either of these,
	 * then we assume that it can bind to any device and is
	 * not truly a more specialized/non-generic driver, so a
	 * return value of -ENODEV should not force the device
	 * to be handled by the generic USB driver, as there
	 * can still be another, more specialized, device driver.
	 *
	 * This accommodates the usbip driver.
	 *
	 * TODO: What if, in the future, there are multiple
	 * specialized USB device drivers for a particular device?
	 * In such cases, there is a need to try all matching
	 * specialised device drivers prior to setting the
	 * use_generic_driver bit.
	 */
	if (udriver->probe)
		error = udriver->probe(udev);
	else if (!udriver->generic_subclass)
		error = -EINVAL;
	if (error == -ENODEV && udriver != &usb_generic_driver &&
	    (udriver->id_table || udriver->match)) {
		udev->use_generic_driver = 1;
		return -EPROBE_DEFER;
	}
	return error;
}

/* called from driver core with dev locked */
static int usb_unbind_device(struct device *dev)
{
	struct usb_device *udev = to_usb_device(dev);
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);

	if (udriver->disconnect)
		udriver->disconnect(udev);
	if (udriver->generic_subclass)
		usb_generic_driver_disconnect(udev);
	if (!udriver->supports_autosuspend)
		usb_autosuspend_device(udev);
	return 0;
}

/* called from driver core with dev locked */
static int usb_probe_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *udev = interface_to_usbdev(intf);
	const struct usb_device_id *id;
	int error = -ENODEV;
	int lpm_disable_error = -ENODEV;

	dev_dbg(dev, "%s\n", __func__);

	intf->needs_binding = 0;

	if (usb_device_is_owned(udev))
		return error;

	if (udev->authorized == 0) {
		dev_info(&intf->dev, "Device is not authorized for usage\n");
		return error;
	} else if (intf->authorized == 0) {
		dev_info(&intf->dev, "Interface %d is not authorized for usage\n",
				intf->altsetting->desc.bInterfaceNumber);
		return error;
	}

	id = usb_match_dynamic_id(intf, driver);
	if (!id)
		id = usb_match_id(intf, driver->id_table);
	if (!id)
		return error;

	dev_dbg(dev, "%s - got id\n", __func__);

	error = usb_autoresume_device(udev);
	if (error)
		return error;

	intf->condition = USB_INTERFACE_BINDING;

	/* Probed interfaces are initially active.  They are
	 * runtime-PM-enabled only if the driver has autosuspend support.
	 * They are sensitive to their children's power states.
	 */
	pm_runtime_set_active(dev);
	pm_suspend_ignore_children(dev, false);
	if (driver->supports_autosuspend)
		pm_runtime_enable(dev);