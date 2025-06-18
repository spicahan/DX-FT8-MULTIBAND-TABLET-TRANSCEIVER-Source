// Host side mocks for HAL to enable self-contained unit test
#ifndef __HOST_MOCKS_H
#define __HOST_MOCKS_H
#include <stdint.h>

// From core_cm7.h
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

// Copied from stm32f7xx_hal_def.h
typedef enum 
{
  HAL_OK       = 0x00,
  HAL_ERROR    = 0x01,
  HAL_BUSY     = 0x02,
  HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;

// From button
enum BandIndex
{
	_40M = 0,
	_30M = 1,
	_20M = 2,
	_17M = 3,
	_15M = 4,
	_12M = 5,
	_10M = 6,
	NumBands = 7
};

typedef struct
{
	uint16_t Frequency;
	char *display;
} FreqStruct;

extern const FreqStruct sBand_Data[NumBands];

extern int BandIndex;
extern int Tune_On; // 0 = Receive, 1 = Xmit Tune Signal
extern int Beacon_On;
extern int Skip_Tx1;
void Init_BoardVersionInput();
void Check_Board_Version();
void DeInit_BoardVersionInput();
void PTT_Out_Init(void);
void start_Si5351(void);
void receive_sequence(void);

// From SDR_Audio
#define ft8_hz 6.25
extern int Xmit_Mode;
extern int xmit_flag, ft8_xmit_counter, ft8_xmit_flag, ft8_xmit_delay;
extern int DSP_Flag;
extern uint16_t buff_offset;
extern int frame_counter;
void start_audio_I2C(void);
void I2S2_RX_ProcessBuffer(uint16_t offset);

// From Process_DSP
extern int ft8_flag, FT_8_counter, ft8_marker;
void init_DSP(void);

void HAL_Delay(volatile uint32_t Delay);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_Init(void);

// From stm32746g_discovery.h
typedef enum 
{  
  BUTTON_WAKEUP = 0,
  BUTTON_TAMPER = 1,
  BUTTON_KEY = 2
}Button_TypeDef;

typedef enum 
{  
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
}ButtonMode_TypeDef;

typedef enum 
{
LED1 = 0,
LED_GREEN = LED1,
}Led_TypeDef;

void EXT_I2C_Init(void);
void BSP_LED_Init(Led_TypeDef Led);
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);

// From stm32746g_discovery_lcd.h
uint32_t BSP_LCD_GetXSize(void);
uint32_t BSP_LCD_GetYSize(void);

// From stm32746g_discovery_ts.h
uint8_t BSP_TS_Init(uint16_t ts_SizeX, uint16_t ts_SizeY);

// From DS3231.h
void DS3231_init(void);
void make_Real_Time(void);
void make_Real_Date(void);
void display_Real_Date(int x, int y);
void display_RealTime(int x, int y);
extern char log_rtc_time_string[13];
extern char log_rtc_date_string[13];

// From SiLabs
enum si5351_clock {
	SI5351_CLK0,
	SI5351_CLK1,
	SI5351_CLK2,
	SI5351_CLK3,
	SI5351_CLK4,
	SI5351_CLK5,
	SI5351_CLK6,
	SI5351_CLK7
};
void output_enable(enum si5351_clock clk, uint8_t enable);

// From decode_ft8
int ft8_decode(void);

#endif