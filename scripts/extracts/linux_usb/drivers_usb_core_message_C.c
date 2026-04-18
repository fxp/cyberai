// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 3/17



/**
 * usb_interrupt_msg - Builds an interrupt urb, sends it off and waits for completion
 * @usb_dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @data: pointer to the data to send
 * @len: length in bytes of the data to send
 * @actual_length: pointer to a location to put the actual length transferred
 *	in bytes
 * @timeout: time in msecs to wait for the message to complete before timing out
 *
 * Context: task context, might sleep.
 *
 * This function sends a simple interrupt message to a specified endpoint and
 * waits for the message to complete, or timeout.
 *
 * Don't use this function from within an interrupt context. If you need
 * an asynchronous message, or need to send a message from within interrupt
 * context, use usb_submit_urb() If a thread in your driver uses this call,
 * make sure your disconnect() method can wait for it to complete. Since you
 * don't have a handle on the URB used, you can't cancel the request.
 *
 * Return:
 * If successful, 0. Otherwise a negative error number. The number of actual
 * bytes transferred will be stored in the @actual_length parameter.
 */
int usb_interrupt_msg(struct usb_device *usb_dev, unsigned int pipe,
		      void *data, int len, int *actual_length, int timeout)
{
	return usb_bulk_msg(usb_dev, pipe, data, len, actual_length, timeout);
}
EXPORT_SYMBOL_GPL(usb_interrupt_msg);

/**
 * usb_bulk_msg - Builds a bulk urb, sends it off and waits for completion
 * @usb_dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @data: pointer to the data to send
 * @len: length in bytes of the data to send
 * @actual_length: pointer to a location to put the actual length transferred
 *	in bytes
 * @timeout: time in msecs to wait for the message to complete before timing out
 *
 * Context: task context, might sleep.
 *
 * This function sends a simple bulk message to a specified endpoint
 * and waits for the message to complete, or timeout.
 *
 * Don't use this function from within an interrupt context. If you need
 * an asynchronous message, or need to send a message from within interrupt
 * context, use usb_submit_urb() If a thread in your driver uses this call,
 * make sure your disconnect() method can wait for it to complete. Since you
 * don't have a handle on the URB used, you can't cancel the request.
 *
 * Because there is no usb_interrupt_msg() and no USBDEVFS_INTERRUPT ioctl,
 * users are forced to abuse this routine by using it to submit URBs for
 * interrupt endpoints.  We will take the liberty of creating an interrupt URB
 * (with the default interval) if the target is an interrupt endpoint.
 *
 * Return:
 * If successful, 0. Otherwise a negative error number. The number of actual
 * bytes transferred will be stored in the @actual_length parameter.
 *
 */
int usb_bulk_msg(struct usb_device *usb_dev, unsigned int pipe,
		 void *data, int len, int *actual_length, int timeout)
{
	struct urb *urb;
	struct usb_host_endpoint *ep;

	ep = usb_pipe_endpoint(usb_dev, pipe);
	if (!ep || len < 0)
		return -EINVAL;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return -ENOMEM;

	if ((ep->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_INT) {
		pipe = (pipe & ~(3 << 30)) | (PIPE_INTERRUPT << 30);
		usb_fill_int_urb(urb, usb_dev, pipe, data, len,
				usb_api_blocking_completion, NULL,
				ep->desc.bInterval);
	} else
		usb_fill_bulk_urb(urb, usb_dev, pipe, data, len,
				usb_api_blocking_completion, NULL);

	return usb_start_wait_urb(urb, timeout, actual_length, false);
}
EXPORT_SYMBOL_GPL(usb_bulk_msg);

/**
 * usb_bulk_msg_killable - Builds a bulk urb, sends it off and waits for completion in a killable state
 * @usb_dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @data: pointer to the data to send
 * @len: length in bytes of the data to send
 * @actual_length: pointer to a location to put the actual length transferred
 *	in bytes
 * @timeout: time in msecs to wait for the message to complete before
 *	timing out (if <= 0, the wait is as long as possible)
 *
 * Context: task context, might sleep.
 *
 * This function is just like usb_blk_msg(), except that it waits in a
 * killable state and there is no limit on the timeout length.
 *
 * Return:
 * If successful, 0. Otherwise a negative error number. The number of actual
 * bytes transferred will be stored in the @actual_length parameter.
 *
 */
int usb_bulk_msg_killable(struct usb_device *usb_dev, unsigned int pipe,
		 void *data, int len, int *actual_length, int timeout)
{
	struct urb *urb;
	struct usb_host_endpoint *ep;

	ep = usb_pipe_endpoint(usb_dev, pipe);
	if (!ep || len < 0)
		return -EINVAL;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return -ENOMEM;