// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 10/41



	/* Request the entire hub descriptor.
	 * hub->descriptor can handle USB_MAXCHILDREN ports,
	 * but a (non-SS) hub can/will return fewer bytes here.
	 */
	ret = get_hub_descriptor(hdev, hub->descriptor);
	if (ret < 0) {
		message = "can't read hub descriptor";
		goto fail;
	}

	maxchild = USB_MAXCHILDREN;
	if (hub_is_superspeed(hdev))
		maxchild = min_t(unsigned, maxchild, USB_SS_MAXPORTS);

	if (hub->descriptor->bNbrPorts > maxchild) {
		message = "hub has too many ports!";
		ret = -ENODEV;
		goto fail;
	} else if (hub->descriptor->bNbrPorts == 0) {
		message = "hub doesn't have any ports!";
		ret = -ENODEV;
		goto fail;
	}

	/*
	 * Accumulate wHubDelay + 40ns for every hub in the tree of devices.
	 * The resulting value will be used for SetIsochDelay() request.
	 */
	if (hub_is_superspeed(hdev) || hub_is_superspeedplus(hdev)) {
		u32 delay = __le16_to_cpu(hub->descriptor->u.ss.wHubDelay);

		if (hdev->parent)
			delay += hdev->parent->hub_delay;

		delay += USB_TP_TRANSMISSION_DELAY;
		hdev->hub_delay = min_t(u32, delay, USB_TP_TRANSMISSION_DELAY_MAX);
	}

	maxchild = hub->descriptor->bNbrPorts;
	dev_info(hub_dev, "%d port%s detected\n", maxchild,
			str_plural(maxchild));

	hub->ports = kzalloc_objs(struct usb_port *, maxchild);
	if (!hub->ports) {
		ret = -ENOMEM;
		goto fail;
	}

	wHubCharacteristics = le16_to_cpu(hub->descriptor->wHubCharacteristics);
	if (hub_is_superspeed(hdev)) {
		unit_load = 150;
		full_load = 900;
	} else {
		unit_load = 100;
		full_load = 500;
	}

	/* FIXME for USB 3.0, skip for now */
	if ((wHubCharacteristics & HUB_CHAR_COMPOUND) &&
			!(hub_is_superspeed(hdev))) {
		char	portstr[USB_MAXCHILDREN + 1];

		for (i = 0; i < maxchild; i++)
			portstr[i] = hub->descriptor->u.hs.DeviceRemovable
				    [((i + 1) / 8)] & (1 << ((i + 1) % 8))
				? 'F' : 'R';
		portstr[maxchild] = 0;
		dev_dbg(hub_dev, "compound device; port removable status: %s\n", portstr);
	} else
		dev_dbg(hub_dev, "standalone hub\n");

	switch (wHubCharacteristics & HUB_CHAR_LPSM) {
	case HUB_CHAR_COMMON_LPSM:
		dev_dbg(hub_dev, "ganged power switching\n");
		break;
	case HUB_CHAR_INDV_PORT_LPSM:
		dev_dbg(hub_dev, "individual port power switching\n");
		break;
	case HUB_CHAR_NO_LPSM:
	case HUB_CHAR_LPSM:
		dev_dbg(hub_dev, "no power switching (usb 1.0)\n");
		break;
	}

	switch (wHubCharacteristics & HUB_CHAR_OCPM) {
	case HUB_CHAR_COMMON_OCPM:
		dev_dbg(hub_dev, "global over-current protection\n");
		break;
	case HUB_CHAR_INDV_PORT_OCPM:
		dev_dbg(hub_dev, "individual port over-current protection\n");
		break;
	case HUB_CHAR_NO_OCPM:
	case HUB_CHAR_OCPM:
		dev_dbg(hub_dev, "no over-current protection\n");
		break;
	}

	spin_lock_init(&hub->tt.lock);
	INIT_LIST_HEAD(&hub->tt.clear_list);
	INIT_WORK(&hub->tt.clear_work, hub_tt_work);
	switch (hdev->descriptor.bDeviceProtocol) {
	case USB_HUB_PR_FS:
		break;
	case USB_HUB_PR_HS_SINGLE_TT:
		dev_dbg(hub_dev, "Single TT\n");
		hub->tt.hub = hdev;
		break;
	case USB_HUB_PR_HS_MULTI_TT:
		ret = usb_set_interface(hdev, 0, 1);
		if (ret == 0) {
			dev_dbg(hub_dev, "TT per port\n");
			hub->tt.multi = 1;
		} else
			dev_err(hub_dev, "Using single TT (err %d)\n",
				ret);
		hub->tt.hub = hdev;
		break;
	case USB_HUB_PR_SS:
		/* USB 3.0 hubs don't have a TT */
		break;
	default:
		dev_dbg(hub_dev, "Unrecognized hub protocol %d\n",
			hdev->descriptor.bDeviceProtocol);
		break;
	}

	/* Note 8 FS bit times == (8 bits / 12000000 bps) ~= 666ns */
	switch (wHubCharacteristics & HUB_CHAR_TTTT) {
	case HUB_TTTT_8_BITS:
		if (hdev->descriptor.bDeviceProtocol != 0) {
			hub->tt.think_time = 666;
			dev_dbg(hub_dev, "TT requires at most %d "
					"FS bit times (%d ns)\n",
				8, hub->tt.think_time);
		}
		break;
	case HUB_TTTT_16_BITS:
		hub->tt.think_time = 666 * 2;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			16, hub->tt.think_time);
		break;
	case HUB_TTTT_24_BITS:
		hub->tt.think_time = 666 * 3;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			24, hub->tt.think_time);
		break;
	case HUB_TTTT_32_BITS:
		hub->tt.think_time = 666 * 4;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			32, hub->tt.think_time);
		break;
	}

	/* probe() zeroes hub->indicator[] */
	if (wHubCharacteristics & HUB_CHAR_PORTIND) {
		hub->has_indicators = 1;
		dev_dbg(hub_dev, "Port indicators are supported\n");
	}

	dev_dbg(hub_dev, "power on to power good time: %dms\n",
		hub->descriptor->bPwrOn2PwrGood * 2);