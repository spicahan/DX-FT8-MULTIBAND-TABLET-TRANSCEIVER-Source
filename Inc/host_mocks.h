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
extern int BandIndex;

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
#define LCD_COLOR_BLUE          ((uint32_t)0xFF0000FF)
#define LCD_COLOR_GREEN         ((uint32_t)0xFF00FF00)
#define LCD_COLOR_RED           ((uint32_t)0xFFFF0000)
#define LCD_COLOR_CYAN          ((uint32_t)0xFF00FFFF)
#define LCD_COLOR_MAGENTA       ((uint32_t)0xFFFF00FF)
#define LCD_COLOR_YELLOW        ((uint32_t)0xFFFFFF00)
#define LCD_COLOR_LIGHTBLUE     ((uint32_t)0xFF8080FF)
#define LCD_COLOR_LIGHTGREEN    ((uint32_t)0xFF80FF80)
#define LCD_COLOR_LIGHTRED      ((uint32_t)0xFFFF8080)
#define LCD_COLOR_LIGHTCYAN     ((uint32_t)0xFF80FFFF)
#define LCD_COLOR_LIGHTMAGENTA  ((uint32_t)0xFFFF80FF)
#define LCD_COLOR_LIGHTYELLOW   ((uint32_t)0xFFFFFF80)
#define LCD_COLOR_DARKBLUE      ((uint32_t)0xFF000080)
#define LCD_COLOR_DARKGREEN     ((uint32_t)0xFF008000)
#define LCD_COLOR_DARKRED       ((uint32_t)0xFF800000)
#define LCD_COLOR_DARKCYAN      ((uint32_t)0xFF008080)
#define LCD_COLOR_DARKMAGENTA   ((uint32_t)0xFF800080)
#define LCD_COLOR_DARKYELLOW    ((uint32_t)0xFF808000)
#define LCD_COLOR_WHITE         ((uint32_t)0xFFFFFFFF)
#define LCD_COLOR_LIGHTGRAY     ((uint32_t)0xFFD3D3D3)
#define LCD_COLOR_GRAY          ((uint32_t)0xFF808080)
#define LCD_COLOR_DARKGRAY      ((uint32_t)0xFF404040)
#define LCD_COLOR_BLACK         ((uint32_t)0xFF000000)
#define LCD_COLOR_BROWN         ((uint32_t)0xFFA52A2A)
#define LCD_COLOR_ORANGE        ((uint32_t)0xFFFFA500)
#define LCD_COLOR_TRANSPARENT   ((uint32_t)0xFF000000)

typedef enum
{
  CENTER_MODE             = 0x01,    /* Center mode */
  RIGHT_MODE              = 0x02,    /* Right mode  */
  LEFT_MODE               = 0x03     /* Left mode   */
}Text_AlignModeTypdef;

typedef struct _tFont
{    
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
  
} sFONT;

extern sFONT Font16;

void BSP_LCD_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, const uint8_t *Text, Text_AlignModeTypdef Mode);
void BSP_LCD_SetBackColor(uint32_t Color);
void BSP_LCD_SetFont(sFONT *fonts);
void BSP_LCD_SetTextColor(uint32_t Color);

// From DS3231.h
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