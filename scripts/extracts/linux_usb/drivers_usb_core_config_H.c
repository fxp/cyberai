// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/config.c
// Segment 8/8



skip_to_next_descriptor:
		total_len -= length;
		buffer += length;
	}
	dev->bos->desc->wTotalLength = cpu_to_le16(buffer - buffer0);

	return 0;

err:
	usb_release_bos_descriptor(dev);
	return ret;
}
