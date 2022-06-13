/* Bluetooth: Mesh Neo
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#include <zephyr.h>
#include "storage.h"

static uint8_t reset_counter;

static void save_on_flash(uint8_t id);

void storage_set_rc(uint8_t counter)
{
	reset_counter = counter;
	save_on_flash(0);
}

uint8_t storage_get_rc(void)
{
	return reset_counter;
}


static void storage_work_handler(struct k_work *work)
{
	settings_save_one("neo/rc", &reset_counter, sizeof(reset_counter));
}

K_WORK_DEFINE(storage_work, storage_work_handler);

static void save_on_flash(uint8_t id)
{
	k_work_submit(&storage_work);
}

static int storage_set(const char *key, size_t len_rd,
		  settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len = 0;
	int key_len;
	const char *next;

	key_len = settings_name_next(key, &next);

	if (!next) {
		if (!strncmp(key, "rc", key_len)) {
			len = read_cb(cb_arg, &reset_counter,
				      sizeof(reset_counter));
		}

		return (len < 0) ? len : 0;
	}

	return -ENOENT;
}

static struct settings_handler neo_settings = {
	.name = "neo",
	.h_set = storage_set,
};

int storage_init(void)
{
	int err;
	err = settings_subsys_init();
	if (err) {
		printk("settings_subsys_init failed (err %d)", err);
		return err;
	}
	err = settings_register(&neo_settings);
	if (err) {
		printk("neo_settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
