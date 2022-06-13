/* main.c - IPDNeo Application */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#include <sys/printk.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/mesh.h>

#include "mesh/ble_mesh.h"

#include "board.h"

#include "sensor/adc.h"
#include "sensor/temp.h"
#include "misc/storage.h"
#include "misc/meshreset.h"
#include "user_led.h"
#include "phase_calc.h"
#include "mesh/device_composition.h"

#include <nrfx_rtc.h>
#include <nrfx_timer.h>

#if defined(CONFIG_MCUMGR)
#include <mgmt/smp_bt.h>
#include "misc/smp_svr.h"
#endif


#define FILTER_CYCLES		14UL
#define FILTER_THRESHOLD	(FILTER_CYCLES / 2)


void display_status(float temp_value_f)
{
    static uint8_t temp_critical_reached = 0;
    
    static uint16_t bitmask_filtered = 0;
    static uint16_t bitmaskfilter[FILTER_CYCLES] = { 0 };
    static uint8_t	bitmaskcnt[16] = { 0 };
    static int bitmaskfilter_index = 0;
    
    static int first_time_shot = 1; //boot without bitfilter delay

    int8_t temp_value = (int8_t)(temp_value_f + 0.5f);
    uint32_t phase_voltage[4] = //{ 0 };
                                { 
                                (uint32_t)phase_calculator_get_value(0),
                                (uint32_t)phase_calculator_get_value(1),
                                (uint32_t)phase_calculator_get_value(2),
                                (uint32_t)phase_calculator_get_value(3),
                                };


    uint16_t bitmask = (phase_present[0] < 10) ? USER_LED_LOST_L1_BIT : USER_LED_LOST_NONE;
    bitmask |= (phase_present[1] < 10) ? USER_LED_LOST_L2_BIT : USER_LED_LOST_NONE;
    bitmask |= (phase_present[2] < 10) ? USER_LED_LOST_L3_BIT : USER_LED_LOST_NONE;

    bitmask |= (phase_present[0] < 10 && phase_present[0] > 0) ? USER_LED_UNKNOWN_L1_BIT : USER_LED_LOST_NONE;
    bitmask |= (phase_present[1] < 10 && phase_present[1] > 0) ? USER_LED_UNKNOWN_L2_BIT : USER_LED_LOST_NONE;
    bitmask |= (phase_present[2] < 10 && phase_present[2] > 0) ? USER_LED_UNKNOWN_L3_BIT : USER_LED_LOST_NONE;

    bitmask |= (n_present < 10) ? USER_LED_LOST_N_BIT : USER_LED_LOST_NONE;
    bitmask |= (n_present < 10 && n_present > 0) ? USER_LED_UNKNOWN_N_BIT : USER_LED_LOST_NONE;
    
    bitmask |= pe_present ? USER_LED_LOST_NONE : USER_LED_LOST_PE_BIT;

    //temperature with 3 K hysteresis
    if(temp_value < 57)
        temp_critical_reached = 0;
    if(temp_value > 60 || temp_critical_reached)
    {
        bitmask |= USER_LED_LOST_TEMP_BIT;
        temp_critical_reached = 1;
    }

    char rot = rotation_phase_get();
    
    if(bitmask == 0)
    {
		bitmask |= (rot == 'R') ? USER_LED_DIR_R_BIT : 0;
		bitmask |= (rot == 'L') ? USER_LED_DIR_L_BIT : 0;
	}	
	
	bitmaskfilter[bitmaskfilter_index] = bitmask;
	bitmaskfilter_index++;
	

	if(bitmaskfilter_index >= FILTER_CYCLES)
	{
		bitmaskfilter_index = 0;
		bitmask_filtered = 0;
		
		memset(bitmaskcnt, 0, sizeof(bitmaskcnt));
		
		for(int i = 0; i < FILTER_CYCLES; i++)
		{
			//process all 16 bits
			for(int bitcnt = 0; bitcnt < sizeof(bitmaskcnt)/sizeof(bitmaskcnt[0]); bitcnt++)
			{
				if(bitmaskfilter[i] & (1UL << bitcnt))
					bitmaskcnt[bitcnt]++;
			}
		}
		//process no roation direction is compares the resulting counters
		for(int bitcnt = 0; bitcnt < USER_LED_DIR_R_BITPOS; bitcnt++)
		{
			if(bitmaskcnt[bitcnt] > FILTER_THRESHOLD)
			{
				bitmask_filtered |= (1UL << bitcnt);
			}			
		}
		
		//set rotation bit if no error is happen, else roation bits are zero
		if(bitmask_filtered == 0)
		{
			if(bitmaskcnt[USER_LED_DIR_R_BITPOS] >= bitmaskcnt[USER_LED_DIR_L_BITPOS])
				bitmask_filtered |= USER_LED_DIR_R_BIT;
			else
				bitmask_filtered |= USER_LED_DIR_L_BIT;
		}
		
		
		first_time_shot = 0;
	}
	
	if(first_time_shot)
		bitmask_filtered = bitmask;
    
    if(onoff != -1)
		return;

    user_led_set_state( ((bitmask_filtered & USER_LED_DIR_R_BIT) ? USER_LED_STATE_ROT_R : ((bitmask_filtered & USER_LED_DIR_L_BIT) ? USER_LED_STATE_ROT_L : USER_LED_STATE_LOST)), bitmask_filtered);

	refresh_sensordata(bitmask_filtered, phase_voltage[0], phase_voltage[1], phase_voltage[2], phase_voltage[3]);
	refresh_statusdata(temp_value);

#if DEBUG_PRINT_ADC_VALUES
    for(int i = 0; i < FILTERCHAN_NUM_OF; i++)
		printk("%d,", (int)(result_vector[i] * 100.0f));
	printk("%d,", (int)pe_filtered);
	printk("0x%04X,", (int)bitmask);
	printk("0x%04X", (int)bitmask_filtered);
	printk("\n");
#endif
}

void main(void)
{
	int err;

	printk("Initializing...\n");
	
	storage_init();


#if defined(CONFIG_MCUMGR)
	smp_svr_init();
#endif

	// Initialize the Bluetooth Subsystem
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	//bt setup in sync (mesh reset works without delay hack)
	bt_ready(err);
	
	user_led_init();
	
#if defined(CONFIG_MCUMGR)
	/* Initialize the Bluetooth mcumgr transport. */
	smp_bt_register();
#endif

	meshreset_boothook();
	
	temp_init();
	
	//wait for LED init sequence
	k_msleep(4000);

	while (1) {
		/*
		printk("k: %d", (int)sqrtf(f+1.1f));
		f *= 10.1f;
		k_msleep(1000);
		*/
		//printk("Data");
		//k_msleep(1000);
		int16_t *data = adc_start_measure();
		if(data == NULL)
			continue;
		//printk(":");
		phase_check_adc_buffer((int16_t(*)[ADC_WIRE_NUMOF])data);
		
		//printk("!");
		display_status(temp_read());
		//for(int i = 0; i < 252 * 4; i+=4)
		//	printk("%d,%d,%d,%d\n", data[i+0], data[i+1], data[i+2], data[i+3]);
	}

}
