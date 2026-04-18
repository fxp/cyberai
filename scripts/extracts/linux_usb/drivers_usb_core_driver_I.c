// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 9/14



/*
 * Unbind all of @udev's marked interfaces and then rebind all of them.
 * This ordering is necessary because some drivers claim several interfaces
 * when they are first probed.
 *
 * The caller must hold @udev's device lock.
 */
void usb_unbind_and_rebind_marked_interfaces(struct usb_device *udev)
{
	unbind_marked_interfaces(udev);
	rebind_marked_interfaces(udev);
}

#ifdef CONFIG_PM

/* Unbind drivers for @udev's interfaces that don't support suspend/resume
 * There is no check for reset_resume here because it can be determined
 * only during resume whether reset_resume is needed.
 *
 * The caller must hold @udev's device lock.
 */
static void unbind_no_pm_drivers_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;
	struct usb_driver	*drv;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];

			if (intf->dev.driver) {
				drv = to_usb_driver(intf->dev.driver);
				if (!drv->suspend || !drv->resume)
					usb_forced_unbind_intf(intf);
			}
		}
	}
}

static int usb_suspend_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	/* For devices that don't have a driver, we do a generic suspend. */
	if (udev->dev.driver)
		udriver = to_usb_device_driver(udev->dev.driver);
	else {
		udev->do_remote_wakeup = 0;
		udriver = &usb_generic_driver;
	}
	if (udriver->suspend)
		status = udriver->suspend(udev, msg);
	if (status == 0 && udriver->generic_subclass)
		status = usb_generic_driver_suspend(udev, msg);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_resume_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED)
		goto done;

	/* Can't resume it if it doesn't have a driver. */
	if (udev->dev.driver == NULL) {
		status = -ENOTCONN;
		goto done;
	}

	/* Non-root devices on a full/low-speed bus must wait for their
	 * companion high-speed root hub, in case a handoff is needed.
	 */
	if (!PMSG_IS_AUTO(msg) && udev->parent && udev->bus->hs_companion)
		device_pm_wait_for_dev(&udev->dev,
				&udev->bus->hs_companion->root_hub->dev);

	if (udev->quirks & USB_QUIRK_RESET_RESUME)
		udev->reset_resume = 1;

	udriver = to_usb_device_driver(udev->dev.driver);
	if (udriver->generic_subclass)
		status = usb_generic_driver_resume(udev, msg);
	if (status == 0 && udriver->resume)
		status = udriver->resume(udev, msg);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_suspend_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED ||
			intf->condition == USB_INTERFACE_UNBOUND)
		goto done;
	driver = to_usb_driver(intf->dev.driver);

	/* at this time we know the driver supports suspend */
	status = driver->suspend(intf, msg);
	if (status && !PMSG_IS_AUTO(msg))
		dev_err(&intf->dev, "suspend error %d\n", status);

 done:
	dev_vdbg(&intf->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_resume_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg, int reset_resume)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED)
		goto done;

	/* Don't let autoresume interfere with unbinding */
	if (intf->condition == USB_INTERFACE_UNBINDING)
		goto done;

	/* Can't resume it if it doesn't have a driver. */
	if (intf->condition == USB_INTERFACE_UNBOUND) {

		/* Carry out a deferred switch to altsetting 0 */
		if (intf->needs_altsetting0 && !intf->dev.power.is_prepared) {
			usb_set_interface(udev, intf->altsetting[0].
					desc.bInterfaceNumber, 0);
			intf->needs_altsetting0 = 0;
		}
		goto done;
	}

	/* Don't resume if the interface is marked for rebinding */
	if (intf->needs_binding)
		goto done;
	driver = to_usb_driver(intf->dev.driver);

	if (reset_resume) {
		if (driver->reset_resume) {
			status = driver->reset_resume(intf);
			if (status)
				dev_err(&intf->dev, "%s error %d\n",
						"reset_resume", status);
		} else {
			intf->needs_binding = 1;
			dev_dbg(&intf->dev, "no reset_resume for driver %s?\n",
					driver->name);
		}
	} else {
		status = driver->resume(intf);
		if (status)
			dev_err(&intf->dev, "resume error %d\n", status);
	}

done:
	dev_vdbg(&intf->dev, "%s: status %d\n", __func__, status);

	/* Later we will unbind the driver and/or reprobe, if necessary */
	return status;
}