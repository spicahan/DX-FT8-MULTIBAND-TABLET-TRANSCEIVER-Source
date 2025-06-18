/*
 * traffic_manager.c
 *
 *  Created on: Feb 29, 2020
 *      Author: user
 */

#include <Display.h>
#include "button.h"
#include "gen_ft8.h"
#include "decode_ft8.h"
#include "stm32746g_discovery_lcd.h"
#include "SDR_Audio.h"
#include "traffic_manager.h"
#include "SiLabs.h"
#include "Process_DSP.h"
#include "Codec_Gains.h"
#include "main.h"

const int ADC_DVC_Gain = 180;
const int ADC_DVC_Off = 90;

#define FT8_TONE_SPACING 625

int Beacon_State;

int RSL_sent;
int QSO_xmit_count;
int RR73_sent;



void setup_to_transmit_on_next_DSP_Flag(void)
{
	ft8_xmit_counter = 0;
	xmit_sequence();
	ft8_transmit_sequence();
	xmit_flag = 1;
	Xmit_DSP_counter = 0;
}

void terminate_QSO(void)
{
	ft8_receive_sequence();
	receive_sequence();
	xmit_flag = 0;
	ft8_xmit_delay = 0;
}

void ft8_transmit_sequence(void)
{
	Set_ADC_DVC(ADC_DVC_Off);
	HAL_Delay(10);
	set_Xmit_Freq();
	HAL_Delay(10);
}

void ft8_receive_sequence(void)
{
	output_enable(SI5351_CLK0, 0);
	HAL_Delay(10);
	Set_ADC_DVC(ADC_DVC_Gain);
}

void tune_On_sequence(void)
{
	Set_ADC_DVC(ADC_DVC_Off);
	HAL_Delay(10);
	set_Xmit_Freq();
	HAL_Delay(10);
	output_enable(SI5351_CLK0, 1);
}

void tune_Off_sequence(void)
{
	output_enable(SI5351_CLK0, 0);
	HAL_Delay(10);
	Set_ADC_DVC(ADC_DVC_Gain);
}

static uint64_t F_Long;

void set_Xmit_Freq(void)
{
	F_Long = ((start_freq * 1000ULL + (uint16_t)NCO_Frequency) * 100ULL);
	set_freq(F_Long, SI5351_CLK0);
}

void set_FT8_Tone(uint8_t ft8_tone)
{
	uint64_t F_FT8 = F_Long + (uint64_t)ft8_tone * FT8_TONE_SPACING;
	set_freq(F_FT8, SI5351_CLK0);
}

void set_Rcvr_Freq(void)
{
	uint64_t F_Receive = ((start_freq * 1000ULL - 10000ULL) * 100ULL * 4ULL);
	set_freq(F_Receive, SI5351_CLK1);
}
