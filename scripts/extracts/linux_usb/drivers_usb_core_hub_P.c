// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 16/41



		/* enable HNP before suspend, it's simpler */
		if (port1 == bus->otg_port) {
			bus->b_hnp_enable = 1;
			err = usb_control_msg(udev,
				usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, 0,
				USB_DEVICE_B_HNP_ENABLE,
				0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
			if (err < 0) {
				/*
				 * OTG MESSAGE: report errors here,
				 * customize to match your product.
				 */
				dev_err(&udev->dev, "can't set HNP mode: %d\n",
									err);
				bus->b_hnp_enable = 0;
			}
		} else if (desc->bLength == sizeof
				(struct usb_otg_descriptor)) {
			/*
			 * We are operating on a legacy OTP device
			 * These should be told that they are operating
			 * on the wrong port if we have another port that does
			 * support HNP
			 */
			if (bus->otg_port != 0) {
				/* Set a_alt_hnp_support for legacy otg device */
				err = usb_control_msg(udev,
					usb_sndctrlpipe(udev, 0),
					USB_REQ_SET_FEATURE, 0,
					USB_DEVICE_A_ALT_HNP_SUPPORT,
					0, NULL, 0,
					USB_CTRL_SET_TIMEOUT);
				if (err < 0)
					dev_err(&udev->dev,
						"set a_alt_hnp_support failed: %d\n",
						err);
			}
		}
	}
#endif
	return err;
}


/**
 * usb_enumerate_device - Read device configs/intfs/otg (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * This is only called by usb_new_device() -- all comments that apply there
 * apply here wrt to environment.
 *
 * If the device is WUSB and not authorized, we don't attempt to read
 * the string descriptors, as they will be errored out by the device
 * until it has been authorized.
 *
 * Return: 0 if successful. A negative error code otherwise.
 */
static int usb_enumerate_device(struct usb_device *udev)
{
	int err;
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	if (udev->config == NULL) {
		err = usb_get_configuration(udev);
		if (err < 0) {
			if (err != -ENODEV)
				dev_err(&udev->dev, "can't read configurations, error %d\n",
						err);
			return err;
		}
	}

	/* read the standard strings and cache them if present */
	udev->product = usb_cache_string(udev, udev->descriptor.iProduct);
	udev->manufacturer = usb_cache_string(udev,
					      udev->descriptor.iManufacturer);
	udev->serial = usb_cache_string(udev, udev->descriptor.iSerialNumber);

	err = usb_enumerate_device_otg(udev);
	if (err < 0)
		return err;

	if (IS_ENABLED(CONFIG_USB_OTG_PRODUCTLIST) && hcd->tpl_support &&
		!is_targeted(udev)) {
		/* Maybe it can talk to us, though we can't talk to it.
		 * (Includes HNP test device.)
		 */
		if (IS_ENABLED(CONFIG_USB_OTG) && (udev->bus->b_hnp_enable
			|| udev->bus->is_b_host)) {
			err = usb_port_suspend(udev, PMSG_AUTO_SUSPEND);
			if (err < 0)
				dev_dbg(&udev->dev, "HNP fail, %d\n", err);
		}
		return -ENOTSUPP;
	}

	usb_detect_interface_quirks(udev);

	return 0;
}

static void set_usb_port_removable(struct usb_device *udev)
{
	struct usb_device *hdev = udev->parent;
	struct usb_hub *hub;
	u8 port = udev->portnum;
	u16 wHubCharacteristics;
	bool removable = true;

	dev_set_removable(&udev->dev, DEVICE_REMOVABLE_UNKNOWN);

	if (!hdev)
		return;

	hub = usb_hub_to_struct_hub(udev->parent);

	/*
	 * If the platform firmware has provided information about a port,
	 * use that to determine whether it's removable.
	 */
	switch (hub->ports[udev->portnum - 1]->connect_type) {
	case USB_PORT_CONNECT_TYPE_HOT_PLUG:
		dev_set_removable(&udev->dev, DEVICE_REMOVABLE);
		return;
	case USB_PORT_CONNECT_TYPE_HARD_WIRED:
	case USB_PORT_NOT_USED:
		dev_set_removable(&udev->dev, DEVICE_FIXED);
		return;
	default:
		break;
	}

	/*
	 * Otherwise, check whether the hub knows whether a port is removable
	 * or not
	 */
	wHubCharacteristics = le16_to_cpu(hub->descriptor->wHubCharacteristics);

	if (!(wHubCharacteristics & HUB_CHAR_COMPOUND))
		return;

	if (hub_is_superspeed(hdev)) {
		if (le16_to_cpu(hub->descriptor->u.ss.DeviceRemovable)
				& (1 << port))
			removable = false;
	} else {
		if (hub->descriptor->u.hs.DeviceRemovable[port / 8] & (1 << (port % 8)))
			removable = false;
	}

	if (removable)
		dev_set_removable(&udev->dev, DEVICE_REMOVABLE);
	else
		dev_set_removable(&udev->dev, DEVICE_FIXED);

}