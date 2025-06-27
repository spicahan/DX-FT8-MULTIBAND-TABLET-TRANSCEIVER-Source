/*
 * decode_ft8.c
 *
 *  Created on: Sep 16, 2019
 *      Author: user
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <Display.h>
#include <gen_ft8.h>

#include "unpack.h"
#include "ldpc.h"
#include "decode.h"
#include "constants.h"
#include "encode.h"
#include "button.h"
#include "main.h"

#include "Process_DSP.h"
#include "log_file.h"
#include "decode_ft8.h"
#include "ADIF.h"
#include "DS3231.h"

const int kLDPC_iterations = 20;
const int kMax_candidates = 20;
const int kMax_decoded_messages = 20;
const unsigned int kMax_message_length = 25;
const int kMin_score = 40; // Minimum sync score threshold for candidates

static int validate_locator(const char locator[]);

Decode new_decoded[25];

int Target_RSL;

int ft8_decode(void)
{
	// Find top candidates by Costas sync score and localize them in time and frequency
	Candidate candidate_list[kMax_candidates];

	int num_candidates = find_sync(export_fft_power, FT_8_counter, ft8_buffer_size, kCostas_map, kMax_candidates, candidate_list, kMin_score);
	char decoded[kMax_decoded_messages][kMax_message_length];

	const float fsk_dev = 6.25f; // tone deviation in Hz and symbol rate

	// Go over candidates and attempt to decode messages
	int num_decoded = 0;

	for (int idx = 0; idx < num_candidates; ++idx)
	{
		Candidate cand = candidate_list[idx];
		float freq_hz = ((float)cand.freq_offset + (float)cand.freq_sub / 2.0f) * fsk_dev;

		float log174[N];
		extract_likelihood(export_fft_power, ft8_buffer_size, cand, kGray_map, log174);

		// bp_decode() produces better decodes, uses way less memory
		uint8_t plain[N];
		int n_errors = 0;
		bp_decode(log174, kLDPC_iterations, plain, &n_errors);

		if (n_errors > 0)
			continue;

		// Extract payload + CRC (first K bits)
		uint8_t a91[K_BYTES];
		pack_bits(plain, K, a91);

		// Extract CRC and check it
		uint16_t chksum = ((a91[9] & 0x07) << 11) | (a91[10] << 3) | (a91[11] >> 5);
		a91[9] &= 0xF8;
		a91[10] = 0;
		a91[11] = 0;
		uint16_t chksum2 = crc(a91, 96 - 14);
		if (chksum != chksum2)
			continue;

		char message[14 + 14 + 7 + 1];

		char call_to[14];
		char call_from[14];
		char locator[7];
		int rc = unpack77_fields(a91, call_to, call_from, locator);
		if (rc < 0)
			continue;

		sprintf(message, "%s %s %s ", call_to, call_from, locator);

		_Bool found = false;
		for (int i = 0; i < num_decoded; ++i)
		{
			if (0 == strcmp(decoded[i], message))
			{
				found = true;
				break;
			}
		}

		float raw_RSL;
		int display_RSL;
		int received_RSL;

		if (!found && num_decoded < kMax_decoded_messages)
		{
			if (strlen(message) < kMax_message_length)
			{
				strcpy(decoded[num_decoded], message);

				new_decoded[num_decoded].sync_score = cand.score;
				new_decoded[num_decoded].freq_hz = (int)freq_hz;

				strcpy(new_decoded[num_decoded].call_to, call_to);
				strcpy(new_decoded[num_decoded].call_from, call_from);
				strcpy(new_decoded[num_decoded].locator, locator);

				new_decoded[num_decoded].slot = slot_state;

				raw_RSL = (float)cand.score;
				display_RSL = (int)((raw_RSL - 160)) / 6;
				new_decoded[num_decoded].snr = display_RSL;

				new_decoded[num_decoded].sequence = Seq_RSL;
				if (validate_locator(locator) == 1)
				{
					strcpy(new_decoded[num_decoded].target_locator, locator);
					new_decoded[num_decoded].sequence = Seq_Locator;
				}
				else
				{
					const char *ptr = locator;
					if (*ptr == 'R')
					{
						ptr++;
					}

					received_RSL = atoi(ptr);
					if (received_RSL < 30) // Prevents an 73 being decoded as a received RSL
					{
						new_decoded[num_decoded].received_snr = received_RSL;
					}
				}

				++num_decoded;
			}
		}
	} // End of big decode loop

	return num_decoded;
}

static int validate_locator(const char locator[])
{
	uint8_t A1, A2, N1, N2;
	uint8_t test = 0;

	A1 = locator[0] - 65; // 'A'
	A2 = locator[1] - 65; // 'A'
	N1 = locator[2] - 48; // '0'
	N2 = locator[3] - 48; // '0'

	if (A1 <= 17) // 'R'
		test++;
	if (A2 > 0 && A2 < 17)
		test++; // block RR73 Arctic and Antarctica
	if (N1 <= 9)
		test++;
	if (N2 <= 9)
		test++;

	if (test == 4)
		return 1;
	else
		return 0;
}

void process_selected_Station(int num_decoded, int TouchIndex)
{
	if (num_decoded > 0 && TouchIndex <= num_decoded)
	{
		strcpy(Target_Call, new_decoded[TouchIndex].call_from);
		strcpy(Target_Locator, new_decoded[TouchIndex].target_locator);
		Target_RSL = new_decoded[TouchIndex].snr;
		target_slot = new_decoded[TouchIndex].slot ^ 1;
		target_freq = new_decoded[TouchIndex].freq_hz;

		if (QSO_Fix == 1)
			set_QSO_Xmit_Freq(target_freq);
	}

	FT8_Touch_Flag = 0;
}

void set_QSO_Xmit_Freq(int freq)
{
	freq = freq - ft8_min_freq;
	cursor = (uint16_t)((float)freq / FFT_Resolution);

	Set_Cursor_Frequency();
	show_variable(400, 25, (int)NCO_Frequency);
}
