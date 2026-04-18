// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 37/41



	/* Lock the device, then check to see if we were
	 * disconnected while waiting for the lock to succeed. */
	usb_lock_device(hdev);
	if (unlikely(hub->disconnected))
		goto out_hdev_lock;

	/* If the hub has died, clean up after it */
	if (hdev->state == USB_STATE_NOTATTACHED) {
		hub->error = -ENODEV;
		hub_quiesce(hub, HUB_DISCONNECT);
		goto out_hdev_lock;
	}

	/* Autoresume */
	ret = usb_autopm_get_interface(intf);
	if (ret) {
		dev_dbg(hub_dev, "Can't autoresume: %d\n", ret);
		goto out_hdev_lock;
	}

	/* If this is an inactive hub, do nothing */
	if (hub->quiescing)
		goto out_autopm;

	if (hub->error) {
		dev_dbg(hub_dev, "resetting for error %d\n", hub->error);

		ret = usb_reset_device(hdev);
		if (ret) {
			dev_dbg(hub_dev, "error resetting hub: %d\n", ret);
			goto out_autopm;
		}

		hub->nerrors = 0;
		hub->error = 0;
	}

	/* deal with port status changes */
	for (i = 1; i <= hdev->maxchild; i++) {
		struct usb_port *port_dev = hub->ports[i - 1];

		if (test_bit(i, hub->event_bits)
				|| test_bit(i, hub->change_bits)
				|| test_bit(i, hub->wakeup_bits)) {
			/*
			 * The get_noresume and barrier ensure that if
			 * the port was in the process of resuming, we
			 * flush that work and keep the port active for
			 * the duration of the port_event().  However,
			 * if the port is runtime pm suspended
			 * (powered-off), we leave it in that state, run
			 * an abbreviated port_event(), and move on.
			 */
			pm_runtime_get_noresume(&port_dev->dev);
			pm_runtime_barrier(&port_dev->dev);
			usb_lock_port(port_dev);
			port_event(hub, i);
			usb_unlock_port(port_dev);
			pm_runtime_put_sync(&port_dev->dev);
		}
	}

	/* deal with hub status changes */
	if (test_and_clear_bit(0, hub->event_bits) == 0)
		;	/* do nothing */
	else if (hub_hub_status(hub, &hubstatus, &hubchange) < 0)
		dev_err(hub_dev, "get_hub_status failed\n");
	else {
		if (hubchange & HUB_CHANGE_LOCAL_POWER) {
			dev_dbg(hub_dev, "power change\n");
			clear_hub_feature(hdev, C_HUB_LOCAL_POWER);
			if (hubstatus & HUB_STATUS_LOCAL_POWER)
				/* FIXME: Is this always true? */
				hub->limited_power = 1;
			else
				hub->limited_power = 0;
		}
		if (hubchange & HUB_CHANGE_OVERCURRENT) {
			u16 status = 0;
			u16 unused;

			dev_dbg(hub_dev, "over-current change\n");
			clear_hub_feature(hdev, C_HUB_OVER_CURRENT);
			msleep(500);	/* Cool down */
			hub_power_on(hub, true);
			hub_hub_status(hub, &status, &unused);
			if (status & HUB_STATUS_OVERCURRENT)
				dev_err(hub_dev, "over-current condition\n");
		}
	}

out_autopm:
	/* Balance the usb_autopm_get_interface() above */
	usb_autopm_put_interface_no_suspend(intf);
out_hdev_lock:
	usb_unlock_device(hdev);

	/* Balance the stuff in kick_hub_wq() and allow autosuspend */
	usb_autopm_put_interface(intf);
	hub_put(hub);

	kcov_remote_stop();
}

static const struct usb_device_id hub_id_table[] = {
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR
                   | USB_DEVICE_ID_MATCH_PRODUCT
                   | USB_DEVICE_ID_MATCH_INT_CLASS,
      .idVendor = USB_VENDOR_SMSC,
      .idProduct = USB_PRODUCT_USB5534B,
      .bInterfaceClass = USB_CLASS_HUB,
      .driver_info = HUB_QUIRK_DISABLE_AUTOSUSPEND},
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR
                   | USB_DEVICE_ID_MATCH_PRODUCT,
      .idVendor = USB_VENDOR_CYPRESS,
      .idProduct = USB_PRODUCT_CY7C65632,
      .driver_info = HUB_QUIRK_DISABLE_AUTOSUSPEND},
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_INT_CLASS,
      .idVendor = USB_VENDOR_GENESYS_LOGIC,
      .bInterfaceClass = USB_CLASS_HUB,
      .driver_info = HUB_QUIRK_CHECK_PORT_AUTOSUSPEND},
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_PRODUCT,
      .idVendor = USB_VENDOR_TEXAS_INSTRUMENTS,
      .idProduct = USB_PRODUCT_TUSB8041_USB2,
      .driver_info = HUB_QUIRK_DISABLE_AUTOSUSPEND},
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_PRODUCT,
      .idVendor = USB_VENDOR_TEXAS_INSTRUMENTS,
      .idProduct = USB_PRODUCT_TUSB8041_USB3,
      .driver_info = HUB_QUIRK_DISABLE_AUTOSUSPEND},
	{ .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_PRODUCT,
	  .idVendor = USB_VENDOR_MICROCHIP,
	  .idProduct = USB_PRODUCT_USB4913,
	  .driver_info = HUB_QUIRK_REDUCE_FRAME_INTR_BINTERVAL},
	{ .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_PRODUCT,
	  .idVendor = USB_VENDOR_MICROCHIP,
	  .idProduct = USB_PRODUCT_USB4914,
	  .driver_info = HUB_QUIRK_REDUCE_FRAME_INTR_BINTERVAL},
	{ .match_flags = USB_DEVICE_ID_MATCH_VENDOR
			| USB_DEVICE_ID_MATCH_PRODUCT,
	  .idVendor = USB_VENDOR_MICROCHIP,
	  .idProduct = USB_PRODUCT_USB4915,
	  .driver_info = HUB_QUIRK_REDUCE_FRAME_INTR_BINTERVAL},
    { .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS,
      .bDeviceClass = USB_CLASS_HUB},
    { .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
      .bInterfaceClass = USB_CLASS_HUB},
    { }						/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, hub_id_table);