/* user_led.c - handle user interface - LEDs */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <sys/printk.h>

#include "board.h"
#include "user_led.h"

struct led_s
{
	const enum user_led_type_e	type;
	const uint8_t 				pins[3];
};

#if defined(CONFIG_BOARD_WALTHERNEOV2)
const struct led_s leds[USER_LED_NUM_OF] = 
{
	[USER_LED_L1] = { USER_LED_TYPE_RGB, {EXT_LED_L1_R_GPIO_PIN, EXT_LED_L1_G_GPIO_PIN, EXT_LED_L1_B_GPIO_PIN } },
	[USER_LED_L2] = { USER_LED_TYPE_RGB, {EXT_LED_L2_R_GPIO_PIN, EXT_LED_L2_G_GPIO_PIN, EXT_LED_L2_B_GPIO_PIN } },
	[USER_LED_L3] = { USER_LED_TYPE_RGB, {EXT_LED_L3_R_GPIO_PIN, EXT_LED_L3_G_GPIO_PIN, EXT_LED_L3_B_GPIO_PIN } },
	[USER_LED_N] = { USER_LED_TYPE_RGB, {EXT_LED_N_R_GPIO_PIN, EXT_LED_N_G_GPIO_PIN, EXT_LED_N_B_GPIO_PIN } },
	[USER_LED_PE] = { USER_LED_TYPE_RGB, {EXT_LED_PE_R_GPIO_PIN, EXT_LED_PE_G_GPIO_PIN, EXT_LED_PE_B_GPIO_PIN } },
	[USER_LED_TEMP] = { USER_LED_TYPE_RGB, {EXT_LED_TEMP_R_GPIO_PIN, EXT_LED_TEMP_G_GPIO_PIN, EXT_LED_TEMP_B_GPIO_PIN } },
};
#elif defined(CONFIG_BOARD_WALTHERNEOV3) || defined(CONFIG_BOARD_WALTHERNEOV4)
const struct led_s leds[USER_LED_NUM_OF] = 
{
	[USER_LED_L1] = { USER_LED_TYPE_RW, {EXT_LED_L1_R_GPIO_PIN, EXT_LED_L1_W_GPIO_PIN, 0xFF } },
	[USER_LED_L2] = { USER_LED_TYPE_RW, {EXT_LED_L2_R_GPIO_PIN, EXT_LED_L2_W_GPIO_PIN, 0xFF } },
	[USER_LED_L3] = { USER_LED_TYPE_RW, {EXT_LED_L3_R_GPIO_PIN, EXT_LED_L3_W_GPIO_PIN, 0xFF } },
	[USER_LED_N] = { USER_LED_TYPE_RW, {EXT_LED_N_R_GPIO_PIN, EXT_LED_N_W_GPIO_PIN, 0xFF } },
	[USER_LED_PE] = { USER_LED_TYPE_RGB, {EXT_LED_PE_R_GPIO_PIN, EXT_LED_PE_G_GPIO_PIN, EXT_LED_PE_B_GPIO_PIN } },
	[USER_LED_TEMP] = { USER_LED_TYPE_RGB, {EXT_LED_TEMP_R_GPIO_PIN, EXT_LED_TEMP_G_GPIO_PIN, EXT_LED_TEMP_B_GPIO_PIN } },
};
#endif

struct device *dev = NULL;

static enum user_led_states_e state_private 	= USER_LED_STATE_LEDTEST;
static uint16_t lost_bm_private 		= 0;

static int num_of_leds = 0;

static int heartbeat_blinks = 0;

static void user_led_loop(struct k_work *work);

K_WORK_DEFINE(led_worker, user_led_loop);

void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&led_worker);
}

K_TIMER_DEFINE(led_timer, my_timer_handler, NULL);

