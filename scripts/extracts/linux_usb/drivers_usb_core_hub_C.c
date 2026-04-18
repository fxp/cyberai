// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 35/41



		/* We mustn't add new devices if the parent hub has
		 * been disconnected; we would race with the
		 * recursively_mark_NOTATTACHED() routine.
		 */
		spin_lock_irq(&device_state_lock);
		if (hdev->state == USB_STATE_NOTATTACHED)
			status = -ENOTCONN;
		else
			port_dev->child = udev;
		spin_unlock_irq(&device_state_lock);
		mutex_unlock(&usb_port_peer_mutex);

		/* Run it through the hoops (find a driver, etc) */
		if (!status) {
			status = usb_new_device(udev);
			if (status) {
				mutex_lock(&usb_port_peer_mutex);
				spin_lock_irq(&device_state_lock);
				port_dev->child = NULL;
				spin_unlock_irq(&device_state_lock);
				mutex_unlock(&usb_port_peer_mutex);
			} else {
				if (hcd->usb_phy && !hdev->parent)
					usb_phy_notify_connect(hcd->usb_phy,
							udev->speed);
			}
		}

		if (status)
			goto loop_disable;

		status = hub_power_remaining(hub);
		if (status)
			dev_dbg(hub->intfdev, "%dmA power budget left\n", status);

		return;

loop_disable:
		hub_port_disable(hub, port1, 1);
loop:
		usb_ep0_reinit(udev);
		release_devnum(udev);
		hub_free_dev(udev);
		if (retry_locked) {
			mutex_unlock(hcd->address0_mutex);
			usb_unlock_port(port_dev);
		}
		usb_put_dev(udev);
		if ((status == -ENOTCONN) || (status == -ENOTSUPP))
			break;

		/* When halfway through our retry count, power-cycle the port */
		if (i == (PORT_INIT_TRIES - 1) / 2) {
			dev_info(&port_dev->dev, "attempt power cycle\n");
			usb_hub_set_port_power(hdev, hub, port1, false);
			msleep(2 * hub_power_on_good_delay(hub));
			usb_hub_set_port_power(hdev, hub, port1, true);
			msleep(hub_power_on_good_delay(hub));
		}
	}
	if (hub->hdev->parent ||
			!hcd->driver->port_handed_over ||
			!(hcd->driver->port_handed_over)(hcd, port1)) {
		if (status != -ENOTCONN && status != -ENODEV)
			dev_err(&port_dev->dev,
					"unable to enumerate USB device\n");
	}

done:
	hub_port_disable(hub, port1, 1);
	if (hcd->driver->relinquish_port && !hub->hdev->parent) {
		if (status != -ENOTCONN && status != -ENODEV)
			hcd->driver->relinquish_port(hcd, port1);
	}
}

/* Handle physical or logical connection change events.
 * This routine is called when:
 *	a port connection-change occurs;
 *	a port enable-change occurs (often caused by EMI);
 *	usb_reset_and_verify_device() encounters changed descriptors (as from
 *		a firmware download)
 * caller already locked the hub
 */
static void hub_port_connect_change(struct usb_hub *hub, int port1,
					u16 portstatus, u16 portchange)
		__must_hold(&port_dev->status_lock)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *udev = port_dev->child;
	struct usb_device_descriptor *descr;
	int status = -ENODEV;

	dev_dbg(&port_dev->dev, "status %04x, change %04x, %s\n", portstatus,
			portchange, portspeed(hub, portstatus));

	if (hub->has_indicators) {
		set_port_led(hub, port1, HUB_LED_AUTO);
		hub->indicator[port1-1] = INDICATOR_AUTO;
	}

#ifdef	CONFIG_USB_OTG
	/* during HNP, don't repeat the debounce */
	if (hub->hdev->bus->is_b_host)
		portchange &= ~(USB_PORT_STAT_C_CONNECTION |
				USB_PORT_STAT_C_ENABLE);
#endif

	/* Try to resuscitate an existing device */
	if ((portstatus & USB_PORT_STAT_CONNECTION) && udev &&
			udev->state != USB_STATE_NOTATTACHED) {
		if (portstatus & USB_PORT_STAT_ENABLE) {
			/*
			 * USB-3 connections are initialized automatically by
			 * the hostcontroller hardware. Therefore check for
			 * changed device descriptors before resuscitating the
			 * device.
			 */
			descr = usb_get_device_descriptor(udev);
			if (IS_ERR(descr)) {
				dev_dbg(&udev->dev,
						"can't read device descriptor %ld\n",
						PTR_ERR(descr));
			} else {
				if (descriptors_changed(udev, descr,
						udev->bos)) {
					dev_dbg(&udev->dev,
							"device descriptor has changed\n");
				} else {
					status = 0; /* Nothing to do */
				}
				kfree(descr);
			}
#ifdef CONFIG_PM
		} else if (udev->state == USB_STATE_SUSPENDED &&
				udev->persist_enabled) {
			/* For a suspended device, treat this as a
			 * remote wakeup event.
			 */
			usb_unlock_port(port_dev);
			status = usb_remote_wakeup(udev);
			usb_lock_port(port_dev);
#endif
		} else {
			/* Don't resuscitate */;
		}
	}
	clear_bit(port1, hub->change_bits);

	/* successfully revalidated the connection */
	if (status == 0)
		return;

	usb_unlock_port(port_dev);
	hub_port_connect(hub, port1, portstatus, portchange);
	usb_lock_port(port_dev);
}

/* Handle notifying userspace about hub over-current events */
static void port_over_current_notify(struct usb_port *port_dev)
{
	char *envp[3] = { NULL, NULL, NULL };
	struct device *hub_dev;
	char *port_dev_path;

	sysfs_notify(&port_dev->dev.kobj, NULL, "over_current_count");

	hub_dev = port_dev->dev.parent;

	if (!hub_dev)
		return;

	port_dev_path = kobject_get_path(&port_dev->dev.kobj, GFP_KERNEL);
	if (!port_dev_path)
		return;

	envp[0] = kasprintf(GFP_KERNEL, "OVER_CURRENT_PORT=%s", port_dev_path);
	if (!envp[0])
		goto exit;