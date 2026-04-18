// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 22/41



/*
 * usb_port_suspend - suspend a usb device's upstream port
 * @udev: device that's no longer in active use, not a root hub
 * Context: must be able to sleep; device not locked; pm locks held
 *
 * Suspends a USB device that isn't in active use, conserving power.
 * Devices may wake out of a suspend, if anything important happens,
 * using the remote wakeup mechanism.  They may also be taken out of
 * suspend by the host, using usb_port_resume().  It's also routine
 * to disconnect devices while they are suspended.
 *
 * This only affects the USB hardware for a device; its interfaces
 * (and, for hubs, child devices) must already have been suspended.
 *
 * Selective port suspend reduces power; most suspended devices draw
 * less than 500 uA.  It's also used in OTG, along with remote wakeup.
 * All devices below the suspended port are also suspended.
 *
 * Devices leave suspend state when the host wakes them up.  Some devices
 * also support "remote wakeup", where the device can activate the USB
 * tree above them to deliver data, such as a keypress or packet.  In
 * some cases, this wakes the USB host.
 *
 * Suspending OTG devices may trigger HNP, if that's been enabled
 * between a pair of dual-role devices.  That will change roles, such
 * as from A-Host to A-Peripheral or from B-Host back to B-Peripheral.
 *
 * Devices on USB hub ports have only one "suspend" state, corresponding
 * to ACPI D2, "may cause the device to lose some context".
 * State transitions include:
 *
 *   - suspend, resume ... when the VBUS power link stays live
 *   - suspend, disconnect ... VBUS lost
 *
 * Once VBUS drop breaks the circuit, the port it's using has to go through
 * normal re-enumeration procedures, starting with enabling VBUS power.
 * Other than re-initializing the hub (plug/unplug, except for root hubs),
 * Linux (2.6) currently has NO mechanisms to initiate that:  no hub_wq
 * timer, no SRP, no requests through sysfs.
 *
 * If Runtime PM isn't enabled or used, non-SuperSpeed devices may not get
 * suspended until their bus goes into global suspend (i.e., the root
 * hub is suspended).  Nevertheless, we change @udev->state to
 * USB_STATE_SUSPENDED as this is the device's "logical" state.  The actual
 * upstream port setting is stored in @udev->port_is_suspended.
 *
 * Returns 0 on success, else negative errno.
 */
int usb_port_suspend(struct usb_device *udev, pm_message_t msg)
{
	struct usb_hub	*hub = usb_hub_to_struct_hub(udev->parent);
	struct usb_port *port_dev = hub->ports[udev->portnum - 1];
	int		port1 = udev->portnum;
	int		status;
	bool		really_suspend = true;

	usb_lock_port(port_dev);

	/* enable remote wakeup when appropriate; this lets the device
	 * wake up the upstream hub (including maybe the root hub).
	 *
	 * NOTE:  OTG devices may issue remote wakeup (or SRP) even when
	 * we don't explicitly enable it here.
	 */
	if (udev->do_remote_wakeup) {
		status = usb_enable_remote_wakeup(udev);
		if (status) {
			dev_dbg(&udev->dev, "won't remote wakeup, status %d\n",
					status);
			/* bail if autosuspend is requested */
			if (PMSG_IS_AUTO(msg))
				goto err_wakeup;
		}
	}

	/* disable USB2 hardware LPM */
	usb_disable_usb2_hardware_lpm(udev);

	if (usb_disable_ltm(udev)) {
		dev_err(&udev->dev, "Failed to disable LTM before suspend\n");
		status = -ENOMEM;
		if (PMSG_IS_AUTO(msg))
			goto err_ltm;
	}

	/* see 7.1.7.6 */
	if (hub_is_superspeed(hub->hdev))
		status = hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_U3);

	/*
	 * For system suspend, we do not need to enable the suspend feature
	 * on individual USB-2 ports.  The devices will automatically go
	 * into suspend a few ms after the root hub stops sending packets.
	 * The USB 2.0 spec calls this "global suspend".
	 *
	 * However, many USB hubs have a bug: They don't relay wakeup requests
	 * from a downstream port if the port's suspend feature isn't on.
	 * Therefore we will turn on the suspend feature if udev or any of its
	 * descendants is enabled for remote wakeup.
	 */
	else if (PMSG_IS_AUTO(msg) || usb_wakeup_enabled_descendants(udev) > 0)
		status = set_port_feature(hub->hdev, port1,
				USB_PORT_FEAT_SUSPEND);
	else {
		really_suspend = false;
		status = 0;
	}
	if (status) {
		/* Check if the port has been suspended for the timeout case
		 * to prevent the suspended port from incorrect handling.
		 */
		if (status == -ETIMEDOUT) {
			int ret;
			u16 portstatus, portchange;

			portstatus = portchange = 0;
			ret = usb_hub_port_status(hub, port1, &portstatus,
					&portchange);

			dev_dbg(&port_dev->dev,
				"suspend timeout, status %04x\n", portstatus);

			if (ret == 0 && port_is_suspended(hub, portstatus)) {
				status = 0;
				goto suspend_done;
			}
		}

		dev_dbg(&port_dev->dev, "can't suspend, status %d\n", status);

		/* Try to enable USB3 LTM again */
		usb_enable_ltm(udev);
 err_ltm:
		/* Try to enable USB2 hardware LPM again */
		usb_enable_usb2_hardware_lpm(udev);