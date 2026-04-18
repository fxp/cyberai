// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 20/41



			/*
			 * If the port is in SS.Inactive or Compliance Mode, the
			 * hot or warm reset failed.  Try another warm reset.
			 */
			if (!warm) {
				dev_dbg(&port_dev->dev,
						"hot reset failed, warm reset\n");
				warm = true;
			}
		}

		dev_dbg(&port_dev->dev,
				"not enabled, trying %sreset again...\n",
				warm ? "warm " : "");
		delay = HUB_LONG_RESET_TIME;
	}

	dev_err(&port_dev->dev, "Cannot enable. Maybe the USB cable is bad?\n");

done:
	if (status == 0) {
		if (port_dev->quirks & USB_PORT_QUIRK_FAST_ENUM)
			usleep_range(10000, 12000);
		else {
			/* TRSTRCY = 10 ms; plus some extra */
			reset_recovery_time = 10 + 40;

			/* Hub needs extra delay after resetting its port. */
			if (hub->hdev->quirks & USB_QUIRK_HUB_SLOW_RESET)
				reset_recovery_time += 100;

			msleep(reset_recovery_time);
		}

		if (udev) {
			struct usb_hcd *hcd = bus_to_hcd(udev->bus);

			update_devnum(udev, 0);
			/* The xHC may think the device is already reset,
			 * so ignore the status.
			 */
			if (hcd->driver->reset_device)
				hcd->driver->reset_device(hcd, udev);

			usb_set_device_state(udev, USB_STATE_DEFAULT);
		}
	} else {
		if (udev)
			usb_set_device_state(udev, USB_STATE_NOTATTACHED);
	}

	if (!hub_is_superspeed(hub->hdev))
		up_read(&ehci_cf_port_reset_rwsem);

	return status;
}

/*
 * hub_port_stop_enumerate - stop USB enumeration or ignore port events
 * @hub: target hub
 * @port1: port num of the port
 * @retries: port retries number of hub_port_init()
 *
 * Return:
 *    true: ignore port actions/events or give up connection attempts.
 *    false: keep original behavior.
 *
 * This function will be based on retries to check whether the port which is
 * marked with early_stop attribute would stop enumeration or ignore events.
 *
 * Note:
 * This function didn't change anything if early_stop is not set, and it will
 * prevent all connection attempts when early_stop is set and the attempts of
 * the port are more than 1.
 */
static bool hub_port_stop_enumerate(struct usb_hub *hub, int port1, int retries)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];

	if (port_dev->early_stop) {
		if (port_dev->ignore_event)
			return true;

		/*
		 * We want unsuccessful attempts to fail quickly.
		 * Since some devices may need one failure during
		 * port initialization, we allow two tries but no
		 * more.
		 */
		if (retries < 2)
			return false;

		port_dev->ignore_event = 1;
	} else
		port_dev->ignore_event = 0;

	return port_dev->ignore_event;
}

/* Check if a port is power on */
int usb_port_is_power_on(struct usb_hub *hub, unsigned int portstatus)
{
	int ret = 0;

	if (hub_is_superspeed(hub->hdev)) {
		if (portstatus & USB_SS_PORT_STAT_POWER)
			ret = 1;
	} else {
		if (portstatus & USB_PORT_STAT_POWER)
			ret = 1;
	}

	return ret;
}

static void usb_lock_port(struct usb_port *port_dev)
		__acquires(&port_dev->status_lock)
{
	mutex_lock(&port_dev->status_lock);
	__acquire(&port_dev->status_lock);
}

static void usb_unlock_port(struct usb_port *port_dev)
		__releases(&port_dev->status_lock)
{
	mutex_unlock(&port_dev->status_lock);
	__release(&port_dev->status_lock);
}

#ifdef	CONFIG_PM

/* Check if a port is suspended(USB2.0 port) or in U3 state(USB3.0 port) */
static int port_is_suspended(struct usb_hub *hub, unsigned portstatus)
{
	int ret = 0;

	if (hub_is_superspeed(hub->hdev)) {
		if ((portstatus & USB_PORT_STAT_LINK_STATE)
				== USB_SS_PORT_LS_U3)
			ret = 1;
	} else {
		if (portstatus & USB_PORT_STAT_SUSPEND)
			ret = 1;
	}

	return ret;
}

/* Determine whether the device on a port is ready for a normal resume,
 * is ready for a reset-resume, or should be disconnected.
 */
static int check_port_resume_type(struct usb_device *udev,
		struct usb_hub *hub, int port1,
		int status, u16 portchange, u16 portstatus)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	int retries = 3;

 retry:
	/* Is a warm reset needed to recover the connection? */
	if (status == 0 && udev->reset_resume
		&& hub_port_warm_reset_required(hub, port1, portstatus)) {
		/* pass */;
	}
	/* Is the device still present? */
	else if (status || port_is_suspended(hub, portstatus) ||
			!usb_port_is_power_on(hub, portstatus)) {
		if (status >= 0)
			status = -ENODEV;
	} else if (!(portstatus & USB_PORT_STAT_CONNECTION)) {
		if (retries--) {
			usleep_range(200, 300);
			status = usb_hub_port_status(hub, port1, &portstatus,
							     &portchange);
			goto retry;
		}
		status = -ENODEV;
	}

	/* Can't do a normal resume if the port isn't enabled,
	 * so try a reset-resume instead.
	 */
	else if (!(portstatus & USB_PORT_STAT_ENABLE) && !udev->reset_resume) {
		if (udev->persist_enabled)
			udev->reset_resume = 1;
		else
			status = -ENODEV;
	}

	if (status) {
		dev_dbg(&port_dev->dev, "status %04x.%04x after resume, %d\n",
				portchange, portstatus, status);
	} else if (udev->reset_resume) {