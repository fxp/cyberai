// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 11/17



	alt = usb_altnum_to_altsetting(iface, alternate);
	if (!alt) {
		dev_warn(&dev->dev, "selecting invalid altsetting %d\n",
			 alternate);
		return -EINVAL;
	}
	/*
	 * usb3 hosts configure the interface in usb_hcd_alloc_bandwidth,
	 * including freeing dropped endpoint ring buffers.
	 * Make sure the interface endpoints are flushed before that
	 */
	usb_disable_interface(dev, iface, false);

	/* Make sure we have enough bandwidth for this alternate interface.
	 * Remove the current alt setting and add the new alt setting.
	 */
	mutex_lock(hcd->bandwidth_mutex);
	/* Disable LPM, and re-enable it once the new alt setting is installed,
	 * so that the xHCI driver can recalculate the U1/U2 timeouts.
	 */
	if (usb_disable_lpm(dev)) {
		dev_err(&iface->dev, "%s Failed to disable LPM\n", __func__);
		mutex_unlock(hcd->bandwidth_mutex);
		return -ENOMEM;
	}
	/* Changing alt-setting also frees any allocated streams */
	for (i = 0; i < iface->cur_altsetting->desc.bNumEndpoints; i++)
		iface->cur_altsetting->endpoint[i].streams = 0;

	ret = usb_hcd_alloc_bandwidth(dev, NULL, iface->cur_altsetting, alt);
	if (ret < 0) {
		dev_info(&dev->dev, "Not enough bandwidth for altsetting %d\n",
				alternate);
		usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		return ret;
	}

	if (dev->quirks & USB_QUIRK_NO_SET_INTF)
		ret = -EPIPE;
	else
		ret = usb_control_msg_send(dev, 0,
					   USB_REQ_SET_INTERFACE,
					   USB_RECIP_INTERFACE, alternate,
					   interface, NULL, 0, 5000,
					   GFP_NOIO);

	/* 9.4.10 says devices don't need this and are free to STALL the
	 * request if the interface only has one alternate setting.
	 */
	if (ret == -EPIPE && iface->num_altsetting == 1) {
		dev_dbg(&dev->dev,
			"manual set_interface for iface %d, alt %d\n",
			interface, alternate);
		manual = 1;
	} else if (ret) {
		/* Re-instate the old alt setting */
		usb_hcd_alloc_bandwidth(dev, NULL, alt, iface->cur_altsetting);
		usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		return ret;
	}
	mutex_unlock(hcd->bandwidth_mutex);

	/* FIXME drivers shouldn't need to replicate/bugfix the logic here
	 * when they implement async or easily-killable versions of this or
	 * other "should-be-internal" functions (like clear_halt).
	 * should hcd+usbcore postprocess control requests?
	 */

	/* prevent submissions using previous endpoint settings */
	if (iface->cur_altsetting != alt) {
		remove_intf_ep_devs(iface);
		usb_remove_sysfs_intf_files(iface);
	}
	usb_disable_interface(dev, iface, true);

	iface->cur_altsetting = alt;

	/* Now that the interface is installed, re-enable LPM. */
	usb_unlocked_enable_lpm(dev);

	/* If the interface only has one altsetting and the device didn't
	 * accept the request, we attempt to carry out the equivalent action
	 * by manually clearing the HALT feature for each endpoint in the
	 * new altsetting.
	 */
	if (manual) {
		for (i = 0; i < alt->desc.bNumEndpoints; i++) {
			epaddr = alt->endpoint[i].desc.bEndpointAddress;
			pipe = __create_pipe(dev,
					USB_ENDPOINT_NUMBER_MASK & epaddr) |
					(usb_endpoint_out(epaddr) ?
					USB_DIR_OUT : USB_DIR_IN);

			usb_clear_halt(dev, pipe);
		}
	}

	/* 9.1.1.5: reset toggles for all endpoints in the new altsetting
	 *
	 * Note:
	 * Despite EP0 is always present in all interfaces/AS, the list of
	 * endpoints from the descriptor does not contain EP0. Due to its
	 * omnipresence one might expect EP0 being considered "affected" by
	 * any SetInterface request and hence assume toggles need to be reset.
	 * However, EP0 toggles are re-synced for every individual transfer
	 * during the SETUP stage - hence EP0 toggles are "don't care" here.
	 * (Likewise, EP0 never "halts" on well designed devices.)
	 */
	usb_enable_interface(dev, iface, true);
	if (device_is_registered(&iface->dev)) {
		usb_create_sysfs_intf_files(iface);
		create_intf_ep_devs(iface);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(usb_set_interface);