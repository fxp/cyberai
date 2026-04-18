// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 29/41



	/*
	 * Enable device initiated U1/U2 with a SetFeature(U1/U2_ENABLE) request
	 * if system exit latency is short enough and device is configured
	 */
	if (usb_device_may_initiate_lpm(udev, USB3_LPM_U1)) {
		if (usb_set_device_initiated_lpm(udev, USB3_LPM_U1, true))
			return;

		if (usb_device_may_initiate_lpm(udev, USB3_LPM_U2))
			usb_set_device_initiated_lpm(udev, USB3_LPM_U2, true);
	}
}
EXPORT_SYMBOL_GPL(usb_enable_lpm);

/* Grab the bandwidth_mutex before calling usb_enable_lpm() */
void usb_unlocked_enable_lpm(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	if (!hcd)
		return;

	mutex_lock(hcd->bandwidth_mutex);
	usb_enable_lpm(udev);
	mutex_unlock(hcd->bandwidth_mutex);
}
EXPORT_SYMBOL_GPL(usb_unlocked_enable_lpm);

/* usb3 devices use U3 for disabled, make sure remote wakeup is disabled */
static void hub_usb3_port_prepare_disable(struct usb_hub *hub,
					  struct usb_port *port_dev)
{
	struct usb_device *udev = port_dev->child;
	int ret;

	if (udev && udev->port_is_suspended && udev->do_remote_wakeup) {
		ret = hub_set_port_link_state(hub, port_dev->portnum,
					      USB_SS_PORT_LS_U0);
		if (!ret) {
			msleep(USB_RESUME_TIMEOUT);
			ret = usb_disable_remote_wakeup(udev);
		}
		if (ret)
			dev_warn(&udev->dev,
				 "Port disable: can't disable remote wake\n");
		udev->do_remote_wakeup = 0;
	}
}

#else	/* CONFIG_PM */

#define hub_suspend		NULL
#define hub_resume		NULL
#define hub_reset_resume	NULL

static inline void hub_usb3_port_prepare_disable(struct usb_hub *hub,
						 struct usb_port *port_dev) { }

int usb_disable_lpm(struct usb_device *udev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(usb_disable_lpm);

void usb_enable_lpm(struct usb_device *udev) { }
EXPORT_SYMBOL_GPL(usb_enable_lpm);

int usb_unlocked_disable_lpm(struct usb_device *udev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(usb_unlocked_disable_lpm);

void usb_unlocked_enable_lpm(struct usb_device *udev) { }
EXPORT_SYMBOL_GPL(usb_unlocked_enable_lpm);

int usb_disable_ltm(struct usb_device *udev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(usb_disable_ltm);

void usb_enable_ltm(struct usb_device *udev) { }
EXPORT_SYMBOL_GPL(usb_enable_ltm);

static int hub_handle_remote_wakeup(struct usb_hub *hub, unsigned int port,
		u16 portstatus, u16 portchange)
{
	return 0;
}

static int usb_req_set_sel(struct usb_device *udev)
{
	return 0;
}

#endif	/* CONFIG_PM */

/*
 * USB-3 does not have a similar link state as USB-2 that will avoid negotiating
 * a connection with a plugged-in cable but will signal the host when the cable
 * is unplugged. Disable remote wake and set link state to U3 for USB-3 devices
 */
static int hub_port_disable(struct usb_hub *hub, int port1, int set_state)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *hdev = hub->hdev;
	int ret = 0;

	if (!hub->error) {
		if (hub_is_superspeed(hub->hdev)) {
			hub_usb3_port_prepare_disable(hub, port_dev);
			ret = hub_set_port_link_state(hub, port_dev->portnum,
						      USB_SS_PORT_LS_U3);
		} else {
			ret = usb_clear_port_feature(hdev, port1,
					USB_PORT_FEAT_ENABLE);
		}
	}
	if (port_dev->child && set_state)
		usb_set_device_state(port_dev->child, USB_STATE_NOTATTACHED);
	if (ret && ret != -ENODEV)
		dev_err(&port_dev->dev, "cannot disable (err = %d)\n", ret);
	return ret;
}

/*
 * usb_port_disable - disable a usb device's upstream port
 * @udev: device to disable
 * Context: @udev locked, must be able to sleep.
 *
 * Disables a USB device that isn't in active use.
 */
int usb_port_disable(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);

	return hub_port_disable(hub, udev->portnum, 0);
}

/* USB 2.0 spec, 7.1.7.3 / fig 7-29:
 *
 * Between connect detection and reset signaling there must be a delay
 * of 100ms at least for debounce and power-settling.  The corresponding
 * timer shall restart whenever the downstream port detects a disconnect.
 *
 * Apparently there are some bluetooth and irda-dongles and a number of
 * low-speed devices for which this debounce period may last over a second.
 * Not covered by the spec - but easy to deal with.
 *
 * This implementation uses a 1500ms total debounce timeout; if the
 * connection isn't stable by then it returns -ETIMEDOUT.  It checks
 * every 25ms for transient disconnects.  When the port status has been
 * unchanged for 100ms it returns the port status.
 */
int hub_port_debounce(struct usb_hub *hub, int port1, bool must_be_connected)
{
	int ret;
	u16 portchange, portstatus;
	unsigned connection = 0xffff;
	int total_time, stable_time = 0;
	struct usb_port *port_dev = hub->ports[port1 - 1];

	for (total_time = 0; ; total_time += HUB_DEBOUNCE_STEP) {
		ret = usb_hub_port_status(hub, port1, &portstatus, &portchange);
		if (ret < 0)
			return ret;