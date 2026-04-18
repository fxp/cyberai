// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 5/17



		urb->dev = NULL;
		urb->pipe = pipe;
		urb->interval = period;
		urb->transfer_flags = urb_flags;
		urb->complete = sg_complete;
		urb->context = io;
		urb->sg = sg;

		if (use_sg) {
			/* There is no single transfer buffer */
			urb->transfer_buffer = NULL;
			urb->num_sgs = nents;

			/* A length of zero means transfer the whole sg list */
			len = length;
			if (len == 0) {
				struct scatterlist	*sg2;
				int			j;

				for_each_sg(sg, sg2, nents, j)
					len += sg2->length;
			}
		} else {
			/*
			 * Some systems can't use DMA; they use PIO instead.
			 * For their sakes, transfer_buffer is set whenever
			 * possible.
			 */
			if (!PageHighMem(sg_page(sg)))
				urb->transfer_buffer = sg_virt(sg);
			else
				urb->transfer_buffer = NULL;

			len = sg->length;
			if (length) {
				len = min_t(size_t, len, length);
				length -= len;
				if (length == 0)
					io->entries = i + 1;
			}
		}
		urb->transfer_buffer_length = len;
	}
	io->urbs[--i]->transfer_flags &= ~URB_NO_INTERRUPT;

	/* transaction state */
	io->count = io->entries;
	io->status = 0;
	io->bytes = 0;
	init_completion(&io->complete);
	return 0;

nomem:
	sg_clean(io);
	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(usb_sg_init);

/**
 * usb_sg_wait - synchronously execute scatter/gather request
 * @io: request block handle, as initialized with usb_sg_init().
 * 	some fields become accessible when this call returns.
 *
 * Context: task context, might sleep.
 *
 * This function blocks until the specified I/O operation completes.  It
 * leverages the grouping of the related I/O requests to get good transfer
 * rates, by queueing the requests.  At higher speeds, such queuing can
 * significantly improve USB throughput.
 *
 * There are three kinds of completion for this function.
 *
 * (1) success, where io->status is zero.  The number of io->bytes
 *     transferred is as requested.
 * (2) error, where io->status is a negative errno value.  The number
 *     of io->bytes transferred before the error is usually less
 *     than requested, and can be nonzero.
 * (3) cancellation, a type of error with status -ECONNRESET that
 *     is initiated by usb_sg_cancel().
 *
 * When this function returns, all memory allocated through usb_sg_init() or
 * this call will have been freed.  The request block parameter may still be
 * passed to usb_sg_cancel(), or it may be freed.  It could also be
 * reinitialized and then reused.
 *
 * Data Transfer Rates:
 *
 * Bulk transfers are valid for full or high speed endpoints.
 * The best full speed data rate is 19 packets of 64 bytes each
 * per frame, or 1216 bytes per millisecond.
 * The best high speed data rate is 13 packets of 512 bytes each
 * per microframe, or 52 KBytes per millisecond.
 *
 * The reason to use interrupt transfers through this API would most likely
 * be to reserve high speed bandwidth, where up to 24 KBytes per millisecond
 * could be transferred.  That capability is less useful for low or full
 * speed interrupt endpoints, which allow at most one packet per millisecond,
 * of at most 8 or 64 bytes (respectively).
 *
 * It is not necessary to call this function to reserve bandwidth for devices
 * under an xHCI host controller, as the bandwidth is reserved when the
 * configuration or interface alt setting is selected.
 */
void usb_sg_wait(struct usb_sg_request *io)
{
	int i;
	int entries = io->entries;

	/* queue the urbs.  */
	spin_lock_irq(&io->lock);
	i = 0;
	while (i < entries && !io->status) {
		int retval;

		io->urbs[i]->dev = io->dev;
		spin_unlock_irq(&io->lock);

		retval = usb_submit_urb(io->urbs[i], GFP_NOIO);

		switch (retval) {
			/* maybe we retrying will recover */
		case -ENXIO:	/* hc didn't queue this one */
		case -EAGAIN:
		case -ENOMEM:
			retval = 0;
			yield();
			break;

			/* no error? continue immediately.
			 *
			 * NOTE: to work better with UHCI (4K I/O buffer may
			 * need 3K of TDs) it may be good to limit how many
			 * URBs are queued at once; N milliseconds?
			 */
		case 0:
			++i;
			cpu_relax();
			break;

			/* fail any uncompleted urbs */
		default:
			io->urbs[i]->status = retval;
			dev_dbg(&io->dev->dev, "%s, submit --> %d\n",
				__func__, retval);
			usb_sg_cancel(io);
		}
		spin_lock_irq(&io->lock);
		if (retval && (io->status == 0 || io->status == -ECONNRESET))
			io->status = retval;
	}
	io->count -= entries - i;
	if (io->count == 0)
		complete(&io->complete);
	spin_unlock_irq(&io->lock);

	/* OK, yes, this could be packaged as non-blocking.
	 * So could the submit loop above ... but it's easier to
	 * solve neither problem than to solve both!
	 */
	wait_for_completion(&io->complete);

	sg_clean(io);
}
EXPORT_SYMBOL_GPL(usb_sg_wait);