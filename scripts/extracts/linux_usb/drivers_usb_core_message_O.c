// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 15/17



	/* Make sure we have bandwidth (and available HCD resources) for this
	 * configuration.  Remove endpoints from the schedule if we're dropping
	 * this configuration to set configuration 0.  After this point, the
	 * host controller will not allow submissions to dropped endpoints.  If
	 * this call fails, the device state is unchanged.
	 */
	mutex_lock(hcd->bandwidth_mutex);
	/* Disable LPM, and re-enable it once the new configuration is
	 * installed, so that the xHCI driver can recalculate the U1/U2
	 * timeouts.
	 */
	if (dev->actconfig && usb_disable_lpm(dev)) {
		dev_err(&dev->dev, "%s Failed to disable LPM\n", __func__);
		mutex_unlock(hcd->bandwidth_mutex);
		ret = -ENOMEM;
		goto free_interfaces;
	}
	ret = usb_hcd_alloc_bandwidth(dev, cp, NULL, NULL);
	if (ret < 0) {
		if (dev->actconfig)
			usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		usb_autosuspend_device(dev);
		goto free_interfaces;
	}

	/*
	 * Initialize the new interface structures and the
	 * hc/hcd/usbcore interface/endpoint state.
	 */
	for (i = 0; i < nintf; ++i) {
		struct usb_interface_cache *intfc;
		struct usb_interface *intf;
		struct usb_host_interface *alt;
		u8 ifnum;

		cp->interface[i] = intf = new_interfaces[i];
		intfc = cp->intf_cache[i];
		intf->altsetting = intfc->altsetting;
		intf->num_altsetting = intfc->num_altsetting;
		intf->authorized = !!HCD_INTF_AUTHORIZED(hcd);
		kref_get(&intfc->ref);

		alt = usb_altnum_to_altsetting(intf, 0);

		/* No altsetting 0?  We'll assume the first altsetting.
		 * We could use a GetInterface call, but if a device is
		 * so non-compliant that it doesn't have altsetting 0
		 * then I wouldn't trust its reply anyway.
		 */
		if (!alt)
			alt = &intf->altsetting[0];

		ifnum = alt->desc.bInterfaceNumber;
		intf->intf_assoc = find_iad(dev, cp, ifnum);
		intf->cur_altsetting = alt;
		usb_enable_interface(dev, intf, true);
		intf->dev.parent = &dev->dev;
		if (usb_of_has_combined_node(dev)) {
			device_set_of_node_from_dev(&intf->dev, &dev->dev);
		} else {
			intf->dev.of_node = usb_of_get_interface_node(dev,
					configuration, ifnum);
		}
		ACPI_COMPANION_SET(&intf->dev, ACPI_COMPANION(&dev->dev));
		intf->dev.driver = NULL;
		intf->dev.bus = &usb_bus_type;
		intf->dev.type = &usb_if_device_type;
		intf->dev.groups = usb_interface_groups;
		INIT_WORK(&intf->reset_ws, __usb_queue_reset_device);
		INIT_WORK(&intf->wireless_status_work, __usb_wireless_status_intf);
		intf->minor = -1;
		device_initialize(&intf->dev);
		pm_runtime_no_callbacks(&intf->dev);
		dev_set_name(&intf->dev, "%d-%s:%d.%d", dev->bus->busnum,
				dev->devpath, configuration, ifnum);
		usb_get_dev(dev);
	}
	kfree(new_interfaces);

	ret = usb_control_msg_send(dev, 0, USB_REQ_SET_CONFIGURATION, 0,
				   configuration, 0, NULL, 0,
				   USB_CTRL_SET_TIMEOUT, GFP_NOIO);
	if (ret && cp) {
		/*
		 * All the old state is gone, so what else can we do?
		 * The device is probably useless now anyway.
		 */
		usb_hcd_alloc_bandwidth(dev, NULL, NULL, NULL);
		for (i = 0; i < nintf; ++i) {
			usb_disable_interface(dev, cp->interface[i], true);
			put_device(&cp->interface[i]->dev);
			cp->interface[i] = NULL;
		}
		cp = NULL;
	}

	dev->actconfig = cp;
	mutex_unlock(hcd->bandwidth_mutex);

	if (!cp) {
		usb_set_device_state(dev, USB_STATE_ADDRESS);

		/* Leave LPM disabled while the device is unconfigured. */
		usb_autosuspend_device(dev);
		return ret;
	}
	usb_set_device_state(dev, USB_STATE_CONFIGURED);

	if (cp->string == NULL &&
			!(dev->quirks & USB_QUIRK_CONFIG_INTF_STRINGS))
		cp->string = usb_cache_string(dev, cp->desc.iConfiguration);

	/* Now that the interfaces are installed, re-enable LPM. */
	usb_unlocked_enable_lpm(dev);
	/* Enable LTM if it was turned off by usb_disable_device. */
	usb_enable_ltm(dev);

	/* Now that all the interfaces are set up, register them
	 * to trigger binding of drivers to interfaces.  probe()
	 * routines may install different altsettings and may
	 * claim() any interfaces not yet bound.  Many class drivers
	 * need that: CDC, audio, video, etc.
	 */
	for (i = 0; i < nintf; ++i) {
		struct usb_interface *intf = cp->interface[i];

		if (intf->dev.of_node &&
		    !of_device_is_available(intf->dev.of_node)) {
			dev_info(&dev->dev, "skipping disabled interface %d\n",
				 intf->cur_altsetting->desc.bInterfaceNumber);
			continue;
		}

		dev_dbg(&dev->dev,
			"adding %s (config #%d, interface %d)\n",
			dev_name(&intf->dev), configuration,
			intf->cur_altsetting->desc.bInterfaceNumber);
		device_enable_async_suspend(&intf->dev);
		ret = device_add(&intf->dev);
		if (ret != 0) {
			dev_err(&dev->dev, "device_add(%s) --> %d\n",
				dev_name(&intf->dev), ret);
			continue;
		}
		create_intf_ep_devs(intf);
	}

	usb_autosuspend_device(dev);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_set_configuration);

static LIST_HEAD(set_config_list);
static DEFINE_SPINLOCK(set_config_lock);