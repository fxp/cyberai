// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 13/14



/**
 * usb_autopm_get_interface - increment a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be incremented
 *
 * This routine should be called by an interface driver when it wants to
 * use @intf and needs to guarantee that it is not suspended.  In addition,
 * the routine prevents @intf from being autosuspended subsequently.  (Note
 * that this will not prevent suspend events originating in the PM core.)
 * This prevention will persist until usb_autopm_put_interface() is called
 * or @intf is unbound.  A typical example would be a character-device
 * driver when its device file is opened.
 *
 * @intf's usage counter is incremented to prevent subsequent autosuspends.
 * However if the autoresume fails then the counter is re-decremented.
 *
 * This routine can run only in process context.
 *
 * Return: 0 on success.
 */
int usb_autopm_get_interface(struct usb_interface *intf)
{
	int	status;

	status = pm_runtime_resume_and_get(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface);

/**
 * usb_autopm_get_interface_async - increment a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be incremented
 *
 * This routine does much the same thing as
 * usb_autopm_get_interface(): It increments @intf's usage counter and
 * queues an autoresume request if the device is suspended.  The
 * differences are that it does not perform any synchronization (callers
 * should hold a private lock and handle all synchronization issues
 * themselves), and it does not autoresume the device directly (it only
 * queues a request).  After a successful call, the device may not yet be
 * resumed.
 *
 * This routine can run in atomic context.
 *
 * Return: 0 on success. A negative error code otherwise.
 */
int usb_autopm_get_interface_async(struct usb_interface *intf)
{
	int	status;

	status = pm_runtime_get(&intf->dev);
	if (status < 0 && status != -EINPROGRESS)
		pm_runtime_put_noidle(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
	if (status > 0 || status == -EINPROGRESS)
		status = 0;
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface_async);

/**
 * usb_autopm_get_interface_no_resume - increment a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be incremented
 *
 * This routine increments @intf's usage counter but does not carry out an
 * autoresume.
 *
 * This routine can run in atomic context.
 */
void usb_autopm_get_interface_no_resume(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);

	usb_mark_last_busy(udev);
	pm_runtime_get_noresume(&intf->dev);
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface_no_resume);

/* Internal routine to check whether we may autosuspend a device. */
static int autosuspend_check(struct usb_device *udev)
{
	int			w, i;
	struct usb_interface	*intf;

	if (udev->state == USB_STATE_NOTATTACHED)
		return -ENODEV;

	/* Fail if autosuspend is disabled, or any interfaces are in use, or
	 * any interface drivers require remote wakeup but it isn't available.
	 */
	w = 0;
	if (udev->actconfig) {
		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];

			/* We don't need to check interfaces that are
			 * disabled for runtime PM.  Either they are unbound
			 * or else their drivers don't support autosuspend
			 * and so they are permanently active.
			 */
			if (intf->dev.power.disable_depth)
				continue;
			if (atomic_read(&intf->dev.power.usage_count) > 0)
				return -EBUSY;
			w |= intf->needs_remote_wakeup;

			/* Don't allow autosuspend if the device will need
			 * a reset-resume and any of its interface drivers
			 * doesn't include support or needs remote wakeup.
			 */
			if (udev->quirks & USB_QUIRK_RESET_RESUME) {
				struct usb_driver *driver;

				driver = to_usb_driver(intf->dev.driver);
				if (!driver->reset_resume ||
						intf->needs_remote_wakeup)
					return -EOPNOTSUPP;
			}
		}
	}
	if (w && !device_can_wakeup(&udev->dev)) {
		dev_dbg(&udev->dev, "remote wakeup needed for autosuspend\n");
		return -EOPNOTSUPP;
	}

	/*
	 * If the device is a direct child of the root hub and the HCD
	 * doesn't handle wakeup requests, don't allow autosuspend when
	 * wakeup is needed.
	 */
	if (w && udev->parent == udev->bus->root_hub &&
			bus_to_hcd(udev->bus)->cant_recv_wakeups) {
		dev_dbg(&udev->dev, "HCD doesn't handle wakeup requests\n");
		return -EOPNOTSUPP;
	}

	udev->do_remote_wakeup = w;
	return 0;
}

int usb_runtime_suspend(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;