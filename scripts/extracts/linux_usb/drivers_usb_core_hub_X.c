// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 24/41



/*
 * There are some SS USB devices which take longer time for link training.
 * XHCI specs 4.19.4 says that when Link training is successful, port
 * sets CCS bit to 1. So if SW reads port status before successful link
 * training, then it will not find device to be present.
 * USB Analyzer log with such buggy devices show that in some cases
 * device switch on the RX termination after long delay of host enabling
 * the VBUS. In few other cases it has been seen that device fails to
 * negotiate link training in first attempt. It has been
 * reported till now that few devices take as long as 2000 ms to train
 * the link after host enabling its VBUS and termination. Following
 * routine implements a 2000 ms timeout for link training. If in a case
 * link trains before timeout, loop will exit earlier.
 *
 * There are also some 2.0 hard drive based devices and 3.0 thumb
 * drives that, when plugged into a 2.0 only port, take a long
 * time to set CCS after VBUS enable.
 *
 * FIXME: If a device was connected before suspend, but was removed
 * while system was asleep, then the loop in the following routine will
 * only exit at timeout.
 *
 * This routine should only be called when persist is enabled.
 */
static int wait_for_connected(struct usb_device *udev,
		struct usb_hub *hub, int port1,
		u16 *portchange, u16 *portstatus)
{
	int status = 0, delay_ms = 0;

	while (delay_ms < 2000) {
		if (status || *portstatus & USB_PORT_STAT_CONNECTION)
			break;
		if (!usb_port_is_power_on(hub, *portstatus)) {
			status = -ENODEV;
			break;
		}
		msleep(20);
		delay_ms += 20;
		status = usb_hub_port_status(hub, port1, portstatus, portchange);
	}
	dev_dbg(&udev->dev, "Waited %dms for CONNECT\n", delay_ms);
	return status;
}

/*
 * usb_port_resume - re-activate a suspended usb device's upstream port
 * @udev: device to re-activate, not a root hub
 * Context: must be able to sleep; device not locked; pm locks held
 *
 * This will re-activate the suspended device, increasing power usage
 * while letting drivers communicate again with its endpoints.
 * USB resume explicitly guarantees that the power session between
 * the host and the device is the same as it was when the device
 * suspended.
 *
 * If @udev->reset_resume is set then this routine won't check that the
 * port is still enabled.  Furthermore, finish_port_resume() above will
 * reset @udev.  The end result is that a broken power session can be
 * recovered and @udev will appear to persist across a loss of VBUS power.
 *
 * For example, if a host controller doesn't maintain VBUS suspend current
 * during a system sleep or is reset when the system wakes up, all the USB
 * power sessions below it will be broken.  This is especially troublesome
 * for mass-storage devices containing mounted filesystems, since the
 * device will appear to have disconnected and all the memory mappings
 * to it will be lost.  Using the USB_PERSIST facility, the device can be
 * made to appear as if it had not disconnected.
 *
 * This facility can be dangerous.  Although usb_reset_and_verify_device() makes
 * every effort to insure that the same device is present after the
 * reset as before, it cannot provide a 100% guarantee.  Furthermore it's
 * quite possible for a device to remain unaltered but its media to be
 * changed.  If the user replaces a flash memory card while the system is
 * asleep, he will have only himself to blame when the filesystem on the
 * new card is corrupted and the system crashes.
 *
 * Returns 0 on success, else negative errno.
 */
int usb_port_resume(struct usb_device *udev, pm_message_t msg)
{
	struct usb_hub	*hub = usb_hub_to_struct_hub(udev->parent);
	struct usb_port *port_dev = hub->ports[udev->portnum  - 1];
	int		port1 = udev->portnum;
	int		status;
	u16		portchange, portstatus;

	if (!test_and_set_bit(port1, hub->child_usage_bits)) {
		status = pm_runtime_resume_and_get(&port_dev->dev);
		if (status < 0) {
			dev_dbg(&udev->dev, "can't resume usb port, status %d\n",
					status);
			return status;
		}
	}

	usb_lock_port(port_dev);

	/* Skip the initial Clear-Suspend step for a remote wakeup */
	status = usb_hub_port_status(hub, port1, &portstatus, &portchange);
	if (status == 0 && !port_is_suspended(hub, portstatus)) {
		if (portchange & USB_PORT_STAT_C_SUSPEND)
			pm_wakeup_event(&udev->dev, 0);
		goto SuspendCleared;
	}

	/* see 7.1.7.7; affects power usage, but not budgeting */
	if (hub_is_superspeed(hub->hdev))
		status = hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_U0);
	else
		status = usb_clear_port_feature(hub->hdev,
				port1, USB_PORT_FEAT_SUSPEND);
	if (status) {
		dev_dbg(&port_dev->dev, "can't resume, status %d\n", status);
	} else {
		/* drive resume for USB_RESUME_TIMEOUT msec */
		dev_dbg(&udev->dev, "usb %sresume\n",
				(PMSG_IS_AUTO(msg) ? "auto-" : ""));
		msleep(USB_RESUME_TIMEOUT);