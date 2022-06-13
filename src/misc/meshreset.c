/* Bluetooth: IPDNeo
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include "storage.h"
#include "meshreset.h"
#include "../user_led.h"

static void meshreset_timer_handler(struct k_timer *dummy);

K_TIMER_DEFINE(reset_counter_timer, meshreset_timer_handler, NULL);

void meshreset_boothook(void)
{
	uint8_t reset_counter = storage_get_rc();
	if (reset_counter >= 4U) {
		reset_counter = 0U;
		printk("BT Mesh reset\n");

		user_led_set_state(USER_LED_STATE_ATTENTION, 0);
		k_msleep(4000);
		bt_mesh_reset();
		k_msleep(4000);
		//disable to release attention state
		user_led_set_state(USER_LED_STATE_ALLOFF, 0);
		
		//set inital LED state
		user_led_set_state(USER_LED_STATE_LEDTEST, 0);
	} else {
		printk("Reset Counter -> %d\n", reset_counter);
		reset_counter++;
	}

	storage_set_rc(reset_counter);

	k_timer_start(&reset_counter_timer, K_MSEC(MESHRESET_BOOTTIMEOUT_MS), K_NO_WAIT);
}

static void meshreset_timer_handler(struct k_timer *dummy)
{
	storage_set_rc(0);
	printk("Reset Counter set to Zero\n");
}


