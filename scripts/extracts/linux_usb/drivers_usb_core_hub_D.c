// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 36/41



	envp[1] = kasprintf(GFP_KERNEL, "OVER_CURRENT_COUNT=%u",
			port_dev->over_current_count);
	if (!envp[1])
		goto exit;

	kobject_uevent_env(&hub_dev->kobj, KOBJ_CHANGE, envp);

exit:
	kfree(envp[1]);
	kfree(envp[0]);
	kfree(port_dev_path);
}

static void port_event(struct usb_hub *hub, int port1)
		__must_hold(&port_dev->status_lock)
{
	int connect_change;
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *udev = port_dev->child;
	struct usb_device *hdev = hub->hdev;
	u16 portstatus, portchange;
	int i = 0;
	int err;

	connect_change = test_bit(port1, hub->change_bits);
	clear_bit(port1, hub->event_bits);
	clear_bit(port1, hub->wakeup_bits);

	if (usb_hub_port_status(hub, port1, &portstatus, &portchange) < 0)
		return;

	if (portchange & USB_PORT_STAT_C_CONNECTION) {
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_CONNECTION);
		connect_change = 1;
	}

	if (portchange & USB_PORT_STAT_C_ENABLE) {
		if (!connect_change)
			dev_dbg(&port_dev->dev, "enable change, status %08x\n",
					portstatus);
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_ENABLE);

		/*
		 * EM interference sometimes causes badly shielded USB devices
		 * to be shutdown by the hub, this hack enables them again.
		 * Works at least with mouse driver.
		 */
		if (!(portstatus & USB_PORT_STAT_ENABLE)
		    && !connect_change && udev) {
			dev_err(&port_dev->dev, "disabled by hub (EMI?), re-enabling...\n");
			connect_change = 1;
		}
	}

	if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
		u16 status = 0, unused;
		port_dev->over_current_count++;
		port_over_current_notify(port_dev);

		dev_dbg(&port_dev->dev, "over-current change #%u\n",
			port_dev->over_current_count);
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_OVER_CURRENT);
		msleep(100);	/* Cool down */
		hub_power_on(hub, true);
		usb_hub_port_status(hub, port1, &status, &unused);
		if (status & USB_PORT_STAT_OVERCURRENT)
			dev_err(&port_dev->dev, "over-current condition\n");
	}

	if (portchange & USB_PORT_STAT_C_RESET) {
		dev_dbg(&port_dev->dev, "reset change\n");
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_RESET);
	}
	if ((portchange & USB_PORT_STAT_C_BH_RESET)
	    && hub_is_superspeed(hdev)) {
		dev_dbg(&port_dev->dev, "warm reset change\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_BH_PORT_RESET);
	}
	if (portchange & USB_PORT_STAT_C_LINK_STATE) {
		dev_dbg(&port_dev->dev, "link state change\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_PORT_LINK_STATE);
	}
	if (portchange & USB_PORT_STAT_C_CONFIG_ERROR) {
		dev_warn(&port_dev->dev, "config error\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_PORT_CONFIG_ERROR);
	}

	/* skip port actions that require the port to be powered on */
	if (!pm_runtime_active(&port_dev->dev))
		return;

	/* skip port actions if ignore_event and early_stop are true */
	if (port_dev->ignore_event && port_dev->early_stop)
		return;

	if (hub_handle_remote_wakeup(hub, port1, portstatus, portchange))
		connect_change = 1;

	/*
	 * Avoid trying to recover a USB3 SS.Inactive port with a warm reset if
	 * the device was disconnected. A 12ms disconnect detect timer in
	 * SS.Inactive state transitions the port to RxDetect automatically.
	 * SS.Inactive link error state is common during device disconnect.
	 */
	while (hub_port_warm_reset_required(hub, port1, portstatus)) {
		if ((i++ < DETECT_DISCONNECT_TRIES) && udev) {
			u16 unused;

			msleep(20);
			usb_hub_port_status(hub, port1, &portstatus, &unused);
			dev_dbg(&port_dev->dev, "Wait for inactive link disconnect detect\n");
			continue;
		} else if (!udev || !(portstatus & USB_PORT_STAT_CONNECTION)
				|| udev->state == USB_STATE_NOTATTACHED) {
			dev_dbg(&port_dev->dev, "do warm reset, port only\n");
			err = hub_port_reset(hub, port1, NULL,
					     HUB_BH_RESET_TIME, true);
			if (!udev && err == -ENOTCONN)
				connect_change = 0;
			else if (err < 0)
				hub_port_disable(hub, port1, 1);
		} else {
			dev_dbg(&port_dev->dev, "do warm reset, full device\n");
			usb_unlock_port(port_dev);
			usb_lock_device(udev);
			usb_reset_device(udev);
			usb_unlock_device(udev);
			usb_lock_port(port_dev);
			connect_change = 0;
		}
		break;
	}

	if (connect_change)
		hub_port_connect_change(hub, port1, portstatus, portchange);
}

static void hub_event(struct work_struct *work)
{
	struct usb_device *hdev;
	struct usb_interface *intf;
	struct usb_hub *hub;
	struct device *hub_dev;
	u16 hubstatus;
	u16 hubchange;
	int i, ret;

	hub = container_of(work, struct usb_hub, events);
	hdev = hub->hdev;
	hub_dev = hub->intfdev;
	intf = to_usb_interface(hub_dev);

	kcov_remote_start_usb((u64)hdev->bus->busnum);

	dev_dbg(hub_dev, "state %d ports %d chg %04x evt %04x\n",
			hdev->state, hdev->maxchild,
			/* NOTE: expects max 15 ports... */
			(u16) hub->change_bits[0],
			(u16) hub->event_bits[0]);