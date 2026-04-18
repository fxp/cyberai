// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 41/41



/**
 * usb_queue_reset_device - Reset a USB device from an atomic context
 * @iface: USB interface belonging to the device to reset
 *
 * This function can be used to reset a USB device from an atomic
 * context, where usb_reset_device() won't work (as it blocks).
 *
 * Doing a reset via this method is functionally equivalent to calling
 * usb_reset_device(), except for the fact that it is delayed to a
 * workqueue. This means that any drivers bound to other interfaces
 * might be unbound, as well as users from usbfs in user space.
 *
 * Corner cases:
 *
 * - Scheduling two resets at the same time from two different drivers
 *   attached to two different interfaces of the same device is
 *   possible; depending on how the driver attached to each interface
 *   handles ->pre_reset(), the second reset might happen or not.
 *
 * - If the reset is delayed so long that the interface is unbound from
 *   its driver, the reset will be skipped.
 *
 * - This function can be called during .probe().  It can also be called
 *   during .disconnect(), but doing so is pointless because the reset
 *   will not occur.  If you really want to reset the device during
 *   .disconnect(), call usb_reset_device() directly -- but watch out
 *   for nested unbinding issues!
 */
void usb_queue_reset_device(struct usb_interface *iface)
{
	if (schedule_work(&iface->reset_ws))
		usb_get_intf(iface);
}
EXPORT_SYMBOL_GPL(usb_queue_reset_device);

/**
 * usb_hub_find_child - Get the pointer of child device
 * attached to the port which is specified by @port1.
 * @hdev: USB device belonging to the usb hub
 * @port1: port num to indicate which port the child device
 *	is attached to.
 *
 * USB drivers call this function to get hub's child device
 * pointer.
 *
 * Return: %NULL if input param is invalid and
 * child's usb_device pointer if non-NULL.
 */
struct usb_device *usb_hub_find_child(struct usb_device *hdev,
		int port1)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	if (port1 < 1 || port1 > hdev->maxchild)
		return NULL;
	return hub->ports[port1 - 1]->child;
}
EXPORT_SYMBOL_GPL(usb_hub_find_child);

void usb_hub_adjust_deviceremovable(struct usb_device *hdev,
		struct usb_hub_descriptor *desc)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);
	enum usb_port_connect_type connect_type;
	int i;

	if (!hub)
		return;

	if (!hub_is_superspeed(hdev)) {
		for (i = 1; i <= hdev->maxchild; i++) {
			struct usb_port *port_dev = hub->ports[i - 1];

			connect_type = port_dev->connect_type;
			if (connect_type == USB_PORT_CONNECT_TYPE_HARD_WIRED) {
				u8 mask = 1 << (i%8);

				if (!(desc->u.hs.DeviceRemovable[i/8] & mask)) {
					dev_dbg(&port_dev->dev, "DeviceRemovable is changed to 1 according to platform information.\n");
					desc->u.hs.DeviceRemovable[i/8]	|= mask;
				}
			}
		}
	} else {
		u16 port_removable = le16_to_cpu(desc->u.ss.DeviceRemovable);

		for (i = 1; i <= hdev->maxchild; i++) {
			struct usb_port *port_dev = hub->ports[i - 1];

			connect_type = port_dev->connect_type;
			if (connect_type == USB_PORT_CONNECT_TYPE_HARD_WIRED) {
				u16 mask = 1 << i;

				if (!(port_removable & mask)) {
					dev_dbg(&port_dev->dev, "DeviceRemovable is changed to 1 according to platform information.\n");
					port_removable |= mask;
				}
			}
		}

		desc->u.ss.DeviceRemovable = cpu_to_le16(port_removable);
	}
}

#ifdef CONFIG_ACPI
/**
 * usb_get_hub_port_acpi_handle - Get the usb port's acpi handle
 * @hdev: USB device belonging to the usb hub
 * @port1: port num of the port
 *
 * Return: Port's acpi handle if successful, %NULL if params are
 * invalid.
 */
acpi_handle usb_get_hub_port_acpi_handle(struct usb_device *hdev,
	int port1)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	if (!hub)
		return NULL;

	return ACPI_HANDLE(&hub->ports[port1 - 1]->dev);
}
#endif
