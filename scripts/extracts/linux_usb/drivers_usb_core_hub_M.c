// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 13/41



	if (hub_configure(hub, &desc->endpoint[0].desc) >= 0) {
		onboard_dev_create_pdevs(hdev, &hub->onboard_devs);

		return 0;
	}

	hub_disconnect(intf);
	return -ENODEV;
}

static int
hub_ioctl(struct usb_interface *intf, unsigned int code, void *user_data)
{
	struct usb_device *hdev = interface_to_usbdev(intf);
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	/* assert ifno == 0 (part of hub spec) */
	switch (code) {
	case USBDEVFS_HUB_PORTINFO: {
		struct usbdevfs_hub_portinfo *info = user_data;
		int i;

		spin_lock_irq(&device_state_lock);
		if (hdev->devnum <= 0)
			info->nports = 0;
		else {
			info->nports = hdev->maxchild;
			for (i = 0; i < info->nports; i++) {
				if (hub->ports[i]->child == NULL)
					info->port[i] = 0;
				else
					info->port[i] =
						hub->ports[i]->child->devnum;
			}
		}
		spin_unlock_irq(&device_state_lock);

		return info->nports + 1;
		}

	default:
		return -ENOSYS;
	}
}

/*
 * Allow user programs to claim ports on a hub.  When a device is attached
 * to one of these "claimed" ports, the program will "own" the device.
 */
static int find_port_owner(struct usb_device *hdev, unsigned port1,
		struct usb_dev_state ***ppowner)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	if (hdev->state == USB_STATE_NOTATTACHED)
		return -ENODEV;
	if (port1 == 0 || port1 > hdev->maxchild)
		return -EINVAL;

	/* Devices not managed by the hub driver
	 * will always have maxchild equal to 0.
	 */
	*ppowner = &(hub->ports[port1 - 1]->port_owner);
	return 0;
}

/* In the following three functions, the caller must hold hdev's lock */
int usb_hub_claim_port(struct usb_device *hdev, unsigned port1,
		       struct usb_dev_state *owner)
{
	int rc;
	struct usb_dev_state **powner;

	rc = find_port_owner(hdev, port1, &powner);
	if (rc)
		return rc;
	if (*powner)
		return -EBUSY;
	*powner = owner;
	return rc;
}
EXPORT_SYMBOL_GPL(usb_hub_claim_port);

int usb_hub_release_port(struct usb_device *hdev, unsigned port1,
			 struct usb_dev_state *owner)
{
	int rc;
	struct usb_dev_state **powner;

	rc = find_port_owner(hdev, port1, &powner);
	if (rc)
		return rc;
	if (*powner != owner)
		return -ENOENT;
	*powner = NULL;
	return rc;
}
EXPORT_SYMBOL_GPL(usb_hub_release_port);

void usb_hub_release_all_ports(struct usb_device *hdev, struct usb_dev_state *owner)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);
	int n;

	for (n = 0; n < hdev->maxchild; n++) {
		if (hub->ports[n]->port_owner == owner)
			hub->ports[n]->port_owner = NULL;
	}

}

/* The caller must hold udev's lock */
bool usb_device_is_owned(struct usb_device *udev)
{
	struct usb_hub *hub;

	if (udev->state == USB_STATE_NOTATTACHED || !udev->parent)
		return false;
	hub = usb_hub_to_struct_hub(udev->parent);
	return !!hub->ports[udev->portnum - 1]->port_owner;
}

static void update_port_device_state(struct usb_device *udev)
{
	struct usb_hub *hub;
	struct usb_port *port_dev;

	if (udev->parent) {
		hub = usb_hub_to_struct_hub(udev->parent);

		/*
		 * The Link Layer Validation System Driver (lvstest)
		 * has a test step to unbind the hub before running the
		 * rest of the procedure. This triggers hub_disconnect
		 * which will set the hub's maxchild to 0, further
		 * resulting in usb_hub_to_struct_hub returning NULL.
		 */
		if (hub) {
			port_dev = hub->ports[udev->portnum - 1];
			WRITE_ONCE(port_dev->state, udev->state);
			sysfs_notify_dirent(port_dev->state_kn);
		}
	}
}

static void update_usb_device_state(struct usb_device *udev,
				    enum usb_device_state new_state)
{
	if (udev->state == USB_STATE_SUSPENDED &&
	    new_state != USB_STATE_SUSPENDED)
		udev->active_duration -= jiffies;
	else if (new_state == USB_STATE_SUSPENDED &&
		 udev->state != USB_STATE_SUSPENDED)
		udev->active_duration += jiffies;

	udev->state = new_state;
	update_port_device_state(udev);
	trace_usb_set_device_state(udev);
}

static void recursively_mark_NOTATTACHED(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);
	int i;

	for (i = 0; i < udev->maxchild; ++i) {
		if (hub->ports[i]->child)
			recursively_mark_NOTATTACHED(hub->ports[i]->child);
	}
	update_usb_device_state(udev, USB_STATE_NOTATTACHED);
}