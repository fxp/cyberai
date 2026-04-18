// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-core.c
// Segment 8/17



	while ((next = fetch_item(start, end, &item)) != NULL) {
		start = next;

		if (item.format != HID_ITEM_FORMAT_SHORT) {
			hid_err(device, "unexpected long global item\n");
			goto out;
		}

		if (dispatch_type[item.type](parser, &item)) {
			hid_err(device, "item %u %u %u %u parsing failed\n",
				item.format,
				(unsigned int)item.size,
				(unsigned int)item.type,
				(unsigned int)item.tag);
			goto out;
		}
	}

	if (start != end) {
		hid_err(device, "item fetching failed at offset %u/%u\n",
			device->rsize - (unsigned int)(end - start),
			device->rsize);
		goto out;
	}

	if (parser->collection_stack_ptr) {
		hid_err(device, "unbalanced collection at end of report description\n");
		goto out;
	}

	if (parser->local.delimiter_depth) {
		hid_err(device, "unbalanced delimiter at end of report description\n");
		goto out;
	}

	/*
	 * fetch initial values in case the device's
	 * default multiplier isn't the recommended 1
	 */
	hid_setup_resolution_multiplier(device);

	device->status |= HID_STAT_PARSED;
	ret = 0;

out:
	kfree(parser->collection_stack);
	return ret;
}

/**
 * hid_open_report - open a driver-specific device report
 *
 * @device: hid device
 *
 * Parse a report description into a hid_device structure. Reports are
 * enumerated, fields are attached to these reports.
 * 0 returned on success, otherwise nonzero error value.
 *
 * This function (or the equivalent hid_parse() macro) should only be
 * called from probe() in drivers, before starting the device.
 */
int hid_open_report(struct hid_device *device)
{
	unsigned int size;
	const u8 *start;
	int error;

	if (WARN_ON(device->status & HID_STAT_PARSED))
		return -EBUSY;

	start = device->bpf_rdesc;
	if (WARN_ON(!start))
		return -ENODEV;
	size = device->bpf_rsize;

	if (device->driver->report_fixup) {
		/*
		 * device->driver->report_fixup() needs to work
		 * on a copy of our report descriptor so it can
		 * change it.
		 */
		u8 *buf __free(kfree) = kmemdup(start, size, GFP_KERNEL);

		if (!buf)
			return -ENOMEM;

		start = device->driver->report_fixup(device, buf, &size);

		/*
		 * The second kmemdup is required in case report_fixup() returns
		 * a static read-only memory, but we have no idea if that memory
		 * needs to be cleaned up or not at the end.
		 */
		start = kmemdup(start, size, GFP_KERNEL);
		if (!start)
			return -ENOMEM;
	}

	device->rdesc = start;
	device->rsize = size;

	error = hid_parse_collections(device);
	if (error) {
		hid_close_report(device);
		return error;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(hid_open_report);

/*
 * Extract/implement a data field from/to a little endian report (bit array).
 *
 * Code sort-of follows HID spec:
 *     http://www.usb.org/developers/hidpage/HID1_11.pdf
 *
 * While the USB HID spec allows unlimited length bit fields in "report
 * descriptors", most devices never use more than 16 bits.
 * One model of UPS is claimed to report "LINEV" as a 32-bit field.
 * Search linux-kernel and linux-usb-devel archives for "hid-core extract".
 */

static u32 __extract(u8 *report, unsigned offset, int n)
{
	unsigned int idx = offset / 8;
	unsigned int bit_nr = 0;
	unsigned int bit_shift = offset % 8;
	int bits_to_copy = 8 - bit_shift;
	u32 value = 0;
	u32 mask = n < 32 ? (1U << n) - 1 : ~0U;

	while (n > 0) {
		value |= ((u32)report[idx] >> bit_shift) << bit_nr;
		n -= bits_to_copy;
		bit_nr += bits_to_copy;
		bits_to_copy = 8;
		bit_shift = 0;
		idx++;
	}

	return value & mask;
}

u32 hid_field_extract(const struct hid_device *hid, u8 *report,
			unsigned offset, unsigned n)
{
	if (n > 32) {
		hid_warn_once(hid, "%s() called with n (%d) > 32! (%s)\n",
			      __func__, n, current->comm);
		n = 32;
	}

	return __extract(report, offset, n);
}
EXPORT_SYMBOL_GPL(hid_field_extract);

/*
 * "implement" : set bits in a little endian bit stream.
 * Same concepts as "extract" (see comments above).
 * The data mangled in the bit stream remains in little endian
 * order the whole time. It make more sense to talk about
 * endianness of register values by considering a register
 * a "cached" copy of the little endian bit stream.
 */

static void __implement(u8 *report, unsigned offset, int n, u32 value)
{
	unsigned int idx = offset / 8;
	unsigned int bit_shift = offset % 8;
	int bits_to_set = 8 - bit_shift;

	while (n - bits_to_set >= 0) {
		report[idx] &= ~(0xff << bit_shift);
		report[idx] |= value << bit_shift;
		value >>= bits_to_set;
		n -= bits_to_set;
		bits_to_set = 8;
		bit_shift = 0;
		idx++;
	}

	/* last nibble */
	if (n) {
		u8 bit_mask = ((1U << n) - 1);
		report[idx] &= ~(bit_mask << bit_shift);
		report[idx] |= value << bit_shift;
	}
}

static void implement(const struct hid_device *hid, u8 *report,
		      unsigned offset, unsigned n, u32 value)
{
	if (unlikely(n > 32)) {
		hid_warn(hid, "%s() called with n (%d) > 32! (%s)\n",
			 __func__, n, current->comm);
		n = 32;
	} else if (n < 32) {
		u32 m = (1U << n) - 1;