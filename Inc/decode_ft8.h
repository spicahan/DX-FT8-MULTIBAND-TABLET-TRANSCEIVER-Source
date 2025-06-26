/*
 * decode_ft8.h
 *
 *  Created on: Nov 2, 2019
 *      Author: user
 */

#ifndef DECODE_FT8_H_
#define DECODE_FT8_H_

#define MAX_MSG_LEN 40

extern int Station_RSL;
extern int Target_RSL;

typedef enum _Sequence
{
    Seq_RSL = 0,
    Seq_Locator
} Sequence;

typedef struct Decode
{
    char call_to[14]; // call also be 'CQ'
    char call_from[14];
    char locator[7]; // can also be a response 'RR73' etc.
    int freq_hz;
    int sync_score;
    int snr;
    int received_snr;
    char target_locator[7];
    int slot;
    Sequence sequence;
} Decode;

extern Decode new_decoded[];

void process_selected_Station(int num_decoded, int TouchIndex);
void set_QSO_Xmit_Freq(int freq);

#endif /* DECODE_FT8_H_ */
