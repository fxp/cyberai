// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 19/41



		/* read and decode port status */
		if (hub_is_superspeedplus(hub->hdev))
			ret = hub_ext_port_status(hub, port1,
						  HUB_EXT_PORT_STATUS,
						  &portstatus, &portchange,
						  &ext_portstatus);
		else
			ret = usb_hub_port_status(hub, port1, &portstatus,
					      &portchange);
		if (ret < 0)
			return ret;

		/*
		 * The port state is unknown until the reset completes.
		 *
		 * On top of that, some chips may require additional time
		 * to re-establish a connection after the reset is complete,
		 * so also wait for the connection to be re-established.
		 */
		if (!(portstatus & USB_PORT_STAT_RESET) &&
		    (portstatus & USB_PORT_STAT_CONNECTION))
			break;

		/* switch to the long delay after two short delay failures */
		if (delay_time >= 2 * HUB_SHORT_RESET_TIME)
			delay = HUB_LONG_RESET_TIME;

		dev_dbg(&hub->ports[port1 - 1]->dev,
				"not %sreset yet, waiting %dms\n",
				warm ? "warm " : "", delay);
	}

	if ((portstatus & USB_PORT_STAT_RESET))
		return -EBUSY;

	if (hub_port_warm_reset_required(hub, port1, portstatus))
		return -ENOTCONN;

	/* Device went away? */
	if (!(portstatus & USB_PORT_STAT_CONNECTION))
		return -ENOTCONN;

	/* Retry if connect change is set but status is still connected.
	 * A USB 3.0 connection may bounce if multiple warm resets were issued,
	 * but the device may have successfully re-connected. Ignore it.
	 */
	if (!hub_is_superspeed(hub->hdev) &&
	    (portchange & USB_PORT_STAT_C_CONNECTION)) {
		usb_clear_port_feature(hub->hdev, port1,
				       USB_PORT_FEAT_C_CONNECTION);
		return -EAGAIN;
	}

	if (!(portstatus & USB_PORT_STAT_ENABLE))
		return -EBUSY;

	if (!udev)
		return 0;

	if (hub_is_superspeedplus(hub->hdev)) {
		/* extended portstatus Rx and Tx lane count are zero based */
		udev->rx_lanes = USB_EXT_PORT_RX_LANES(ext_portstatus) + 1;
		udev->tx_lanes = USB_EXT_PORT_TX_LANES(ext_portstatus) + 1;
		udev->ssp_rate = get_port_ssp_rate(hub->hdev, ext_portstatus);
	} else {
		udev->rx_lanes = 1;
		udev->tx_lanes = 1;
		udev->ssp_rate = USB_SSP_GEN_UNKNOWN;
	}
	if (udev->ssp_rate != USB_SSP_GEN_UNKNOWN)
		udev->speed = USB_SPEED_SUPER_PLUS;
	else if (hub_is_superspeed(hub->hdev))
		udev->speed = USB_SPEED_SUPER;
	else if (portstatus & USB_PORT_STAT_HIGH_SPEED)
		udev->speed = USB_SPEED_HIGH;
	else if (portstatus & USB_PORT_STAT_LOW_SPEED)
		udev->speed = USB_SPEED_LOW;
	else
		udev->speed = USB_SPEED_FULL;
	return 0;
}

/* Handle port reset and port warm(BH) reset (for USB3 protocol ports) */
static int hub_port_reset(struct usb_hub *hub, int port1,
			struct usb_device *udev, unsigned int delay, bool warm)
{
	int i, status;
	u16 portchange, portstatus;
	struct usb_port *port_dev = hub->ports[port1 - 1];
	int reset_recovery_time;

	if (!hub_is_superspeed(hub->hdev)) {
		if (warm) {
			dev_err(hub->intfdev, "only USB3 hub support "
						"warm reset\n");
			return -EINVAL;
		}
		/* Block EHCI CF initialization during the port reset.
		 * Some companion controllers don't like it when they mix.
		 */
		down_read(&ehci_cf_port_reset_rwsem);
	} else if (!warm) {
		/*
		 * If the caller hasn't explicitly requested a warm reset,
		 * double check and see if one is needed.
		 */
		if (usb_hub_port_status(hub, port1, &portstatus,
					&portchange) == 0)
			if (hub_port_warm_reset_required(hub, port1,
							portstatus))
				warm = true;
	}
	clear_bit(port1, hub->warm_reset_bits);

	/* Reset the port */
	for (i = 0; i < PORT_RESET_TRIES; i++) {
		status = set_port_feature(hub->hdev, port1, (warm ?
					USB_PORT_FEAT_BH_PORT_RESET :
					USB_PORT_FEAT_RESET));
		if (status == -ENODEV) {
			;	/* The hub is gone */
		} else if (status) {
			dev_err(&port_dev->dev,
					"cannot %sreset (err = %d)\n",
					warm ? "warm " : "", status);
		} else {
			status = hub_port_wait_reset(hub, port1, udev, delay,
								warm);
			if (status && status != -ENOTCONN && status != -ENODEV)
				dev_dbg(hub->intfdev,
						"port_wait_reset: err = %d\n",
						status);
		}

		/*
		 * Check for disconnect or reset, and bail out after several
		 * reset attempts to avoid warm reset loop.
		 */
		if (status == 0 || status == -ENOTCONN || status == -ENODEV ||
		    (status == -EBUSY && i == PORT_RESET_TRIES - 1)) {
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_RESET);

			if (!hub_is_superspeed(hub->hdev))
				goto done;

			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_BH_PORT_RESET);
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_PORT_LINK_STATE);

			if (udev)
				usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);

			/*
			 * If a USB 3.0 device migrates from reset to an error
			 * state, re-issue the warm reset.
			 */
			if (usb_hub_port_status(hub, port1,
					&portstatus, &portchange) < 0)
				goto done;

			if (!hub_port_warm_reset_required(hub, port1,
					portstatus))
				goto done;