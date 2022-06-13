#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>
#include "../board.h"


#if defined(CONFIG_BOARD_WALTHERNEOV2)
enum adc_channel_mapping_e
{
	ADC_WIRE_L1 = 3,
	ADC_WIRE_L2 = 2,
	ADC_WIRE_L3 = 1,
	ADC_WIRE_N  = 0,
	ADC_WIRE_NUMOF = 4,
};
#elif defined(CONFIG_BOARD_WALTHERNEOV3)
enum adc_channel_mapping_e
{
	ADC_WIRE_L1 = 0,
	ADC_WIRE_L2 = 1,
	ADC_WIRE_L3 = 2,
	ADC_WIRE_N = 3,
	ADC_WIRE_PE = 4,
	ADC_WIRE_NUMOF = 5,
};
#elif defined(CONFIG_BOARD_WALTHERNEOV4)
enum adc_channel_mapping_e
{
	ADC_WIRE_L1 = 0,
	ADC_WIRE_L2 = 1,
	ADC_WIRE_L3 = 2,
	ADC_WIRE_N = 3,
	ADC_WIRE_PE = 4,
	ADC_WIRE_NUMOF = 5,
	//todo: add hardware coding
};
#endif

#define ADC_SAMPLES_PER_MEAS	(int)(234)


int adc_init(void);
int16_t *adc_start_measure(void);

#endif
