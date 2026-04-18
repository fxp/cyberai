// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 21/41



		/* Late port handoff can set status-change bits */
		if (portchange & USB_PORT_STAT_C_CONNECTION)
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);
		if (portchange & USB_PORT_STAT_C_ENABLE)
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_ENABLE);

		/*
		 * Whatever made this reset-resume necessary may have
		 * turned on the port1 bit in hub->change_bits.  But after
		 * a successful reset-resume we want the bit to be clear;
		 * if it was on it would indicate that something happened
		 * following the reset-resume.
		 */
		clear_bit(port1, hub->change_bits);
	}

	return status;
}

int usb_disable_ltm(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	/* Check if the roothub and device supports LTM. */
	if (!usb_device_supports_ltm(hcd->self.root_hub) ||
			!usb_device_supports_ltm(udev))
		return 0;

	/* Clear Feature LTM Enable can only be sent if the device is
	 * configured.
	 */
	if (!udev->actconfig)
		return 0;

	return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_CLEAR_FEATURE, USB_RECIP_DEVICE,
			USB_DEVICE_LTM_ENABLE, 0, NULL, 0,
			USB_CTRL_SET_TIMEOUT);
}
EXPORT_SYMBOL_GPL(usb_disable_ltm);

void usb_enable_ltm(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	/* Check if the roothub and device supports LTM. */
	if (!usb_device_supports_ltm(hcd->self.root_hub) ||
			!usb_device_supports_ltm(udev))
		return;

	/* Set Feature LTM Enable can only be sent if the device is
	 * configured.
	 */
	if (!udev->actconfig)
		return;

	usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_SET_FEATURE, USB_RECIP_DEVICE,
			USB_DEVICE_LTM_ENABLE, 0, NULL, 0,
			USB_CTRL_SET_TIMEOUT);
}
EXPORT_SYMBOL_GPL(usb_enable_ltm);

/*
 * usb_enable_remote_wakeup - enable remote wakeup for a device
 * @udev: target device
 *
 * For USB-2 devices: Set the device's remote wakeup feature.
 *
 * For USB-3 devices: Assume there's only one function on the device and
 * enable remote wake for the first interface.  FIXME if the interface
 * association descriptor shows there's more than one function.
 */
static int usb_enable_remote_wakeup(struct usb_device *udev)
{
	if (udev->speed < USB_SPEED_SUPER)
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	else
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_INTERFACE,
				USB_INTRF_FUNC_SUSPEND,
				USB_INTRF_FUNC_SUSPEND_RW |
					USB_INTRF_FUNC_SUSPEND_LP,
				NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/*
 * usb_disable_remote_wakeup - disable remote wakeup for a device
 * @udev: target device
 *
 * For USB-2 devices: Clear the device's remote wakeup feature.
 *
 * For USB-3 devices: Assume there's only one function on the device and
 * disable remote wake for the first interface.  FIXME if the interface
 * association descriptor shows there's more than one function.
 */
static int usb_disable_remote_wakeup(struct usb_device *udev)
{
	if (udev->speed < USB_SPEED_SUPER)
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	else
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_INTERFACE,
				USB_INTRF_FUNC_SUSPEND,	0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
}

/* Count of wakeup-enabled devices at or below udev */
unsigned usb_wakeup_enabled_descendants(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);

	return udev->do_remote_wakeup +
			(hub ? hub->wakeup_enabled_descendants : 0);
}
EXPORT_SYMBOL_GPL(usb_wakeup_enabled_descendants);