void user_led_set_color(enum user_led_e led, enum user_led_color_e color)
{
	switch(color)
	{
		case USER_LED_COLOR_NONE:
		{
			gpio_pin_set(dev, leds[led].pins[0], 0);
			gpio_pin_set(dev, leds[led].pins[1], 0);
			if(leds[led].type == USER_LED_TYPE_RGB)
				gpio_pin_set(dev, leds[led].pins[2], 0);
		}
		break;
		case USER_LED_COLOR_RED:
		{
			gpio_pin_set(dev, leds[led].pins[0], 1);
			gpio_pin_set(dev, leds[led].pins[1], 0);
			if(leds[led].type == USER_LED_TYPE_RGB)
				gpio_pin_set(dev, leds[led].pins[2], 0);
		}
		break;
		case USER_LED_COLOR_GREEN:
		{
			gpio_pin_set(dev, leds[led].pins[0], 0);
			gpio_pin_set(dev, leds[led].pins[1], 1);
			if(leds[led].type == USER_LED_TYPE_RGB)
				gpio_pin_set(dev, leds[led].pins[2], 0);
		}
		break;
		case USER_LED_COLOR_BLUE:
		{
			gpio_pin_set(dev, leds[led].pins[0], 0);
			gpio_pin_set(dev, leds[led].pins[1], 0);
			if(leds[led].type == USER_LED_TYPE_RGB)
				gpio_pin_set(dev, leds[led].pins[2], 1);
		}
		break;
		case USER_LED_COLOR_WHITE:
		{
			if(leds[led].type == USER_LED_TYPE_RGB)
			{
				gpio_pin_set(dev, leds[led].pins[0], 1);
				gpio_pin_set(dev, leds[led].pins[1], 1);
				gpio_pin_set(dev, leds[led].pins[2], 1);
			}
			else
			{
				gpio_pin_set(dev, leds[led].pins[0], 0);
				gpio_pin_set(dev, leds[led].pins[1], 1);
			}
		}
		break;
	}
}

void user_led_start_heartbeatblink(int blinknum)
{
	heartbeat_blinks = blinknum;
}

void user_led_set_state(enum user_led_states_e state, uint16_t lost_bitmask)
{
	//don't delete attention state if not end
	if(state_private == USER_LED_STATE_ATTENTION && state != USER_LED_STATE_ALLOFF)
		return;
		
	//printk("New state: %d\n", state);
		
    state_private = state;
    lost_bm_private = lost_bitmask;
}

