// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 8/17



	ret = usb_get_descriptor(udev, USB_DT_DEVICE, 0, desc, sizeof(*desc));
	if (ret == sizeof(*desc))
		return desc;

	if (ret >= 0)
		ret = -EMSGSIZE;
	kfree(desc);
	return ERR_PTR(ret);
}

/*
 * usb_set_isoch_delay - informs the device of the packet transmit delay
 * @dev: the device whose delay is to be informed
 * Context: task context, might sleep
 *
 * Since this is an optional request, we don't bother if it fails.
 */
int usb_set_isoch_delay(struct usb_device *dev)
{
	/* skip hub devices */
	if (dev->descriptor.bDeviceClass == USB_CLASS_HUB)
		return 0;

	/* skip non-SS/non-SSP devices */
	if (dev->speed < USB_SPEED_SUPER)
		return 0;

	return usb_control_msg_send(dev, 0,
			USB_REQ_SET_ISOCH_DELAY,
			USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
			dev->hub_delay, 0, NULL, 0,
			USB_CTRL_SET_TIMEOUT,
			GFP_NOIO);
}

/**
 * usb_get_status - issues a GET_STATUS call
 * @dev: the device whose status is being checked
 * @recip: USB_RECIP_*; for device, interface, or endpoint
 * @type: USB_STATUS_TYPE_*; for standard or PTM status types
 * @target: zero (for device), else interface or endpoint number
 * @data: pointer to two bytes of bitmap data
 *
 * Context: task context, might sleep.
 *
 * Returns device, interface, or endpoint status.  Normally only of
 * interest to see if the device is self powered, or has enabled the
 * remote wakeup facility; or whether a bulk or interrupt endpoint
 * is halted ("stalled").
 *
 * Bits in these status bitmaps are set using the SET_FEATURE request,
 * and cleared using the CLEAR_FEATURE request.  The usb_clear_halt()
 * function should be used to clear halt ("stall") status.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns 0 and the status value in *@data (in host byte order) on success,
 * or else the status code from the underlying usb_control_msg() call.
 */
int usb_get_status(struct usb_device *dev, int recip, int type, int target,
		void *data)
{
	int ret;
	void *status;
	int length;

	switch (type) {
	case USB_STATUS_TYPE_STANDARD:
		length = 2;
		break;
	case USB_STATUS_TYPE_PTM:
		if (recip != USB_RECIP_DEVICE)
			return -EINVAL;

		length = 4;
		break;
	default:
		return -EINVAL;
	}

	status =  kmalloc(length, GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_STATUS, USB_DIR_IN | recip, USB_STATUS_TYPE_STANDARD,
		target, status, length, USB_CTRL_GET_TIMEOUT);

	switch (ret) {
	case 4:
		if (type != USB_STATUS_TYPE_PTM) {
			ret = -EIO;
			break;
		}

		*(u32 *) data = le32_to_cpu(*(__le32 *) status);
		ret = 0;
		break;
	case 2:
		if (type != USB_STATUS_TYPE_STANDARD) {
			ret = -EIO;
			break;
		}

		*(u16 *) data = le16_to_cpu(*(__le16 *) status);
		ret = 0;
		break;
	default:
		ret = -EIO;
	}

	kfree(status);
	return ret;
}
EXPORT_SYMBOL_GPL(usb_get_status);

/**
 * usb_clear_halt - tells device to clear endpoint halt/stall condition
 * @dev: device whose endpoint is halted
 * @pipe: endpoint "pipe" being cleared
 *
 * Context: task context, might sleep.
 *
 * This is used to clear halt conditions for bulk and interrupt endpoints,
 * as reported by URB completion status.  Endpoints that are halted are
 * sometimes referred to as being "stalled".  Such endpoints are unable
 * to transmit or receive data until the halt status is cleared.  Any URBs
 * queued for such an endpoint should normally be unlinked by the driver
 * before clearing the halt condition, as described in sections 5.7.5
 * and 5.8.5 of the USB 2.0 spec.
 *
 * Note that control and isochronous endpoints don't halt, although control
 * endpoints report "protocol stall" (for unsupported requests) using the
 * same status code used to report a true stall.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 * If a thread in your driver uses this call, make sure your disconnect()
 * method can wait for it to complete.
 *
 * Return: Zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_clear_halt(struct usb_device *dev, int pipe)
{
	int result;
	int endp = usb_pipeendpoint(pipe);

	if (usb_pipein(pipe))
		endp |= USB_DIR_IN;

	/* we don't care if it wasn't halted first. in fact some devices
	 * (like some ibmcam model 1 units) seem to expect hosts to make
	 * this request for iso endpoints, which can't halt!
	 */
	result = usb_control_msg_send(dev, 0,
				      USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
				      USB_ENDPOINT_HALT, endp, NULL, 0,
				      USB_CTRL_SET_TIMEOUT, GFP_NOIO);

	/* don't un-halt or force to DATA0 except on success */
	if (result)
		return result;

	/* NOTE:  seems like Microsoft and Apple don't bother verifying
	 * the clear "took", so some devices could lock up if you check...
	 * such as the Hagiwara FlashGate DUAL.  So we won't bother.
	 *
	 * NOTE:  make sure the logic here doesn't diverge much from
	 * the copy in usb-storage, for as long as we need two copies.
	 */