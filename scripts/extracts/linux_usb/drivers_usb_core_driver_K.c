// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 11/14



	if (udev->state == USB_STATE_NOTATTACHED) {
		status = -ENODEV;
		goto done;
	}
	udev->can_submit = 1;
	if (msg.event == PM_EVENT_RESUME)
		offload_active = usb_offload_check(udev);

	/* Resume the device */
	if (udev->state == USB_STATE_SUSPENDED || udev->reset_resume) {
		if (!offload_active)
			status = usb_resume_device(udev, msg);
		else
			dev_dbg(&udev->dev,
				"device offloaded, skip resume.\n");
	}

	/* Resume the interfaces */
	if (status == 0 && udev->actconfig) {
		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];
			/*
			 * Interfaces with remote wakeup aren't suspended
			 * while the controller is active. This preserves
			 * pending interrupt urbs, allowing interrupt events
			 * to be handled during system suspend.
			 */
			if (offload_active && intf->needs_remote_wakeup) {
				dev_dbg(&intf->dev,
					"device offloaded, skip resume.\n");
				continue;
			}
			usb_resume_interface(udev, intf, msg,
					udev->reset_resume);
		}
	}
	usb_mark_last_busy(udev);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	usb_offload_set_pm_locked(udev, false);
	if (!status)
		udev->reset_resume = 0;
	return status;
}

static void choose_wakeup(struct usb_device *udev, pm_message_t msg)
{
	int	w;

	/*
	 * For FREEZE/QUIESCE, disable remote wakeups so no interrupts get
	 * generated.
	 */
	if (msg.event == PM_EVENT_FREEZE || msg.event == PM_EVENT_QUIESCE) {
		w = 0;

	} else {
		/*
		 * Enable remote wakeup if it is allowed, even if no interface
		 * drivers actually want it.
		 */
		w = device_may_wakeup(&udev->dev);
	}

	/*
	 * If the device is autosuspended with the wrong wakeup setting,
	 * autoresume now so the setting can be changed.
	 */
	if (udev->state == USB_STATE_SUSPENDED && w != udev->do_remote_wakeup)
		pm_runtime_resume(&udev->dev);
	udev->do_remote_wakeup = w;
}

/* The device lock is held by the PM core */
int usb_suspend(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev = to_usb_device(dev);
	int r;

	unbind_no_pm_drivers_interfaces(udev);

	/* From now on we are sure all drivers support suspend/resume
	 * but not necessarily reset_resume()
	 * so we may still need to unbind and rebind upon resume
	 */
	choose_wakeup(udev, msg);
	r = usb_suspend_both(udev, msg);
	if (r)
		return r;

	if (udev->quirks & USB_QUIRK_DISCONNECT_SUSPEND)
		usb_port_disable(udev);

	return 0;
}

/* The device lock is held by the PM core */
int usb_resume_complete(struct device *dev)
{
	struct usb_device *udev = to_usb_device(dev);

	/* For PM complete calls, all we do is rebind interfaces
	 * whose needs_binding flag is set
	 */
	if (udev->state != USB_STATE_NOTATTACHED)
		rebind_marked_interfaces(udev);
	return 0;
}

/* The device lock is held by the PM core */
int usb_resume(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;

	/* For all calls, take the device back to full power and
	 * tell the PM core in case it was autosuspended previously.
	 * Unbind the interfaces that will need rebinding later,
	 * because they fail to support reset_resume.
	 * (This can't be done in usb_resume_interface()
	 * above because it doesn't own the right set of locks.)
	 */
	status = usb_resume_both(udev, msg);
	if (status == 0) {
		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
		unbind_marked_interfaces(udev);
	}

	/* Avoid PM error messages for devices disconnected while suspended
	 * as we'll display regular disconnect messages just a bit later.
	 */
	if (status == -ENODEV || status == -ESHUTDOWN)
		status = 0;
	return status;
}

/**
 * usb_enable_autosuspend - allow a USB device to be autosuspended
 * @udev: the USB device which may be autosuspended
 *
 * This routine allows @udev to be autosuspended.  An autosuspend won't
 * take place until the autosuspend_delay has elapsed and all the other
 * necessary conditions are satisfied.
 *
 * The caller must hold @udev's device lock.
 */
void usb_enable_autosuspend(struct usb_device *udev)
{
	pm_runtime_allow(&udev->dev);
}
EXPORT_SYMBOL_GPL(usb_enable_autosuspend);

/**
 * usb_disable_autosuspend - prevent a USB device from being autosuspended
 * @udev: the USB device which may not be autosuspended
 *
 * This routine prevents @udev from being autosuspended and wakes it up
 * if it is already autosuspended.
 *
 * The caller must hold @udev's device lock.
 */
void usb_disable_autosuspend(struct usb_device *udev)
{
	pm_runtime_forbid(&udev->dev);
}
EXPORT_SYMBOL_GPL(usb_disable_autosuspend);