/*
 * button.c
 *
 *  Created on: Feb 18, 2016
 *      Author: user
 */

#include <Display.h>
#include <stdlib.h>
#include "button.h"
#include "stm32746g_discovery_lcd.h"
#include "SDR_Audio.h"
#include "Codec_Gains.h"
#include "decode_ft8.h"
#include "main.h"
#include "stm32fxxx_hal.h"
#include "Process_DSP.h"
#include "log_file.h"
#include "gen_ft8.h"
#include "traffic_manager.h"
#include "DS3231.h"
#include "SiLabs.h"
#include "options.h"

int Tune_On; // 0 = Receive, 1 = Xmit Tune Signal
int Beacon_On;
int Arm_Tune;
int Auto_Sync;
uint16_t start_freq;
int BandIndex;
int QSO_Fix;
int CQ_Mode_Index;
int Free_Index;

int AGC_Gain = 20;
int Band_Minimum;
int Skip_Tx1;

int EditingFreq=0;
int EditingComment=0;
int EditingCall=0;
int EditingGrid=0;
int EditingFree1=0;
int EditingFree2=0;

char EditingText[MESSAGE_SIZE]={'\0'};

char display_frequency[BAND_DATA_SIZE] = "14.075";
const char *start;

FreqStruct sBand_Data[NumBands] =
	{
		{// 40,
		 7074, "7.074"},
		{// 30,
		 10136, "10.136"},
		{// 20,
		 14074, "14.075"},
		{// 17,
		 18100, "18.101"},
		{// 15,
		 21074, "21.075"},
		{// 12,
		 24915, "24.916"},
		{// 10,
		 28074, "28.075"}};

