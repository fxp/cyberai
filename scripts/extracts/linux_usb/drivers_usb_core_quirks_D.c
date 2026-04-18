// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/quirks.c
// Segment 4/5



	/* SKYMEDI USB_DRIVE */
	{ USB_DEVICE(0x1516, 0x8628), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Razer - Razer Blade Keyboard */
	{ USB_DEVICE(0x1532, 0x0116), .driver_info =
			USB_QUIRK_LINEAR_UFRAME_INTR_BINTERVAL },
	/* Razer - Razer Kiyo Pro Webcam */
	{ USB_DEVICE(0x1532, 0x0e05), .driver_info = USB_QUIRK_NO_LPM },

	/* Lenovo ThinkPad OneLink+ Dock twin hub controllers (VIA Labs VL812) */
	{ USB_DEVICE(0x17ef, 0x1018), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x17ef, 0x1019), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Lenovo USB-C to Ethernet Adapter RTL8153-04 */
	{ USB_DEVICE(0x17ef, 0x720c), .driver_info = USB_QUIRK_NO_LPM },

	/* Lenovo Powered USB-C Travel Hub (4X90S92381, RTL8153 GigE) */
	{ USB_DEVICE(0x17ef, 0x721e), .driver_info = USB_QUIRK_NO_LPM },

	/* Lenovo ThinkCenter A630Z TI024Gen3 usb-audio */
	{ USB_DEVICE(0x17ef, 0xa012), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },

	/* Lenovo ThinkPad USB-C Dock Gen2 Ethernet (RTL8153 GigE) */
	{ USB_DEVICE(0x17ef, 0xa387), .driver_info = USB_QUIRK_NO_LPM },

	/* BUILDWIN Photo Frame */
	{ USB_DEVICE(0x1908, 0x1315), .driver_info =
			USB_QUIRK_HONOR_BNUMINTERFACES },

	/* Protocol and OTG Electrical Test Device */
	{ USB_DEVICE(0x1a0a, 0x0200), .driver_info =
			USB_QUIRK_LINEAR_UFRAME_INTR_BINTERVAL },

	/* Terminus Technology Inc. Hub */
	{ USB_DEVICE(0x1a40, 0x0101), .driver_info = USB_QUIRK_HUB_SLOW_RESET },

	/* Corsair K70 RGB */
	{ USB_DEVICE(0x1b1c, 0x1b13), .driver_info = USB_QUIRK_DELAY_INIT |
	  USB_QUIRK_DELAY_CTRL_MSG },

	/* Corsair Strafe */
	{ USB_DEVICE(0x1b1c, 0x1b15), .driver_info = USB_QUIRK_DELAY_INIT |
	  USB_QUIRK_DELAY_CTRL_MSG },

	/* Corsair Strafe RGB */
	{ USB_DEVICE(0x1b1c, 0x1b20), .driver_info = USB_QUIRK_DELAY_INIT |
	  USB_QUIRK_DELAY_CTRL_MSG },

	/* Corsair K70 LUX RGB */
	{ USB_DEVICE(0x1b1c, 0x1b33), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Corsair K70 LUX */
	{ USB_DEVICE(0x1b1c, 0x1b36), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Corsair K70 RGB RAPDIFIRE */
	{ USB_DEVICE(0x1b1c, 0x1b38), .driver_info = USB_QUIRK_DELAY_INIT |
	  USB_QUIRK_DELAY_CTRL_MSG },

	/* START BP-850k Printer */
	{ USB_DEVICE(0x1bc3, 0x0003), .driver_info = USB_QUIRK_NO_SET_INTF },

	/* MIDI keyboard WORLDE MINI */
	{ USB_DEVICE(0x1c75, 0x0204), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Acer C120 LED Projector */
	{ USB_DEVICE(0x1de1, 0xc102), .driver_info = USB_QUIRK_NO_LPM },

	/* Blackmagic Design Intensity Shuttle */
	{ USB_DEVICE(0x1edb, 0xbd3b), .driver_info = USB_QUIRK_NO_LPM },

	/* Blackmagic Design UltraStudio SDI */
	{ USB_DEVICE(0x1edb, 0xbd4f), .driver_info = USB_QUIRK_NO_LPM },

	/* Teclast disk */
	{ USB_DEVICE(0x1f75, 0x0917), .driver_info = USB_QUIRK_NO_LPM },

	/* Hauppauge HVR-950q */
	{ USB_DEVICE(0x2040, 0x7200), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* VLI disk */
	{ USB_DEVICE(0x2109, 0x0711), .driver_info = USB_QUIRK_NO_LPM },

	/* Raydium Touchscreen */
	{ USB_DEVICE(0x2386, 0x3114), .driver_info = USB_QUIRK_NO_LPM },

	{ USB_DEVICE(0x2386, 0x3119), .driver_info = USB_QUIRK_NO_LPM },

	{ USB_DEVICE(0x2386, 0x350e), .driver_info = USB_QUIRK_NO_LPM },

	/* UGREEN 35871 - BOS descriptor fetch hangs at SuperSpeed Plus */
	{ USB_DEVICE(0x2b89, 0x5871), .driver_info = USB_QUIRK_NO_BOS },

	/* APTIV AUTOMOTIVE HUB */
	{ USB_DEVICE(0x2c48, 0x0132), .driver_info =
			USB_QUIRK_SHORT_SET_ADDRESS_REQ_TIMEOUT },

	/* DJI CineSSD */
	{ USB_DEVICE(0x2ca3, 0x0031), .driver_info = USB_QUIRK_NO_LPM },

	/* Alcor Link AK9563 SC Reader used in 2022 Lenovo ThinkPads */
	{ USB_DEVICE(0x2ce3, 0x9563), .driver_info = USB_QUIRK_NO_LPM },

	/* ezcap401 - BOS descriptor fetch hangs at SuperSpeed Plus */
	{ USB_DEVICE(0x32ed, 0x0401), .driver_info = USB_QUIRK_NO_BOS },

	/* DELL USB GEN2 */
	{ USB_DEVICE(0x413c, 0xb062), .driver_info = USB_QUIRK_NO_LPM | USB_QUIRK_RESET_RESUME },

	/* VCOM device */
	{ USB_DEVICE(0x4296, 0x7570), .driver_info = USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Noji-MCS SmartCard Reader */
	{ USB_DEVICE(0x5131, 0x2007), .driver_info = USB_QUIRK_FORCE_ONE_CONFIG },

	/* INTEL VALUE SSD */
	{ USB_DEVICE(0x8086, 0xf1a5), .driver_info = USB_QUIRK_RESET_RESUME },

	{ }  /* terminating entry must be last */
};

static const struct usb_device_id usb_interface_quirk_list[] = {
	/* Logitech UVC Cameras */
	{ USB_VENDOR_AND_INTERFACE_INFO(0x046d, USB_CLASS_VIDEO, 1, 0),
	  .driver_info = USB_QUIRK_RESET_RESUME },

	{ }  /* terminating entry must be last */
};

static const struct usb_device_id usb_amd_resume_quirk_list[] = {
	/* Lenovo Mouse with Pixart controller */
	{ USB_DEVICE(0x17ef, 0x602e), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Pixart Mouse */
	{ USB_DEVICE(0x093a, 0x2500), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x093a, 0x2510), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x093a, 0x2521), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x03f0, 0x2b4a), .driver_info = USB_QUIRK_RESET_RESUME },