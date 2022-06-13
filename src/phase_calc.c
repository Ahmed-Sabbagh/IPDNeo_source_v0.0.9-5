
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "nrf.h"
#include "user_led.h"
#include "phase_calc.h"
#include "misc/filter.h"

#define PHASE_CALC_TIMEOUT_ZEROC    (int)(2000 / 50 * 2) //2000 Hz Sample Clock / 50 Hz * 2 (2x Reserve)
#define MIDVOLTAGE_OFFSET           2040

#define ADC_SAMPLE_COUNT            ADC_SAMPLES_PER_MEAS

int16_t adc_double_buffer[2][ADC_WIRE_NUMOF][ADC_SAMPLE_COUNT]; // 4 or 5 adc meas over 100 ms
int16_t (*adc_double_buffer_p)[ADC_SAMPLE_COUNT] = adc_double_buffer[0]; 
int16_t (*adc_buffer_ready)[ADC_SAMPLE_COUNT] = NULL;

static uint16_t           rms_values[FILTERCHAN_NUM_OF];
float					  result_vector[FILTERCHAN_NUM_OF];
static int16_t			  avr_value_pe = 0;
float 					  pe_filtered = 0.0f;
int                       phase_present[3] = { 0 };
int                       n_present = 0;
int						  pe_present = 0;
int16_t                   max_n;
int16_t                   min_n;

RunAVRFilter_t			  pe_filter = { 0.0f, 4, 0, 0.0f };

typedef struct
{
    int8_t            threshold_ff_zc; //check if rising or falling wave detection is active -> -1 falling detected, +1 rising

    uint32_t          timeout;
    const int16_t     threshold_zerocross; //set range 
} Phase_Options_t;


static Phase_Options_t phases[3] = 
{
    {
        .threshold_ff_zc = 1,
        .timeout = 0,
        .threshold_zerocross = 100,
    },
    {
        .threshold_ff_zc = 1,
        .timeout = 0,
        .threshold_zerocross = 100,
    },
    {
        .threshold_ff_zc = 1,
        .timeout = 0,
        .threshold_zerocross = 100,
    },
};

static float avr_values[FILTERCHAN_NUM_OF];

static float scale_values = 0.42f;

static char phase_rotation_direction = 'X';

static void     	rotation_direction_add_trigger(uint8_t phase, uint8_t timeout);
static void     	phase_decisiontree(void);
static void 		phase_check_direction(int16_t buffer[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF]);
static uint16_t 	phase_rms_calulation(int16_t buffer[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF], int index, int substractor_index, int16_t substractor_fixed);
static int16_t 		phase_avr_calulation(int16_t adc_values[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF], int index);

static void phase_check_direction(int16_t buffer[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF])
{
	phases[0].timeout = 0;
	phases[1].timeout = 0;
	phases[2].timeout = 0;
	
	for(int i = 0; i < ADC_SAMPLE_COUNT; i++)
	{
		for(int phase_i = 0; phase_i < 3; phase_i++)
		{
			int16_t phase_values[3] = { buffer[i][ADC_WIRE_L1] - buffer[i][ADC_WIRE_N], buffer[i][ADC_WIRE_L2] - buffer[i][ADC_WIRE_N], buffer[i][ADC_WIRE_L3] - buffer[i][ADC_WIRE_N] };
			if( (phases[phase_i].threshold_ff_zc == -1) && (phase_values[phase_i] >= phases[phase_i].threshold_zerocross) )
			{
				phases[phase_i].threshold_ff_zc = 1;
				
				rotation_direction_add_trigger(phase_i, 0);
			}
			else if( (phases[phase_i].threshold_ff_zc == 1) && (phase_values[phase_i] <= -phases[phase_i].threshold_zerocross) )
			{
				phases[phase_i].threshold_ff_zc = -1;
			}

			if(phases[phase_i].timeout > PHASE_CALC_TIMEOUT_ZEROC)
			{
				rotation_direction_add_trigger(phase_i, 1);
			}
		}
	}
}

