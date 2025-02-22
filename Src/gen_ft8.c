/*
 * gen_ft8.c
 *
 *  Created on: Oct 24, 2019
 *      Author: user
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "pack.h"
#include "encode.h"
#include "constants.h"

#include "gen_ft8.h"

#include <stdio.h>

#include "ff.h"		/* Declarations of FatFs API */
#include "diskio.h" /* Declarations of device I/O functions */
#include "stdio.h"
#include "stm32746g_discovery_sd.h"
#include "stm32746g_discovery.h"

#include "stm32746g_discovery_lcd.h"

#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "arm_math.h"
#include <string.h>
#include "decode_ft8.h"
#include "Display.h"
#include "log_file.h"
#include "traffic_manager.h"
#include "ADIF.h"

#include "button.h"

char Station_Call[7];	// six character call sign + /0
char Locator[5];		// four character locator  + /0
char Target_Call[7];	// six character call sign + /0
char Target_Locator[5]; // four character locator  + /0
int Station_RSL;

char reply_message[21];
char reply_message_list[18][8];
int reply_message_count;

const int display_start_x = 240;
const int display_start_y = 240;
const int display_width = 230;

static uint8_t isInitialized = 0;

/* Fatfs structure */
static FATFS FS;
static FIL fil;

const char CQ[] = "CQ";
const char CQ_DX[] = "CQ_DX";
const char CQ_POTA[] = "CQ_POTA";
const char CQ_QRPP[] = "QRPP";
const char Beacon_seventy_three[] = "RR73";
const char QSO_seventy_three[] = "73";
const uint8_t blank[] = "                  ";

int Free_Text_Max = 0;
static char Free_Text1[20];
static char Free_Text2[20];

void set_cq(void)
{
	char message[28];
	uint8_t packed[K_BYTES];
	if (Free_Index == 0)
	{
		const char *mode = CQ;
		switch (CQ_Mode_Index)
		{
			case 0:
			default:
				break;
			case 1:
				mode = CQ_DX;
				break;
			case 2:
				mode = CQ_POTA;
				break;
			case 3:
				mode = CQ_QRPP;
				break;
		}

		sprintf(message, "%s %s %s", mode, Station_Call, Locator);
	}
	else
	{
		switch(Free_Index)
		{
			default:
			case 0:
				break;
			case 1:
				sprintf(message, "%s", Free_Text1);
				break;
			case 2:
				sprintf(message, "%s", Free_Text2);
				break;
		}
	}

	pack77(message, packed);
	genft8(packed, tones);

	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, blank, LEFT_MODE);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, (const uint8_t *)message, LEFT_MODE);
}

static int in_range(int num, int min, int max)
{
	if (num < min)
		return min;
	if (num > max)
		return max;
	return num;
}

void set_reply(uint16_t index)
{
	uint8_t packed[K_BYTES];
	char RSL[5];

	if (index == 0)
	{
		itoa(in_range(Target_RSL, -999, 9999), RSL, 10);
		sprintf(reply_message, "%s %s %s", Target_Call, Station_Call, RSL);
	}
	else if (index == 1)
	{
		sprintf(reply_message, "%s %s %s", Target_Call, Station_Call,
				Beacon_seventy_three);
		if (Station_RSL != 99)
			write_ADIF_Log();
	}

	strcpy(current_Beacon_xmit_message, reply_message);
	update_Beacon_log_display(1);

	pack77(reply_message, packed);
	genft8(packed, tones);

	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, blank, LEFT_MODE);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, (const uint8_t *)reply_message, LEFT_MODE);
}

static char xmit_messages[3][20];

void compose_messages(void)
{
	char RSL[5];

	itoa(in_range(Target_RSL, -999, 9999), RSL, 10);

	sprintf(xmit_messages[0], "%s %s %s", Target_Call, Station_Call, Locator);
	sprintf(xmit_messages[1], "%s %s R%s", Target_Call, Station_Call, RSL);
	sprintf(xmit_messages[2], "%s %s %s", Target_Call, Station_Call,
			QSO_seventy_three);

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, (const uint8_t *)xmit_messages[0], LEFT_MODE);
}

