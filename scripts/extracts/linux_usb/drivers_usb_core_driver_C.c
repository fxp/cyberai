// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/driver.c
// Segment 3/14



	/* If the new driver doesn't allow hub-initiated LPM, and we can't
	 * disable hub-initiated LPM, then fail the probe.
	 *
	 * Otherwise, leaving LPM enabled should be harmless, because the
	 * endpoint intervals should remain the same, and the U1/U2 timeouts
	 * should remain the same.
	 *
	 * If we need to install alt setting 0 before probe, or another alt
	 * setting during probe, that should also be fine.  usb_set_interface()
	 * will attempt to disable LPM, and fail if it can't disable it.
	 */
	if (driver->disable_hub_initiated_lpm) {
		lpm_disable_error = usb_unlocked_disable_lpm(udev);
		if (lpm_disable_error) {
			dev_err(&intf->dev, "%s Failed to disable LPM for driver %s\n",
				__func__, driver->name);
			error = lpm_disable_error;
			goto err;
		}
	}

	/* Carry out a deferred switch to altsetting 0 */
	if (intf->needs_altsetting0) {
		error = usb_set_interface(udev, intf->altsetting[0].
				desc.bInterfaceNumber, 0);
		if (error < 0)
			goto err;
		intf->needs_altsetting0 = 0;
	}

	error = driver->probe(intf, id);
	if (error)
		goto err;

	intf->condition = USB_INTERFACE_BOUND;

	/* If the LPM disable succeeded, balance the ref counts. */
	if (!lpm_disable_error)
		usb_unlocked_enable_lpm(udev);

	usb_autosuspend_device(udev);
	return error;

 err:
	usb_set_intfdata(intf, NULL);
	intf->needs_remote_wakeup = 0;
	intf->condition = USB_INTERFACE_UNBOUND;

	/* If the LPM disable succeeded, balance the ref counts. */
	if (!lpm_disable_error)
		usb_unlocked_enable_lpm(udev);

	/* Unbound interfaces are always runtime-PM-disabled and -suspended */
	if (driver->supports_autosuspend)
		pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);

	usb_autosuspend_device(udev);
	return error;
}

/* called from driver core with dev locked */
static int usb_unbind_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_host_endpoint *ep, **eps = NULL;
	struct usb_device *udev;
	int i, j, error, r;
	int lpm_disable_error = -ENODEV;

	intf->condition = USB_INTERFACE_UNBINDING;

	/* Autoresume for set_interface call below */
	udev = interface_to_usbdev(intf);
	error = usb_autoresume_device(udev);

	/* If hub-initiated LPM policy may change, attempt to disable LPM until
	 * the driver is unbound.  If LPM isn't disabled, that's fine because it
	 * wouldn't be enabled unless all the bound interfaces supported
	 * hub-initiated LPM.
	 */
	if (driver->disable_hub_initiated_lpm)
		lpm_disable_error = usb_unlocked_disable_lpm(udev);

	/*
	 * Terminate all URBs for this interface unless the driver
	 * supports "soft" unbinding and the device is still present.
	 */
	if (!driver->soft_unbind || udev->state == USB_STATE_NOTATTACHED)
		usb_disable_interface(udev, intf, false);

	driver->disconnect(intf);

	/* Free streams */
	for (i = 0, j = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep = &intf->cur_altsetting->endpoint[i];
		if (ep->streams == 0)
			continue;
		if (j == 0) {
			eps = kmalloc_array(USB_MAXENDPOINTS, sizeof(void *),
				      GFP_KERNEL);
			if (!eps)
				break;
		}
		eps[j++] = ep;
	}
	if (j) {
		usb_free_streams(intf, eps, j, GFP_KERNEL);
		kfree(eps);
	}

	/* Reset other interface state.
	 * We cannot do a Set-Interface if the device is suspended or
	 * if it is prepared for a system sleep (since installing a new
	 * altsetting means creating new endpoint device entries).
	 * When either of these happens, defer the Set-Interface.
	 */
	if (intf->cur_altsetting->desc.bAlternateSetting == 0) {
		/* Already in altsetting 0 so skip Set-Interface.
		 * Just re-enable it without affecting the endpoint toggles.
		 */
		usb_enable_interface(udev, intf, false);
	} else if (!error && !intf->dev.power.is_prepared) {
		r = usb_set_interface(udev, intf->altsetting[0].
				desc.bInterfaceNumber, 0);
		if (r < 0)
			intf->needs_altsetting0 = 1;
	} else {
		intf->needs_altsetting0 = 1;
	}
	usb_set_intfdata(intf, NULL);

	intf->condition = USB_INTERFACE_UNBOUND;
	intf->needs_remote_wakeup = 0;

	/* Attempt to re-enable USB3 LPM, if the disable succeeded. */
	if (!lpm_disable_error)
		usb_unlocked_enable_lpm(udev);

	/* Unbound interfaces are always runtime-PM-disabled and -suspended */
	if (driver->supports_autosuspend)
		pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);

	if (!error)
		usb_autosuspend_device(udev);

	return 0;
}

static void usb_shutdown_interface(struct device *dev)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_driver *driver;

	if (!dev->driver)
		return;

	driver = to_usb_driver(dev->driver);
	if (driver->shutdown)
		driver->shutdown(intf);
}