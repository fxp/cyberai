// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 32/41



	/* Why interleave GET_DESCRIPTOR and SET_ADDRESS this way?
	 * Because device hardware and firmware is sometimes buggy in
	 * this area, and this is how Linux has done it for ages.
	 * Change it cautiously.
	 *
	 * NOTE:  If use_new_scheme() is true we will start by issuing
	 * a 64-byte GET_DESCRIPTOR request.  This is what Windows does,
	 * so it may help with some non-standards-compliant devices.
	 * Otherwise we start with SET_ADDRESS and then try to read the
	 * first 8 bytes of the device descriptor to get the ep0 maxpacket
	 * value.
	 */
	do_new_scheme = use_new_scheme(udev, retry_counter, port_dev);

	for (retries = 0; retries < GET_DESCRIPTOR_TRIES; (++retries, msleep(100))) {
		if (hub_port_stop_enumerate(hub, port1, retries)) {
			retval = -ENODEV;
			break;
		}

		if (do_new_scheme) {
			retval = hub_enable_device(udev);
			if (retval < 0) {
				dev_err(&udev->dev,
					"hub failed to enable device, error %d\n",
					retval);
				goto fail;
			}

			maxp0 = get_bMaxPacketSize0(udev, buf,
					GET_DESCRIPTOR_BUFSIZE, retries == 0);
			if (maxp0 > 0 && !initial &&
					maxp0 != udev->descriptor.bMaxPacketSize0) {
				dev_err(&udev->dev, "device reset changed ep0 maxpacket size!\n");
				retval = -ENODEV;
				goto fail;
			}

			retval = hub_port_reset(hub, port1, udev, delay, false);
			if (retval < 0)		/* error or disconnect */
				goto fail;
			if (oldspeed != udev->speed) {
				dev_dbg(&udev->dev,
					"device reset changed speed!\n");
				retval = -ENODEV;
				goto fail;
			}
			if (maxp0 < 0) {
				if (maxp0 != -ENODEV)
					dev_err(&udev->dev, "device descriptor read/64, error %d\n",
							maxp0);
				retval = maxp0;
				continue;
			}
		}

		for (operations = 0; operations < SET_ADDRESS_TRIES; ++operations) {
			retval = hub_set_address(udev, devnum);
			if (retval >= 0)
				break;
			msleep(200);
		}
		if (retval < 0) {
			if (retval != -ENODEV)
				dev_err(&udev->dev, "device not accepting address %d, error %d\n",
						devnum, retval);
			goto fail;
		}
		if (udev->speed >= USB_SPEED_SUPER) {
			devnum = udev->devnum;
			dev_info(&udev->dev,
					"%s SuperSpeed%s%s USB device number %d using %s\n",
					(udev->config) ? "reset" : "new",
				 (udev->speed == USB_SPEED_SUPER_PLUS) ?
						" Plus" : "",
				 (udev->ssp_rate == USB_SSP_GEN_2x2) ?
						" Gen 2x2" :
				 (udev->ssp_rate == USB_SSP_GEN_2x1) ?
						" Gen 2x1" :
				 (udev->ssp_rate == USB_SSP_GEN_1x2) ?
						" Gen 1x2" : "",
				 devnum, driver_name);
		}

		/*
		 * cope with hardware quirkiness:
		 *  - let SET_ADDRESS settle, some device hardware wants it
		 *  - read ep0 maxpacket even for high and low speed,
		 */
		msleep(10);

		if (do_new_scheme)
			break;

		maxp0 = get_bMaxPacketSize0(udev, buf, 8, retries == 0);
		if (maxp0 < 0) {
			retval = maxp0;
			if (retval != -ENODEV)
				dev_err(&udev->dev,
					"device descriptor read/8, error %d\n",
					retval);
		} else {
			u32 delay;

			if (!initial && maxp0 != udev->descriptor.bMaxPacketSize0) {
				dev_err(&udev->dev, "device reset changed ep0 maxpacket size!\n");
				retval = -ENODEV;
				goto fail;
			}

			delay = udev->parent->hub_delay;
			udev->hub_delay = min_t(u32, delay,
						USB_TP_TRANSMISSION_DELAY_MAX);
			retval = usb_set_isoch_delay(udev);
			if (retval) {
				dev_dbg(&udev->dev,
					"Failed set isoch delay, error %d\n",
					retval);
				retval = 0;
			}
			break;
		}
	}
	if (retval)
		goto fail;

	/*
	 * Check the ep0 maxpacket guess and correct it if necessary.
	 * maxp0 is the value stored in the device descriptor;
	 * i is the value it encodes (logarithmic for SuperSpeed or greater).
	 */
	i = maxp0;
	if (udev->speed >= USB_SPEED_SUPER) {
		if (maxp0 <= 16)
			i = 1 << maxp0;
		else
			i = 0;		/* Invalid */
	}
	if (usb_endpoint_maxp(&udev->ep0.desc) == i) {
		;	/* Initial ep0 maxpacket guess is right */
	} else if (((udev->speed == USB_SPEED_FULL ||
				udev->speed == USB_SPEED_HIGH) &&
			(i == 8 || i == 16 || i == 32 || i == 64)) ||
			(udev->speed >= USB_SPEED_SUPER && i > 0)) {
		/* Initial guess is wrong; use the descriptor's value */
		if (udev->speed == USB_SPEED_FULL)
			dev_dbg(&udev->dev, "ep0 maxpacket = %d\n", i);
		else
			dev_warn(&udev->dev, "Using ep0 maxpacket: %d\n", i);
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(i);
		usb_ep0_reinit(udev);
	} else {
		/* Initial guess is wrong and descriptor's value is invalid */
		dev_err(&udev->dev, "Invalid ep0 maxpacket: %d\n", maxp0);
		retval = -EMSGSIZE;
		goto fail;
	}

	descr = usb_get_device_descriptor(udev);
	if (IS_ERR(descr)) {
		retval = PTR_ERR(descr);
		if (retval != -ENODEV)
			dev_err(&udev->dev, "device descriptor read/all, error %d\n",
					retval);
		goto fail;
	}
	if (initial)
		udev->descriptor = *descr;
	else
		*dev_descr = *descr;
	kfree(descr);