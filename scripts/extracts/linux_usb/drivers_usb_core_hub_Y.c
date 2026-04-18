// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 25/41



		/* Virtual root hubs can trigger on GET_PORT_STATUS to
		 * stop resume signaling.  Then finish the resume
		 * sequence.
		 */
		status = usb_hub_port_status(hub, port1, &portstatus, &portchange);
	}

 SuspendCleared:
	if (status == 0) {
		udev->port_is_suspended = 0;
		if (hub_is_superspeed(hub->hdev)) {
			if (portchange & USB_PORT_STAT_C_LINK_STATE)
				usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_PORT_LINK_STATE);
		} else {
			if (portchange & USB_PORT_STAT_C_SUSPEND)
				usb_clear_port_feature(hub->hdev, port1,
						USB_PORT_FEAT_C_SUSPEND);
		}

		/* TRSMRCY = 10 msec */
		msleep(10);
	}

	if (udev->persist_enabled)
		status = wait_for_connected(udev, hub, port1, &portchange,
				&portstatus);

	status = check_port_resume_type(udev,
			hub, port1, status, portchange, portstatus);
	if (status == 0)
		status = finish_port_resume(udev);
	if (status < 0) {
		dev_dbg(&udev->dev, "can't resume, status %d\n", status);
		hub_port_logical_disconnect(hub, port1);
	} else  {
		/* Try to enable USB2 hardware LPM */
		usb_enable_usb2_hardware_lpm(udev);

		/* Try to enable USB3 LTM */
		usb_enable_ltm(udev);
	}

	usb_unlock_port(port_dev);

	return status;
}

int usb_remote_wakeup(struct usb_device *udev)
{
	int	status = 0;

	usb_lock_device(udev);
	if (udev->state == USB_STATE_SUSPENDED) {
		dev_dbg(&udev->dev, "usb %sresume\n", "wakeup-");
		status = usb_autoresume_device(udev);
		if (status == 0) {
			/* Let the drivers do their thing, then... */
			usb_autosuspend_device(udev);
		}
	}
	usb_unlock_device(udev);
	return status;
}

/* Returns 1 if there was a remote wakeup and a connect status change. */
static int hub_handle_remote_wakeup(struct usb_hub *hub, unsigned int port,
		u16 portstatus, u16 portchange)
		__must_hold(&port_dev->status_lock)
{
	struct usb_port *port_dev = hub->ports[port - 1];
	struct usb_device *hdev;
	struct usb_device *udev;
	int connect_change = 0;
	u16 link_state;
	int ret;

	hdev = hub->hdev;
	udev = port_dev->child;
	if (!hub_is_superspeed(hdev)) {
		if (!(portchange & USB_PORT_STAT_C_SUSPEND))
			return 0;
		usb_clear_port_feature(hdev, port, USB_PORT_FEAT_C_SUSPEND);
	} else {
		link_state = portstatus & USB_PORT_STAT_LINK_STATE;
		if (!udev || udev->state != USB_STATE_SUSPENDED ||
				(link_state != USB_SS_PORT_LS_U0 &&
				 link_state != USB_SS_PORT_LS_U1 &&
				 link_state != USB_SS_PORT_LS_U2))
			return 0;
	}

	if (udev) {
		/* TRSMRCY = 10 msec */
		msleep(10);

		usb_unlock_port(port_dev);
		ret = usb_remote_wakeup(udev);
		usb_lock_port(port_dev);
		if (ret < 0)
			connect_change = 1;
	} else {
		ret = -ENODEV;
		hub_port_disable(hub, port, 1);
	}
	dev_dbg(&port_dev->dev, "resume, status %d\n", ret);
	return connect_change;
}

static int check_ports_changed(struct usb_hub *hub)
{
	int port1;

	for (port1 = 1; port1 <= hub->hdev->maxchild; ++port1) {
		u16 portstatus, portchange;
		int status;

		status = usb_hub_port_status(hub, port1, &portstatus, &portchange);
		if (!status && portchange)
			return 1;
	}
	return 0;
}

static int hub_suspend(struct usb_interface *intf, pm_message_t msg)
{
	struct usb_hub		*hub = usb_get_intfdata(intf);
	struct usb_device	*hdev = hub->hdev;
	unsigned		port1;

	/*
	 * Warn if children aren't already suspended.
	 * Also, add up the number of wakeup-enabled descendants.
	 */
	hub->wakeup_enabled_descendants = 0;
	for (port1 = 1; port1 <= hdev->maxchild; port1++) {
		struct usb_port *port_dev = hub->ports[port1 - 1];
		struct usb_device *udev = port_dev->child;

		if (udev && udev->can_submit) {
			dev_warn(&port_dev->dev, "device %s not suspended yet\n",
					dev_name(&udev->dev));
			if (PMSG_IS_AUTO(msg))
				return -EBUSY;
		}
		if (udev)
			hub->wakeup_enabled_descendants +=
					usb_wakeup_enabled_descendants(udev);
	}

	if (hdev->do_remote_wakeup && hub->quirk_check_port_auto_suspend) {
		/* check if there are changes pending on hub ports */
		if (check_ports_changed(hub)) {
			if (PMSG_IS_AUTO(msg))
				return -EBUSY;
			pm_wakeup_event(&hdev->dev, 2000);
		}
	}

	if (hub_is_superspeed(hdev) && hdev->do_remote_wakeup) {
		/* Enable hub to send remote wakeup for all ports. */
		for (port1 = 1; port1 <= hdev->maxchild; port1++) {
			set_port_feature(hdev,
					 port1 |
					 USB_PORT_FEAT_REMOTE_WAKE_CONNECT |
					 USB_PORT_FEAT_REMOTE_WAKE_DISCONNECT |
					 USB_PORT_FEAT_REMOTE_WAKE_OVER_CURRENT,
					 USB_PORT_FEAT_REMOTE_WAKE_MASK);
		}
	}

	dev_dbg(&intf->dev, "%s\n", __func__);

	/* stop hub_wq and related activity */
	hub_quiesce(hub, HUB_SUSPEND);
	return 0;
}

/* Report wakeup requests from the ports of a resuming root hub */
static void report_wakeup_requests(struct usb_hub *hub)
{
	struct usb_device	*hdev = hub->hdev;
	struct usb_device	*udev;
	struct usb_hcd		*hcd;
	unsigned long		resuming_ports;
	int			i;

	if (hdev->parent)
		return;		/* Not a root hub */

	hcd = bus_to_hcd(hdev->bus);
	if (hcd->driver->get_resuming_ports) {