// Source: /Users/xiaopingfeng/Projects/cyberai/targets/linux-6.12-sparse/drivers/hid/hid-input.c
// Segment 16/16



	list_for_each_entry_safe(hidinput, next, &hid->inputs, list) {
		list_del(&hidinput->list);
		if (hidinput->registered)
			input_unregister_device(hidinput->input);
		else
			input_free_device(hidinput->input);
		kfree(hidinput->name);
		kfree(hidinput);
	}

	/* led_work is spawned by input_dev callbacks, but doesn't access the
	 * parent input_dev at all. Once all input devices are removed, we
	 * know that led_work will never get restarted, so we can cancel it
	 * synchronously and are safe. */
	cancel_work_sync(&hid->led_work);
}
EXPORT_SYMBOL_GPL(hidinput_disconnect);

void hidinput_reset_resume(struct hid_device *hid)
{
	/* renegotiate host-device shared state after reset */
	hidinput_change_resolution_multipliers(hid);
}
EXPORT_SYMBOL_GPL(hidinput_reset_resume);

#ifdef CONFIG_HID_KUNIT_TEST
#include "hid-input-test.c"
#endif