ButtonStruct sButtonData[NumButtons] = {
	{// button 0  inhibit xmit either as beacon or answer CQ
	 /*text0*/ "Clr ",
	 /*text1*/ "Clr ",
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 0,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 1 control Beaconing / CQ chasing
	 /*text0*/ "QSO ",
	 /*text1*/ "Becn",
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 55,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 2 control Tune
	 /*text0*/ "Tune",
	 /*text1*/ "Tune",
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 110,
	 /*y*/ line2,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 3 display R/T status
	 /*text0*/ "Rx",
	 /*text1*/ "Tx",
	 /*blank*/ "  ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 165,
	 /*y*/ line2,
	 /*w*/ 0, // setting the width and height to 0 turns off touch response , display only
	 /*h*/ 0},

	{// button 4 CQ or free mode
	 /*text0*/ " CQ ",
	 /*text1*/ "Free",
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 200,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 5 QSO Response Freq 0 fixed, 1 Match received station frequency
	 /*text0*/ "Fixd",
	 /*text1*/ "Rcvd",
	 /*blank*/ "   ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 255,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 6 Sync FT8
	 /*text0*/ "Sync",
	 /*text1*/ "Sync",
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 310,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 7 Lower Gain
	 /*text0*/ " G- ",
	 /*text1*/ " G- ",
	 /*blank*/ "    ",
	 /*Active*/ 2,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 365,
	 /*y*/ line2,
	 /*w*/ button_bar_width - 20,
	 /*h*/ 30},

	{// button 8 Raise Gain
	 /*text0*/ " G+ ",
	 /*text1*/ " G+ ",
	 /*blank*/ "    ",
	 /*Active*/ 2,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 435,
	 /*y*/ line2,
	 /*w*/ button_bar_width,
	 /*h*/ 30},

	{// button 9 Lower Cursor
	 /*text0*/ "F- ",
	 /*text1*/ "F- ",
	 /*blank*/ "    ",
	 /*Active*/ 2,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 360,
	 /*y*/ line0,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 10 Raise Cursor
	 /*text0*/ "F+ ",
	 /*text1*/ "F+ ",
	 /*blank*/ "    ",
	 /*Active*/ 2,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 450,
	 /*y*/ line0,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 11 Band Down
	 /*text0*/ "Band-",
	 /*text1*/ "Band-",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 270,
	 /*y*/ 40,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 12 Band Up
	 /*text0*/ "Band+",
	 /*text1*/ "Band+",
	 /*blank*/ "     ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 420,
	 /*y*/ 40,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 13 Xmit for Calibration
	 /*text0*/ "Xmit",
	 /*text1*/ "Xmit",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 420,
	 /*y*/ SETUP_line1,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 14 Save Calibration Data
	 /*text0*/ "Save",
	 /*text1*/ "Save",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 356,
	 /*y*/ 65,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 15 Save RTC Time
	 /*text0*/ "Set ",
	 /*text1*/ "Set ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Button,
	 /*y*/ RTC_line2,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 16 Hours
	 /*text0*/ "Hr- ",
	 /*text1*/ "Hr- ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line0,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 17 Hours
	 /*text0*/ "Hr+ ",
	 /*text1*/ "Hr+ ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line0,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 18 minutes
	 /*text0*/ "Min-",
	 /*text1*/ "Min-",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line1,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 19 minutes
	 /*text0*/ "Min+",
	 /*text1*/ "Min+",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line1,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 20 Seconds
	 /*text0*/ "Sec-",
	 /*text1*/ "Sec-",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line2,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 21 Seconds
	 /*text0*/ "Sec+",
	 /*text1*/ "Sec+",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line2,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 22 Day
	 /*text0*/ "Day-",
	 /*text1*/ "Day-",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line3,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 23 Day
	 /*text0*/ "Day+",
	 /*text1*/ "Day+",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line3,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 24 Month
	 /*text0*/ "Mon-",
	 /*text1*/ "Mon-",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 25 Month
	 /*text0*/ "Mon+",
	 /*text1*/ "Mon+",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 26 Year
	 /*text0*/ "Yr- ",
	 /*text1*/ "Yr- ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Sub,
	 /*y*/ RTC_line5,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 27 Year
	 /*text0*/ "Yr+ ",
	 /*text1*/ "Yr+ ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Add,
	 /*y*/ RTC_line5,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 28 Save  RTC Date
	 /*text0*/ "Set ",
	 /*text1*/ "Set ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ RTC_Button,
	 /*y*/ RTC_line5,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 29 Standard CQ
	 /*text0*/ " CQ ",
	 /*text1*/ " CQ ",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 1,
	 /*x*/ 240,
	 /*y*/ SETUP_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 30 CQ SOTA
	 /*text0*/ "SOTA",
	 /*text1*/ "SOTA",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 300,
	 /*y*/ SETUP_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 31 CQ POTA
	 /*text0*/ "POTA",
	 /*text1*/ "POTA",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 360,
	 /*y*/ SETUP_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 32 CQ QRP
	 /*text0*/ "QRP ",
	 /*text1*/ "QRP ",
	 /*blank*/ "   ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 420,
	 /*y*/ SETUP_line4,
	 /*w*/ button_width,
	 /*h*/ 30},

	{// button 33 Free Text 1
	 /*text0*/ "Free1",
	 /*text1*/ "Free1",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 256,
	 /*y*/ SETUP_line5,
	 /*w*/ 160,
	 /*h*/ 30},

	{// button 34 Free Text 2
	 /*text0*/ "Free2",
	 /*text1*/ "Free2",
	 /*blank*/ "    ",
	 /*Active*/ 0,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 256,
	 /*y*/ SETUP_line6,
	 /*w*/ 160,
	 /*h*/ 30},

	{// button 35 SkipTx1
	 /*text0*/ "SkipTx1",
	 /*text1*/ "SkipTx1",
	 /*blank*/ "        ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 356,
	 /*y*/ SETUP_line2,
	 /*w*/ 110,
	 /*h*/ 30},

	{// button 36 EditCall
	 /*text0*/ Station_Call,
	 /*text1*/ Station_Call,
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 240,
	 /*y*/ SETUP_line3,
	 /*w*/ 70,
	 /*h*/ 30},

	{// button 37 EditGrid
	 /*text0*/ Locator,
	 /*text1*/ Locator,
	 /*blank*/ "    ",
	 /*Active*/ 1,
	 /*Displayed*/ 1,
	 /*state*/ 0,
	 /*x*/ 356,
	 /*y*/ SETUP_line3,
	 /*w*/ 60,
	 /*h*/ 30},


	 //text0, text1, blank, Active, Displayed, state,   x,   y,   w,   h

	/*38*/ {   "E",  "E",  "  ",  0,      1,         0,     240,  SETUP_line5,  16,  30},    //EditFreeText1
	/*39*/ {   "E",  "E",  "  ",  0,      1,         0,     240,  SETUP_line6,  16,  30},    //EditFreeText2

	/*40*/ {   "Edit",  "Edit",  "  ",  0,      1,         0,     270,  SETUP_line1,  60,  30},    //EditFreq
	/*41*/ {  "COMMENT",  "COMMENT",  "  ",  0,      1,         0,     240,  SETUP_line2,  110,  30},    //EditComment
	/*42*/ {  "COMMENT",  "COMMENT",  " ",  0,      1,         0,     KEYBASE_X,  SETUP_line0,  259,  30},    //EditingWindow

  	/*	Key1	*/ {	"1"	,	" 1 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*0	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key2	*/ {	"2"	,	" 2 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*1	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key3	*/ {	"3"	,	" 3 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*2	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key4	*/ {	"4"	,	" 4 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*3	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key5	*/ {	"5"	,	" 5 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*4	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key6	*/ {	"6"	,	" 6 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*5	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key7	*/ {	"7"	,	" 7 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*6	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key8	*/ {	"8"	,	" 8 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*7	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key9	*/ {	"9"	,	" 9 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*8	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Key0	*/ {	"0"	,	" 0 "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*9	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyDash	*/ {	"-"	,	" - "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*10	,	KEYBASE_Y+KEYHIGHT*0	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyQ	*/ {	"Q"	,	" Q "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*0	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyW	*/ {	"W"	,	" W "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*1	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyE	*/ {	"E"	,	" E "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*2	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyR	*/ {	"R"	,	" R "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*3	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyT	*/ {	"T"	,	" T "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*4	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyY	*/ {	"Y"	,	" Y "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*5	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyU	*/ {	"U"	,	" U "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*6	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyI	*/ {	"I"	,	" I "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*7	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyO	*/ {	"O"	,	" O "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*8	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyP	*/ {	"P"	,	" P "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*9	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	keyPlus	*/ {	"+"	,	" + "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*10	,	KEYBASE_Y+KEYHIGHT*1	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyA	*/ {	"A"	,	" A "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*0	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyS	*/ {	"S"	,	" S "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*1	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyD	*/ {	"D"	,	" D "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*2	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyF	*/ {	"F"	,	" F "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*3	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyG	*/ {	"G"	,	" G "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*4	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyH	*/ {	"H"	,	" H "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*5	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyJ	*/ {	"J"	,	" J "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*6	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyK	*/ {	"K"	,	" K "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*7	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyL	*/ {	"L"	,	" L "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*8	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	Keydot	*/ {	"."	,	" . "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*9	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeySlash*/ {	"/"	,	" / "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*10	,	KEYBASE_Y+KEYHIGHT*2	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyZ	*/ {	"Z"	,	" Z "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*0	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyX	*/ {	"X"	,	" X "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*1	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyC	*/ {	"C"	,	" C "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*2	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyV	*/ {	"V"	,	" V "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*3	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyB	*/ {	"B"	,	" B "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*4	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyN	*/ {	"N"	,	" N "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*5	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
  	/*	KeyM	*/ {	"M"	,	" M "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*6	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
	/*	KeyQMark*/ {	"?"	,	" ? "	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*7	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
	/*	KeySpace*/ {	"SPC"	,	"SPC"	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*8	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},
	/*	KeyBack	*/ {	"<--"	,	"<--"	,"   "	, 0, 1, 0,	KEYBASE_X + KEYWIDTH*10	,	KEYBASE_Y+KEYHIGHT*3	,	KEYWIDTH	,	KEYHIGHT	},

}; // end of button definition

void drawButton(uint16_t button)
{
	BSP_LCD_SetFont(&Font16);
	if (sButtonData[button].Active > 0)
	{
		if (sButtonData[button].state == 1)
			BSP_LCD_SetBackColor(LCD_COLOR_RED);
		else
			BSP_LCD_SetBackColor(LCD_COLOR_BLUE);

		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

		BSP_LCD_DisplayStringAt(sButtonData[button].x, sButtonData[button].y + 15,
								sButtonData[button].state == 1 ? (const uint8_t *)sButtonData[button].text1 : (const uint8_t *)sButtonData[button].text0,
								LEFT_MODE);

		BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	}
}
void drawKey(uint16_t button)
{
	BSP_LCD_SetFont(&Font24);
	if (sButtonData[button].Active > 0)
	{
		if (sButtonData[button].state == 1)
			BSP_LCD_SetBackColor(LCD_COLOR_RED);
		else
			BSP_LCD_SetBackColor(LCD_COLOR_BLUE);

		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

		BSP_LCD_DisplayStringAt(sButtonData[button].x, sButtonData[button].y + 15,
								sButtonData[button].state == 1 ? (const uint8_t *)sButtonData[button].text1 : (const uint8_t *)sButtonData[button].text0,
								LEFT_MODE);

		BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	}
}
void checkButton(void)
{
	for (uint16_t button = Clear; button < NumButtons; button++)
	{
		if (testButton(sButtonData[button].x, sButtonData[button].y, sButtonData[button].w, sButtonData[button].h) == 1)
		{
			switch (sButtonData[button].Active)
			{
			case 0:
				break;

			case 1:
				sButtonData[button].state = !sButtonData[button].state;
				drawButton(button);
				executeButton(button);
				break;

			case 2:
				executeButton(button);
				break;

			case 3:
				executeCalibrationButton(button);
				break;
			}
		}
	}
}

void SelectFilterBlock()
{
	if (Band_Minimum == _40M)
	{
		if (BandIndex < _17M) // i.e. 40M, 30M or 20M
			RLY_Select_20to40();
		else
			RLY_Select_10to17();
	}
}

static void toggle_button_state(int button)
{
	sButtonData[button].state = 1;
	drawButton(button);
	HAL_Delay(10);
	sButtonData[button].state = 0;
	drawButton(button);
}

static void toggle_key_state(int button)
{
	HAL_Delay(10);
	sButtonData[button].state = 0;
	drawKey(button);
}

static void update_CQFree_buttons()
{
	// Reset 4 CQ buttons and 2 free text buttons first
	sButtonData[StandardCQ].state = 0;
	sButtonData[CQSOTA].state = 0;
	sButtonData[CQPOTA].state = 0;
	sButtonData[QRPP].state = 0;
	sButtonData[FreeText1].state = 0;
	sButtonData[FreeText2].state = 0;
	// Pick the one button to highlight
	if (free_text) {
		// WARNING! assuming FreeIndex1 and FreeIndex2 are continuous values
		sButtonData[FreeText1 + Free_Index].state = 1;
	} else {
		// WARNING! assuming StandardCQ, CQSOTA, CQPOTA, QRPP are continuous values
		sButtonData[StandardCQ + CQ_Mode_Index].state = 1;
		sButtonData[CQFree].text0 = sButtonData[StandardCQ + CQ_Mode_Index].text0;
	}
	sButtonData[CQFree].state = free_text;
	drawButton(StandardCQ);
	drawButton(CQSOTA);
	drawButton(CQPOTA);
	drawButton(QRPP);
	drawButton(CQFree);
	drawButton(FreeText1);
	drawButton(FreeText2);
}

void executeButton(uint16_t index)
{
	switch (index)
	{
	case Clear:
		clr_pressed = true;
		toggle_button_state(Clear);
		break;

	case QSOBeacon:
		if (!sButtonData[QSOBeacon].state)
		{
			Beacon_On = 0;
			Beacon_State = 0;
		}
		else
		{
			Beacon_On = 1;
			Beacon_State = 1;
		}
		break;

	case Tune:
		if (!sButtonData[Tune].state)
		{
			//tune_Off_sequence();
			Tune_On = 0;
			//Arm_Tune = 0;
			//xmit_flag = 0;
			//receive_sequence();
			erase_Cal_Display();
		}
		else
		{
			Tune_On = 1; // Turns off display of FT8 traffic
			setup_Cal_Display();
			//Arm_Tune = 0;
		}
		break;

	case RxTx:
		// no code required, all dependent stuff works off of button state
		break;

	case CQFree:
		free_text = sButtonData[CQFree].state;
		update_CQFree_buttons();
		break;

	case FixedReceived:
		if (sButtonData[FixedReceived].state)
			QSO_Fix = 1;
		else
			QSO_Fix = 0;
		break;

	case Sync:
		if (!sButtonData[Sync].state)
			Auto_Sync = 0;
		else
			Auto_Sync = 1;
		break;

	case GainDown:
		if (AGC_Gain >= 3)
			AGC_Gain--;
		show_short(667, 255, AGC_Gain);
		Set_PGA_Gain(AGC_Gain);
		break;

	case GainUp:
		if (AGC_Gain < 31)
			AGC_Gain++;
		show_short(667, 255, AGC_Gain);
		Set_PGA_Gain(AGC_Gain);
		break;

	case FreqDown:
		if (cursor > 0)
		{
			cursor--;
			NCO_Frequency = (double)(cursor + ft8_min_bin) * FFT_Resolution;
		}
		show_variable(400, 25, (int)NCO_Frequency);
		break;

	case FreqUp:
		if (cursor < (ft8_buffer_size - ft8_min_bin - 8))
		{
			// limits highest NCO frequency to (3875 - 50)hz
			cursor++;
			NCO_Frequency = (double)(cursor + ft8_min_bin) * FFT_Resolution;
		}
		show_variable(400, 25, (int)NCO_Frequency);
		break;

	case TxCalibrate:
		if (!sButtonData[TxCalibrate].state)
		{
			tune_Off_sequence();
			Arm_Tune = 0;
			xmit_flag = 0;
			receive_sequence();
		}
		else
		{
			xmit_sequence();
			HAL_Delay(10);
			xmit_flag = 1;
			tune_On_sequence();
			Arm_Tune = 1;
		}
		break;

	case SaveBand:
		Options_SetValue(OPTION_Band_Index, BandIndex);
		Options_StoreValue(OPTION_Band_Index);
		start_freq = sBand_Data[BandIndex].Frequency;
		show_wide(380, 0, start_freq);

		sprintf(display_frequency, "%s", sBand_Data[BandIndex].display);

		set_Rcvr_Freq();
		HAL_Delay(10);

		SelectFilterBlock();

		toggle_button_state(SaveBand);
		break;

	case SaveRTCTime:
		set_RTC_to_TimeEdit();
		toggle_button_state(SaveRTCTime);
		break;

	case SaveRTCDate:
		set_RTC_to_DateEdit();
		toggle_button_state(SaveRTCDate);
		break;

	case StandardCQ:
	case CQSOTA:
	case CQPOTA:
	case QRPP:
		free_text = !sButtonData[index].state;
		if (!free_text) {
			// WARNING! assuming StandardCQ, CQSOTA, CQPOTA, QRPP are continuous values
			CQ_Mode_Index = index - StandardCQ;
		}
		update_CQFree_buttons();
		break;

	case FreeText1:
	case FreeText2:
		free_text = sButtonData[index].state;
		if (free_text) {
			// WARNING! Assuming FreeText1 and FreeText2 are continuous values
			Free_Index = index - FreeText1;
		}
		update_CQFree_buttons();
		break;

	case SkipTx1:
		Skip_Tx1 ^= 1;
		sButtonData[SkipTx1].state = Skip_Tx1;
		drawButton(SkipTx1);
		break;

	case EditCall:
		strcpy(EditingText,Station_Call);
		EnableKeyboard();
		EditingCall = 1;
		toggle_button_state(EditCall);
		break;

	case EditGrid:
		strcpy(EditingText,Locator);
		EnableKeyboard();
		EditingGrid = 1;
		toggle_button_state(EditGrid);
		break;

	case EditFreq:
		sprintf(display_frequency, "%i", sBand_Data[BandIndex].Frequency);
		strcpy(EditingText,display_frequency);
		EnableKeyboard();
		EditingFreq = 1;
		toggle_button_state(EditFreq);
		break;

	case EditComment:
		strcpy(EditingText,Comment);
		sButtonData[EditingWindow].text0 = Comment;
		EnableKeyboard();
		EditingComment = 1;
		toggle_button_state(EditComment);
		break;


	case EditFreeText1: //
		strcpy(EditingText,Free_Text1);
		EnableKeyboard();
		EditingFree1 = 1;
		toggle_button_state(EditFreeText1);
		break;

	case EditFreeText2: //
		strcpy(EditingText,Free_Text2);
		EnableKeyboard();
		EditingFree2 = 1;
		toggle_button_state(EditFreeText2);
		break;

	case EditingWindow: //
		toggle_key_state(EditingWindow);

		if(EditingFreq == 1){
			EditingFreq = 0;
			sBand_Data[BandIndex].Frequency = (uint16_t)atoi(EditingText);
			strcpy(sBand_Data[BandIndex].display, EditingText);
		}
		if(EditingComment == 1){
			EditingComment = 0;
			strcpy(Comment, EditingText);
		}
		if(EditingCall == 1){
			EditingCall = 0;
			strcpy(Station_Call, EditingText);
			sButtonData[EditCall].text0 = Station_Call;

		}
		if(EditingGrid == 1){
			EditingGrid = 0;
			strcpy(Locator, EditingText);
			sButtonData[EditGrid].text0 = Locator;
		}
		if(EditingFree1 == 1){
			EditingFree1 = 0;
			strcpy(Free_Text1, EditingText);
			sButtonData[FreeText1].text0 = Free_Text1;
		}
		if(EditingFree2 == 1){
			EditingFree2 = 0;
			strcpy(Free_Text2, EditingText);
			sButtonData[FreeText2].text0 = Free_Text2;
		}
		DisableKeyboard();
		update_stationdata();
		break;

	case Key1	: AppendChar(EditingText, '1'); UpdateEditingWindow(); toggle_key_state(Key1	); break;
	case Key2	: AppendChar(EditingText, '2'); UpdateEditingWindow(); toggle_key_state(Key2	); break;
	case Key3	: AppendChar(EditingText, '3'); UpdateEditingWindow(); toggle_key_state(Key3	); break;
	case Key4	: AppendChar(EditingText, '4'); UpdateEditingWindow(); toggle_key_state(Key4	); break;
	case Key5	: AppendChar(EditingText, '5'); UpdateEditingWindow(); toggle_key_state(Key5	); break;
	case Key6	: AppendChar(EditingText, '6'); UpdateEditingWindow(); toggle_key_state(Key6	); break;
	case Key7	: AppendChar(EditingText, '7'); UpdateEditingWindow(); toggle_key_state(Key7	); break;
	case Key8	: AppendChar(EditingText, '8'); UpdateEditingWindow(); toggle_key_state(Key8	); break;
	case Key9	: AppendChar(EditingText, '9'); UpdateEditingWindow(); toggle_key_state(Key9	); break;
	case Key0	: AppendChar(EditingText, '0'); UpdateEditingWindow(); toggle_key_state(Key0	); break;
	case KeyDash	: AppendChar(EditingText, '-'); UpdateEditingWindow(); toggle_key_state(KeyDash	); break;
	case KeyQ	: AppendChar(EditingText, 'Q'); UpdateEditingWindow(); toggle_key_state(KeyQ	); break;
	case KeyW	: AppendChar(EditingText, 'W'); UpdateEditingWindow(); toggle_key_state(KeyW	); break;
	case KeyE	: AppendChar(EditingText, 'E'); UpdateEditingWindow(); toggle_key_state(KeyE	); break;
	case KeyR	: AppendChar(EditingText, 'R'); UpdateEditingWindow(); toggle_key_state(KeyR	); break;
	case KeyT	: AppendChar(EditingText, 'T'); UpdateEditingWindow(); toggle_key_state(KeyT	); break;
	case KeyY	: AppendChar(EditingText, 'Y'); UpdateEditingWindow(); toggle_key_state(KeyY	); break;
	case KeyU	: AppendChar(EditingText, 'U'); UpdateEditingWindow(); toggle_key_state(KeyU	); break;
	case KeyI	: AppendChar(EditingText, 'I'); UpdateEditingWindow(); toggle_key_state(KeyI	); break;
	case KeyO	: AppendChar(EditingText, 'O'); UpdateEditingWindow(); toggle_key_state(KeyO	); break;
	case KeyP	: AppendChar(EditingText, 'P'); UpdateEditingWindow(); toggle_key_state(KeyP	); break;
	case keyPlus	: AppendChar(EditingText, '+'); UpdateEditingWindow(); toggle_key_state(keyPlus	); break;
	case KeyA	: AppendChar(EditingText, 'A'); UpdateEditingWindow(); toggle_key_state(KeyA	); break;
	case KeyS	: AppendChar(EditingText, 'S'); UpdateEditingWindow(); toggle_key_state(KeyS	); break;
	case KeyD	: AppendChar(EditingText, 'D'); UpdateEditingWindow(); toggle_key_state(KeyD	); break;
	case KeyF	: AppendChar(EditingText, 'F'); UpdateEditingWindow(); toggle_key_state(KeyF	); break;
	case KeyG	: AppendChar(EditingText, 'G'); UpdateEditingWindow(); toggle_key_state(KeyG	); break;
	case KeyH	: AppendChar(EditingText, 'H'); UpdateEditingWindow(); toggle_key_state(KeyH	); break;
	case KeyJ	: AppendChar(EditingText, 'J'); UpdateEditingWindow(); toggle_key_state(KeyJ	); break;
	case KeyK	: AppendChar(EditingText, 'K'); UpdateEditingWindow(); toggle_key_state(KeyK	); break;
	case KeyL	: AppendChar(EditingText, 'L'); UpdateEditingWindow(); toggle_key_state(KeyL	); break;
	case Keydot	: AppendChar(EditingText, '.'); UpdateEditingWindow(); toggle_key_state(Keydot	); break;
	case KeySlash   : AppendChar(EditingText, '/'); UpdateEditingWindow(); toggle_key_state(KeySlash ); break;
	case KeyZ	: AppendChar(EditingText, 'Z'); UpdateEditingWindow(); toggle_key_state(KeyZ	); break;
	case KeyX	: AppendChar(EditingText, 'X'); UpdateEditingWindow(); toggle_key_state(KeyX	); break;
	case KeyC	: AppendChar(EditingText, 'C'); UpdateEditingWindow(); toggle_key_state(KeyC	); break;
	case KeyV	: AppendChar(EditingText, 'V'); UpdateEditingWindow(); toggle_key_state(KeyV	); break;
	case KeyB	: AppendChar(EditingText, 'B'); UpdateEditingWindow(); toggle_key_state(KeyB	); break;
	case KeyN	: AppendChar(EditingText, 'N'); UpdateEditingWindow(); toggle_key_state(KeyN	); break;
	case KeyM	: AppendChar(EditingText, 'M'); UpdateEditingWindow(); toggle_key_state(KeyM	); break;
	case KeyQMark   : AppendChar(EditingText, '?'); UpdateEditingWindow(); toggle_key_state(KeyQMark ); break;
	case KeySpace   : AppendChar(EditingText, ' '); UpdateEditingWindow(); toggle_key_state(KeySpace ); break;
	case KeyBack	: DeleteLastChar(EditingText);  UpdateEditingWindow(); toggle_key_state(KeyBack	); break;

	}
}

static void processButton(int id, int isIncrement, int isDate)
{
	RTCStruct *data = &s_RTC_Data[id];
	if (isIncrement ? data->data < data->Maximum : data->data > data->Minimum)
	{
		data->data = isIncrement ? data->data + 1 : data->data - 1;
	}
	else
	{
		data->data = isIncrement ? data->Minimum : data->Maximum;
	}
	isDate ? display_RTC_DateEdit(RTC_Button - 20, RTC_line3 + 15) : display_RTC_TimeEdit(RTC_Button - 20, RTC_line0 + 15);
}

void executeCalibrationButton(uint16_t index)
{
	switch (index)
	{
	case BandDown:
		if (BandIndex > Band_Minimum)
		{
			BandIndex--;
			show_wide(340, 55, sBand_Data[BandIndex].Frequency);
			sprintf(display_frequency, "%s", sBand_Data[BandIndex].display);
		}
		break;

	case BandUp:
		if (BandIndex < _10M)
		{
			BandIndex++;
			show_wide(340, 55, sBand_Data[BandIndex].Frequency);
			sprintf(display_frequency, "%s", sBand_Data[BandIndex].display);
		}
		break;

	case HourDown:
		processButton(3, 0, 0);
		break;

	case HourUp:
		processButton(3, 1, 0);
		break;

	case MinuteDown:
		processButton(4, 0, 0);
		break;

	case MinuteUp:
		processButton(4, 1, 0);
		break;

	case SecondDown:
		processButton(5, 0, 0);
		break;

	case SecondUp:
		processButton(5, 1, 0);
		break;

	case DayDown:
		processButton(0, 0, 1);
		break;

	case DayUp:
		processButton(0, 1, 1);
		break;

	case MonthDown:
		processButton(1, 0, 1);
		break;

	case MonthUp:
		processButton(1, 1, 1);
		break;

	case YearDown:
		processButton(2, 0, 1);
		break;

	case YearUp:
		processButton(2, 1, 1);
		break;
	}
}

void setup_Cal_Display(void)
{
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(0, FFT_H, 480, 201);

	sButtonData[BandDown].Active = 3;
	sButtonData[BandUp].Active = 3;

	for (int button = TxCalibrate; button <= SaveRTCTime; ++button)
	{
		sButtonData[button].Active = 1;
	}

	for (int button = HourDown; button < SaveRTCDate; button++)
	{
		sButtonData[button].Active = 3;
		drawButton(button);
	}

	sButtonData[SaveRTCDate].Active = 1;

	for (int button = BandDown; button <= SaveRTCTime; ++button)
	{
		drawButton(button);
	}

	drawButton(SaveRTCDate);

	sButtonData[SkipTx1].state = Skip_Tx1;
	drawButton(SkipTx1);
	for (int button = StandardCQ; button < NumButtons-43-1; ++button) //43:skip keyboard
	{
		sButtonData[button].Active = 1;
		drawButton(button);
	}

	show_wide(340, 55, start_freq);

	load_RealTime();
	display_RTC_TimeEdit(RTC_Button - 20, RTC_line0 + 15);

	load_RealDate();
	display_RTC_DateEdit(RTC_Button - 20, RTC_line3 + 15);
}

void erase_Cal_Display(void)
{
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(0, FFT_H, 480, 201);

	for (int button = BandDown; button < StandardCQ; button++)
	{
		sButtonData[button].Active = 0;
	}

	for (int button = StandardCQ; button < NumButtons; ++button)
	{
		sButtonData[button].Active = 0;
		drawButton(button);
	}

	for (int button = TxCalibrate; button <= SaveRTCTime; ++button)
	{
		sButtonData[button].state = 0;
	}

	sButtonData[SaveRTCDate].state = 0;

	sButtonData[CQFree].Active = 1;
	drawButton(CQFree);

	Options_SetValue(OPTION_Skip_Tx1, Skip_Tx1);
	Options_StoreValue(OPTION_Skip_Tx1);
}

void PTT_Out_Init(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	__HAL_RCC_GPIOI_CLK_ENABLE();

	gpio_init_structure.Pin = GPIO_PIN_2; // D8  RXSW
	gpio_init_structure.Mode = GPIO_MODE_OUTPUT_OD;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOI, &gpio_init_structure);

	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_SET); // Set = Receive connect

	__HAL_RCC_GPIOA_CLK_ENABLE();

	gpio_init_structure.Pin = GPIO_PIN_15; // D9 TXSW
	gpio_init_structure.Mode = GPIO_MODE_OUTPUT_OD;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOA, &gpio_init_structure);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // Set = Receive short
}

void Init_BoardVersionInput(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	__HAL_RCC_GPIOH_CLK_ENABLE();

	gpio_init_structure.Pin = GPIO_PIN_6; // D6  BTS
	gpio_init_structure.Mode = GPIO_MODE_INPUT;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOH, &gpio_init_structure);

	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET); // Set = Receive connect
}

void DeInit_BoardVersionInput(void)
{
	HAL_GPIO_DeInit(GPIOH, GPIO_PIN_6);
}

void PTT_Out_Set(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_SET);
}

void PTT_Out_RST_Clr(void)
{
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
}

void RLY_Select_20to40(void)
{
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
}

void RLY_Select_10to17(void)
{
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
}

static void Init_BandSwitchOutput(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	gpio_init_structure.Pin = GPIO_PIN_3; // D7  RLY
	gpio_init_structure.Mode = GPIO_MODE_OUTPUT_OD;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOI, &gpio_init_structure);

	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
}

