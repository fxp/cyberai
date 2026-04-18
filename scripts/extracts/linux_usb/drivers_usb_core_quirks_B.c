// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/quirks.c
// Segment 2/5



	/* USB3503 */
	{ USB_DEVICE(0x0424, 0x3503), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Microsoft Wireless Laser Mouse 6000 Receiver */
	{ USB_DEVICE(0x045e, 0x00e1), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Microsoft LifeCam-VX700 v2.0 */
	{ USB_DEVICE(0x045e, 0x0770), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Microsoft Surface Dock Ethernet (RTL8153 GigE) */
	{ USB_DEVICE(0x045e, 0x07c6), .driver_info = USB_QUIRK_NO_LPM },

	/* Cherry Stream G230 2.0 (G85-231) and 3.0 (G85-232) */
	{ USB_DEVICE(0x046a, 0x0023), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech HD Webcam C270 */
	{ USB_DEVICE(0x046d, 0x0825), .driver_info = USB_QUIRK_RESET_RESUME |
		USB_QUIRK_NO_LPM},

	/* Logitech HD Pro Webcams C920, C920-C, C922, C925e and C930e */
	{ USB_DEVICE(0x046d, 0x082d), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x046d, 0x0841), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x046d, 0x0843), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x046d, 0x085b), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x046d, 0x085c), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Logitech ConferenceCam CC3000e */
	{ USB_DEVICE(0x046d, 0x0847), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x046d, 0x0848), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Logitech PTZ Pro Camera */
	{ USB_DEVICE(0x046d, 0x0853), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Logitech Screen Share */
	{ USB_DEVICE(0x046d, 0x086c), .driver_info = USB_QUIRK_NO_LPM },

	/* Logitech Quickcam Fusion */
	{ USB_DEVICE(0x046d, 0x08c1), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Quickcam Orbit MP */
	{ USB_DEVICE(0x046d, 0x08c2), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Quickcam Pro for Notebook */
	{ USB_DEVICE(0x046d, 0x08c3), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Quickcam Pro 5000 */
	{ USB_DEVICE(0x046d, 0x08c5), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Quickcam OEM Dell Notebook */
	{ USB_DEVICE(0x046d, 0x08c6), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Quickcam OEM Cisco VT Camera II */
	{ USB_DEVICE(0x046d, 0x08c7), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Logitech Harmony 700-series */
	{ USB_DEVICE(0x046d, 0xc122), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Philips PSC805 audio device */
	{ USB_DEVICE(0x0471, 0x0155), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Plantronic Audio 655 DSP */
	{ USB_DEVICE(0x047f, 0xc008), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Plantronic Audio 648 USB */
	{ USB_DEVICE(0x047f, 0xc013), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Artisman Watchdog Dongle */
	{ USB_DEVICE(0x04b4, 0x0526), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Microchip Joss Optical infrared touchboard device */
	{ USB_DEVICE(0x04d8, 0x000c), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* CarrolTouch 4000U */
	{ USB_DEVICE(0x04e7, 0x0009), .driver_info = USB_QUIRK_RESET_RESUME },

	/* CarrolTouch 4500U */
	{ USB_DEVICE(0x04e7, 0x0030), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Samsung Android phone modem - ID conflict with SPH-I500 */
	{ USB_DEVICE(0x04e8, 0x6601), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Elan Touchscreen */
	{ USB_DEVICE(0x04f3, 0x0089), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	{ USB_DEVICE(0x04f3, 0x009b), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	{ USB_DEVICE(0x04f3, 0x010c), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	{ USB_DEVICE(0x04f3, 0x0125), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	{ USB_DEVICE(0x04f3, 0x016f), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	{ USB_DEVICE(0x04f3, 0x0381), .driver_info =
			USB_QUIRK_NO_LPM },

	{ USB_DEVICE(0x04f3, 0x21b8), .driver_info =
			USB_QUIRK_DEVICE_QUALIFIER },

	/* Roland SC-8820 */
	{ USB_DEVICE(0x0582, 0x0007), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Edirol SD-20 */
	{ USB_DEVICE(0x0582, 0x0027), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Alcor Micro Corp. Hub */
	{ USB_DEVICE(0x058f, 0x9254), .driver_info = USB_QUIRK_RESET_RESUME },

	/* appletouch */
	{ USB_DEVICE(0x05ac, 0x021a), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Genesys Logic hub, internally used by KY-688 USB 3.1 Type-C Hub */
	{ USB_DEVICE(0x05e3, 0x0612), .driver_info = USB_QUIRK_NO_LPM },

	/* ELSA MicroLink 56K */
	{ USB_DEVICE(0x05cc, 0x2267), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Genesys Logic hub, internally used by Moshi USB to Ethernet Adapter */
	{ USB_DEVICE(0x05e3, 0x0616), .driver_info = USB_QUIRK_NO_LPM },

	/* Avision AV600U */
	{ USB_DEVICE(0x0638, 0x0a13), .driver_info =
	  USB_QUIRK_STRING_FETCH_255 },

	/* Prolific Single-LUN Mass Storage Card Reader */
	{ USB_DEVICE(0x067b, 0x2731), .driver_info = USB_QUIRK_DELAY_INIT |
	  USB_QUIRK_NO_LPM },

	/* Saitek Cyborg Gold Joystick */
	{ USB_DEVICE(0x06a3, 0x0006), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Agfa SNAPSCAN 1212U */
	{ USB_DEVICE(0x06bd, 0x0001), .driver_info = USB_QUIRK_RESET_RESUME },