// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 7/17



	/* Try to read the string descriptor by asking for the maximum
	 * possible number of bytes */
	if (dev->quirks & USB_QUIRK_STRING_FETCH_255)
		rc = -EIO;
	else
		rc = usb_get_string(dev, langid, index, buf, 255);

	/* If that failed try to read the descriptor length, then
	 * ask for just that many bytes */
	if (rc < 2) {
		rc = usb_get_string(dev, langid, index, buf, 2);
		if (rc == 2)
			rc = usb_get_string(dev, langid, index, buf, buf[0]);
	}

	if (rc >= 2) {
		if (!buf[0] && !buf[1])
			usb_try_string_workarounds(buf, &rc);

		/* There might be extra junk at the end of the descriptor */
		if (buf[0] < rc)
			rc = buf[0];

		rc = rc - (rc & 1); /* force a multiple of two */
	}

	if (rc < 2)
		rc = (rc < 0 ? rc : -EINVAL);

	return rc;
}

static int usb_get_langid(struct usb_device *dev, unsigned char *tbuf)
{
	int err;

	if (dev->have_langid)
		return 0;

	if (dev->string_langid < 0)
		return -EPIPE;

	err = usb_string_sub(dev, 0, 0, tbuf);

	/* If the string was reported but is malformed, default to english
	 * (0x0409) */
	if (err == -ENODATA || (err > 0 && err < 4)) {
		dev->string_langid = 0x0409;
		dev->have_langid = 1;
		dev_err(&dev->dev,
			"language id specifier not provided by device, defaulting to English\n");
		return 0;
	}

	/* In case of all other errors, we assume the device is not able to
	 * deal with strings at all. Set string_langid to -1 in order to
	 * prevent any string to be retrieved from the device */
	if (err < 0) {
		dev_info(&dev->dev, "string descriptor 0 read error: %d\n",
					err);
		dev->string_langid = -1;
		return -EPIPE;
	}

	/* always use the first langid listed */
	dev->string_langid = tbuf[2] | (tbuf[3] << 8);
	dev->have_langid = 1;
	dev_dbg(&dev->dev, "default language 0x%04x\n",
				dev->string_langid);
	return 0;
}

/**
 * usb_string - returns UTF-8 version of a string descriptor
 * @dev: the device whose string descriptor is being retrieved
 * @index: the number of the descriptor
 * @buf: where to put the string
 * @size: how big is "buf"?
 *
 * Context: task context, might sleep.
 *
 * This converts the UTF-16LE encoded strings returned by devices, from
 * usb_get_string_descriptor(), to null-terminated UTF-8 encoded ones
 * that are more usable in most kernel contexts.  Note that this function
 * chooses strings in the first language supported by the device.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Return: length of the string (>= 0) or usb_control_msg status (< 0).
 */
int usb_string(struct usb_device *dev, int index, char *buf, size_t size)
{
	unsigned char *tbuf;
	int err;

	if (dev->state == USB_STATE_SUSPENDED)
		return -EHOSTUNREACH;
	if (size <= 0 || !buf)
		return -EINVAL;
	buf[0] = 0;
	if (index <= 0 || index >= 256)
		return -EINVAL;
	tbuf = kmalloc(256, GFP_NOIO);
	if (!tbuf)
		return -ENOMEM;

	err = usb_get_langid(dev, tbuf);
	if (err < 0)
		goto errout;

	err = usb_string_sub(dev, dev->string_langid, index, tbuf);
	if (err < 0)
		goto errout;

	size--;		/* leave room for trailing NULL char in output buffer */
	err = utf16s_to_utf8s((wchar_t *) &tbuf[2], (err - 2) / 2,
			UTF16_LITTLE_ENDIAN, buf, size);
	buf[err] = 0;

	if (tbuf[1] != USB_DT_STRING)
		dev_dbg(&dev->dev,
			"wrong descriptor type %02x for string %d (\"%s\")\n",
			tbuf[1], index, buf);

 errout:
	kfree(tbuf);
	return err;
}
EXPORT_SYMBOL_GPL(usb_string);

/* one UTF-8-encoded 16-bit character has at most three bytes */
#define MAX_USB_STRING_SIZE (127 * 3 + 1)

/**
 * usb_cache_string - read a string descriptor and cache it for later use
 * @udev: the device whose string descriptor is being read
 * @index: the descriptor index
 *
 * Return: A pointer to a kmalloc'ed buffer containing the descriptor string,
 * or %NULL if the index is 0 or the string could not be read.
 */
char *usb_cache_string(struct usb_device *udev, int index)
{
	char *buf;
	char *smallbuf = NULL;
	int len;

	if (index <= 0)
		return NULL;

	buf = kmalloc(MAX_USB_STRING_SIZE, GFP_NOIO);
	if (buf) {
		len = usb_string(udev, index, buf, MAX_USB_STRING_SIZE);
		if (len > 0) {
			smallbuf = kmalloc(++len, GFP_NOIO);
			if (!smallbuf)
				return buf;
			memcpy(smallbuf, buf, len);
		}
		kfree(buf);
	}
	return smallbuf;
}
EXPORT_SYMBOL_GPL(usb_cache_string);

/*
 * usb_get_device_descriptor - read the device descriptor
 * @udev: the device whose device descriptor should be read
 *
 * Context: task context, might sleep.
 *
 * Not exported, only for use by the core.  If drivers really want to read
 * the device descriptor directly, they can call usb_get_descriptor() with
 * type = USB_DT_DEVICE and index = 0.
 *
 * Returns: a pointer to a dynamically allocated usb_device_descriptor
 * structure (which the caller must deallocate), or an ERR_PTR value.
 */
struct usb_device_descriptor *usb_get_device_descriptor(struct usb_device *udev)
{
	struct usb_device_descriptor *desc;
	int ret;

	desc = kmalloc_obj(*desc, GFP_NOIO);
	if (!desc)
		return ERR_PTR(-ENOMEM);