void Check_Board_Version(void)
{
	Band_Minimum = _20M;

	// GPIO Pin 6 is grounded for new model
	if (HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_6) == 0)
	{
		Init_BandSwitchOutput();

		Band_Minimum = _40M;
	}

	Options_SetMinimum(Band_Minimum);
}

void set_codec_input_gain(void)
{
	Set_PGA_Gain(AGC_Gain);
	HAL_Delay(10);
	Set_ADC_DVC(190);
}

void receive_sequence(void)
{
	PTT_Out_Set(); // set output high to connect receiver to antenna
	HAL_Delay(10);
	sButtonData[RxTx].state = 0;
	drawButton(RxTx);
}

void xmit_sequence(void)
{
	PTT_Out_RST_Clr(); // set output low to disconnect receiver from antenna
	HAL_Delay(10);
	sButtonData[RxTx].state = 1;
	drawButton(RxTx);
}

const uint64_t F_boot = 11229600000ULL;

void start_Si5351(void)
{
	init(SI5351_CRYSTAL_LOAD_0PF, 0);
	drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
	drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
	drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
	set_freq(F_boot, SI5351_CLK1);
	HAL_Delay(10);
	output_enable(SI5351_CLK1, 1);
	HAL_Delay(20);
	set_Rcvr_Freq();
}

