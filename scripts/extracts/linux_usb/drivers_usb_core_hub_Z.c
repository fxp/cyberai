// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/hub.c
// Segment 26/41



		/*
		 * The get_resuming_ports() method returns a bitmap (origin 0)
		 * of ports which have started wakeup signaling but have not
		 * yet finished resuming.  During system resume we will
		 * resume all the enabled ports, regardless of any wakeup
		 * signals, which means the wakeup requests would be lost.
		 * To prevent this, report them to the PM core here.
		 */
		resuming_ports = hcd->driver->get_resuming_ports(hcd);
		for (i = 0; i < hdev->maxchild; ++i) {
			if (test_bit(i, &resuming_ports)) {
				udev = hub->ports[i]->child;
				if (udev)
					pm_wakeup_event(&udev->dev, 0);
			}
		}
	}
}

static int hub_resume(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	dev_dbg(&intf->dev, "%s\n", __func__);
	hub_activate(hub, HUB_RESUME);

	/*
	 * This should be called only for system resume, not runtime resume.
	 * We can't tell the difference here, so some wakeup requests will be
	 * reported at the wrong time or more than once.  This shouldn't
	 * matter much, so long as they do get reported.
	 */
	report_wakeup_requests(hub);
	return 0;
}

static int hub_reset_resume(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	dev_dbg(&intf->dev, "%s\n", __func__);
	hub_activate(hub, HUB_RESET_RESUME);
	return 0;
}

/**
 * usb_root_hub_lost_power - called by HCD if the root hub lost Vbus power
 * @rhdev: struct usb_device for the root hub
 *
 * The USB host controller driver calls this function when its root hub
 * is resumed and Vbus power has been interrupted or the controller
 * has been reset.  The routine marks @rhdev as having lost power.
 * When the hub driver is resumed it will take notice and carry out
 * power-session recovery for all the "USB-PERSIST"-enabled child devices;
 * the others will be disconnected.
 */
void usb_root_hub_lost_power(struct usb_device *rhdev)
{
	dev_notice(&rhdev->dev, "root hub lost power or was reset\n");
	rhdev->reset_resume = 1;
}
EXPORT_SYMBOL_GPL(usb_root_hub_lost_power);

static const char * const usb3_lpm_names[]  = {
	"U0",
	"U1",
	"U2",
	"U3",
};

/*
 * Send a Set SEL control transfer to the device, prior to enabling
 * device-initiated U1 or U2.  This lets the device know the exit latencies from
 * the time the device initiates a U1 or U2 exit, to the time it will receive a
 * packet from the host.
 *
 * This function will fail if the SEL or PEL values for udev are greater than
 * the maximum allowed values for the link state to be enabled.
 */
static int usb_req_set_sel(struct usb_device *udev)
{
	struct usb_set_sel_req *sel_values;
	unsigned long long u1_sel;
	unsigned long long u1_pel;
	unsigned long long u2_sel;
	unsigned long long u2_pel;
	int ret;

	if (!udev->parent || udev->speed < USB_SPEED_SUPER || !udev->lpm_capable)
		return 0;

	/* Convert SEL and PEL stored in ns to us */
	u1_sel = DIV_ROUND_UP(udev->u1_params.sel, 1000);
	u1_pel = DIV_ROUND_UP(udev->u1_params.pel, 1000);
	u2_sel = DIV_ROUND_UP(udev->u2_params.sel, 1000);
	u2_pel = DIV_ROUND_UP(udev->u2_params.pel, 1000);

	/*
	 * Make sure that the calculated SEL and PEL values for the link
	 * state we're enabling aren't bigger than the max SEL/PEL
	 * value that will fit in the SET SEL control transfer.
	 * Otherwise the device would get an incorrect idea of the exit
	 * latency for the link state, and could start a device-initiated
	 * U1/U2 when the exit latencies are too high.
	 */
	if (u1_sel > USB3_LPM_MAX_U1_SEL_PEL ||
	    u1_pel > USB3_LPM_MAX_U1_SEL_PEL ||
	    u2_sel > USB3_LPM_MAX_U2_SEL_PEL ||
	    u2_pel > USB3_LPM_MAX_U2_SEL_PEL) {
		dev_dbg(&udev->dev, "Device-initiated U1/U2 disabled due to long SEL or PEL\n");
		return -EINVAL;
	}

	/*
	 * usb_enable_lpm() can be called as part of a failed device reset,
	 * which may be initiated by an error path of a mass storage driver.
	 * Therefore, use GFP_NOIO.
	 */
	sel_values = kmalloc_obj(*(sel_values), GFP_NOIO);
	if (!sel_values)
		return -ENOMEM;

	sel_values->u1_sel = u1_sel;
	sel_values->u1_pel = u1_pel;
	sel_values->u2_sel = cpu_to_le16(u2_sel);
	sel_values->u2_pel = cpu_to_le16(u2_pel);

	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_SET_SEL,
			USB_RECIP_DEVICE,
			0, 0,
			sel_values, sizeof *(sel_values),
			USB_CTRL_SET_TIMEOUT);
	kfree(sel_values);

	if (ret > 0)
		udev->lpm_devinit_allow = 1;

	return ret;
}

/*
 * Enable or disable device-initiated U1 or U2 transitions.
 */
static int usb_set_device_initiated_lpm(struct usb_device *udev,
		enum usb3_link_state state, bool enable)
{
	int ret;
	int feature;

	switch (state) {
	case USB3_LPM_U1:
		feature = USB_DEVICE_U1_ENABLE;
		break;
	case USB3_LPM_U2:
		feature = USB_DEVICE_U2_ENABLE;
		break;
	default:
		dev_warn(&udev->dev, "%s: Can't %s non-U1 or U2 state.\n",
				__func__, str_enable_disable(enable));
		return -EINVAL;
	}