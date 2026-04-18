// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 30/41



		if (!(portchange & USB_PORT_STAT_C_CONNECTION) &&
		     (portstatus & USB_PORT_STAT_CONNECTION) == connection) {
			if (!must_be_connected ||
			     (connection == USB_PORT_STAT_CONNECTION))
				stable_time += HUB_DEBOUNCE_STEP;
			if (stable_time >= HUB_DEBOUNCE_STABLE)
				break;
		} else {
			stable_time = 0;
			connection = portstatus & USB_PORT_STAT_CONNECTION;
		}

		if (portchange & USB_PORT_STAT_C_CONNECTION) {
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);
		}

		if (total_time >= HUB_DEBOUNCE_TIMEOUT)
			break;
		msleep(HUB_DEBOUNCE_STEP);
	}

	dev_dbg(&port_dev->dev, "debounce total %dms stable %dms status 0x%x\n",
			total_time, stable_time, portstatus);

	if (stable_time < HUB_DEBOUNCE_STABLE)
		return -ETIMEDOUT;
	return portstatus;
}

void usb_ep0_reinit(struct usb_device *udev)
{
	usb_disable_endpoint(udev, 0 + USB_DIR_IN, true);
	usb_disable_endpoint(udev, 0 + USB_DIR_OUT, true);
	usb_enable_endpoint(udev, &udev->ep0, true);
}
EXPORT_SYMBOL_GPL(usb_ep0_reinit);

static int hub_set_address(struct usb_device *udev, int devnum)
{
	int retval;
	unsigned int timeout_ms = USB_CTRL_SET_TIMEOUT;
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);

	if (hub->hdev->quirks & USB_QUIRK_SHORT_SET_ADDRESS_REQ_TIMEOUT)
		timeout_ms = USB_SHORT_SET_ADDRESS_REQ_TIMEOUT;

	/*
	 * The host controller will choose the device address,
	 * instead of the core having chosen it earlier
	 */
	if (!hcd->driver->address_device && devnum <= 1)
		return -EINVAL;
	if (udev->state == USB_STATE_ADDRESS)
		return 0;
	if (udev->state != USB_STATE_DEFAULT)
		return -EINVAL;
	if (hcd->driver->address_device)
		retval = hcd->driver->address_device(hcd, udev, timeout_ms);
	else
		retval = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_ADDRESS, 0, devnum, 0,
				NULL, 0, timeout_ms);
	if (retval == 0) {
		update_devnum(udev, devnum);
		/* Device now using proper address. */
		usb_set_device_state(udev, USB_STATE_ADDRESS);
		usb_ep0_reinit(udev);
	}
	return retval;
}

/*
 * There are reports of USB 3.0 devices that say they support USB 2.0 Link PM
 * when they're plugged into a USB 2.0 port, but they don't work when LPM is
 * enabled.
 *
 * Only enable USB 2.0 Link PM if the port is internal (hardwired), or the
 * device says it supports the new USB 2.0 Link PM errata by setting the BESL
 * support bit in the BOS descriptor.
 */
static void hub_set_initial_usb2_lpm_policy(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);
	int connect_type = USB_PORT_CONNECT_TYPE_UNKNOWN;

	if (!udev->usb2_hw_lpm_capable || !udev->bos)
		return;

	if (hub)
		connect_type = hub->ports[udev->portnum - 1]->connect_type;

	if ((udev->bos->ext_cap->bmAttributes & cpu_to_le32(USB_BESL_SUPPORT)) ||
			connect_type == USB_PORT_CONNECT_TYPE_HARD_WIRED) {
		udev->usb2_hw_lpm_allowed = 1;
		usb_enable_usb2_hardware_lpm(udev);
	}
}

static int hub_enable_device(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	if (!hcd->driver->enable_device)
		return 0;
	if (udev->state == USB_STATE_ADDRESS)
		return 0;
	if (udev->state != USB_STATE_DEFAULT)
		return -EINVAL;

	return hcd->driver->enable_device(hcd, udev);
}

/*
 * Get the bMaxPacketSize0 value during initialization by reading the
 * device's device descriptor.  Since we don't already know this value,
 * the transfer is unsafe and it ignores I/O errors, only testing for
 * reasonable received values.
 *
 * For "old scheme" initialization, size will be 8 so we read just the
 * start of the device descriptor, which should work okay regardless of
 * the actual bMaxPacketSize0 value.  For "new scheme" initialization,
 * size will be 64 (and buf will point to a sufficiently large buffer),
 * which might not be kosher according to the USB spec but it's what
 * Windows does and what many devices expect.
 *
 * Returns: bMaxPacketSize0 or a negative error code.
 */
static int get_bMaxPacketSize0(struct usb_device *udev,
		struct usb_device_descriptor *buf, int size, bool first_time)
{
	int i, rc;

	/*
	 * Retry on all errors; some devices are flakey.
	 * 255 is for WUSB devices, we actually need to use
	 * 512 (WUSB1.0[4.8.1]).
	 */
	for (i = 0; i < GET_MAXPACKET0_TRIES; ++i) {
		/* Start with invalid values in case the transfer fails */
		buf->bDescriptorType = buf->bMaxPacketSize0 = 0;
		rc = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
				USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
				USB_DT_DEVICE << 8, 0,
				buf, size,
				initial_descriptor_timeout);
		switch (buf->bMaxPacketSize0) {
		case 8: case 16: case 32: case 64: case 9:
			if (buf->bDescriptorType == USB_DT_DEVICE) {
				rc = buf->bMaxPacketSize0;
				break;
			}
			fallthrough;
		default:
			if (rc >= 0)
				rc = -EPROTO;
			break;
		}