// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/message.c
// Segment 12/17



/**
 * usb_reset_configuration - lightweight device reset
 * @dev: the device whose configuration is being reset
 *
 * This issues a standard SET_CONFIGURATION request to the device using
 * the current configuration.  The effect is to reset most USB-related
 * state in the device, including interface altsettings (reset to zero),
 * endpoint halts (cleared), and endpoint state (only for bulk and interrupt
 * endpoints).  Other usbcore state is unchanged, including bindings of
 * usb device drivers to interfaces.
 *
 * Because this affects multiple interfaces, avoid using this with composite
 * (multi-interface) devices.  Instead, the driver for each interface may
 * use usb_set_interface() on the interfaces it claims.  Be careful though;
 * some devices don't support the SET_INTERFACE request, and others won't
 * reset all the interface state (notably endpoint state).  Resetting the whole
 * configuration would affect other drivers' interfaces.
 *
 * The caller must own the device lock.
 *
 * Return: Zero on success, else a negative error code.
 *
 * If this routine fails the device will probably be in an unusable state
 * with endpoints disabled, and interfaces only partially enabled.
 */
int usb_reset_configuration(struct usb_device *dev)
{
	int			i, retval;
	struct usb_host_config	*config;
	struct usb_hcd *hcd = bus_to_hcd(dev->bus);

	if (dev->state == USB_STATE_SUSPENDED)
		return -EHOSTUNREACH;

	/* caller must have locked the device and must own
	 * the usb bus readlock (so driver bindings are stable);
	 * calls during probe() are fine
	 */

	usb_disable_device_endpoints(dev, 1); /* skip ep0*/

	config = dev->actconfig;
	retval = 0;
	mutex_lock(hcd->bandwidth_mutex);
	/* Disable LPM, and re-enable it once the configuration is reset, so
	 * that the xHCI driver can recalculate the U1/U2 timeouts.
	 */
	if (usb_disable_lpm(dev)) {
		dev_err(&dev->dev, "%s Failed to disable LPM\n", __func__);
		mutex_unlock(hcd->bandwidth_mutex);
		return -ENOMEM;
	}

	/* xHCI adds all endpoints in usb_hcd_alloc_bandwidth */
	retval = usb_hcd_alloc_bandwidth(dev, config, NULL, NULL);
	if (retval < 0) {
		usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		return retval;
	}
	retval = usb_control_msg_send(dev, 0, USB_REQ_SET_CONFIGURATION, 0,
				      config->desc.bConfigurationValue, 0,
				      NULL, 0, USB_CTRL_SET_TIMEOUT,
				      GFP_NOIO);
	if (retval) {
		usb_hcd_alloc_bandwidth(dev, NULL, NULL, NULL);
		usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		return retval;
	}
	mutex_unlock(hcd->bandwidth_mutex);

	/* re-init hc/hcd interface/endpoint state */
	for (i = 0; i < config->desc.bNumInterfaces; i++) {
		struct usb_interface *intf = config->interface[i];
		struct usb_host_interface *alt;

		alt = usb_altnum_to_altsetting(intf, 0);

		/* No altsetting 0?  We'll assume the first altsetting.
		 * We could use a GetInterface call, but if a device is
		 * so non-compliant that it doesn't have altsetting 0
		 * then I wouldn't trust its reply anyway.
		 */
		if (!alt)
			alt = &intf->altsetting[0];

		if (alt != intf->cur_altsetting) {
			remove_intf_ep_devs(intf);
			usb_remove_sysfs_intf_files(intf);
		}
		intf->cur_altsetting = alt;
		usb_enable_interface(dev, intf, true);
		if (device_is_registered(&intf->dev)) {
			usb_create_sysfs_intf_files(intf);
			create_intf_ep_devs(intf);
		}
	}
	/* Now that the interfaces are installed, re-enable LPM. */
	usb_unlocked_enable_lpm(dev);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_reset_configuration);

static void usb_release_interface(struct device *dev)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_interface_cache *intfc =
			altsetting_to_usb_interface_cache(intf->altsetting);

	kref_put(&intfc->ref, usb_release_interface_cache);
	usb_put_dev(interface_to_usbdev(intf));
	of_node_put(dev->of_node);
	kfree(intf);
}

/*
 * usb_deauthorize_interface - deauthorize an USB interface
 *
 * @intf: USB interface structure
 */
void usb_deauthorize_interface(struct usb_interface *intf)
{
	struct device *dev = &intf->dev;

	device_lock(dev->parent);

	if (intf->authorized) {
		device_lock(dev);
		intf->authorized = 0;
		device_unlock(dev);

		usb_forced_unbind_intf(intf);
	}

	device_unlock(dev->parent);
}

/*
 * usb_authorize_interface - authorize an USB interface
 *
 * @intf: USB interface structure
 */
void usb_authorize_interface(struct usb_interface *intf)
{
	struct device *dev = &intf->dev;

	if (!intf->authorized) {
		device_lock(dev);
		intf->authorized = 1; /* authorize interface */
		device_unlock(dev);
	}
}

static int usb_if_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct usb_device *usb_dev;
	const struct usb_interface *intf;
	const struct usb_host_interface *alt;

	intf = to_usb_interface(dev);
	usb_dev = interface_to_usbdev(intf);
	alt = intf->cur_altsetting;