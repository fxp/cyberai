// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 12/41



	pm_suspend_ignore_children(&intf->dev, false);

	if (hub->quirk_disable_autosuspend)
		usb_autopm_put_interface(intf);

	onboard_dev_destroy_pdevs(&hub->onboard_devs);

	hub_put(hub);
}

static bool hub_descriptor_is_sane(struct usb_host_interface *desc)
{
	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if (desc->desc.bInterfaceSubClass != 0 &&
	    desc->desc.bInterfaceSubClass != 1)
		return false;

	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (desc->desc.bNumEndpoints != 1)
		return false;

	/* If the first endpoint is not interrupt IN, we'd better punt! */
	if (!usb_endpoint_is_int_in(&desc->endpoint[0].desc))
		return false;

        return true;
}

static int hub_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *desc;
	struct usb_device *hdev;
	struct usb_hub *hub;

	desc = intf->cur_altsetting;
	hdev = interface_to_usbdev(intf);

	/*
	 * The USB 2.0 spec prohibits hubs from having more than one
	 * configuration or interface, and we rely on this prohibition.
	 * Refuse to accept a device that violates it.
	 */
	if (hdev->descriptor.bNumConfigurations > 1 ||
			hdev->actconfig->desc.bNumInterfaces > 1) {
		dev_err(&intf->dev, "Invalid hub with more than one config or interface\n");
		return -EINVAL;
	}

	/*
	 * Set default autosuspend delay as 0 to speedup bus suspend,
	 * based on the below considerations:
	 *
	 * - Unlike other drivers, the hub driver does not rely on the
	 *   autosuspend delay to provide enough time to handle a wakeup
	 *   event, and the submitted status URB is just to check future
	 *   change on hub downstream ports, so it is safe to do it.
	 *
	 * - The patch might cause one or more auto supend/resume for
	 *   below very rare devices when they are plugged into hub
	 *   first time:
	 *
	 *   	devices having trouble initializing, and disconnect
	 *   	themselves from the bus and then reconnect a second
	 *   	or so later
	 *
	 *   	devices just for downloading firmware, and disconnects
	 *   	themselves after completing it
	 *
	 *   For these quite rare devices, their drivers may change the
	 *   autosuspend delay of their parent hub in the probe() to one
	 *   appropriate value to avoid the subtle problem if someone
	 *   does care it.
	 *
	 * - The patch may cause one or more auto suspend/resume on
	 *   hub during running 'lsusb', but it is probably too
	 *   infrequent to worry about.
	 *
	 * - Change autosuspend delay of hub can avoid unnecessary auto
	 *   suspend timer for hub, also may decrease power consumption
	 *   of USB bus.
	 *
	 * - If user has indicated to prevent autosuspend by passing
	 *   usbcore.autosuspend = -1 then keep autosuspend disabled.
	 */
#ifdef CONFIG_PM
	if (hdev->dev.power.autosuspend_delay >= 0)
		pm_runtime_set_autosuspend_delay(&hdev->dev, 0);
#endif

	/*
	 * Hubs have proper suspend/resume support, except for root hubs
	 * where the controller driver doesn't have bus_suspend and
	 * bus_resume methods.
	 */
	if (hdev->parent) {		/* normal device */
		usb_enable_autosuspend(hdev);
	} else {			/* root hub */
		const struct hc_driver *drv = bus_to_hcd(hdev->bus)->driver;

		if (drv->bus_suspend && drv->bus_resume)
			usb_enable_autosuspend(hdev);
	}

	if (hdev->level == MAX_TOPO_LEVEL) {
		dev_err(&intf->dev,
			"Unsupported bus topology: hub nested too deep\n");
		return -E2BIG;
	}

#ifdef	CONFIG_USB_OTG_DISABLE_EXTERNAL_HUB
	if (hdev->parent) {
		dev_warn(&intf->dev, "ignoring external hub\n");
		return -ENODEV;
	}
#endif

	if (!hub_descriptor_is_sane(desc)) {
		dev_err(&intf->dev, "bad descriptor, ignoring hub\n");
		return -EIO;
	}

	/* We found a hub */
	dev_info(&intf->dev, "USB hub found\n");

	hub = kzalloc_obj(*hub);
	if (!hub)
		return -ENOMEM;

	kref_init(&hub->kref);
	hub->intfdev = &intf->dev;
	hub->hdev = hdev;
	INIT_DELAYED_WORK(&hub->leds, led_work);
	INIT_DELAYED_WORK(&hub->init_work, NULL);
	INIT_DELAYED_WORK(&hub->post_resume_work, hub_post_resume);
	INIT_WORK(&hub->events, hub_event);
	INIT_LIST_HEAD(&hub->onboard_devs);
	spin_lock_init(&hub->irq_urb_lock);
	timer_setup(&hub->irq_urb_retry, hub_retry_irq_urb, 0);
	usb_get_intf(intf);
	usb_get_dev(hdev);

	usb_set_intfdata(intf, hub);
	intf->needs_remote_wakeup = 1;
	pm_suspend_ignore_children(&intf->dev, true);

	if (hdev->speed == USB_SPEED_HIGH)
		highspeed_hubs++;

	if (id->driver_info & HUB_QUIRK_CHECK_PORT_AUTOSUSPEND)
		hub->quirk_check_port_auto_suspend = 1;

	if (id->driver_info & HUB_QUIRK_DISABLE_AUTOSUSPEND) {
		hub->quirk_disable_autosuspend = 1;
		usb_autopm_get_interface_no_resume(intf);
	}

	if ((id->driver_info & HUB_QUIRK_REDUCE_FRAME_INTR_BINTERVAL) &&
	    desc->endpoint[0].desc.bInterval > USB_REDUCE_FRAME_INTR_BINTERVAL) {
		desc->endpoint[0].desc.bInterval =
			USB_REDUCE_FRAME_INTR_BINTERVAL;
		/* Tell the HCD about the interrupt ep's new bInterval */
		usb_set_interface(hdev, 0, 0);
	}