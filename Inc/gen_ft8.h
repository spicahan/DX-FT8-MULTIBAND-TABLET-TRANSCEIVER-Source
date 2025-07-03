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

extern char Station_Call[CALLSIGN_SIZE];  // seven character call sign (e.g. 3DA0XYZ) + optional /P + null terminator
extern char Locator[LOCATOR_SIZE];        // four character locator  + /0
extern char Target_Call[CALLSIGN_SIZE];   // same as Station_Call
extern char Target_Locator[LOCATOR_SIZE]; // same as Locator
extern int Target_RSL;

// Copied from Display.h
// TODO: refactor
#define MESSAGE_SIZE 40

extern char SDPath[4]; /* SD card logical drive path */

extern int CQ_Mode_Index;
extern int Free_Index;
extern char Free_Text1[MESSAGE_SIZE];
extern char Free_Text2[MESSAGE_SIZE];
extern char Comment[MESSAGE_SIZE];

void Read_Station_File(void);
void SD_Initialize(void);

void queue_custom_text(const char *plain_text);   /* needed by autoseq_engine */

#endif /* GEN_FT8_H_ */
