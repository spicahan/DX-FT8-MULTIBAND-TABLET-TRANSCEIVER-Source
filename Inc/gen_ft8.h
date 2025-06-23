/*
 * gen_ft8.h
 *
 *  Created on: Oct 30, 2019
 *      Author: user
 */

#ifndef GEN_FT8_H_
#define GEN_FT8_H_

#include <stdint.h>

#define CALLSIGN_SIZE 10
#define LOCATOR_SIZE 5
#define MAX_BLANK_SIZE 21

extern char Station_Call[CALLSIGN_SIZE];  // seven character call sign (e.g. 3DA0XYZ) + optional /P + null terminator
extern char Locator[LOCATOR_SIZE];        // four character locator  + /0
extern char Target_Call[CALLSIGN_SIZE];   // same as Station_Call
extern char Target_Locator[LOCATOR_SIZE]; // same as Locator
extern int Target_RSL;
extern const uint8_t blank[MAX_BLANK_SIZE];

#define REPLY_MESSAGE_SIZE 28
// Copied from Display.h
// TODO: refactor
#define MESSAGE_SIZE 40

extern char reply_message[REPLY_MESSAGE_SIZE];
extern char reply_message_list[REPLY_MESSAGE_SIZE][8];
extern int reply_message_count;

extern char SDPath[4]; /* SD card logical drive path */

extern int CQ_Mode_Index;
extern int Free_Index;
extern char Free_Text1[MESSAGE_SIZE];
extern char Free_Text2[MESSAGE_SIZE];

void Read_Station_File(void);
void SD_Initialize(void);

void display_queued_message(const char*);
void display_xmitting_message(const char*);

#endif /* GEN_FT8_H_ */
