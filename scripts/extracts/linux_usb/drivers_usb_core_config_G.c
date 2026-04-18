// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/config.c
// Segment 7/8



	for (cfgno = 0; cfgno < ncfg; cfgno++) {
		/* We grab just the first descriptor so we know how long
		 * the whole configuration is */
		result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
		    desc, USB_DT_CONFIG_SIZE);
		if (result < 0) {
			dev_err(ddev, "unable to read config index %d "
			    "descriptor/%s: %d\n", cfgno, "start", result);
			if (result != -EPIPE)
				goto err;
			dev_notice(ddev, "chopping to %d config(s)\n", cfgno);
			dev->descriptor.bNumConfigurations = cfgno;
			break;
		} else if (result < 4) {
			dev_err(ddev, "config index %d descriptor too short "
			    "(expected %i, got %i)\n", cfgno,
			    USB_DT_CONFIG_SIZE, result);
			result = -EINVAL;
			goto err;
		}
		length = max_t(int, le16_to_cpu(desc->wTotalLength),
		    USB_DT_CONFIG_SIZE);

		/* Now that we know the length, get the whole thing */
		bigbuffer = kmalloc(length, GFP_KERNEL);
		if (!bigbuffer) {
			result = -ENOMEM;
			goto err;
		}

		if (dev->quirks & USB_QUIRK_DELAY_INIT)
			msleep(200);

		result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
		    bigbuffer, length);
		if (result < 0) {
			dev_err(ddev, "unable to read config index %d "
			    "descriptor/%s\n", cfgno, "all");
			kfree(bigbuffer);
			goto err;
		}
		if (result < length) {
			dev_notice(ddev, "config index %d descriptor too short "
			    "(expected %i, got %i)\n", cfgno, length, result);
			length = result;
		}

		dev->rawdescriptors[cfgno] = bigbuffer;

		result = usb_parse_configuration(dev, cfgno,
		    &dev->config[cfgno], bigbuffer, length);
		if (result < 0) {
			++cfgno;
			goto err;
		}
	}

err:
	kfree(desc);
	dev->descriptor.bNumConfigurations = cfgno;

	return result;
}

void usb_release_bos_descriptor(struct usb_device *dev)
{
	if (dev->bos) {
		kfree(dev->bos->desc);
		kfree(dev->bos);
		dev->bos = NULL;
	}
}

static const __u8 bos_desc_len[256] = {
	[USB_CAP_TYPE_WIRELESS_USB] = USB_DT_USB_WIRELESS_CAP_SIZE,
	[USB_CAP_TYPE_EXT]          = USB_DT_USB_EXT_CAP_SIZE,
	[USB_SS_CAP_TYPE]           = USB_DT_USB_SS_CAP_SIZE,
	[USB_SSP_CAP_TYPE]          = USB_DT_USB_SSP_CAP_SIZE(1),
	[CONTAINER_ID_TYPE]         = USB_DT_USB_SS_CONTN_ID_SIZE,
	[USB_PTM_CAP_TYPE]          = USB_DT_USB_PTM_ID_SIZE,
};

/* Get BOS descriptor set */
int usb_get_bos_descriptor(struct usb_device *dev)
{
	struct device *ddev = &dev->dev;
	struct usb_bos_descriptor *bos;
	struct usb_dev_cap_header *cap;
	struct usb_ssp_cap_descriptor *ssp_cap;
	unsigned char *buffer, *buffer0;
	int length, total_len, num, i, ssac;
	__u8 cap_type;
	int ret;

	if (dev->quirks & USB_QUIRK_NO_BOS) {
		dev_dbg(ddev, "skipping BOS descriptor\n");
		return -ENOMSG;
	}

	bos = kzalloc_obj(*bos);
	if (!bos)
		return -ENOMEM;

	/* Get BOS descriptor */
	ret = usb_get_descriptor(dev, USB_DT_BOS, 0, bos, USB_DT_BOS_SIZE);
	if (ret < USB_DT_BOS_SIZE || bos->bLength < USB_DT_BOS_SIZE) {
		dev_notice(ddev, "unable to get BOS descriptor or descriptor too short\n");
		if (ret >= 0)
			ret = -ENOMSG;
		kfree(bos);
		return ret;
	}

	length = bos->bLength;
	total_len = le16_to_cpu(bos->wTotalLength);
	num = bos->bNumDeviceCaps;
	kfree(bos);
	if (total_len < length)
		return -EINVAL;

	dev->bos = kzalloc_obj(*dev->bos);
	if (!dev->bos)
		return -ENOMEM;

	/* Now let's get the whole BOS descriptor set */
	buffer = kzalloc(total_len, GFP_KERNEL);
	if (!buffer) {
		ret = -ENOMEM;
		goto err;
	}
	dev->bos->desc = (struct usb_bos_descriptor *)buffer;

	ret = usb_get_descriptor(dev, USB_DT_BOS, 0, buffer, total_len);
	if (ret < total_len) {
		dev_notice(ddev, "unable to get BOS descriptor set\n");
		if (ret >= 0)
			ret = -ENOMSG;
		goto err;
	}

	buffer0 = buffer;
	total_len -= length;
	buffer += length;

	for (i = 0; i < num; i++) {
		cap = (struct usb_dev_cap_header *)buffer;

		if (total_len < sizeof(*cap) || total_len < cap->bLength) {
			dev->bos->desc->bNumDeviceCaps = i;
			break;
		}
		cap_type = cap->bDevCapabilityType;
		length = cap->bLength;
		if (bos_desc_len[cap_type] && length < bos_desc_len[cap_type]) {
			dev->bos->desc->bNumDeviceCaps = i;
			break;
		}

		if (cap->bDescriptorType != USB_DT_DEVICE_CAPABILITY) {
			dev_notice(ddev, "descriptor type invalid, skip\n");
			goto skip_to_next_descriptor;
		}

		switch (cap_type) {
		case USB_CAP_TYPE_EXT:
			dev->bos->ext_cap =
				(struct usb_ext_cap_descriptor *)buffer;
			break;
		case USB_SS_CAP_TYPE:
			dev->bos->ss_cap =
				(struct usb_ss_cap_descriptor *)buffer;
			break;
		case USB_SSP_CAP_TYPE:
			ssp_cap = (struct usb_ssp_cap_descriptor *)buffer;
			ssac = (le32_to_cpu(ssp_cap->bmAttributes) &
				USB_SSP_SUBLINK_SPEED_ATTRIBS);
			if (length >= USB_DT_USB_SSP_CAP_SIZE(ssac))
				dev->bos->ssp_cap = ssp_cap;
			break;
		case CONTAINER_ID_TYPE:
			dev->bos->ss_id =
				(struct usb_ss_container_id_descriptor *)buffer;
			break;
		case USB_PTM_CAP_TYPE:
			dev->bos->ptm_cap =
				(struct usb_ptm_cap_descriptor *)buffer;
			break;
		default:
			break;
		}