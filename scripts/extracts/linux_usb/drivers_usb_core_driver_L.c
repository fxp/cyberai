// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 12/14



/**
 * usb_autosuspend_device - delayed autosuspend of a USB device and its interfaces
 * @udev: the usb_device to autosuspend
 *
 * This routine should be called when a core subsystem is finished using
 * @udev and wants to allow it to autosuspend.  Examples would be when
 * @udev's device file in usbfs is closed or after a configuration change.
 *
 * @udev's usage counter is decremented; if it drops to 0 and all the
 * interfaces are inactive then a delayed autosuspend will be attempted.
 * The attempt may fail (see autosuspend_check()).
 *
 * The caller must hold @udev's device lock.
 *
 * This routine can run only in process context.
 */
void usb_autosuspend_device(struct usb_device *udev)
{
	int	status;

	usb_mark_last_busy(udev);
	status = pm_runtime_put_sync_autosuspend(&udev->dev);
	dev_vdbg(&udev->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&udev->dev.power.usage_count),
			status);
}

/**
 * usb_autoresume_device - immediately autoresume a USB device and its interfaces
 * @udev: the usb_device to autoresume
 *
 * This routine should be called when a core subsystem wants to use @udev
 * and needs to guarantee that it is not suspended.  No autosuspend will
 * occur until usb_autosuspend_device() is called.  (Note that this will
 * not prevent suspend events originating in the PM core.)  Examples would
 * be when @udev's device file in usbfs is opened or when a remote-wakeup
 * request is received.
 *
 * @udev's usage counter is incremented to prevent subsequent autosuspends.
 * However if the autoresume fails then the usage counter is re-decremented.
 *
 * The caller must hold @udev's device lock.
 *
 * This routine can run only in process context.
 *
 * Return: 0 on success. A negative error code otherwise.
 */
int usb_autoresume_device(struct usb_device *udev)
{
	int	status;

	status = pm_runtime_resume_and_get(&udev->dev);
	dev_vdbg(&udev->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&udev->dev.power.usage_count),
			status);
	return status;
}

/**
 * usb_autopm_put_interface - decrement a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be decremented
 *
 * This routine should be called by an interface driver when it is
 * finished using @intf and wants to allow it to autosuspend.  A typical
 * example would be a character-device driver when its device file is
 * closed.
 *
 * The routine decrements @intf's usage counter.  When the counter reaches
 * 0, a delayed autosuspend request for @intf's device is attempted.  The
 * attempt may fail (see autosuspend_check()).
 *
 * This routine can run only in process context.
 */
void usb_autopm_put_interface(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status;

	usb_mark_last_busy(udev);
	status = pm_runtime_put_sync(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface);

/**
 * usb_autopm_put_interface_async - decrement a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be decremented
 *
 * This routine does much the same thing as usb_autopm_put_interface():
 * It decrements @intf's usage counter and schedules a delayed
 * autosuspend request if the counter is <= 0.  The difference is that it
 * does not perform any synchronization; callers should hold a private
 * lock and handle all synchronization issues themselves.
 *
 * Typically a driver would call this routine during an URB's completion
 * handler, if no more URBs were pending.
 *
 * This routine can run in atomic context.
 */
void usb_autopm_put_interface_async(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);

	usb_mark_last_busy(udev);
	pm_runtime_put(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count));
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface_async);

/**
 * usb_autopm_put_interface_no_suspend - decrement a USB interface's PM-usage counter
 * @intf: the usb_interface whose counter should be decremented
 *
 * This routine decrements @intf's usage counter but does not carry out an
 * autosuspend.
 *
 * This routine can run in atomic context.
 */
void usb_autopm_put_interface_no_suspend(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);

	usb_mark_last_busy(udev);
	pm_runtime_put_noidle(&intf->dev);
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface_no_suspend);