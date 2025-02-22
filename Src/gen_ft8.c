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

char Station_Call[7];	  // six character call sign + /0
char Locator[5];		  // four character locator  + /0
char Target_Call[7];	  // six character call sign + /0
char Target_Locator[5];	  // four character locator  + /0
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
const char Beacon_seventy_three[] = "RR73";
const char QSO_seventy_three[] = "73";
const uint8_t blank[] = "                  ";

int Free_Text_Max = 0;

void set_cq(void)
{
	char message[28];
	uint8_t packed[K_BYTES];
	if(Send_Free == 0 ){
		if(CQ_Mode_Index == 0)
			sprintf(message, "CQ %s %s", Station_Call, Locator);
		else if(CQ_Mode_Index == 1)
			sprintf(message, "CQ_DX %s %s", Station_Call, Locator);
		else if(CQ_Mode_Index == 2)
			sprintf(message, "CQ_POTA %s %s", Station_Call, Locator);
		else //if(CQ_Mode_Index == 3)
			sprintf(message, "CQ_QRPP %s %s", Station_Call, Locator);
	}
	else {//if(Send_Free == 1)
		if(Free_Index == 1) sprintf(message, "%s", Free_Text1);
		else if(Free_Index == 2) sprintf(message, "%s", Free_Text2);
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

void Read_Station_File(void)
{
	uint16_t result = 0;
	size_t i;
	char read_buffer[64];

	f_mount(&FS, SDPath, 1);
	if (f_open(&fil, "StationData.txt", FA_OPEN_ALWAYS | FA_READ) == FR_OK)
	{
		char *call_part, *locator_part = NULL, *extra_part = NULL;
		memset(read_buffer, 0, sizeof(read_buffer));
		f_lseek(&fil, 0);
		f_gets(read_buffer, sizeof(read_buffer), &fil);

		Station_Call[0] = 0;
		call_part = strtok(read_buffer, ":\r\n");
		if (call_part != NULL)
			locator_part = strtok(NULL, ":\r\n");
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
			{
				strcpy(Locator, locator_part);
				for (i = 0; i < strlen(Locator); ++i)
				{
					if (!isalnum((int)Locator[i]))
					{
						Locator[0] = 0;
						break;
					}
				}
			}
		}

		Free_Text1[0] = 0;
		extra_part = strtok(NULL, ":\r\n");
		if (extra_part != NULL)
		{
            strncpy(Free_Text1, extra_part, sizeof(Free_Text1) - 1);
            Free_Text1[sizeof(Free_Text1) - 1] = 0; // Null-terminate
            Free_Text_Max = 1;
            sButtonData[33].text0 = Free_Text1;
            sButtonData[33].text1 = Free_Text1;
		}
		Free_Text2[0] = 0;
		extra_part = strtok(NULL, ":\r\n");
		if (extra_part != NULL)
		{
            strncpy(Free_Text2, extra_part, sizeof(Free_Text2) - 1);
            Free_Text2[sizeof(Free_Text2) - 1] = 0; // Null-terminate
            Free_Text_Max = 2;
            sButtonData[34].text0 = Free_Text2;
            sButtonData[34].text1 = Free_Text2;
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
