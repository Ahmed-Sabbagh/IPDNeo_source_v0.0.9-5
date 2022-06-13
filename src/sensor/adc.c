/* adc.c - handle ADC usage */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#include <string.h>
#include <zephyr.h>
#include <drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <sys/printk.h>
#include <hal/nrf_timer.h>
#include <nrfx_timer.h>
#include <nrfx_rtc.h>
#include <nrfx_ppi.h>

#include "adc.h"

static int16_t 					m_sample_buffer[ADC_SAMPLES_PER_MEAS][ADC_WIRE_NUMOF] = { 0 };
static nrf_ppi_channel_t		m_ppi_channel;

K_SEM_DEFINE(isr_sem_adc, 0, 1);
K_MUTEX_DEFINE(mutex_adc);

#define DT_DRV_COMPAT nordic_nrf_saadc

DEVICE_DECLARE(adc_0);

static void saadc_irq_handler(void *param)
{
	if (nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);

		nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_STOP);
		nrf_saadc_disable(NRF_SAADC);
		
		k_sem_give(&isr_sem_adc);
	}
	else
	{
		printk("Unhandled ADC event\n");
	}
}

const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);

static void rtc_config(void)
{
	nrf_rtc_prescaler_set(rtc.p_reg, 13);
	
	//enable for ppi support
	nrfx_rtc_tick_enable(&rtc, false);
	
	nrf_rtc_event_clear(rtc.p_reg, NRF_RTC_EVENT_TICK);
	nrf_rtc_event_enable(rtc.p_reg, NRF_RTC_INT_TICK_MASK);
	nrf_rtc_int_enable(rtc.p_reg, NRF_RTC_INT_TICK_MASK);
	
	nrf_rtc_task_trigger(rtc.p_reg, NRF_RTC_TASK_START);
}

static int init_timerppi(void)
{
	rtc_config();
	
	uint32_t timer_compare_event_addr = nrfx_rtc_event_address_get(&rtc, NRF_RTC_EVENT_TICK);
	uint32_t saadc_sample_event_addr = nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);

	// setup ppi channel so that timer compare event is triggering sample task in SAADC
	nrfx_ppi_channel_alloc(&m_ppi_channel);
	nrfx_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, saadc_sample_event_addr);
	
	return 0;
}