void que_message(int index)
{
	uint8_t packed[K_BYTES];

	pack77(xmit_messages[index], packed);
	genft8(packed, tones);

	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y - 20, blank, LEFT_MODE);

	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y - 20, (const uint8_t *)xmit_messages[index], LEFT_MODE);

	strcpy(current_QSO_xmit_message, xmit_messages[index]);

	if (index == 2 && Station_RSL != 99)
		write_ADIF_Log();
}

void clear_qued_message(void)
{
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y - 20, blank, LEFT_MODE);
}

void clear_xmit_messages(void)
{
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_DisplayStringAt(display_start_x, display_start_y, blank, LEFT_MODE);
}

static void set_text(char *text, const char *source, int field_id)
{
	strcpy(text, source);
	for (int i = 0; i < strlen(text); ++i)
	{
		if (!isalnum((int)text[i]))
		{
			text[0] = 0;
			break;
		}
	}

	if (field_id >= 0)
	{
		sButtonData[field_id].text0 = text;
		sButtonData[field_id].text1 = text;
	}
}

static const char* delimiters = ":\r\n";

void Read_Station_File(void)
{
	uint16_t result = 0;
	size_t i;
	char read_buffer[64];

	f_mount(&FS, SDPath, 1);
	if (f_open(&fil, "StationData.txt", FA_OPEN_ALWAYS | FA_READ) == FR_OK)
	{
		char *call_part, *locator_part = NULL, *free_text1_part = NULL, *free_text2_part = NULL;
		memset(read_buffer, 0, sizeof(read_buffer));
		f_lseek(&fil, 0);
		f_gets(read_buffer, sizeof(read_buffer), &fil);

		Station_Call[0] = 0;
		call_part = strtok(read_buffer, delimiters);
		if (call_part != NULL)
			locator_part = strtok(NULL, delimiters);
		if (locator_part != NULL)
			free_text1_part = strtok(NULL, delimiters);
		if (free_text1_part != NULL)
			free_text2_part = strtok(NULL, delimiters);

		if (call_part != NULL)
		{
			i = strlen(call_part);
			result = i > 0 && i < sizeof(Station_Call) ? 1 : 0;
			if (result != 0)
			{
				strcpy(Station_Call, call_part);
				for (i = 0; i < strlen(Station_Call); ++i)
				{
					if (!isprint((int)Station_Call[i]) || isspace((int)Station_Call[i]))
					{
						Station_Call[0] = 0;
						break;
					}
				}
			}
		}

		Locator[0] = 0;
		if (result != 0 && locator_part != NULL)
		{
			i = strlen(locator_part);
			result = i > 0 && i < sizeof(Locator) ? 1 : 0;
			if (result != 0)
				set_text(Locator, locator_part, -1);
		}

		Free_Text1[0] = 0;
		if (result != 0 && free_text1_part != NULL)
		{
			i = strlen(free_text1_part);
			result = i < sizeof(Free_Text1) ? 1 : 0;
			if (i > 0 && result != 0)
				set_text(Free_Text1, free_text1_part, FreeText1);
		}

		Free_Text2[0] = 0;
		if (result != 0 && free_text2_part != NULL)
		{
			i = strlen(free_text2_part);
			result = i < sizeof(Free_Text2) ? 1 : 0;
			if (i > 0 && result != 0)
				set_text(Free_Text2, free_text2_part, FreeText2);
		}

		f_close(&fil);
	}
}

void clear_reply_message_box(void)
{
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(display_start_x, 40, display_width, 215);
}

void SD_Initialize(void)
{
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);

	if (isInitialized == 0)
	{
		if (BSP_SD_Init() == MSD_OK)
		{
			BSP_SD_ITConfig();
			isInitialized = 1;
			FATFS_LinkDriver(&SD_Driver, SDPath);
		}
		else
		{
			BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"Insert SD.", LEFT_MODE);
			while (BSP_SD_IsDetected() != SD_PRESENT)
			{
				HAL_Delay(100);
			}
			BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"Reboot Now.", LEFT_MODE);
		}
	}
}
