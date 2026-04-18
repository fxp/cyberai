// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 1/17

// SPDX-License-Identifier: GPL-2.0
/*
 * message.c - synchronous message handling
 *
 * Released under the GPLv2 only.
 */

#include <linux/acpi.h>
#include <linux/pci.h>	/* for scatterlist macros */
#include <linux/usb.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/nls.h>
#include <linux/device.h>
#include <linux/scatterlist.h>
#include <linux/usb/cdc.h>
#include <linux/usb/quirks.h>
#include <linux/usb/hcd.h>	/* for usbcore internals */
#include <linux/usb/of.h>
#include <asm/byteorder.h>

#include "usb.h"

static void cancel_async_set_config(struct usb_device *udev);

struct api_context {
	struct completion	done;
	int			status;
};

static void usb_api_blocking_completion(struct urb *urb)
{
	struct api_context *ctx = urb->context;

	ctx->status = urb->status;
	complete(&ctx->done);
}


/*
 * Starts urb and waits for completion or timeout.
 * Whether or not the wait is killable depends on the flag passed in.
 * For example, compare usb_bulk_msg() and usb_bulk_msg_killable().
 *
 * For non-killable waits, we enforce a maximum limit on the timeout value.
 */
static int usb_start_wait_urb(struct urb *urb, int timeout, int *actual_length,
		bool killable)
{
	struct api_context ctx;
	unsigned long expire;
	int retval;
	long rc;

	init_completion(&ctx.done);
	urb->context = &ctx;
	urb->actual_length = 0;
	retval = usb_submit_urb(urb, GFP_NOIO);
	if (unlikely(retval))
		goto out;

	if (!killable && (timeout <= 0 || timeout > USB_MAX_SYNCHRONOUS_TIMEOUT))
		timeout = USB_MAX_SYNCHRONOUS_TIMEOUT;
	expire = (timeout > 0) ? msecs_to_jiffies(timeout) : MAX_SCHEDULE_TIMEOUT;
	if (killable)
		rc = wait_for_completion_killable_timeout(&ctx.done, expire);
	else
		rc = wait_for_completion_timeout(&ctx.done, expire);
	if (rc <= 0) {
		usb_kill_urb(urb);
		if (ctx.status != -ENOENT)
			retval = ctx.status;
		else if (rc == 0)
			retval = -ETIMEDOUT;
		else
			retval = rc;

		dev_dbg(&urb->dev->dev,
			"%s timed out or killed on ep%d%s len=%u/%u\n",
			current->comm,
			usb_endpoint_num(&urb->ep->desc),
			usb_urb_dir_in(urb) ? "in" : "out",
			urb->actual_length,
			urb->transfer_buffer_length);
	} else
		retval = ctx.status;
out:
	if (actual_length)
		*actual_length = urb->actual_length;

	usb_free_urb(urb);
	return retval;
}

/*-------------------------------------------------------------------*/
/* returns status (negative) or length (positive) */
static int usb_internal_control_msg(struct usb_device *usb_dev,
				    unsigned int pipe,
				    struct usb_ctrlrequest *cmd,
				    void *data, int len, int timeout)
{
	struct urb *urb;
	int retv;
	int length;

	urb = usb_alloc_urb(0, GFP_NOIO);
	if (!urb)
		return -ENOMEM;

	usb_fill_control_urb(urb, usb_dev, pipe, (unsigned char *)cmd, data,
			     len, usb_api_blocking_completion, NULL);

	retv = usb_start_wait_urb(urb, timeout, &length, false);
	if (retv < 0)
		return retv;
	else
		return length;
}

/**
 * usb_control_msg - Builds a control urb, sends it off and waits for completion
 * @dev: pointer to the usb device to send the message to
 * @pipe: endpoint "pipe" to send the message to
 * @request: USB message request value
 * @requesttype: USB message request type value
 * @value: USB message value
 * @index: USB message index value
 * @data: pointer to the data to send
 * @size: length in bytes of the data to send
 * @timeout: time in msecs to wait for the message to complete before timing out
 *
 * Context: task context, might sleep.
 *
 * This function sends a simple control message to a specified endpoint and
 * waits for the message to complete, or timeout.
 *
 * Don't use this function from within an interrupt context. If you need
 * an asynchronous message, or need to send a message from within interrupt
 * context, use usb_submit_urb(). If a thread in your driver uses this call,
 * make sure your disconnect() method can wait for it to complete. Since you
 * don't have a handle on the URB used, you can't cancel the request.
 *
 * Return: If successful, the number of bytes transferred. Otherwise, a negative
 * error number.
 */
int usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request,
		    __u8 requesttype, __u16 value, __u16 index, void *data,
		    __u16 size, int timeout)
{
	struct usb_ctrlrequest *dr;
	int ret;

	dr = kmalloc_obj(struct usb_ctrlrequest, GFP_NOIO);
	if (!dr)
		return -ENOMEM;

	dr->bRequestType = requesttype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16(value);
	dr->wIndex = cpu_to_le16(index);
	dr->wLength = cpu_to_le16(size);

	ret = usb_internal_control_msg(dev, pipe, dr, data, size, timeout);

	/* Linger a bit, prior to the next control message. */
	if (dev->quirks & USB_QUIRK_DELAY_CTRL_MSG)
		msleep(200);

	kfree(dr);

	return ret;
}
EXPORT_SYMBOL_GPL(usb_control_msg);