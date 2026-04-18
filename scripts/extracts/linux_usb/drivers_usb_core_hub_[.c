// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 27/41



	if (udev->state != USB_STATE_CONFIGURED) {
		dev_dbg(&udev->dev, "%s: Can't %s %s state "
				"for unconfigured device.\n",
				__func__, str_enable_disable(enable),
				usb3_lpm_names[state]);
		return -EINVAL;
	}

	if (enable) {
		/*
		 * Now send the control transfer to enable device-initiated LPM
		 * for either U1 or U2.
		 */
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE,
				USB_RECIP_DEVICE,
				feature,
				0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	} else {
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_CLEAR_FEATURE,
				USB_RECIP_DEVICE,
				feature,
				0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	}
	if (ret < 0) {
		dev_warn(&udev->dev, "%s of device-initiated %s failed.\n",
			 str_enable_disable(enable), usb3_lpm_names[state]);
		return -EBUSY;
	}
	return 0;
}

static int usb_set_lpm_timeout(struct usb_device *udev,
		enum usb3_link_state state, int timeout)
{
	int ret;
	int feature;

	switch (state) {
	case USB3_LPM_U1:
		feature = USB_PORT_FEAT_U1_TIMEOUT;
		break;
	case USB3_LPM_U2:
		feature = USB_PORT_FEAT_U2_TIMEOUT;
		break;
	default:
		dev_warn(&udev->dev, "%s: Can't set timeout for non-U1 or U2 state.\n",
				__func__);
		return -EINVAL;
	}

	if (state == USB3_LPM_U1 && timeout > USB3_LPM_U1_MAX_TIMEOUT &&
			timeout != USB3_LPM_DEVICE_INITIATED) {
		dev_warn(&udev->dev, "Failed to set %s timeout to 0x%x, "
				"which is a reserved value.\n",
				usb3_lpm_names[state], timeout);
		return -EINVAL;
	}

	ret = set_port_feature(udev->parent,
			USB_PORT_LPM_TIMEOUT(timeout) | udev->portnum,
			feature);
	if (ret < 0) {
		dev_warn(&udev->dev, "Failed to set %s timeout to 0x%x,"
				"error code %i\n", usb3_lpm_names[state],
				timeout, ret);
		return -EBUSY;
	}
	if (state == USB3_LPM_U1)
		udev->u1_params.timeout = timeout;
	else
		udev->u2_params.timeout = timeout;
	return 0;
}

/*
 * Don't allow device intiated U1/U2 if device isn't in the configured state,
 * or the system exit latency + one bus interval is greater than the minimum
 * service interval of any active periodic endpoint. See USB 3.2 section 9.4.9
 */
static bool usb_device_may_initiate_lpm(struct usb_device *udev,
					enum usb3_link_state state)
{
	unsigned int sel;		/* us */
	int i, j;

	if (!udev->lpm_devinit_allow || !udev->actconfig)
		return false;

	if (state == USB3_LPM_U1)
		sel = DIV_ROUND_UP(udev->u1_params.sel, 1000);
	else if (state == USB3_LPM_U2)
		sel = DIV_ROUND_UP(udev->u2_params.sel, 1000);
	else
		return false;

	for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_interface *intf;
		struct usb_endpoint_descriptor *desc;
		unsigned int interval;

		intf = udev->actconfig->interface[i];
		if (!intf)
			continue;

		for (j = 0; j < intf->cur_altsetting->desc.bNumEndpoints; j++) {
			desc = &intf->cur_altsetting->endpoint[j].desc;

			if (usb_endpoint_xfer_int(desc) ||
			    usb_endpoint_xfer_isoc(desc)) {
				interval = (1 << (desc->bInterval - 1)) * 125;
				if (sel + 125 > interval)
					return false;
			}
		}
	}
	return true;
}

/*
 * Enable the hub-initiated U1/U2 idle timeouts, and enable device-initiated
 * U1/U2 entry.
 *
 * We will attempt to enable U1 or U2, but there are no guarantees that the
 * control transfers to set the hub timeout or enable device-initiated U1/U2
 * will be successful.
 *
 * If the control transfer to enable device-initiated U1/U2 entry fails, then
 * hub-initiated U1/U2 will be disabled.
 *
 * If we cannot set the parent hub U1/U2 timeout, we attempt to let the xHCI
 * driver know about it.  If that call fails, it should be harmless, and just
 * take up more slightly more bus bandwidth for unnecessary U1/U2 exit latency.
 */
static int usb_enable_link_state(struct usb_hcd *hcd, struct usb_device *udev,
		enum usb3_link_state state)
{
	int timeout;
	__u8 u1_mel;
	__le16 u2_mel;

	/* Skip if the device BOS descriptor couldn't be read */
	if (!udev->bos)
		return -EINVAL;

	u1_mel = udev->bos->ss_cap->bU1devExitLat;
	u2_mel = udev->bos->ss_cap->bU2DevExitLat;

	/* If the device says it doesn't have *any* exit latency to come out of
	 * U1 or U2, it's probably lying.  Assume it doesn't implement that link
	 * state.
	 */
	if ((state == USB3_LPM_U1 && u1_mel == 0) ||
			(state == USB3_LPM_U2 && u2_mel == 0))
		return -EINVAL;

	/* We allow the host controller to set the U1/U2 timeout internally
	 * first, so that it can change its schedule to account for the
	 * additional latency to send data to a device in a lower power
	 * link state.
	 */
	timeout = hcd->driver->enable_usb3_lpm_timeout(hcd, udev, state);

	/* xHCI host controller doesn't want to enable this LPM state. */
	if (timeout == 0)
		return -EINVAL;

	if (timeout < 0) {
		dev_warn(&udev->dev, "Could not enable %s link state, "
				"xHCI error %i.\n", usb3_lpm_names[state],
				timeout);
		return timeout;
	}