void FT8_Sync(void)
{
	start_time = HAL_GetTick();
	ft8_flag = 1;
	FT_8_counter = 0;
	ft8_marker = 1;
}

void EnableKeyboard(void)
{
	//disable buttons
	for(int i=NUMMAINBUTTON; i<NumButtons-NUMKEYS; i++)
		sButtonData[i].Active = 0;

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(0, 50, 480, 180);

	HAL_Delay(200);
	//Enable Keyboard
	for(int i=NumButtons-NUMKEYS; i<NumButtons; i++) {
		sButtonData[i].Active = 2;
		drawKey(i);
	}
	sButtonData[EditingWindow].Active = 1;
	drawKey(EditingWindow);
}

void DisableKeyboard(void)
{
	//disable keyboard
	for(int i=NumButtons-NUMKEYS; i<NumButtons; i++) {
		sButtonData[i].Active = 0;
	}

	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(0, 50, 480, 180);

	setup_Cal_Display();
	show_wide(340, 55, sBand_Data[BandIndex].Frequency);
}

void AppendChar(char *str, char c){
	int len = strlen(str);
	if(len < MESSAGE_SIZE-1) {
		str[len] = c;
		str[len+1] = '\0';
	}
}

void DeleteLastChar(char *str){
	int len = strlen(str);
	if(len > 0) str[len-1] = '\0';
}

void UpdateEditingWindow(void) {
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_FillRect(KEYBASE_X, SETUP_line0, 259, 40);
	sButtonData[EditingWindow].text0 = EditingText;
	sButtonData[EditingWindow].text1 = EditingText;
	drawKey(EditingWindow);
}


