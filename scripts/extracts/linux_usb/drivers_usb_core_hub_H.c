// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 40/41



/**
 * usb_reset_device - warn interface drivers and perform a USB port reset
 * @udev: device to reset (not in NOTATTACHED state)
 *
 * Warns all drivers bound to registered interfaces (using their pre_reset
 * method), performs the port reset, and then lets the drivers know that
 * the reset is over (using their post_reset method).
 *
 * Return: The same as for usb_reset_and_verify_device().
 * However, if a reset is already in progress (for instance, if a
 * driver doesn't have pre_reset() or post_reset() callbacks, and while
 * being unbound or re-bound during the ongoing reset its disconnect()
 * or probe() routine tries to perform a second, nested reset), the
 * routine returns -EINPROGRESS.
 *
 * Note:
 * The caller must own the device lock.  For example, it's safe to use
 * this from a driver probe() routine after downloading new firmware.
 * For calls that might not occur during probe(), drivers should lock
 * the device using usb_lock_device_for_reset().
 *
 * If an interface is currently being probed or disconnected, we assume
 * its driver knows how to handle resets.  For all other interfaces,
 * if the driver doesn't have pre_reset and post_reset methods then
 * we attempt to unbind it and rebind afterward.
 */
int usb_reset_device(struct usb_device *udev)
{
	int ret;
	int i;
	unsigned int noio_flag;
	struct usb_port *port_dev;
	struct usb_host_config *config = udev->actconfig;
	struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);

	if (udev->state == USB_STATE_NOTATTACHED) {
		dev_dbg(&udev->dev, "device reset not allowed in state %d\n",
				udev->state);
		return -EINVAL;
	}

	if (!udev->parent) {
		/* this requires hcd-specific logic; see ohci_restart() */
		dev_dbg(&udev->dev, "%s for root hub!\n", __func__);
		return -EISDIR;
	}

	if (udev->reset_in_progress)
		return -EINPROGRESS;
	udev->reset_in_progress = 1;

	port_dev = hub->ports[udev->portnum - 1];

	/*
	 * Don't allocate memory with GFP_KERNEL in current
	 * context to avoid possible deadlock if usb mass
	 * storage interface or usbnet interface(iSCSI case)
	 * is included in current configuration. The easist
	 * approach is to do it for every device reset,
	 * because the device 'memalloc_noio' flag may have
	 * not been set before reseting the usb device.
	 */
	noio_flag = memalloc_noio_save();

	/* Prevent autosuspend during the reset */
	usb_autoresume_device(udev);

	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			struct usb_interface *cintf = config->interface[i];
			struct usb_driver *drv;
			int unbind = 0;

			if (cintf->dev.driver) {
				drv = to_usb_driver(cintf->dev.driver);
				if (drv->pre_reset && drv->post_reset)
					unbind = (drv->pre_reset)(cintf);
				else if (cintf->condition ==
						USB_INTERFACE_BOUND)
					unbind = 1;
				if (unbind)
					usb_forced_unbind_intf(cintf);
			}
		}
	}

	usb_lock_port(port_dev);
	ret = usb_reset_and_verify_device(udev);
	usb_unlock_port(port_dev);

	if (config) {
		for (i = config->desc.bNumInterfaces - 1; i >= 0; --i) {
			struct usb_interface *cintf = config->interface[i];
			struct usb_driver *drv;
			int rebind = cintf->needs_binding;

			if (!rebind && cintf->dev.driver) {
				drv = to_usb_driver(cintf->dev.driver);
				if (drv->post_reset)
					rebind = (drv->post_reset)(cintf);
				else if (cintf->condition ==
						USB_INTERFACE_BOUND)
					rebind = 1;
				if (rebind)
					cintf->needs_binding = 1;
			}
		}

		/* If the reset failed, hub_wq will unbind drivers later */
		if (ret == 0)
			usb_unbind_and_rebind_marked_interfaces(udev);
	}

	usb_autosuspend_device(udev);
	memalloc_noio_restore(noio_flag);
	udev->reset_in_progress = 0;
	return ret;
}
EXPORT_SYMBOL_GPL(usb_reset_device);
