// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/usb/core/quirks.c
// Segment 3/5



	/* Guillemot Webcam Hercules Dualpix Exchange (2nd ID) */
	{ USB_DEVICE(0x06f8, 0x0804), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Guillemot Webcam Hercules Dualpix Exchange*/
	{ USB_DEVICE(0x06f8, 0x3005), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Guillemot Hercules DJ Console audio card (BZ 208357) */
	{ USB_DEVICE(0x06f8, 0xb000), .driver_info =
			USB_QUIRK_ENDPOINT_IGNORE },

	/* Midiman M-Audio Keystation 88es */
	{ USB_DEVICE(0x0763, 0x0192), .driver_info = USB_QUIRK_RESET_RESUME },

	/* SanDisk Ultra Fit and Ultra Flair */
	{ USB_DEVICE(0x0781, 0x5583), .driver_info = USB_QUIRK_NO_LPM },
	{ USB_DEVICE(0x0781, 0x5591), .driver_info = USB_QUIRK_NO_LPM },

	/* SanDisk Corp. SanDisk 3.2Gen1 */
	{ USB_DEVICE(0x0781, 0x5596), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x0781, 0x55a3), .driver_info = USB_QUIRK_DELAY_INIT },

	/* SanDisk Extreme 55AE */
	{ USB_DEVICE(0x0781, 0x55ae), .driver_info = USB_QUIRK_NO_LPM },

	/* Avermedia Live Gamer Ultra 2.1 (GC553G2) - BOS descriptor fetch hangs at SuperSpeed Plus */
	{ USB_DEVICE(0x07ca, 0x2553), .driver_info = USB_QUIRK_NO_BOS },

	/* Realforce 87U Keyboard */
	{ USB_DEVICE(0x0853, 0x011b), .driver_info = USB_QUIRK_NO_LPM },

	/* M-Systems Flash Disk Pioneers */
	{ USB_DEVICE(0x08ec, 0x1000), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Baum Vario Ultra */
	{ USB_DEVICE(0x0904, 0x6101), .driver_info =
			USB_QUIRK_LINEAR_FRAME_INTR_BINTERVAL },
	{ USB_DEVICE(0x0904, 0x6102), .driver_info =
			USB_QUIRK_LINEAR_FRAME_INTR_BINTERVAL },
	{ USB_DEVICE(0x0904, 0x6103), .driver_info =
			USB_QUIRK_LINEAR_FRAME_INTR_BINTERVAL },

	/* Silicon Motion Flash Drive */
	{ USB_DEVICE(0x090c, 0x1000), .driver_info = USB_QUIRK_DELAY_INIT },
	{ USB_DEVICE(0x090c, 0x2000), .driver_info = USB_QUIRK_DELAY_INIT },

	/* Sound Devices USBPre2 */
	{ USB_DEVICE(0x0926, 0x0202), .driver_info =
			USB_QUIRK_ENDPOINT_IGNORE },

	/* Sound Devices MixPre-D */
	{ USB_DEVICE(0x0926, 0x0208), .driver_info =
			USB_QUIRK_ENDPOINT_IGNORE },

	/* Keytouch QWERTY Panel keyboard */
	{ USB_DEVICE(0x0926, 0x3333), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Kingston DataTraveler 3.0 */
	{ USB_DEVICE(0x0951, 0x1666), .driver_info = USB_QUIRK_NO_LPM },

	/* TOSHIBA TransMemory-Mx */
	{ USB_DEVICE(0x0930, 0x1408), .driver_info = USB_QUIRK_NO_LPM },

	/* NVIDIA Jetson devices in Force Recovery mode */
	{ USB_DEVICE(0x0955, 0x7018), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7019), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7418), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7721), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7c18), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7e19), .driver_info = USB_QUIRK_RESET_RESUME },
	{ USB_DEVICE(0x0955, 0x7f21), .driver_info = USB_QUIRK_RESET_RESUME },

	/* X-Rite/Gretag-Macbeth Eye-One Pro display colorimeter */
	{ USB_DEVICE(0x0971, 0x2000), .driver_info = USB_QUIRK_NO_SET_INTF },

	/* ELMO L-12F document camera */
	{ USB_DEVICE(0x09a1, 0x0028), .driver_info = USB_QUIRK_DELAY_CTRL_MSG },

	/* Broadcom BCM92035DGROM BT dongle */
	{ USB_DEVICE(0x0a5c, 0x2021), .driver_info = USB_QUIRK_RESET_RESUME },

	/* MAYA44USB sound device */
	{ USB_DEVICE(0x0a92, 0x0091), .driver_info = USB_QUIRK_RESET_RESUME },

	/* ASUS Base Station(T100) */
	{ USB_DEVICE(0x0b05, 0x17e0), .driver_info =
			USB_QUIRK_IGNORE_REMOTE_WAKEUP },

	/* ASUS TUF 4K PRO - BOS descriptor fetch hangs at SuperSpeed Plus */
	{ USB_DEVICE(0x0b05, 0x1ab9), .driver_info = USB_QUIRK_NO_BOS },

	/* Realtek Semiconductor Corp. Mass Storage Device (Multicard Reader)*/
	{ USB_DEVICE(0x0bda, 0x0151), .driver_info = USB_QUIRK_CONFIG_INTF_STRINGS },

	/* Realtek hub in Dell WD19 (Type-C) */
	{ USB_DEVICE(0x0bda, 0x0487), .driver_info = USB_QUIRK_NO_LPM },

	/* Generic RTL8153 based ethernet adapters */
	{ USB_DEVICE(0x0bda, 0x8153), .driver_info = USB_QUIRK_NO_LPM },

	/* SONiX USB DEVICE Touchpad */
	{ USB_DEVICE(0x0c45, 0x7056), .driver_info =
			USB_QUIRK_IGNORE_REMOTE_WAKEUP },

	/* Elgato 4K X - BOS descriptor fetch hangs at SuperSpeed Plus */
	{ USB_DEVICE(0x0fd9, 0x009b), .driver_info = USB_QUIRK_NO_BOS },

	/* Sony Xperia XZ1 Compact (lilac) smartphone in fastboot mode */
	{ USB_DEVICE(0x0fce, 0x0dde), .driver_info = USB_QUIRK_NO_LPM },

	/* Action Semiconductor flash disk */
	{ USB_DEVICE(0x10d6, 0x2200), .driver_info =
			USB_QUIRK_STRING_FETCH_255 },

	/* novation SoundControl XL */
	{ USB_DEVICE(0x1235, 0x0061), .driver_info = USB_QUIRK_RESET_RESUME },

	/* Focusrite Scarlett Solo USB */
	{ USB_DEVICE(0x1235, 0x8211), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },

	/* Huawei 4G LTE module */
	{ USB_DEVICE(0x12d1, 0x15bb), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },
	{ USB_DEVICE(0x12d1, 0x15c1), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },
	{ USB_DEVICE(0x12d1, 0x15c3), .driver_info =
			USB_QUIRK_DISCONNECT_SUSPEND },