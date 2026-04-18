// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 10/14



/**
 * usb_suspend_both - suspend a USB device and its interfaces
 * @udev: the usb_device to suspend
 * @msg: Power Management message describing this state transition
 *
 * This is the central routine for suspending USB devices.  It calls the
 * suspend methods for all the interface drivers in @udev and then calls
 * the suspend method for @udev itself.  When the routine is called in
 * autosuspend, if an error occurs at any stage, all the interfaces
 * which were suspended are resumed so that they remain in the same
 * state as the device, but when called from system sleep, all error
 * from suspend methods of interfaces and the non-root-hub device itself
 * are simply ignored, so all suspended interfaces are only resumed
 * to the device's state when @udev is root-hub and its suspend method
 * returns failure.
 *
 * Autosuspend requests originating from a child device or an interface
 * driver may be made without the protection of @udev's device lock, but
 * all other suspend calls will hold the lock.  Usbcore will insure that
 * method calls do not arrive during bind, unbind, or reset operations.
 * However drivers must be prepared to handle suspend calls arriving at
 * unpredictable times.
 *
 * This routine can run only in process context.
 *
 * Return: 0 if the suspend succeeded.
 */
static int usb_suspend_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i = 0, n = 0;
	struct usb_interface	*intf;
	bool			offload_active = false;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	usb_offload_set_pm_locked(udev, true);
	if (msg.event == PM_EVENT_SUSPEND && usb_offload_check(udev)) {
		dev_dbg(&udev->dev, "device offloaded, skip suspend.\n");
		offload_active = true;
	}

	/* Suspend all the interfaces and then udev itself */
	if (udev->actconfig) {
		n = udev->actconfig->desc.bNumInterfaces;
		for (i = n - 1; i >= 0; --i) {
			intf = udev->actconfig->interface[i];
			/*
			 * Don't suspend interfaces with remote wakeup while
			 * the controller is active. This preserves pending
			 * interrupt urbs, allowing interrupt events to be
			 * handled during system suspend.
			 */
			if (offload_active && intf->needs_remote_wakeup) {
				dev_dbg(&intf->dev,
					"device offloaded, skip suspend.\n");
				continue;
			}
			status = usb_suspend_interface(udev, intf, msg);

			/* Ignore errors during system sleep transitions */
			if (!PMSG_IS_AUTO(msg))
				status = 0;
			if (status != 0)
				break;
		}
	}
	if (status == 0) {
		if (!offload_active)
			status = usb_suspend_device(udev, msg);

		/*
		 * Ignore errors from non-root-hub devices during
		 * system sleep transitions.  For the most part,
		 * these devices should go to low power anyway when
		 * the entire bus is suspended.
		 */
		if (udev->parent && !PMSG_IS_AUTO(msg))
			status = 0;

		/*
		 * If the device is inaccessible, don't try to resume
		 * suspended interfaces and just return the error.
		 */
		if (status && status != -EBUSY) {
			int err;
			u16 devstat;

			err = usb_get_std_status(udev, USB_RECIP_DEVICE, 0,
						 &devstat);
			if (err) {
				dev_err(&udev->dev,
					"Failed to suspend device, error %d\n",
					status);
				goto done;
			}
		}
	}

	/* If the suspend failed, resume interfaces that did get suspended */
	if (status != 0) {
		if (udev->actconfig) {
			msg.event ^= (PM_EVENT_SUSPEND | PM_EVENT_RESUME);
			while (++i < n) {
				intf = udev->actconfig->interface[i];
				usb_resume_interface(udev, intf, msg, 0);
			}
		}

	/* If the suspend succeeded then prevent any more URB submissions
	 * and flush any outstanding URBs.
	 */
	} else {
		udev->can_submit = 0;
		if (!offload_active) {
			for (i = 0; i < 16; ++i) {
				usb_hcd_flush_endpoint(udev, udev->ep_out[i]);
				usb_hcd_flush_endpoint(udev, udev->ep_in[i]);
			}
		}
	}

 done:
	if (status != 0)
		usb_offload_set_pm_locked(udev, false);
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

/**
 * usb_resume_both - resume a USB device and its interfaces
 * @udev: the usb_device to resume
 * @msg: Power Management message describing this state transition
 *
 * This is the central routine for resuming USB devices.  It calls the
 * resume method for @udev and then calls the resume methods for all
 * the interface drivers in @udev.
 *
 * Autoresume requests originating from a child device or an interface
 * driver may be made without the protection of @udev's device lock, but
 * all other resume calls will hold the lock.  Usbcore will insure that
 * method calls do not arrive during bind, unbind, or reset operations.
 * However drivers must be prepared to handle resume calls arriving at
 * unpredictable times.
 *
 * This routine can run only in process context.
 *
 * Return: 0 on success.
 */
static int usb_resume_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i;
	struct usb_interface	*intf;
	bool			offload_active = false;