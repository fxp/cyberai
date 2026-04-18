// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/config.c
// Segment 3/8



	if (d->bLength >= USB_DT_ENDPOINT_AUDIO_SIZE)
		n = USB_DT_ENDPOINT_AUDIO_SIZE;
	else if (d->bLength >= USB_DT_ENDPOINT_SIZE)
		n = USB_DT_ENDPOINT_SIZE;
	else {
		dev_notice(ddev, "config %d interface %d altsetting %d has an "
		    "invalid endpoint descriptor of length %d, skipping\n",
		    cfgno, inum, asnum, d->bLength);
		goto skip_to_next_endpoint_or_interface_descriptor;
	}

	i = usb_endpoint_num(d);
	if (i == 0) {
		dev_notice(ddev, "config %d interface %d altsetting %d has an "
		    "invalid descriptor for endpoint zero, skipping\n",
		    cfgno, inum, asnum);
		goto skip_to_next_endpoint_or_interface_descriptor;
	}

	/* Only store as many endpoints as we have room for */
	if (ifp->desc.bNumEndpoints >= num_ep)
		goto skip_to_next_endpoint_or_interface_descriptor;

	/* Save a copy of the descriptor and use it instead of the original */
	endpoint = &ifp->endpoint[ifp->desc.bNumEndpoints];
	memcpy(&endpoint->desc, d, n);
	d = &endpoint->desc;

	/* Clear the reserved bits in bEndpointAddress */
	i = d->bEndpointAddress &
			(USB_ENDPOINT_DIR_MASK | USB_ENDPOINT_NUMBER_MASK);
	if (i != d->bEndpointAddress) {
		dev_notice(ddev, "config %d interface %d altsetting %d has an endpoint descriptor with address 0x%X, changing to 0x%X\n",
		    cfgno, inum, asnum, d->bEndpointAddress, i);
		endpoint->desc.bEndpointAddress = i;
	}

	/* Check for duplicate endpoint addresses */
	if (config_endpoint_is_duplicate(config, inum, asnum, d)) {
		dev_notice(ddev, "config %d interface %d altsetting %d has a duplicate endpoint with address 0x%X, skipping\n",
				cfgno, inum, asnum, d->bEndpointAddress);
		goto skip_to_next_endpoint_or_interface_descriptor;
	}

	/* Ignore some endpoints */
	if (udev->quirks & USB_QUIRK_ENDPOINT_IGNORE) {
		if (usb_endpoint_is_ignored(udev, ifp, d)) {
			dev_notice(ddev, "config %d interface %d altsetting %d has an ignored endpoint with address 0x%X, skipping\n",
					cfgno, inum, asnum,
					d->bEndpointAddress);
			goto skip_to_next_endpoint_or_interface_descriptor;
		}
	}

	/* Accept this endpoint */
	++ifp->desc.bNumEndpoints;
	INIT_LIST_HEAD(&endpoint->urb_list);

	/*
	 * Fix up bInterval values outside the legal range.
	 * Use 10 or 8 ms if no proper value can be guessed.
	 */
	i = 0;		/* i = min, j = max, n = default */
	j = 255;
	if (usb_endpoint_xfer_int(d)) {
		i = 1;
		switch (udev->speed) {
		case USB_SPEED_SUPER_PLUS:
		case USB_SPEED_SUPER:
		case USB_SPEED_HIGH:
			/*
			 * Many device manufacturers are using full-speed
			 * bInterval values in high-speed interrupt endpoint
			 * descriptors. Try to fix those and fall back to an
			 * 8-ms default value otherwise.
			 */
			n = fls(d->bInterval*8);
			if (n == 0)
				n = 7;	/* 8 ms = 2^(7-1) uframes */
			j = 16;

			/*
			 * Adjust bInterval for quirked devices.
			 */
			/*
			 * This quirk fixes bIntervals reported in ms.
			 */
			if (udev->quirks & USB_QUIRK_LINEAR_FRAME_INTR_BINTERVAL) {
				n = clamp(fls(d->bInterval) + 3, i, j);
				i = j = n;
			}
			/*
			 * This quirk fixes bIntervals reported in
			 * linear microframes.
			 */
			if (udev->quirks & USB_QUIRK_LINEAR_UFRAME_INTR_BINTERVAL) {
				n = clamp(fls(d->bInterval), i, j);
				i = j = n;
			}
			break;
		default:		/* USB_SPEED_FULL or _LOW */
			/*
			 * For low-speed, 10 ms is the official minimum.
			 * But some "overclocked" devices might want faster
			 * polling so we'll allow it.
			 */
			n = 10;
			break;
		}
	} else if (usb_endpoint_xfer_isoc(d)) {
		i = 1;
		j = 16;
		switch (udev->speed) {
		case USB_SPEED_HIGH:
			n = 7;		/* 8 ms = 2^(7-1) uframes */
			break;
		default:		/* USB_SPEED_FULL */
			n = 4;		/* 8 ms = 2^(4-1) frames */
			break;
		}
	}
	if (d->bInterval < i || d->bInterval > j) {
		dev_notice(ddev, "config %d interface %d altsetting %d "
		    "endpoint 0x%X has an invalid bInterval %d, "
		    "changing to %d\n",
		    cfgno, inum, asnum,
		    d->bEndpointAddress, d->bInterval, n);
		endpoint->desc.bInterval = n;
	}

	/* Some buggy low-speed devices have Bulk endpoints, which is
	 * explicitly forbidden by the USB spec.  In an attempt to make
	 * them usable, we will try treating them as Interrupt endpoints.
	 */
	if (udev->speed == USB_SPEED_LOW && usb_endpoint_xfer_bulk(d)) {
		dev_notice(ddev, "config %d interface %d altsetting %d "
		    "endpoint 0x%X is Bulk; changing to Interrupt\n",
		    cfgno, inum, asnum, d->bEndpointAddress);
		endpoint->desc.bmAttributes = USB_ENDPOINT_XFER_INT;
		endpoint->desc.bInterval = 1;
		if (usb_endpoint_maxp(&endpoint->desc) > 8)
			endpoint->desc.wMaxPacketSize = cpu_to_le16(8);
	}

	/*
	 * Validate the wMaxPacketSize field.
	 * eUSB2 devices (see USB 2.0 Double Isochronous IN ECN 9.6.6 Endpoint)
	 * and devices with isochronous endpoints in altsetting 0 (see USB 2.0
	 * end of section 5.6.3) have wMaxPacketSize = 0.
	 * So don't warn about those.
	 */
	maxp = le16_to_cpu(endpoint->desc.wMaxPacketSize);