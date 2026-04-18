// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 31/41



		/*
		 * Some devices time out if they are powered on
		 * when already connected. They need a second
		 * reset, so return early. But only on the first
		 * attempt, lest we get into a time-out/reset loop.
		 */
		if (rc > 0 || (rc == -ETIMEDOUT && first_time &&
				udev->speed > USB_SPEED_FULL))
			break;
	}
	return rc;
}

#define GET_DESCRIPTOR_BUFSIZE	64

/* Reset device, (re)assign address, get device descriptor.
 * Device connection must be stable, no more debouncing needed.
 * Returns device in USB_STATE_ADDRESS, except on error.
 *
 * If this is called for an already-existing device (as part of
 * usb_reset_and_verify_device), the caller must own the device lock and
 * the port lock.  For a newly detected device that is not accessible
 * through any global pointers, it's not necessary to lock the device,
 * but it is still necessary to lock the port.
 *
 * For a newly detected device, @dev_descr must be NULL.  The device
 * descriptor retrieved from the device will then be stored in
 * @udev->descriptor.  For an already existing device, @dev_descr
 * must be non-NULL.  The device descriptor will be stored there,
 * not in @udev->descriptor, because descriptors for registered
 * devices are meant to be immutable.
 */
static int
hub_port_init(struct usb_hub *hub, struct usb_device *udev, int port1,
		int retry_counter, struct usb_device_descriptor *dev_descr)
{
	struct usb_device	*hdev = hub->hdev;
	struct usb_hcd		*hcd = bus_to_hcd(hdev->bus);
	struct usb_port		*port_dev = hub->ports[port1 - 1];
	int			retries, operations, retval, i;
	unsigned		delay = HUB_SHORT_RESET_TIME;
	enum usb_device_speed	oldspeed = udev->speed;
	const char		*speed;
	int			devnum = udev->devnum;
	const char		*driver_name;
	bool			do_new_scheme;
	const bool		initial = !dev_descr;
	int			maxp0;
	struct usb_device_descriptor	*buf, *descr;

	buf = kmalloc(GET_DESCRIPTOR_BUFSIZE, GFP_NOIO);
	if (!buf)
		return -ENOMEM;

	/* root hub ports have a slightly longer reset period
	 * (from USB 2.0 spec, section 7.1.7.5)
	 */
	if (!hdev->parent) {
		delay = HUB_ROOT_RESET_TIME;
		if (port1 == hdev->bus->otg_port)
			hdev->bus->b_hnp_enable = 0;
	}

	/* Some low speed devices have problems with the quick delay, so */
	/*  be a bit pessimistic with those devices. RHbug #23670 */
	if (oldspeed == USB_SPEED_LOW)
		delay = HUB_LONG_RESET_TIME;

	/* Reset the device; full speed may morph to high speed */
	/* FIXME a USB 2.0 device may morph into SuperSpeed on reset. */
	retval = hub_port_reset(hub, port1, udev, delay, false);
	if (retval < 0)		/* error or disconnect */
		goto fail;
	/* success, speed is known */

	retval = -ENODEV;

	/* Don't allow speed changes at reset, except usb 3.0 to faster */
	if (oldspeed != USB_SPEED_UNKNOWN && oldspeed != udev->speed &&
	    !(oldspeed == USB_SPEED_SUPER && udev->speed > oldspeed)) {
		dev_dbg(&udev->dev, "device reset changed speed!\n");
		goto fail;
	}
	oldspeed = udev->speed;

	if (initial) {
		/* USB 2.0 section 5.5.3 talks about ep0 maxpacket ...
		 * it's fixed size except for full speed devices.
		 */
		switch (udev->speed) {
		case USB_SPEED_SUPER_PLUS:
		case USB_SPEED_SUPER:
			udev->ep0.desc.wMaxPacketSize = cpu_to_le16(512);
			break;
		case USB_SPEED_HIGH:		/* fixed at 64 */
			udev->ep0.desc.wMaxPacketSize = cpu_to_le16(64);
			break;
		case USB_SPEED_FULL:		/* 8, 16, 32, or 64 */
			/* to determine the ep0 maxpacket size, try to read
			 * the device descriptor to get bMaxPacketSize0 and
			 * then correct our initial guess.
			 */
			udev->ep0.desc.wMaxPacketSize = cpu_to_le16(64);
			break;
		case USB_SPEED_LOW:		/* fixed at 8 */
			udev->ep0.desc.wMaxPacketSize = cpu_to_le16(8);
			break;
		default:
			goto fail;
		}
	}

	speed = usb_speed_string(udev->speed);

	/*
	 * The controller driver may be NULL if the controller device
	 * is the middle device between platform device and roothub.
	 * This middle device may not need a device driver due to
	 * all hardware control can be at platform device driver, this
	 * platform device is usually a dual-role USB controller device.
	 */
	if (udev->bus->controller->driver)
		driver_name = udev->bus->controller->driver->name;
	else
		driver_name = udev->bus->sysdev->driver->name;

	if (udev->speed < USB_SPEED_SUPER)
		dev_info(&udev->dev,
				"%s %s USB device number %d using %s\n",
				(initial ? "new" : "reset"), speed,
				devnum, driver_name);

	if (initial) {
		/* Set up TT records, if needed  */
		if (hdev->tt) {
			udev->tt = hdev->tt;
			udev->ttport = hdev->ttport;
		} else if (udev->speed != USB_SPEED_HIGH
				&& hdev->speed == USB_SPEED_HIGH) {
			if (!hub->tt.hub) {
				dev_err(&udev->dev, "parent hub has no TT\n");
				retval = -EINVAL;
				goto fail;
			}
			udev->tt = &hub->tt;
			udev->ttport = port1;
		}
	}