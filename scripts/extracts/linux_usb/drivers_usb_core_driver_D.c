// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 4/14



/**
 * usb_driver_claim_interface - bind a driver to an interface
 * @driver: the driver to be bound
 * @iface: the interface to which it will be bound; must be in the
 *	usb device's active configuration
 * @data: driver data associated with that interface
 *
 * This is used by usb device drivers that need to claim more than one
 * interface on a device when probing (audio and acm are current examples).
 * No device driver should directly modify internal usb_interface or
 * usb_device structure members.
 *
 * Callers must own the device lock, so driver probe() entries don't need
 * extra locking, but other call contexts may need to explicitly claim that
 * lock.
 *
 * Return: 0 on success.
 */
int usb_driver_claim_interface(struct usb_driver *driver,
				struct usb_interface *iface, void *data)
{
	struct device *dev;
	int retval = 0;

	if (!iface)
		return -ENODEV;

	dev = &iface->dev;
	if (dev->driver)
		return -EBUSY;

	/* reject claim if interface is not authorized */
	if (!iface->authorized)
		return -ENODEV;

	dev->driver = &driver->driver;
	usb_set_intfdata(iface, data);
	iface->needs_binding = 0;

	iface->condition = USB_INTERFACE_BOUND;

	/* Claimed interfaces are initially inactive (suspended) and
	 * runtime-PM-enabled, but only if the driver has autosuspend
	 * support.  Otherwise they are marked active, to prevent the
	 * device from being autosuspended, but left disabled.  In either
	 * case they are sensitive to their children's power states.
	 */
	pm_suspend_ignore_children(dev, false);
	if (driver->supports_autosuspend)
		pm_runtime_enable(dev);
	else
		pm_runtime_set_active(dev);

	/* if interface was already added, bind now; else let
	 * the future device_add() bind it, bypassing probe()
	 */
	if (device_is_registered(dev))
		retval = device_bind_driver(dev);

	if (retval) {
		dev->driver = NULL;
		usb_set_intfdata(iface, NULL);
		iface->needs_remote_wakeup = 0;
		iface->condition = USB_INTERFACE_UNBOUND;

		/*
		 * Unbound interfaces are always runtime-PM-disabled
		 * and runtime-PM-suspended
		 */
		if (driver->supports_autosuspend)
			pm_runtime_disable(dev);
		pm_runtime_set_suspended(dev);
	}

	return retval;
}
EXPORT_SYMBOL_GPL(usb_driver_claim_interface);

/**
 * usb_driver_release_interface - unbind a driver from an interface
 * @driver: the driver to be unbound
 * @iface: the interface from which it will be unbound
 *
 * This can be used by drivers to release an interface without waiting
 * for their disconnect() methods to be called.  In typical cases this
 * also causes the driver disconnect() method to be called.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 * Callers must own the device lock, so driver disconnect() entries don't
 * need extra locking, but other call contexts may need to explicitly claim
 * that lock.
 */
void usb_driver_release_interface(struct usb_driver *driver,
					struct usb_interface *iface)
{
	struct device *dev = &iface->dev;

	/* this should never happen, don't release something that's not ours */
	if (!dev->driver || dev->driver != &driver->driver)
		return;

	/* don't release from within disconnect() */
	if (iface->condition != USB_INTERFACE_BOUND)
		return;
	iface->condition = USB_INTERFACE_UNBINDING;

	/* Release via the driver core only if the interface
	 * has already been registered
	 */
	if (device_is_registered(dev)) {
		device_release_driver(dev);
	} else {
		device_lock(dev);
		usb_unbind_interface(dev);
		dev->driver = NULL;
		device_unlock(dev);
	}
}
EXPORT_SYMBOL_GPL(usb_driver_release_interface);

/* returns 0 if no match, 1 if match */
int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	/* No need to test id->bcdDevice_lo != 0, since 0 is never
	   greater than any unsigned number. */
	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}