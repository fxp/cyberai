// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 18/41



	usb_dev->authorized = 1;
	/* Choose and set the configuration.  This registers the interfaces
	 * with the driver core and lets interface drivers bind to them.
	 */
	c = usb_choose_configuration(usb_dev);
	if (c >= 0) {
		result = usb_set_configuration(usb_dev, c);
		if (result) {
			dev_err(&usb_dev->dev,
				"can't set config #%d, error %d\n", c, result);
			/* This need not be fatal.  The user can try to
			 * set other configurations. */
		}
	}
	dev_info(&usb_dev->dev, "authorized to connect\n");

	usb_autosuspend_device(usb_dev);
error_autoresume:
out_authorized:
	usb_unlock_device(usb_dev);	/* complements locktree */
	return result;
}

/**
 * get_port_ssp_rate - Match the extended port status to SSP rate
 * @hdev: The hub device
 * @ext_portstatus: extended port status
 *
 * Match the extended port status speed id to the SuperSpeed Plus sublink speed
 * capability attributes. Base on the number of connected lanes and speed,
 * return the corresponding enum usb_ssp_rate.
 */
static enum usb_ssp_rate get_port_ssp_rate(struct usb_device *hdev,
					   u32 ext_portstatus)
{
	struct usb_ssp_cap_descriptor *ssp_cap;
	u32 attr;
	u8 speed_id;
	u8 ssac;
	u8 lanes;
	int i;

	if (!hdev->bos)
		goto out;

	ssp_cap = hdev->bos->ssp_cap;
	if (!ssp_cap)
		goto out;

	speed_id = ext_portstatus & USB_EXT_PORT_STAT_RX_SPEED_ID;
	lanes = USB_EXT_PORT_RX_LANES(ext_portstatus) + 1;

	ssac = le32_to_cpu(ssp_cap->bmAttributes) &
		USB_SSP_SUBLINK_SPEED_ATTRIBS;

	for (i = 0; i <= ssac; i++) {
		u8 ssid;

		attr = le32_to_cpu(ssp_cap->bmSublinkSpeedAttr[i]);
		ssid = FIELD_GET(USB_SSP_SUBLINK_SPEED_SSID, attr);
		if (speed_id == ssid) {
			u16 mantissa;
			u8 lse;
			u8 type;

			/*
			 * Note: currently asymmetric lane types are only
			 * applicable for SSIC operate in SuperSpeed protocol
			 */
			type = FIELD_GET(USB_SSP_SUBLINK_SPEED_ST, attr);
			if (type == USB_SSP_SUBLINK_SPEED_ST_ASYM_RX ||
			    type == USB_SSP_SUBLINK_SPEED_ST_ASYM_TX)
				goto out;

			if (FIELD_GET(USB_SSP_SUBLINK_SPEED_LP, attr) !=
			    USB_SSP_SUBLINK_SPEED_LP_SSP)
				goto out;

			lse = FIELD_GET(USB_SSP_SUBLINK_SPEED_LSE, attr);
			mantissa = FIELD_GET(USB_SSP_SUBLINK_SPEED_LSM, attr);

			/* Convert to Gbps */
			for (; lse < USB_SSP_SUBLINK_SPEED_LSE_GBPS; lse++)
				mantissa /= 1000;

			if (mantissa >= 10 && lanes == 1)
				return USB_SSP_GEN_2x1;

			if (mantissa >= 10 && lanes == 2)
				return USB_SSP_GEN_2x2;

			if (mantissa >= 5 && lanes == 2)
				return USB_SSP_GEN_1x2;

			goto out;
		}
	}

out:
	return USB_SSP_GEN_UNKNOWN;
}

#ifdef CONFIG_USB_FEW_INIT_RETRIES
#define PORT_RESET_TRIES	2
#define SET_ADDRESS_TRIES	1
#define GET_DESCRIPTOR_TRIES	1
#define GET_MAXPACKET0_TRIES	1
#define PORT_INIT_TRIES		4

#else
#define PORT_RESET_TRIES	5
#define SET_ADDRESS_TRIES	2
#define GET_DESCRIPTOR_TRIES	2
#define GET_MAXPACKET0_TRIES	3
#define PORT_INIT_TRIES		4
#endif	/* CONFIG_USB_FEW_INIT_RETRIES */

#define DETECT_DISCONNECT_TRIES 5

#define HUB_ROOT_RESET_TIME	60	/* times are in msec */
#define HUB_SHORT_RESET_TIME	10
#define HUB_BH_RESET_TIME	50
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	800

static bool use_new_scheme(struct usb_device *udev, int retry,
			   struct usb_port *port_dev)
{
	int old_scheme_first_port =
		(port_dev->quirks & USB_PORT_QUIRK_OLD_SCHEME) ||
		old_scheme_first;

	/*
	 * "New scheme" enumeration causes an extra state transition to be
	 * exposed to an xhci host and causes USB3 devices to receive control
	 * commands in the default state.  This has been seen to cause
	 * enumeration failures, so disable this enumeration scheme for USB3
	 * devices.
	 */
	if (udev->speed >= USB_SPEED_SUPER)
		return false;

	/*
	 * If use_both_schemes is set, use the first scheme (whichever
	 * it is) for the larger half of the retries, then use the other
	 * scheme.  Otherwise, use the first scheme for all the retries.
	 */
	if (use_both_schemes && retry >= (PORT_INIT_TRIES + 1) / 2)
		return old_scheme_first_port;	/* Second half */
	return !old_scheme_first_port;		/* First half or all */
}

/* Is a USB 3.0 port in the Inactive or Compliance Mode state?
 * Port warm reset is required to recover
 */
static bool hub_port_warm_reset_required(struct usb_hub *hub, int port1,
		u16 portstatus)
{
	u16 link_state;

	if (!hub_is_superspeed(hub->hdev))
		return false;

	if (test_bit(port1, hub->warm_reset_bits))
		return true;

	link_state = portstatus & USB_PORT_STAT_LINK_STATE;
	return link_state == USB_SS_PORT_LS_SS_INACTIVE
		|| link_state == USB_SS_PORT_LS_COMP_MOD;
}

static int hub_port_wait_reset(struct usb_hub *hub, int port1,
			struct usb_device *udev, unsigned int delay, bool warm)
{
	int delay_time, ret;
	u16 portstatus;
	u16 portchange;
	u32 ext_portstatus = 0;

	for (delay_time = 0;
			delay_time < HUB_RESET_TIMEOUT;
			delay_time += delay) {
		/* wait to give the device a chance to reset */
		msleep(delay);