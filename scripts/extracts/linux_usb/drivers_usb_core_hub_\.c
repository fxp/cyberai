// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 28/41



	if (usb_set_lpm_timeout(udev, state, timeout)) {
		/* If we can't set the parent hub U1/U2 timeout,
		 * device-initiated LPM won't be allowed either, so let the xHCI
		 * host know that this link state won't be enabled.
		 */
		hcd->driver->disable_usb3_lpm_timeout(hcd, udev, state);
		return -EBUSY;
	}

	if (state == USB3_LPM_U1)
		udev->usb3_lpm_u1_enabled = 1;
	else if (state == USB3_LPM_U2)
		udev->usb3_lpm_u2_enabled = 1;

	return 0;
}
/*
 * Disable the hub-initiated U1/U2 idle timeouts, and disable device-initiated
 * U1/U2 entry.
 *
 * If this function returns -EBUSY, the parent hub will still allow U1/U2 entry.
 * If zero is returned, the parent will not allow the link to go into U1/U2.
 *
 * If zero is returned, device-initiated U1/U2 entry may still be enabled, but
 * it won't have an effect on the bus link state because the parent hub will
 * still disallow device-initiated U1/U2 entry.
 *
 * If zero is returned, the xHCI host controller may still think U1/U2 entry is
 * possible.  The result will be slightly more bus bandwidth will be taken up
 * (to account for U1/U2 exit latency), but it should be harmless.
 */
static int usb_disable_link_state(struct usb_hcd *hcd, struct usb_device *udev,
		enum usb3_link_state state)
{
	switch (state) {
	case USB3_LPM_U1:
	case USB3_LPM_U2:
		break;
	default:
		dev_warn(&udev->dev, "%s: Can't disable non-U1 or U2 state.\n",
				__func__);
		return -EINVAL;
	}

	if (usb_set_lpm_timeout(udev, state, 0))
		return -EBUSY;

	if (hcd->driver->disable_usb3_lpm_timeout(hcd, udev, state))
		dev_warn(&udev->dev, "Could not disable xHCI %s timeout, "
				"bus schedule bandwidth may be impacted.\n",
				usb3_lpm_names[state]);

	/* As soon as usb_set_lpm_timeout(0) return 0, hub initiated LPM
	 * is disabled. Hub will disallows link to enter U1/U2 as well,
	 * even device is initiating LPM. Hence LPM is disabled if hub LPM
	 * timeout set to 0, no matter device-initiated LPM is disabled or
	 * not.
	 */
	if (state == USB3_LPM_U1)
		udev->usb3_lpm_u1_enabled = 0;
	else if (state == USB3_LPM_U2)
		udev->usb3_lpm_u2_enabled = 0;

	return 0;
}

/*
 * Disable hub-initiated and device-initiated U1 and U2 entry.
 * Caller must own the bandwidth_mutex.
 *
 * This will call usb_enable_lpm() on failure, which will decrement
 * lpm_disable_count, and will re-enable LPM if lpm_disable_count reaches zero.
 */
int usb_disable_lpm(struct usb_device *udev)
{
	struct usb_hcd *hcd;
	int err;

	if (!udev || !udev->parent ||
			udev->speed < USB_SPEED_SUPER ||
			!udev->lpm_capable ||
			udev->state < USB_STATE_CONFIGURED)
		return 0;

	hcd = bus_to_hcd(udev->bus);
	if (!hcd || !hcd->driver->disable_usb3_lpm_timeout)
		return 0;

	udev->lpm_disable_count++;
	if ((udev->u1_params.timeout == 0 && udev->u2_params.timeout == 0))
		return 0;

	/* If LPM is enabled, attempt to disable it. */
	if (usb_disable_link_state(hcd, udev, USB3_LPM_U1))
		goto disable_failed;
	if (usb_disable_link_state(hcd, udev, USB3_LPM_U2))
		goto disable_failed;

	err = usb_set_device_initiated_lpm(udev, USB3_LPM_U1, false);
	if (!err)
		usb_set_device_initiated_lpm(udev, USB3_LPM_U2, false);

	return 0;

disable_failed:
	udev->lpm_disable_count--;

	return -EBUSY;
}
EXPORT_SYMBOL_GPL(usb_disable_lpm);

/* Grab the bandwidth_mutex before calling usb_disable_lpm() */
int usb_unlocked_disable_lpm(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	int ret;

	if (!hcd)
		return -EINVAL;

	mutex_lock(hcd->bandwidth_mutex);
	ret = usb_disable_lpm(udev);
	mutex_unlock(hcd->bandwidth_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(usb_unlocked_disable_lpm);

/*
 * Attempt to enable device-initiated and hub-initiated U1 and U2 entry.  The
 * xHCI host policy may prevent U1 or U2 from being enabled.
 *
 * Other callers may have disabled link PM, so U1 and U2 entry will be disabled
 * until the lpm_disable_count drops to zero.  Caller must own the
 * bandwidth_mutex.
 */
void usb_enable_lpm(struct usb_device *udev)
{
	struct usb_hcd *hcd;
	struct usb_hub *hub;
	struct usb_port *port_dev;

	if (!udev || !udev->parent ||
			udev->speed < USB_SPEED_SUPER ||
			!udev->lpm_capable ||
			udev->state < USB_STATE_CONFIGURED)
		return;

	udev->lpm_disable_count--;
	hcd = bus_to_hcd(udev->bus);
	/* Double check that we can both enable and disable LPM.
	 * Device must be configured to accept set feature U1/U2 timeout.
	 */
	if (!hcd || !hcd->driver->enable_usb3_lpm_timeout ||
			!hcd->driver->disable_usb3_lpm_timeout)
		return;

	if (udev->lpm_disable_count > 0)
		return;

	hub = usb_hub_to_struct_hub(udev->parent);
	if (!hub)
		return;

	port_dev = hub->ports[udev->portnum - 1];

	if (port_dev->usb3_lpm_u1_permit)
		if (usb_enable_link_state(hcd, udev, USB3_LPM_U1))
			return;

	if (port_dev->usb3_lpm_u2_permit)
		if (usb_enable_link_state(hcd, udev, USB3_LPM_U2))
			return;