static int init_saadc(void)
{
	NRF_SAADC->RESOLUTION = 2 ; //12 bit resolution
	NRF_SAADC->OVERSAMPLE = 0; //bypass over sampling

	NRF_SAADC->RESULT.PTR = (uint32_t)m_sample_buffer;
	NRF_SAADC->RESULT.MAXCNT = sizeof(m_sample_buffer)/sizeof(int16_t); 
	NRF_SAADC->SAMPLERATE= 0;  //no autotrigger

	NRF_SAADC->CH[0].CONFIG = 	(SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) | \
															(SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos ) | \
															(SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos ) | \
															(SAADC_CH_CONFIG_REFSEL_VDD1_4 << SAADC_CH_CONFIG_REFSEL_Pos) | \
															(SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos ) | \
															(SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos ) | \
															(SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos ) ;
															
	NRF_SAADC->CH[1].CONFIG = 	(SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) | \
															(SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos ) | \
															(SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos ) | \
															(SAADC_CH_CONFIG_REFSEL_VDD1_4 << SAADC_CH_CONFIG_REFSEL_Pos) | \
															(SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos ) | \
															(SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos ) | \
															(SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos ) ;

	NRF_SAADC->CH[2].CONFIG = 	(SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) | \
															(SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos ) | \
															(SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos ) | \
															(SAADC_CH_CONFIG_REFSEL_VDD1_4 << SAADC_CH_CONFIG_REFSEL_Pos) | \
															(SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos ) | \
															(SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos ) | \
															(SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos ) ;
															
	NRF_SAADC->CH[3].CONFIG = 	(SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) | \
															(SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos ) | \
															(SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos ) | \
															(SAADC_CH_CONFIG_REFSEL_VDD1_4 << SAADC_CH_CONFIG_REFSEL_Pos) | \
															(SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos ) | \
															(SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos ) | \
															(SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos ) ;
															
	NRF_SAADC->CH[4].CONFIG = 	(SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) | \
														(SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos ) | \
														(SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos ) | \
														(SAADC_CH_CONFIG_REFSEL_VDD1_4 << SAADC_CH_CONFIG_REFSEL_Pos) | \
														(SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos ) | \
														(SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos ) | \
														(SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos ) ;
														

	NRF_SAADC ->CH[0].LIMIT = 0;
	NRF_SAADC ->CH[0].PSELN = 0;
	NRF_SAADC ->CH[1].LIMIT = 0;
	NRF_SAADC ->CH[1].PSELN = 0;
	NRF_SAADC ->CH[2].LIMIT = 0;
	NRF_SAADC ->CH[2].PSELN = 0;
	NRF_SAADC ->CH[3].LIMIT = 0;
	NRF_SAADC ->CH[3].PSELN = 0;
	NRF_SAADC ->CH[4].LIMIT = 0;
	NRF_SAADC ->CH[4].PSELN = 0;


#if defined(CONFIG_BOARD_WALTHERNEOV2)
	NRF_SAADC ->CH[0].PSELP = 1;
	NRF_SAADC ->CH[1].PSELP = 2;
	NRF_SAADC ->CH[2].PSELP = 3;
	NRF_SAADC ->CH[3].PSELP = 4;
#elif defined(CONFIG_BOARD_WALTHERNEOV3)
	NRF_SAADC ->CH[0].PSELP = 4;
	NRF_SAADC ->CH[1].PSELP = 3;
	NRF_SAADC ->CH[2].PSELP = 7;
	NRF_SAADC ->CH[3].PSELP = 1; //workaround - patch for N
	NRF_SAADC ->CH[4].PSELP = 8;
#elif defined(CONFIG_BOARD_WALTHERNEOV4)
	NRF_SAADC ->CH[0].PSELP = 4;
	NRF_SAADC ->CH[1].PSELP = 3;
	NRF_SAADC ->CH[2].PSELP = 7;
	NRF_SAADC ->CH[3].PSELP = 1;
	NRF_SAADC ->CH[4].PSELP = 8;
	//todo: add hardwarecoding later
#endif
	
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    saadc_irq_handler, DEVICE_GET(adc_0), 0);
		    
	irq_enable(DT_INST_IRQN(0));
	
	NRF_SAADC ->INTEN = 0x00000002; //enable int for end

	return 0;
}

static void deinit_saadc(void)
{
	NRF_SAADC->INTEN = 0;
	irq_disable(DT_INST_IRQN(0));
	nrfx_ppi_channel_disable(m_ppi_channel);
	nrf_saadc_disable(NRF_SAADC);
}

int adc_init(void)
{
	return init_saadc();

}

int16_t *adc_start_measure(void)
{
	int16_t *buf = NULL;
	
	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));
	
	k_mutex_lock(&mutex_adc, K_FOREVER);
	
	init_saadc();
	
	init_timerppi();
	
	nrf_saadc_enable(NRF_SAADC);
	nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
	
	nrfx_ppi_channel_enable(m_ppi_channel);//check if here is correct
	
	
	if (k_sem_take(&isr_sem_adc, K_MSEC(500)) != 0) 
	{
		printk("ADC sampling data failed - buffer: %u\n", NRF_SAADC->RESULT.AMOUNT);
		//buf = (int16_t*)m_sample_buffer; //demo
	}
	else
	{
		buf = (int16_t*)m_sample_buffer;
	}
	
	deinit_saadc();
	
	
	k_mutex_unlock(&mutex_adc);
	
	if(0) {

		int index_list[] = { 0 , 1, ADC_SAMPLES_PER_MEAS - 1 }; 
		for(int i = 0; i < sizeof(index_list)/sizeof(index_list[0]); i++)
		{
#if defined(CONFIG_BOARD_WALTHERNEOV2)
			//printk("%d, %d, %d, %d\n", m_sample_buffer[index_list[i]][0], m_sample_buffer[index_list[i]][1], m_sample_buffer[index_list[i]][2], m_sample_buffer[index_list[i]][3]);
#elif defined(CONFIG_BOARD_WALTHERNEOV3) || defined(CONFIG_BOARD_WALTHERNEOV4)
			//printk("%d, %d, %d, %d, %d\n", m_sample_buffer[index_list[i]][0], m_sample_buffer[index_list[i]][1], m_sample_buffer[index_list[i]][2], m_sample_buffer[index_list[i]][3], m_sample_buffer[index_list[i]][4]);
#endif
		}

	}
	
	

	return buf;
}