void phase_check_adc_buffer(int16_t buffer[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF])
{
    if(buffer != NULL)
    {
        rms_values[FILTERCHAN_L1] = 	phase_rms_calulation(buffer, ADC_WIRE_L1, -1, MIDVOLTAGE_OFFSET);
        rms_values[FILTERCHAN_L2] = 	phase_rms_calulation(buffer, ADC_WIRE_L2, -1, MIDVOLTAGE_OFFSET);
        rms_values[FILTERCHAN_L3] = 	phase_rms_calulation(buffer, ADC_WIRE_L3, -1, MIDVOLTAGE_OFFSET);
        rms_values[FILTERCHAN_N] = 		phase_rms_calulation(buffer, ADC_WIRE_N, -1, MIDVOLTAGE_OFFSET);
        rms_values[FILTERCHAN_L1_N] = 	phase_rms_calulation(buffer, ADC_WIRE_L1, ADC_WIRE_N, 0);
        rms_values[FILTERCHAN_L2_N] = 	phase_rms_calulation(buffer, ADC_WIRE_L2, ADC_WIRE_N, 0);
        rms_values[FILTERCHAN_L3_N] = 	phase_rms_calulation(buffer, ADC_WIRE_L3, ADC_WIRE_N, 0);
        rms_values[FILTERCHAN_L1_L2] = 	phase_rms_calulation(buffer, ADC_WIRE_L1, ADC_WIRE_L2, 0);
        rms_values[FILTERCHAN_L1_L3] = 	phase_rms_calulation(buffer, ADC_WIRE_L1, ADC_WIRE_L3, 0);
        rms_values[FILTERCHAN_L3_L2] = 	phase_rms_calulation(buffer, ADC_WIRE_L3, ADC_WIRE_L2, 0);
#if defined(CONFIG_BOARD_WALTHERNEOV3) || defined(CONFIG_BOARD_WALTHERNEOV4)
        avr_value_pe = 					phase_avr_calulation(buffer, ADC_WIRE_PE);
#endif


		//get min sample of n channel
        min_n = INT16_MAX;
        for(int i = 0; i < ADC_SAMPLE_COUNT; i++)
        {
            if(min_n > buffer[i][ADC_WIRE_N])
                min_n = buffer[i][ADC_WIRE_N];
        }
        min_n -= MIDVOLTAGE_OFFSET;
        
        avr_values[FILTERCHAN_L1] = 	(float)rms_values[FILTERCHAN_L1_N] * scale_values;
        avr_values[FILTERCHAN_L2] = 	(float)rms_values[FILTERCHAN_L2_N] * scale_values;
        avr_values[FILTERCHAN_L3] = 	(float)rms_values[FILTERCHAN_L3_N] * scale_values;
        avr_values[FILTERCHAN_N] = 		(float)rms_values[FILTERCHAN_N];

        phase_decisiontree();
        
        phase_check_direction(buffer);
    }
}

static int16_t phase_avr_calulation(int16_t adc_values[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF], int index)
{
	int32_t sum = 0;
	
	for(int i = 0; i < ADC_SAMPLE_COUNT; i++)
		sum += adc_values[i][index];
		
    return (int16_t)((float)sum / ADC_SAMPLE_COUNT);
}

static uint16_t phase_rms_calulation(int16_t adc_values[ADC_SAMPLE_COUNT][ADC_WIRE_NUMOF], int index, int substractor_index, int16_t substractor_fixed)
{
    uint32_t sum = 0;
    if(substractor_fixed != 0 || substractor_index < 0)
    {
        for(int i = 0; i < ADC_SAMPLE_COUNT; i++)
            sum += (adc_values[i][index] - substractor_fixed) * (adc_values[i][index] - substractor_fixed);
    }
    else
    {
        for(int i = 0; i < ADC_SAMPLE_COUNT; i++)
            sum += (adc_values[i][index] - adc_values[i][substractor_index]) * (adc_values[i][index] - adc_values[i][substractor_index]);
    }

    //use floating point unit for sqrt function (round)
    return (uint16_t)(sqrtf((float)sum / ADC_SAMPLE_COUNT) + 0.5f);
}