void user_led_loop(struct k_work *work)
{
	static uint32_t                 stop_time = 0;
	static uint8_t					inital_time = 0;
	static uint32_t                 timer = 0;
	static uint8_t                  state_switcher = 0;
	static char                     old_rot_dir = 0;
	static enum user_led_states_e   old_state = USER_LED_STATE_BOOT;

	//critical section enter
	//int key = irq_lock(); //sometimes we stuck here ???
	volatile enum user_led_states_e state = state_private;
	volatile uint16_t lost_bitmask = lost_bm_private;
	//critical section exit
	//irq_unlock(key);

	char rot_dir = 0;

	timer++;

	if(state != old_state)
	{
		for(int i = 0; i < USER_LED_NUM_OF; i++)
		{
			user_led_set_color(i, USER_LED_COLOR_NONE);
		}

		old_state = state;
	}

	switch(state)
	{
		case USER_LED_STATE_ALLON:
		{
			for(int i = 0; i < USER_LED_NUM_OF; i++)
			{
				user_led_set_color(i, USER_LED_COLOR_WHITE);
			}
		}
		break;
		case USER_LED_STATE_ALLOFF:
		{
			for(int i = 0; i < USER_LED_NUM_OF; i++)
			{
				user_led_set_color(i, USER_LED_COLOR_NONE);
			}
		}
		break;
		case USER_LED_STATE_ATTENTION:
		{
			///blink all leds
			if(state_switcher)
			{
				for(int i = 0; i < USER_LED_NUM_OF; i++)
				{
					user_led_set_color(i, USER_LED_COLOR_WHITE);
				}
				state_switcher = 0;
			}
			else
			{
				for(int i = 0; i < USER_LED_NUM_OF; i++)
				{
					user_led_set_color(i, USER_LED_COLOR_NONE);
				}
				state_switcher = 1;
			}
		}
		break;
		case USER_LED_STATE_LOST:
		{
			old_rot_dir = 0;
			//each second
			if( (timer - stop_time) < 10)
				break;

			stop_time = timer;

			if(state_switcher)
			{
				///blink red
				for(int i = 0; i < USER_LED_NUM_OF; i++)
				{
					if(i != USER_LED_TEMP || heartbeat_blinks == 0)
					{
						if( lost_bitmask & ((1UL << i) | (1UL << (i + 8))) )
							user_led_set_color(i, USER_LED_COLOR_RED);
						else
							user_led_set_color(i, USER_LED_COLOR_NONE);
					}
				}

				//toggle
				state_switcher = 0;
			}
			else
			{
				//blink white
				for(int i = 0; i < USER_LED_TEMP; i++)
				{
					if(lost_bitmask & (1UL << i))
						user_led_set_color(i, USER_LED_COLOR_NONE);
					else
						user_led_set_color(i, USER_LED_COLOR_WHITE);
				}
				
				if(heartbeat_blinks == 0)
				{
					//green for temperature led
					if(lost_bitmask & (1UL << USER_LED_TEMP))
						user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_NONE);
					else
						user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_GREEN);
				}

				//toggle
				state_switcher = 1;
			}
		}
		break;
		case USER_LED_STATE_ROT_R:
		{
			rot_dir = 'R';

			if(old_rot_dir != rot_dir)
			{
				state_switcher = 0;
				old_rot_dir = rot_dir;
			}
		}
		break;
		case USER_LED_STATE_ROT_L:
		{
			rot_dir = 'L';

			//set new state
			if(old_rot_dir != rot_dir)
			{
				state_switcher = 0;
				old_rot_dir = rot_dir;
			}
		}
		break;
		case USER_LED_STATE_LEDTEST:
		{
			if(state_switcher >= num_of_leds * 2)
			{
				state_private = USER_LED_STATE_BOOT;
				state_switcher = 0;
				break;
			}
			
			int led_index = state_switcher / 2;
			int cnt = 0;
			
			for(int i = 0; i < USER_LED_NUM_OF; i++)
			{
				for(int pin_index = 0; pin_index < 3; pin_index++)
				{
					if(leds[i].pins[pin_index] != 0xFF)
					{
						if(cnt == led_index)
						{
							if(state_switcher % 2 == 0)
								gpio_pin_set(dev, leds[i].pins[pin_index], 1);
							else
								gpio_pin_set(dev, leds[i].pins[pin_index], 0);
						}
						cnt++;
					}
				}
			}
			
			state_switcher++;
		}
		break;
		case USER_LED_STATE_BOOT:
		{
			//nothing to do
			state_switcher = 0;
		}
		break;
	}

	if(old_rot_dir == 'R' || old_rot_dir == 'L')
	{
		//each 300 ms
		if( (timer - stop_time) < 3)
			return;
		stop_time = timer;

		int led_index = (old_rot_dir == 'R') ? state_switcher : (2 - state_switcher);

		for(int i = 0; i < 3; i++)
		{
			if(i != led_index)
				user_led_set_color(i, USER_LED_COLOR_NONE);
			else
				user_led_set_color(i, USER_LED_COLOR_WHITE);
		}

		//set N, PE, Temp
		user_led_set_color(USER_LED_PE, USER_LED_COLOR_WHITE);
		if(heartbeat_blinks == 0)
			user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_GREEN);
		user_led_set_color(USER_LED_N, USER_LED_COLOR_WHITE);

		state_switcher++;
		if(state_switcher >= 4)
			state_switcher = 0;
	}

	if(heartbeat_blinks > 0)
	{
		//avoid first very short time step
		if(!inital_time)
		{
			user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_NONE);
			inital_time = 1;
		}
		else
		{
			if(timer % 2 == 0)
			{
				user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_BLUE);
			}
			else
			{
				user_led_set_color(USER_LED_TEMP, USER_LED_COLOR_NONE);
				heartbeat_blinks--;
			}
		}

		if(heartbeat_blinks == 0)
			inital_time = 0;
	}
}

void user_led_init(void)
{
	dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (dev == NULL) {
		printk("GPIO device not found\r\n");
		return;
	}
	
	num_of_leds = 0;

	for(int i = 0; i < USER_LED_NUM_OF; i++)
	{
		for(int pin_index = 0; pin_index < 3; pin_index++)
		{
			if(leds[i].pins[pin_index] == 0xFF)
				continue;
				
			num_of_leds++;
				
			int ret = gpio_pin_configure(dev, leds[i].pins[pin_index], GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW);
			if (ret < 0) {
				printk("GPIO config failed\r\n");
				return;
			}
			
			gpio_pin_set(dev, leds[i].pins[pin_index], 0);
		}

		//gpio_pin_set(dev, user_led_pins[i], 1);
		//k_msleep(100);
		//gpio_pin_set(dev, user_led_pins[i], 0);
	}

	/* start periodic timer that expires once every 100 ms */
	k_timer_start(&led_timer, K_MSEC(100), K_MSEC(100));
}
