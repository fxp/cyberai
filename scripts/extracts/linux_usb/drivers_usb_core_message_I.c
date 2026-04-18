// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 9/17



	usb_reset_endpoint(dev, endp);

	return 0;
}
EXPORT_SYMBOL_GPL(usb_clear_halt);

static int create_intf_ep_devs(struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *alt = intf->cur_altsetting;
	int i;

	if (intf->ep_devs_created || intf->unregistering)
		return 0;

	for (i = 0; i < alt->desc.bNumEndpoints; ++i)
		(void) usb_create_ep_devs(&intf->dev, &alt->endpoint[i], udev);
	intf->ep_devs_created = 1;
	return 0;
}

static void remove_intf_ep_devs(struct usb_interface *intf)
{
	struct usb_host_interface *alt = intf->cur_altsetting;
	int i;

	if (!intf->ep_devs_created)
		return;

	for (i = 0; i < alt->desc.bNumEndpoints; ++i)
		usb_remove_ep_devs(&alt->endpoint[i]);
	intf->ep_devs_created = 0;
}

/**
 * usb_disable_endpoint -- Disable an endpoint by address
 * @dev: the device whose endpoint is being disabled
 * @epaddr: the endpoint's address.  Endpoint number for output,
 *	endpoint number + USB_DIR_IN for input
 * @reset_hardware: flag to erase any endpoint state stored in the
 *	controller hardware
 *
 * Disables the endpoint for URB submission and nukes all pending URBs.
 * If @reset_hardware is set then also deallocates hcd/hardware state
 * for the endpoint.
 */
void usb_disable_endpoint(struct usb_device *dev, unsigned int epaddr,
		bool reset_hardware)
{
	unsigned int epnum = epaddr & USB_ENDPOINT_NUMBER_MASK;
	struct usb_host_endpoint *ep;

	if (!dev)
		return;

	if (usb_endpoint_out(epaddr)) {
		ep = dev->ep_out[epnum];
		if (reset_hardware && epnum != 0)
			dev->ep_out[epnum] = NULL;
	} else {
		ep = dev->ep_in[epnum];
		if (reset_hardware && epnum != 0)
			dev->ep_in[epnum] = NULL;
	}
	if (ep) {
		ep->enabled = 0;
		usb_hcd_flush_endpoint(dev, ep);
		if (reset_hardware)
			usb_hcd_disable_endpoint(dev, ep);
	}
}

/**
 * usb_reset_endpoint - Reset an endpoint's state.
 * @dev: the device whose endpoint is to be reset
 * @epaddr: the endpoint's address.  Endpoint number for output,
 *	endpoint number + USB_DIR_IN for input
 *
 * Resets any host-side endpoint state such as the toggle bit,
 * sequence number or current window.
 */
void usb_reset_endpoint(struct usb_device *dev, unsigned int epaddr)
{
	unsigned int epnum = epaddr & USB_ENDPOINT_NUMBER_MASK;
	struct usb_host_endpoint *ep;

	if (usb_endpoint_out(epaddr))
		ep = dev->ep_out[epnum];
	else
		ep = dev->ep_in[epnum];
	if (ep)
		usb_hcd_reset_endpoint(dev, ep);
}
EXPORT_SYMBOL_GPL(usb_reset_endpoint);


/**
 * usb_disable_interface -- Disable all endpoints for an interface
 * @dev: the device whose interface is being disabled
 * @intf: pointer to the interface descriptor
 * @reset_hardware: flag to erase any endpoint state stored in the
 *	controller hardware
 *
 * Disables all the endpoints for the interface's current altsetting.
 */
void usb_disable_interface(struct usb_device *dev, struct usb_interface *intf,
		bool reset_hardware)
{
	struct usb_host_interface *alt = intf->cur_altsetting;
	int i;

	for (i = 0; i < alt->desc.bNumEndpoints; ++i) {
		usb_disable_endpoint(dev,
				alt->endpoint[i].desc.bEndpointAddress,
				reset_hardware);
	}
}

/*
 * usb_disable_device_endpoints -- Disable all endpoints for a device
 * @dev: the device whose endpoints are being disabled
 * @skip_ep0: 0 to disable endpoint 0, 1 to skip it.
 */
static void usb_disable_device_endpoints(struct usb_device *dev, int skip_ep0)
{
	struct usb_hcd *hcd = bus_to_hcd(dev->bus);
	int i;

	if (hcd->driver->check_bandwidth) {
		/* First pass: Cancel URBs, leave endpoint pointers intact. */
		for (i = skip_ep0; i < 16; ++i) {
			usb_disable_endpoint(dev, i, false);
			usb_disable_endpoint(dev, i + USB_DIR_IN, false);
		}
		/* Remove endpoints from the host controller internal state */
		mutex_lock(hcd->bandwidth_mutex);
		usb_hcd_alloc_bandwidth(dev, NULL, NULL, NULL);
		mutex_unlock(hcd->bandwidth_mutex);
	}
	/* Second pass: remove endpoint pointers */
	for (i = skip_ep0; i < 16; ++i) {
		usb_disable_endpoint(dev, i, true);
		usb_disable_endpoint(dev, i + USB_DIR_IN, true);
	}
}

/**
 * usb_disable_device - Disable all the endpoints for a USB device
 * @dev: the device whose endpoints are being disabled
 * @skip_ep0: 0 to disable endpoint 0, 1 to skip it.
 *
 * Disables all the device's endpoints, potentially including endpoint 0.
 * Deallocates hcd/hardware state for the endpoints (nuking all or most
 * pending urbs) and usbcore state for the interfaces, so that usbcore
 * must usb_set_configuration() before any interfaces could be used.
 */
void usb_disable_device(struct usb_device *dev, int skip_ep0)
{
	int i;

	/* getting rid of interfaces will disconnect
	 * any drivers bound to them (a key side effect)
	 */
	if (dev->actconfig) {
		/*
		 * FIXME: In order to avoid self-deadlock involving the
		 * bandwidth_mutex, we have to mark all the interfaces
		 * before unregistering any of them.
		 */
		for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++)
			dev->actconfig->interface[i]->unregistering = 1;