static void phase_decisiontree(void)
{
	//get phase (no n compensation) maximum
    uint16_t max_phase = rms_values[FILTERCHAN_L1];
    if(max_phase < rms_values[FILTERCHAN_L2])
        max_phase = rms_values[FILTERCHAN_L2];
    if(max_phase < rms_values[FILTERCHAN_L3])
        max_phase = rms_values[FILTERCHAN_L3];

	//TODO: deceide from a scaled rms value here?
    if(max_phase < 100)
    {
        phase_present[0] = 0;
        phase_present[1] = 0;
        phase_present[2] = 0;
        n_present = 0;
        pe_present = 0;
        return;
    }

	//scale all channels with max phase -> works also for 110 V AC grid
    for(int i = 0; i < FILTERCHAN_NUM_OF; i++)
        result_vector[i] = (float)rms_values[i] / max_phase;

	//check each phase if present
    int second_block[3] = { 7, 7, 8 };
    int third_block[3] = { 8, 9, 9 };
    for(int i = 0; i < 3; i++)
    {
        if(result_vector[0 + i] <= 0.4f)
            phase_present[i] = -1;
        else
        {
            if(result_vector[second_block[i]] <= 0.5f)
                phase_present[i] = 2;
            else
            {
                if(result_vector[third_block[i]] <= 0.5f)
                {
                    phase_present[i] = 3;
                }
                else
                {
                    if(result_vector[FILTERCHAN_L1_N + i] <= 0.5f)
                    {
                        phase_present[i] = 4;
                    }
                    else
                    {
                        if(result_vector[0 + i] <= 0.85f)
                            phase_present[i] = 5;
                        else
                            phase_present[i] = 10;
                    }
                }
            }
        }
    }

	// check if N present
    float n_min_f = (float)min_n / max_phase;

    if(n_min_f <= 0.2f)
    {
        if(result_vector[3] <= 0.4f)
            n_present = -1;
        else
        {
            if(result_vector[4] <= 0.5f)
                n_present = 1;
            else
            {
                if(result_vector[5] <= 0.5f)
                    n_present = 2;
                else
                {
                    if(result_vector[6] <= 0.5f)
                        n_present = 3;
                    else
                    {
                        if(result_vector[3] <= 0.95f)
                            n_present = 4;
                        else
                            n_present = 10;
                     }
                }
            }
        }
    }
    else
        n_present = 11;
        
#if defined(CONFIG_BOARD_WALTHERNEOV3) || defined(CONFIG_BOARD_WALTHERNEOV4)
    pe_filtered = runavrfilter(&pe_filter, (float)avr_value_pe);
    if(pe_filtered > 20.0f)
		pe_present = 1;
	else
		pe_present = 0;
#endif
}

float phase_calculator_get_value(uint8_t channel)
{
    return avr_values[channel];
}

char rotation_phase_get(void)
{
    return phase_rotation_direction;
}

static void rotation_direction_add_trigger(uint8_t phase, uint8_t timeout)
{
    /*
    Pos1  Pos2	Pos3	
    L1    L2	L3	R
    L1    L3	L2	L
    L2    L3	L1	R
    L2    L1	L3	L
    L3    L1	L2	R
    L3    L2	L1	L
    */
    static const uint8_t lookup_table_R[3] = { 0x39, 0x1E, 0x27 };
    static const uint8_t lookup_table_L[3] = { 0x2D, 0x36, 0x1B };

    static uint8_t order = 0;
    static uint8_t order_pos = 0;
        
    if(!timeout)
    {
        order |= ((phase + 1) & 0x03) << (order_pos * 2);
        phases[phase].timeout = 0;
    }

    order_pos++;
    if(order_pos >= 3)
    {
        //lookup table search for phase direction
        char phase_dir = 'X';
        for(int i = 0; i < 3; i++)
        {
            if(order == lookup_table_R[i])
            {
                phase_dir = 'R';
                //user_led_set_color(USER_LED_PE, USER_LED_COLOR_RED); //debug test
                break;
            }
            else if(order == lookup_table_L[i])
            {
                phase_dir = 'L';
                //user_led_set_color(USER_LED_PE, USER_LED_COLOR_WHITE); //debug test
                break;
            }
        }

        //debug test
        //if(phase_dir == 'X')
        //    user_led_set_color(USER_LED_PE, USER_LED_COLOR_NONE);

        //reset and restart phase rotation
        order_pos = 0;
        order = 0;

        //set new information about the phase 
        //TODO: filter?
        phase_rotation_direction = phase_dir;
    }
}
