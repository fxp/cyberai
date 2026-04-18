// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 38/41



static struct usb_driver hub_driver = {
	.name =		"hub",
	.probe =	hub_probe,
	.disconnect =	hub_disconnect,
	.suspend =	hub_suspend,
	.resume =	hub_resume,
	.reset_resume =	hub_reset_resume,
	.pre_reset =	hub_pre_reset,
	.post_reset =	hub_post_reset,
	.unlocked_ioctl = hub_ioctl,
	.id_table =	hub_id_table,
	.supports_autosuspend =	1,
};

int usb_hub_init(void)
{
	if (usb_register(&hub_driver) < 0) {
		printk(KERN_ERR "%s: can't register hub driver\n",
			usbcore_name);
		return -1;
	}

	/*
	 * The workqueue needs to be freezable to avoid interfering with
	 * USB-PERSIST port handover. Otherwise it might see that a full-speed
	 * device was gone before the EHCI controller had handed its port
	 * over to the companion full-speed controller.
	 */
	hub_wq = alloc_workqueue("usb_hub_wq", WQ_FREEZABLE | WQ_PERCPU, 0);
	if (hub_wq)
		return 0;

	/* Fall through if kernel_thread failed */
	usb_deregister(&hub_driver);
	pr_err("%s: can't allocate workqueue for usb hub\n", usbcore_name);

	return -1;
}

void usb_hub_cleanup(void)
{
	destroy_workqueue(hub_wq);

	/*
	 * Hub resources are freed for us by usb_deregister. It calls
	 * usb_driver_purge on every device which in turn calls that
	 * devices disconnect function if it is using this driver.
	 * The hub_disconnect function takes care of releasing the
	 * individual hub resources. -greg
	 */
	usb_deregister(&hub_driver);
} /* usb_hub_cleanup() */

/**
 * hub_hc_release_resources - clear resources used by host controller
 * @udev: pointer to device being released
 *
 * Context: task context, might sleep
 *
 * Function releases the host controller resources in correct order before
 * making any operation on resuming usb device. The host controller resources
 * allocated for devices in tree should be released starting from the last
 * usb device in tree toward the root hub. This function is used only during
 * resuming device when usb device require reinitialization – that is, when
 * flag udev->reset_resume is set.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
static void hub_hc_release_resources(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	int i;

	/* Release up resources for all children before this device */
	for (i = 0; i < udev->maxchild; i++)
		if (hub->ports[i]->child)
			hub_hc_release_resources(hub->ports[i]->child);

	if (hcd->driver->reset_device)
		hcd->driver->reset_device(hcd, udev);
}

/**
 * usb_reset_and_verify_device - perform a USB port reset to reinitialize a device
 * @udev: device to reset (not in SUSPENDED or NOTATTACHED state)
 *
 * WARNING - don't use this routine to reset a composite device
 * (one with multiple interfaces owned by separate drivers)!
 * Use usb_reset_device() instead.
 *
 * Do a port reset, reassign the device's address, and establish its
 * former operating configuration.  If the reset fails, or the device's
 * descriptors change from their values before the reset, or the original
 * configuration and altsettings cannot be restored, a flag will be set
 * telling hub_wq to pretend the device has been disconnected and then
 * re-connected.  All drivers will be unbound, and the device will be
 * re-enumerated and probed all over again.
 *
 * Return: 0 if the reset succeeded, -ENODEV if the device has been
 * flagged for logical disconnection, or some other negative error code
 * if the reset wasn't even attempted.
 *
 * Note:
 * The caller must own the device lock and the port lock, the latter is
 * taken by usb_reset_device().  For example, it's safe to use
 * usb_reset_device() from a driver probe() routine after downloading
 * new firmware.  For calls that might not occur during probe(), drivers
 * should lock the device using usb_lock_device_for_reset().
 *
 * Locking exception: This routine may also be called from within an
 * autoresume handler.  Such usage won't conflict with other tasks
 * holding the device lock because these tasks should always call
 * usb_autopm_resume_device(), thereby preventing any unwanted
 * autoresume.  The autoresume handler is expected to have already
 * acquired the port lock before calling this routine.
 */
static int usb_reset_and_verify_device(struct usb_device *udev)
{
	struct usb_device		*parent_hdev = udev->parent;
	struct usb_hub			*parent_hub;
	struct usb_hcd			*hcd = bus_to_hcd(udev->bus);
	struct usb_device_descriptor	descriptor;
	struct usb_interface		*intf;
	struct usb_host_bos		*bos;
	int				i, j, ret = 0;
	int				port1 = udev->portnum;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED) {
		dev_dbg(&udev->dev, "device reset not allowed in state %d\n",
				udev->state);
		return -EINVAL;
	}

	if (!parent_hdev)
		return -EISDIR;

	parent_hub = usb_hub_to_struct_hub(parent_hdev);

	/* Disable USB2 hardware LPM.
	 * It will be re-enabled by the enumeration process.
	 */
	usb_disable_usb2_hardware_lpm(udev);