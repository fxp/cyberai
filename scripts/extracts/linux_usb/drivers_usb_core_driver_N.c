// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 14/14



	/* A USB device can be suspended if it passes the various autosuspend
	 * checks.  Runtime suspend for a USB device means suspending all the
	 * interfaces and then the device itself.
	 */
	if (autosuspend_check(udev) != 0)
		return -EAGAIN;

	status = usb_suspend_both(udev, PMSG_AUTO_SUSPEND);

	/* Allow a retry if autosuspend failed temporarily */
	if (status == -EAGAIN || status == -EBUSY)
		usb_mark_last_busy(udev);

	/*
	 * The PM core reacts badly unless the return code is 0,
	 * -EAGAIN, or -EBUSY, so always return -EBUSY on an error
	 * (except for root hubs, because they don't suspend through
	 * an upstream port like other USB devices).
	 */
	if (status != 0 && udev->parent)
		return -EBUSY;
	return status;
}

int usb_runtime_resume(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;

	/* Runtime resume for a USB device means resuming both the device
	 * and all its interfaces.
	 */
	status = usb_resume_both(udev, PMSG_AUTO_RESUME);
	return status;
}

int usb_runtime_idle(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);

	/* An idle USB device can be suspended if it passes the various
	 * autosuspend checks.
	 */
	if (autosuspend_check(udev) == 0)
		pm_runtime_autosuspend(dev);
	/* Tell the core not to suspend it, though. */
	return -EBUSY;
}

static int usb_set_usb2_hardware_lpm(struct usb_device *udev, int enable)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	int ret = -EPERM;

	if (hcd->driver->set_usb2_hw_lpm) {
		ret = hcd->driver->set_usb2_hw_lpm(hcd, udev, enable);
		if (!ret)
			udev->usb2_hw_lpm_enabled = enable;
	}

	return ret;
}

int usb_enable_usb2_hardware_lpm(struct usb_device *udev)
{
	if (!udev->usb2_hw_lpm_capable ||
	    !udev->usb2_hw_lpm_allowed ||
	    udev->usb2_hw_lpm_enabled)
		return 0;

	return usb_set_usb2_hardware_lpm(udev, 1);
}

int usb_disable_usb2_hardware_lpm(struct usb_device *udev)
{
	if (!udev->usb2_hw_lpm_enabled)
		return 0;

	return usb_set_usb2_hardware_lpm(udev, 0);
}

#endif /* CONFIG_PM */

const struct bus_type usb_bus_type = {
	.name =		"usb",
	.match =	usb_device_match,
	.uevent =	usb_uevent,
	.need_parent_lock =	true,
};
