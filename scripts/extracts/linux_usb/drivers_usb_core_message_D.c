// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 4/17



	if ((ep->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_INT) {
		pipe = (pipe & ~(3 << 30)) | (PIPE_INTERRUPT << 30);
		usb_fill_int_urb(urb, usb_dev, pipe, data, len,
				usb_api_blocking_completion, NULL,
				ep->desc.bInterval);
	} else
		usb_fill_bulk_urb(urb, usb_dev, pipe, data, len,
				usb_api_blocking_completion, NULL);

	return usb_start_wait_urb(urb, timeout, actual_length, true);
}
EXPORT_SYMBOL_GPL(usb_bulk_msg_killable);

/*-------------------------------------------------------------------*/

static void sg_clean(struct usb_sg_request *io)
{
	if (io->urbs) {
		while (io->entries--)
			usb_free_urb(io->urbs[io->entries]);
		kfree(io->urbs);
		io->urbs = NULL;
	}
	io->dev = NULL;
}

static void sg_complete(struct urb *urb)
{
	unsigned long flags;
	struct usb_sg_request *io = urb->context;
	int status = urb->status;

	spin_lock_irqsave(&io->lock, flags);

	/* In 2.5 we require hcds' endpoint queues not to progress after fault
	 * reports, until the completion callback (this!) returns.  That lets
	 * device driver code (like this routine) unlink queued urbs first,
	 * if it needs to, since the HC won't work on them at all.  So it's
	 * not possible for page N+1 to overwrite page N, and so on.
	 *
	 * That's only for "hard" faults; "soft" faults (unlinks) sometimes
	 * complete before the HCD can get requests away from hardware,
	 * though never during cleanup after a hard fault.
	 */
	if (io->status
			&& (io->status != -ECONNRESET
				|| status != -ECONNRESET)
			&& urb->actual_length) {
		dev_err(io->dev->bus->controller,
			"dev %s ep%d%s scatterlist error %d/%d\n",
			io->dev->devpath,
			usb_endpoint_num(&urb->ep->desc),
			usb_urb_dir_in(urb) ? "in" : "out",
			status, io->status);
		/* BUG (); */
	}

	if (io->status == 0 && status && status != -ECONNRESET) {
		int i, found, retval;

		io->status = status;

		/* the previous urbs, and this one, completed already.
		 * unlink pending urbs so they won't rx/tx bad data.
		 * careful: unlink can sometimes be synchronous...
		 */
		spin_unlock_irqrestore(&io->lock, flags);
		for (i = 0, found = 0; i < io->entries; i++) {
			if (!io->urbs[i])
				continue;
			if (found) {
				usb_block_urb(io->urbs[i]);
				retval = usb_unlink_urb(io->urbs[i]);
				if (retval != -EINPROGRESS &&
				    retval != -ENODEV &&
				    retval != -EBUSY &&
				    retval != -EIDRM)
					dev_err(&io->dev->dev,
						"%s, unlink --> %d\n",
						__func__, retval);
			} else if (urb == io->urbs[i])
				found = 1;
		}
		spin_lock_irqsave(&io->lock, flags);
	}

	/* on the last completion, signal usb_sg_wait() */
	io->bytes += urb->actual_length;
	io->count--;
	if (!io->count)
		complete(&io->complete);

	spin_unlock_irqrestore(&io->lock, flags);
}


/**
 * usb_sg_init - initializes scatterlist-based bulk/interrupt I/O request
 * @io: request block being initialized.  until usb_sg_wait() returns,
 *	treat this as a pointer to an opaque block of memory,
 * @dev: the usb device that will send or receive the data
 * @pipe: endpoint "pipe" used to transfer the data
 * @period: polling rate for interrupt endpoints, in frames or
 * 	(for high speed endpoints) microframes; ignored for bulk
 * @sg: scatterlist entries
 * @nents: how many entries in the scatterlist
 * @length: how many bytes to send from the scatterlist, or zero to
 * 	send every byte identified in the list.
 * @mem_flags: SLAB_* flags affecting memory allocations in this call
 *
 * This initializes a scatter/gather request, allocating resources such as
 * I/O mappings and urb memory (except maybe memory used by USB controller
 * drivers).
 *
 * The request must be issued using usb_sg_wait(), which waits for the I/O to
 * complete (or to be canceled) and then cleans up all resources allocated by
 * usb_sg_init().
 *
 * The request may be canceled with usb_sg_cancel(), either before or after
 * usb_sg_wait() is called.
 *
 * Return: Zero for success, else a negative errno value.
 */
int usb_sg_init(struct usb_sg_request *io, struct usb_device *dev,
		unsigned pipe, unsigned	period, struct scatterlist *sg,
		int nents, size_t length, gfp_t mem_flags)
{
	int i;
	int urb_flags;
	int use_sg;

	if (!io || !dev || !sg
			|| usb_pipecontrol(pipe)
			|| usb_pipeisoc(pipe)
			|| nents <= 0)
		return -EINVAL;

	spin_lock_init(&io->lock);
	io->dev = dev;
	io->pipe = pipe;

	if (dev->bus->sg_tablesize > 0) {
		use_sg = true;
		io->entries = 1;
	} else {
		use_sg = false;
		io->entries = nents;
	}

	/* initialize all the urbs we'll use */
	io->urbs = kmalloc_objs(*io->urbs, io->entries, mem_flags);
	if (!io->urbs)
		goto nomem;

	urb_flags = URB_NO_INTERRUPT;
	if (usb_pipein(pipe))
		urb_flags |= URB_SHORT_NOT_OK;

	for_each_sg(sg, sg, io->entries, i) {
		struct urb *urb;
		unsigned len;

		urb = usb_alloc_urb(0, mem_flags);
		if (!urb) {
			io->entries = i;
			goto nomem;
		}
		io->urbs[i] = urb;