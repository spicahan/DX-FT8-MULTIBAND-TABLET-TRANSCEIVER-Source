/*
 * Process_DSP.c
 *
 *  Created on: Feb 6, 2016
 *      Author: user
 */

#include "SDR_Audio.h"
#include "arm_math.h"
#include "Process_DSP.h"
#include "arm_common_tables.h"
#include "main.h"
#include "Display.h"
#include <math.h>
#include <errno.h>
#include "decode.h"
#include "FIR_Coefficients.h"

double NCO_Frequency;
int ft8_flag, FT_8_counter, ft8_marker;
uint8_t FFT_Buffer[ft8_buffer_size];
uint8_t export_fft_power[ft8_msg_samples * ft8_buffer_size * 4];

q15_t FIR_State_I[NUM_FIR_COEF + (BUFFERSIZE / 4) - 1];
q15_t FIR_State_Q[NUM_FIR_COEF + (BUFFERSIZE / 4) - 1];

static int offset_step;
static arm_rfft_instance_q15 fft_inst;

static arm_fir_instance_q15 S_FIR_I_32K = {NUM_FIR_COEF, FIR_State_I, coeff_fir_I_32K};
static arm_fir_instance_q15 S_FIR_Q_32K = {NUM_FIR_COEF, FIR_State_Q, coeff_fir_Q_32K};

static q15_t window_dsp_buffer[FFT_SIZE];
static q15_t extract_signal[input_gulp_size * 3]; // was float
static q15_t dsp_output[FFT_SIZE * 2];
static q15_t FFT_Scale[FFT_SIZE * 2];
static q15_t FFT_Magnitude[FFT_SIZE];

static int32_t FFT_Mag_10[FFT_SIZE / 2];

static float mag_db[FFT_SIZE / 2 + 1];
static float window[FFT_SIZE];

void Process_FIR_I_32K(void)
{
	arm_fir_q15(&S_FIR_I_32K, FIR_I_In, FIR_I_Out, BUFFERSIZE / 4);
}

void Process_FIR_Q_32K(void)
{
	arm_fir_q15(&S_FIR_Q_32K, FIR_Q_In, FIR_Q_Out, BUFFERSIZE / 4);
}

static float ft_blackman_i(int i, int N)
{
	float alpha = 0.16f; // or 2860/18608
	float a0 = (1 - alpha) / 2;
	float a1 = 1.0f / 2;
	float a2 = alpha / 2;

	float x1 = cosf(2 * (float)M_PI * i / (N - 1));
	float x2 = 2 * x1 * x1 - 1; // Use double angle formula

	return a0 - a1 * x1 + a2 * x2;
}

void init_DSP(void)
{
	arm_rfft_init_q15(&fft_inst, FFT_SIZE, 0, 1);
	for (int i = 0; i < FFT_SIZE; ++i)
	{
		window[i] = ft_blackman_i(i, FFT_SIZE);
	}
	offset_step = (int)ft8_buffer_size * 4;
}

void process_FT8_FFT(void)
{
	for (int i = 0; i < input_gulp_size; i++)
	{
		extract_signal[i] = extract_signal[i + input_gulp_size];
		extract_signal[i + input_gulp_size] = extract_signal[i + 2 * input_gulp_size];
		extract_signal[i + 2 * input_gulp_size] = FT8_Data[i];
	}

	if (ft8_flag)
	{
		int offset = offset_step * FT_8_counter;
		extract_power(offset);

		for (int k = 0; k < ft8_buffer_size; k++)
		{
			FFT_Buffer[k] = export_fft_power[k + offset] / 4;
		}

		Display_WF();

		++FT_8_counter;

		// Add an extra decode attempt at 3.2s before the final decode
		decode_flag = (FT_8_counter == 71 || FT_8_counter == ft8_msg_samples);

		if (FT_8_counter == ft8_msg_samples) {
			ft8_flag = 0;
		}

	}
}

// Compute FFT magnitudes (log power) for each time slot in the signal
void extract_power(int offset)
{
	// Loop over two possible time offsets (0 and block_size/2)
	for (int time_sub = 0; time_sub <= input_gulp_size / 2; time_sub += input_gulp_size / 2)
	{
		for (int i = 0; i < FFT_SIZE; i++)
			window_dsp_buffer[i] = (q15_t)((float)extract_signal[i + time_sub] * window[i]);

		arm_rfft_q15(&fft_inst, window_dsp_buffer, dsp_output);
		arm_shift_q15(dsp_output, 5, FFT_Scale, FFT_SIZE * 2);
		arm_cmplx_mag_squared_q15(FFT_Scale, FFT_Magnitude, FFT_SIZE);

		for (int j = 0; j < FFT_SIZE / 2; j++)
		{
			FFT_Mag_10[j] = 10 * (int32_t)FFT_Magnitude[j];
			mag_db[j] = 5.0 * log((float)FFT_Mag_10[j] + 0.1);
		}

		// Loop over two possible frequency bin offsets (for averaging)
		for (int freq_sub = 0; freq_sub < 2; ++freq_sub)
		{
			for (int k = 0; k < ft8_buffer_size; ++k)
			{
				int mag_offset = k * 2 + freq_sub;
				float db1 = mag_db[mag_offset];
				float db2 = mag_db[mag_offset + 1];
				float db = (db1 + db2) / 2;

				int scaled = (int)(db);

				export_fft_power[offset++] = (uint8_t)(scaled < 0) ? 0 : ((scaled > 63) ? 63 : scaled);
			}
		}
	}
}
