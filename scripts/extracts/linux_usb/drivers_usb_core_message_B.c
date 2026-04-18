// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 2/17



/**
 * usb_control_msg_send - Builds a control "send" message, sends it off and waits for completion
 * @dev: pointer to the usb device to send the message to
 * @endpoint: endpoint to send the message to
 * @request: USB message request value
 * @requesttype: USB message request type value
 * @value: USB message value
 * @index: USB message index value
 * @driver_data: pointer to the data to send
 * @size: length in bytes of the data to send
 * @timeout: time in msecs to wait for the message to complete before timing out
 * @memflags: the flags for memory allocation for buffers
 *
 * Context: !in_interrupt ()
 *
 * This function sends a control message to a specified endpoint that is not
 * expected to fill in a response (i.e. a "send message") and waits for the
 * message to complete, or timeout.
 *
 * Do not use this function from within an interrupt context. If you need
 * an asynchronous message, or need to send a message from within interrupt
 * context, use usb_submit_urb(). If a thread in your driver uses this call,
 * make sure your disconnect() method can wait for it to complete. Since you
 * don't have a handle on the URB used, you can't cancel the request.
 *
 * The data pointer can be made to a reference on the stack, or anywhere else,
 * as it will not be modified at all.  This does not have the restriction that
 * usb_control_msg() has where the data pointer must be to dynamically allocated
 * memory (i.e. memory that can be successfully DMAed to a device).
 *
 * Return: If successful, 0 is returned, Otherwise, a negative error number.
 */
int usb_control_msg_send(struct usb_device *dev, __u8 endpoint, __u8 request,
			 __u8 requesttype, __u16 value, __u16 index,
			 const void *driver_data, __u16 size, int timeout,
			 gfp_t memflags)
{
	unsigned int pipe = usb_sndctrlpipe(dev, endpoint);
	int ret;
	u8 *data = NULL;

	if (size) {
		data = kmemdup(driver_data, size, memflags);
		if (!data)
			return -ENOMEM;
	}

	ret = usb_control_msg(dev, pipe, request, requesttype, value, index,
			      data, size, timeout);
	kfree(data);

	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(usb_control_msg_send);

/**
 * usb_control_msg_recv - Builds a control "receive" message, sends it off and waits for completion
 * @dev: pointer to the usb device to send the message to
 * @endpoint: endpoint to send the message to
 * @request: USB message request value
 * @requesttype: USB message request type value
 * @value: USB message value
 * @index: USB message index value
 * @driver_data: pointer to the data to be filled in by the message
 * @size: length in bytes of the data to be received
 * @timeout: time in msecs to wait for the message to complete before timing out
 * @memflags: the flags for memory allocation for buffers
 *
 * Context: !in_interrupt ()
 *
 * This function sends a control message to a specified endpoint that is
 * expected to fill in a response (i.e. a "receive message") and waits for the
 * message to complete, or timeout.
 *
 * Do not use this function from within an interrupt context. If you need
 * an asynchronous message, or need to send a message from within interrupt
 * context, use usb_submit_urb(). If a thread in your driver uses this call,
 * make sure your disconnect() method can wait for it to complete. Since you
 * don't have a handle on the URB used, you can't cancel the request.
 *
 * The data pointer can be made to a reference on the stack, or anywhere else
 * that can be successfully written to.  This function does not have the
 * restriction that usb_control_msg() has where the data pointer must be to
 * dynamically allocated memory (i.e. memory that can be successfully DMAed to a
 * device).
 *
 * The "whole" message must be properly received from the device in order for
 * this function to be successful.  If a device returns less than the expected
 * amount of data, then the function will fail.  Do not use this for messages
 * where a variable amount of data might be returned.
 *
 * Return: If successful, 0 is returned, Otherwise, a negative error number.
 */
int usb_control_msg_recv(struct usb_device *dev, __u8 endpoint, __u8 request,
			 __u8 requesttype, __u16 value, __u16 index,
			 void *driver_data, __u16 size, int timeout,
			 gfp_t memflags)
{
	unsigned int pipe = usb_rcvctrlpipe(dev, endpoint);
	int ret;
	u8 *data;

	if (!size || !driver_data)
		return -EINVAL;

	data = kmalloc(size, memflags);
	if (!data)
		return -ENOMEM;

	ret = usb_control_msg(dev, pipe, request, requesttype, value, index,
			      data, size, timeout);

	if (ret < 0)
		goto exit;

	if (ret == size) {
		memcpy(driver_data, data, size);
		ret = 0;
	} else {
		ret = -EREMOTEIO;
	}

exit:
	kfree(data);
	return ret;
}
EXPORT_SYMBOL_GPL(usb_control_msg_recv);