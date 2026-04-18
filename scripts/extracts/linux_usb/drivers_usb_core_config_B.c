// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/config.c
// Segment 2/8



	if ((usb_endpoint_xfer_control(&ep->desc) ||
			usb_endpoint_xfer_int(&ep->desc)) &&
				desc->bmAttributes != 0) {
		dev_notice(ddev, "%s endpoint with bmAttributes = %d in "
				"config %d interface %d altsetting %d ep %d: "
				"setting to zero\n",
				usb_endpoint_xfer_control(&ep->desc) ? "Control" : "Bulk",
				desc->bmAttributes,
				cfgno, inum, asnum, ep->desc.bEndpointAddress);
		ep->ss_ep_comp.bmAttributes = 0;
	} else if (usb_endpoint_xfer_bulk(&ep->desc) &&
			desc->bmAttributes > 16) {
		dev_notice(ddev, "Bulk endpoint with more than 65536 streams in "
				"config %d interface %d altsetting %d ep %d: "
				"setting to max\n",
				cfgno, inum, asnum, ep->desc.bEndpointAddress);
		ep->ss_ep_comp.bmAttributes = 16;
	} else if (usb_endpoint_xfer_isoc(&ep->desc) &&
		   !USB_SS_SSP_ISOC_COMP(desc->bmAttributes) &&
		   USB_SS_MULT(desc->bmAttributes) > 3) {
		dev_notice(ddev, "Isoc endpoint has Mult of %d in "
				"config %d interface %d altsetting %d ep %d: "
				"setting to 3\n",
				USB_SS_MULT(desc->bmAttributes),
				cfgno, inum, asnum, ep->desc.bEndpointAddress);
		ep->ss_ep_comp.bmAttributes = 2;
	}

	if (usb_endpoint_xfer_isoc(&ep->desc))
		max_tx = (desc->bMaxBurst + 1) *
			(USB_SS_MULT(desc->bmAttributes)) *
			usb_endpoint_maxp(&ep->desc);
	else if (usb_endpoint_xfer_int(&ep->desc))
		max_tx = usb_endpoint_maxp(&ep->desc) *
			(desc->bMaxBurst + 1);
	else
		max_tx = 999999;
	if (le16_to_cpu(desc->wBytesPerInterval) > max_tx) {
		dev_notice(ddev, "%s endpoint with wBytesPerInterval of %d in "
				"config %d interface %d altsetting %d ep %d: "
				"setting to %d\n",
				usb_endpoint_xfer_isoc(&ep->desc) ? "Isoc" : "Int",
				le16_to_cpu(desc->wBytesPerInterval),
				cfgno, inum, asnum, ep->desc.bEndpointAddress,
				max_tx);
		ep->ss_ep_comp.wBytesPerInterval = cpu_to_le16(max_tx);
	}
	/* Parse a possible SuperSpeedPlus isoc ep companion descriptor */
	if (usb_endpoint_xfer_isoc(&ep->desc) &&
	    USB_SS_SSP_ISOC_COMP(desc->bmAttributes))
		usb_parse_ssp_isoc_endpoint_companion(ddev, cfgno, inum, asnum,
							ep, buffer, size);
}

static const unsigned short low_speed_maxpacket_maxes[4] = {
	[USB_ENDPOINT_XFER_CONTROL] = 8,
	[USB_ENDPOINT_XFER_ISOC] = 0,
	[USB_ENDPOINT_XFER_BULK] = 0,
	[USB_ENDPOINT_XFER_INT] = 8,
};
static const unsigned short full_speed_maxpacket_maxes[4] = {
	[USB_ENDPOINT_XFER_CONTROL] = 64,
	[USB_ENDPOINT_XFER_ISOC] = 1023,
	[USB_ENDPOINT_XFER_BULK] = 64,
	[USB_ENDPOINT_XFER_INT] = 64,
};
static const unsigned short high_speed_maxpacket_maxes[4] = {
	[USB_ENDPOINT_XFER_CONTROL] = 64,
	[USB_ENDPOINT_XFER_ISOC] = 1024,

	/* Bulk should be 512, but some devices use 1024: we will warn below */
	[USB_ENDPOINT_XFER_BULK] = 1024,
	[USB_ENDPOINT_XFER_INT] = 1024,
};
static const unsigned short super_speed_maxpacket_maxes[4] = {
	[USB_ENDPOINT_XFER_CONTROL] = 512,
	[USB_ENDPOINT_XFER_ISOC] = 1024,
	[USB_ENDPOINT_XFER_BULK] = 1024,
	[USB_ENDPOINT_XFER_INT] = 1024,
};

static bool endpoint_is_duplicate(struct usb_endpoint_descriptor *e1,
		struct usb_endpoint_descriptor *e2)
{
	if (e1->bEndpointAddress == e2->bEndpointAddress)
		return true;

	if (usb_endpoint_xfer_control(e1) || usb_endpoint_xfer_control(e2)) {
		if (usb_endpoint_num(e1) == usb_endpoint_num(e2))
			return true;
	}

	return false;
}

/*
 * Check for duplicate endpoint addresses in other interfaces and in the
 * altsetting currently being parsed.
 */
static bool config_endpoint_is_duplicate(struct usb_host_config *config,
		int inum, int asnum, struct usb_endpoint_descriptor *d)
{
	struct usb_endpoint_descriptor *epd;
	struct usb_interface_cache *intfc;
	struct usb_host_interface *alt;
	int i, j, k;

	for (i = 0; i < config->desc.bNumInterfaces; ++i) {
		intfc = config->intf_cache[i];

		for (j = 0; j < intfc->num_altsetting; ++j) {
			alt = &intfc->altsetting[j];

			if (alt->desc.bInterfaceNumber == inum &&
					alt->desc.bAlternateSetting != asnum)
				continue;

			for (k = 0; k < alt->desc.bNumEndpoints; ++k) {
				epd = &alt->endpoint[k].desc;

				if (endpoint_is_duplicate(epd, d))
					return true;
			}
		}
	}

	return false;
}

static int usb_parse_endpoint(struct device *ddev, int cfgno,
		struct usb_host_config *config, int inum, int asnum,
		struct usb_host_interface *ifp, int num_ep,
		unsigned char *buffer, int size)
{
	struct usb_device *udev = to_usb_device(ddev);
	unsigned char *buffer0 = buffer;
	struct usb_endpoint_descriptor *d;
	struct usb_host_endpoint *endpoint;
	int n, i, j, retval;
	unsigned int maxp;
	const unsigned short *maxpacket_maxes;
	u16 bcdUSB;

	d = (struct usb_endpoint_descriptor *) buffer;
	bcdUSB = le16_to_cpu(udev->descriptor.bcdUSB);
	buffer += d->bLength;
	size -= d->bLength;