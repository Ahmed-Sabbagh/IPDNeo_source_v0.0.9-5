/* temp.c - handle internal chip temperature reading */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#include <string.h>
#include <zephyr.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

#define TEMP_DEVICE_NAME			DT_LABEL(DT_INST(0, nordic_nrf_temp))

static struct device *temp_dev = NULL;

void temp_init(void)
{
	printk("Init temperature sensor...\n");
	
	temp_dev = device_get_binding(TEMP_DEVICE_NAME);
	if(!temp_dev)
	{
		printk("No temperature device: %s found\n", TEMP_DEVICE_NAME);
	}
}

float temp_read(void)
{
	if(temp_dev == NULL)
		return 0.0f;
		
	struct sensor_value temp_value;
	int ret = sensor_sample_fetch(temp_dev);
	if(ret)
	{
		printk("Fetch temperature value failed\n");
		return 0.0f;
	}
	ret = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &temp_value);
	if(ret)
	{
		printk("Get channel temperature value failed\n");
		return 0.0f;
	}
	
	return (float)sensor_value_to_double(&temp_value);
}
