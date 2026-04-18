// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 14/41



/**
 * usb_set_device_state - change a device's current state (usbcore, hcds)
 * @udev: pointer to device whose state should be changed
 * @new_state: new state value to be stored
 *
 * udev->state is _not_ fully protected by the device lock.  Although
 * most transitions are made only while holding the lock, the state can
 * can change to USB_STATE_NOTATTACHED at almost any time.  This
 * is so that devices can be marked as disconnected as soon as possible,
 * without having to wait for any semaphores to be released.  As a result,
 * all changes to any device's state must be protected by the
 * device_state_lock spinlock.
 *
 * Once a device has been added to the device tree, all changes to its state
 * should be made using this routine.  The state should _not_ be set directly.
 *
 * If udev->state is already USB_STATE_NOTATTACHED then no change is made.
 * Otherwise udev->state is set to new_state, and if new_state is
 * USB_STATE_NOTATTACHED then all of udev's descendants' states are also set
 * to USB_STATE_NOTATTACHED.
 */
void usb_set_device_state(struct usb_device *udev,
		enum usb_device_state new_state)
{
	unsigned long flags;
	int wakeup = -1;

	spin_lock_irqsave(&device_state_lock, flags);
	if (udev->state == USB_STATE_NOTATTACHED)
		;	/* do nothing */
	else if (new_state != USB_STATE_NOTATTACHED) {

		/* root hub wakeup capabilities are managed out-of-band
		 * and may involve silicon errata ... ignore them here.
		 */
		if (udev->parent) {
			if (udev->state == USB_STATE_SUSPENDED
					|| new_state == USB_STATE_SUSPENDED)
				;	/* No change to wakeup settings */
			else if (new_state == USB_STATE_CONFIGURED)
				wakeup = (udev->quirks &
					USB_QUIRK_IGNORE_REMOTE_WAKEUP) ? 0 :
					udev->actconfig->desc.bmAttributes &
					USB_CONFIG_ATT_WAKEUP;
			else
				wakeup = 0;
		}
		update_usb_device_state(udev, new_state);
	} else
		recursively_mark_NOTATTACHED(udev);
	spin_unlock_irqrestore(&device_state_lock, flags);
	if (wakeup >= 0)
		device_set_wakeup_capable(&udev->dev, wakeup);
}
EXPORT_SYMBOL_GPL(usb_set_device_state);

/*
 * Choose a device number.
 *
 * Device numbers are used as filenames in usbfs.  On USB-1.1 and
 * USB-2.0 buses they are also used as device addresses, however on
 * USB-3.0 buses the address is assigned by the controller hardware
 * and it usually is not the same as the device number.
 *
 * Devices connected under xHCI are not as simple.  The host controller
 * supports virtualization, so the hardware assigns device addresses and
 * the HCD must setup data structures before issuing a set address
 * command to the hardware.
 */
static void choose_devnum(struct usb_device *udev)
{
	int		devnum;
	struct usb_bus	*bus = udev->bus;

	/* be safe when more hub events are proceed in parallel */
	mutex_lock(&bus->devnum_next_mutex);

	/* Try to allocate the next devnum beginning at bus->devnum_next. */
	devnum = find_next_zero_bit(bus->devmap, 128, bus->devnum_next);
	if (devnum >= 128)
		devnum = find_next_zero_bit(bus->devmap, 128, 1);
	bus->devnum_next = (devnum >= 127 ? 1 : devnum + 1);
	if (devnum < 128) {
		set_bit(devnum, bus->devmap);
		udev->devnum = devnum;
	}
	mutex_unlock(&bus->devnum_next_mutex);
}

static void release_devnum(struct usb_device *udev)
{
	if (udev->devnum > 0) {
		clear_bit(udev->devnum, udev->bus->devmap);
		udev->devnum = -1;
	}
}

static void update_devnum(struct usb_device *udev, int devnum)
{
	udev->devnum = devnum;
	if (!udev->devaddr)
		udev->devaddr = (u8)devnum;
}

static void hub_free_dev(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	/* Root hubs aren't real devices, so don't free HCD resources */
	if (hcd->driver->free_dev && udev->parent)
		hcd->driver->free_dev(hcd, udev);
}

static void hub_disconnect_children(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);
	int i;

	/* Free up all the children before we remove this device */
	for (i = 0; i < udev->maxchild; i++) {
		if (hub->ports[i]->child)
			usb_disconnect(&hub->ports[i]->child);
	}
}

/**
 * usb_disconnect - disconnect a device (usbcore-internal)
 * @pdev: pointer to device being disconnected
 *
 * Context: task context, might sleep
 *
 * Something got disconnected. Get rid of it and all of its children.
 *
 * If *pdev is a normal device then the parent hub must already be locked.
 * If *pdev is a root hub then the caller must hold the usb_bus_idr_lock,
 * which protects the set of root hubs as well as the list of buses.
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
void usb_disconnect(struct usb_device **pdev)
{
	struct usb_port *port_dev = NULL;
	struct usb_device *udev = *pdev;
	struct usb_hub *hub = NULL;
	int port1 = 1;