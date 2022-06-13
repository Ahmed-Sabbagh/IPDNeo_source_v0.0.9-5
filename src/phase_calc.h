#ifndef PHASE_CALC_H_
#define PHASE_CALC_H_

#include <stdint.h>
#include "sensor/adc.h"

enum avrfilter_channels_e
{
    FILTERCHAN_L1     = 0,
    FILTERCHAN_L2,
    FILTERCHAN_L3,
    FILTERCHAN_N,
    FILTERCHAN_L1_N,
    FILTERCHAN_L2_N,
    FILTERCHAN_L3_N,
    FILTERCHAN_L1_L2,
    FILTERCHAN_L1_L3,
    FILTERCHAN_L3_L2,
    FILTERCHAN_NUM_OF,
};

extern int				phase_present[3];
extern int				n_present;
extern int				pe_present;
extern float			pe_filtered;
extern int16_t			max_n;
extern int16_t			min_n;
extern float			result_vector[FILTERCHAN_NUM_OF];

void	phase_calculator_init(float scaler);
void	phase_check_adc_buffer(int16_t buffer[ADC_SAMPLES_PER_MEAS][ADC_WIRE_NUMOF]);
void	phase_calculator_add_adc_values(int16_t adc_value_L1, int16_t adc_value_L2, int16_t adc_value_L3, int16_t adc_value_N);
float	phase_calculator_get_value(uint8_t channel);
char	rotation_phase_get(void);

#endif /* PHASE_CALC_H_ */
