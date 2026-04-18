// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 39/41



	bos = udev->bos;
	udev->bos = NULL;

	if (udev->reset_resume)
		hub_hc_release_resources(udev);

	mutex_lock(hcd->address0_mutex);

	for (i = 0; i < PORT_INIT_TRIES; ++i) {
		if (hub_port_stop_enumerate(parent_hub, port1, i)) {
			ret = -ENODEV;
			break;
		}

		/* ep0 maxpacket size may change; let the HCD know about it.
		 * Other endpoints will be handled by re-enumeration. */
		usb_ep0_reinit(udev);
		ret = hub_port_init(parent_hub, udev, port1, i, &descriptor);
		if (ret >= 0 || ret == -ENOTCONN || ret == -ENODEV)
			break;
	}
	mutex_unlock(hcd->address0_mutex);

	if (ret < 0)
		goto re_enumerate;

	/* Device might have changed firmware (DFU or similar) */
	if (descriptors_changed(udev, &descriptor, bos)) {
		dev_info(&udev->dev, "device firmware changed\n");
		goto re_enumerate;
	}

	/* Restore the device's previous configuration */
	if (!udev->actconfig)
		goto done;

	/*
	 * Some devices can't handle setting default altsetting 0 with a
	 * Set-Interface request. Disable host-side endpoints of those
	 * interfaces here. Enable and reset them back after host has set
	 * its internal endpoint structures during usb_hcd_alloc_bandwith()
	 */
	for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
		intf = udev->actconfig->interface[i];
		if (intf->cur_altsetting->desc.bAlternateSetting == 0)
			usb_disable_interface(udev, intf, true);
	}

	mutex_lock(hcd->bandwidth_mutex);
	ret = usb_hcd_alloc_bandwidth(udev, udev->actconfig, NULL, NULL);
	if (ret < 0) {
		dev_warn(&udev->dev,
				"Busted HC?  Not enough HCD resources for "
				"old configuration.\n");
		mutex_unlock(hcd->bandwidth_mutex);
		goto re_enumerate;
	}
	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_SET_CONFIGURATION, 0,
			udev->actconfig->desc.bConfigurationValue, 0,
			NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(&udev->dev,
			"can't restore configuration #%d (error=%d)\n",
			udev->actconfig->desc.bConfigurationValue, ret);
		mutex_unlock(hcd->bandwidth_mutex);
		goto re_enumerate;
	}
	mutex_unlock(hcd->bandwidth_mutex);
	usb_set_device_state(udev, USB_STATE_CONFIGURED);

	/* Put interfaces back into the same altsettings as before.
	 * Don't bother to send the Set-Interface request for interfaces
	 * that were already in altsetting 0; besides being unnecessary,
	 * many devices can't handle it.  Instead just reset the host-side
	 * endpoint state.
	 */
	for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_host_config *config = udev->actconfig;
		struct usb_interface_descriptor *desc;

		intf = config->interface[i];
		desc = &intf->cur_altsetting->desc;
		if (desc->bAlternateSetting == 0) {
			usb_enable_interface(udev, intf, true);
			ret = 0;
		} else {
			/* Let the bandwidth allocation function know that this
			 * device has been reset, and it will have to use
			 * alternate setting 0 as the current alternate setting.
			 */
			intf->resetting_device = 1;
			ret = usb_set_interface(udev, desc->bInterfaceNumber,
					desc->bAlternateSetting);
			intf->resetting_device = 0;
		}
		if (ret < 0) {
			dev_err(&udev->dev, "failed to restore interface %d "
				"altsetting %d (error=%d)\n",
				desc->bInterfaceNumber,
				desc->bAlternateSetting,
				ret);
			goto re_enumerate;
		}
		/* Resetting also frees any allocated streams */
		for (j = 0; j < intf->cur_altsetting->desc.bNumEndpoints; j++)
			intf->cur_altsetting->endpoint[j].streams = 0;
	}

done:
	/* Now that the alt settings are re-installed, enable LTM and LPM. */
	usb_enable_usb2_hardware_lpm(udev);
	usb_unlocked_enable_lpm(udev);
	usb_enable_ltm(udev);
	usb_release_bos_descriptor(udev);
	udev->bos = bos;
	return 0;

re_enumerate:
	usb_release_bos_descriptor(udev);
	udev->bos = bos;
	hub_port_logical_disconnect(parent_hub, port1);
	return -ENODEV;
}