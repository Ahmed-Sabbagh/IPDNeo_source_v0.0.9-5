/* Bluetooth: Mesh Sensor Model
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#include <bluetooth/uuid.h>
#include "ble_mesh.h"
#include "device_composition.h"

#ifdef OOB_AUTH_ENABLE

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	printk("OOB Number: %u\n", number);
	return 0;
}

static int output_string(const char *str)
{
	printk("OOB String: %s\n", str);
	return 0;
}

#endif

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

// nRFgo Studio generated UUID
// Random generated UUID -> 9A60xxxx-A7D6-4FA1-8838-A57FC7BAEFC5
//                                                  ^^^^^^^^^^^^
//                                                  will be overwritten by internal module UUID (random mac)
static uint8_t dev_uuid[16] = { BT_UUID_128_ENCODE(0x9A600000, 0xA7D6, 0x4FA1, 0x8838, 0xA57FC7BAEFC5) };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,

#ifdef OOB_AUTH_ENABLE

	.output_size = 6,
	.output_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING,
	.output_number = output_number,
	.output_string = output_string,

#endif

	.complete = prov_complete,
	.reset = prov_reset,
};

void bt_ready(int err)
{
	struct bt_le_oob oob;

	if(err == 0)
		printk("Bluetooth initialized\n");
	else
		return;

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* Use identity address as device UUID */
	if (bt_le_oob_get_local(BT_ID_DEFAULT, &oob)) {
		printk("Identity Address unavailable\n");
	} else {
		memcpy(dev_uuid, oob.addr.a.val, 6);
	}

	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

	printk("Mesh initialized\n");
}
