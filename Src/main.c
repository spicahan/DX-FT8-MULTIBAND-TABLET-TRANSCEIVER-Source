/**
 ******************************************************************************
 * @file    USB_Host/HID_Standalone/Src/main.c
 * @author  MCD Application Team
 * @version V1.0.2
 * @date    18-November-2015
 * @brief   USB host HID Mouse and Keyboard demo main file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#ifndef HOST_HAL_MOCK
#include "stm32f7xx_hal_rcc.h"
#include "stm32746g_discovery_ts.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32f7xx_hal_tim.h"
#include "arm_math.h"

#include "SDR_Audio.h"
#include "Process_DSP.h"
#include "Codec_Gains.h"
#include "button.h"

#include "DS3231.h"

#include "SiLabs.h"

#include "options.h"
#endif

#include "autoseq_engine.h"
#include "constants.h"
#include "decode_ft8.h"
#include "gen_ft8.h"
#include "log_file.h"
#include "Display.h"
#include "qso_display.h"
#include "traffic_manager.h"

uint32_t start_time, ft8_time;

int QSO_xmit = 0;
int target_slot;
int target_freq;
int slot_state = 0;
int decode_flag = 0;
// Used for skipping the TX slot
int was_txing = 0;
bool clr_pressed = false;
bool free_text = false;
bool tx_pressed = false;

// Autoseq TX text buffer
static char autoseq_txbuf[MAX_MSG_LEN];
// Autoseq current QSO state text
static char autoseq_state_str[MAX_LINE_LEN];

static int master_decoded = 0;

static bool worked_qsos_in_display = false;

#ifndef HOST_HAL_MOCK
/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);
static void HID_InitApplication(void);
#endif

// Helper function for updating TX region display
void tx_display_update()
{
	if (Tune_On || worked_qsos_in_display) {
		return;
	}
	if (xmit_flag) {
		display_txing_message(autoseq_txbuf);
	} else {
		display_queued_message(autoseq_txbuf);
	}
	autoseq_get_qso_state(autoseq_state_str);
	display_qso_state(autoseq_state_str);
}

static void update_synchronization(void)
{
	uint32_t current_time = HAL_GetTick();
	ft8_time = current_time - start_time;

	// Update slot and reset RX
	int current_slot = ft8_time / 15000 % 2;
	if (current_slot != slot_state)
	{
		// toggle the slot state
#ifdef HOST_HAL_MOCK
		printf("\n----------------------------------------\n");
		printf("slot state %d -> %d\n", slot_state, slot_state ^ 1);
#endif
		slot_state ^= 1;
		if (was_txing) {
			autoseq_tick();
		}
		was_txing = 0;

		ft8_flag = 1;
		FT_8_counter = 0;
		ft8_marker = 1;

		tx_display_update();
	}

	// Check if TX is intended
	if (QSO_xmit && target_slot == slot_state && FT_8_counter < 29)
	{
		setup_to_transmit_on_next_DSP_Flag(); // TODO: move to main.c
		QSO_xmit = 0;
		was_txing = 1;
		// Partial TX, set the TX counter based on current ft8_time
		ft8_xmit_counter = (ft8_time % 15000) / 160; // 160ms per symbol
		// Log the TX
		char log_str[128];
		make_Real_Time();
		make_Real_Date();
		snprintf(log_str, sizeof(log_str), "T [%s %s][%s] %s",
				 log_rtc_date_string,
				 log_rtc_time_string,
				 sBand_Data[BandIndex].display,
				 autoseq_txbuf);
		Write_RxTxLog_Data(log_str);
		tx_display_update();
	}
}

#ifndef HOST_HAL_MOCK
int main(void)
{
	CPU_CACHE_Enable();

	HAL_Init();

	/* Configure the System clock to have a frequency of 200 MHz */
	SystemClock_Config();

	start_audio_I2C();

	PTT_Out_Init();

	Init_BoardVersionInput();
	Check_Board_Version();
	DeInit_BoardVersionInput();

	HID_InitApplication(); // really sets up LCD Display, leftover from example
	HAL_Delay(10);
	BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

	initalize_constants();

	init_DSP();

	SD_Initialize();
	Read_Station_File();
	setup_display();

	Options_Initialize();

	EXT_I2C_Init();
	HAL_Delay(10);
	DS3231_init();
	display_Real_Date(0, 240);

	start_Si5351();

	Set_Cursor_Frequency();
	show_variable(400, 25, (int)NCO_Frequency);
	show_short(667, 255, AGC_Gain);
	start_duplex();
	HAL_Delay(10);
	set_codec_input_gain();
	HAL_Delay(10);
	receive_sequence();
	HAL_Delay(10);
	Set_Headphone_Gain(30);
	Init_Log_File();
	FT8_Sync();
	HAL_Delay(10);
#else
int main(int argc, char *argv[]) {
	if (argc == 2) {
		test_data_file = argv[1];
	}
#endif

	autoseq_init(Station_Call, Locator);


	while (1)
	{
		if (DSP_Flag)
		{
			I2S2_RX_ProcessBuffer(buff_offset);

			if (xmit_flag)
			{
				// Start sending FT8 messages about 0.1 to 0.5 seconds into the time slot
				// to match the observed behavior.
				if (ft8_xmit_delay >= 28)
				{
					if (!Arm_Tune)
					{
						if ((ft8_xmit_counter < 79) && (frame_counter == 2))
						{
							set_FT8_Tone(tones[ft8_xmit_counter]);
							ft8_xmit_counter++;
						}

						if (ft8_xmit_counter == 79)
						{
							xmit_flag = 0;
							ft8_receive_sequence();
							receive_sequence();
							ft8_xmit_delay = 0;
						}
					}
				}
				else
				{
					if (++ft8_xmit_delay == 16)
						output_enable(SI5351_CLK0, 1);
				}
			}

			display_RealTime(100, 240);

			if (Tune_On)
			{
				display_Real_Date(0, 240);
			}

			DSP_Flag = 0;

		}

		if (decode_flag && !Tune_On && !xmit_flag)
		{
			master_decoded = ft8_decode();
#ifdef HOST_HAL_MOCK
			// Check if we should exit the main
			if (master_decoded == -1) {
				return 0;
			}
#endif
			display_messages(new_decoded, master_decoded);
			// Write all the decoded messages to RxTxLog
			make_Real_Time();
			make_Real_Date();
			for (int i = 0; i < master_decoded; ++i) {
				char log_str[64];
				snprintf(log_str, sizeof(log_str), "%c [%s %s][%s] %s %s %s %2i %d",
				         was_txing ? 'O' : 'R',
						 log_rtc_date_string,
						 log_rtc_time_string,
						 sBand_Data[BandIndex].display,
						 new_decoded[i].call_to,
						 new_decoded[i].call_from,
						 new_decoded[i].locator,
						 new_decoded[i].snr,
						 new_decoded[i].freq_hz);
				Write_RxTxLog_Data(log_str);
			}
			if (!was_txing) {
				for (int i = 0; i < master_decoded; i++)
				{
					// TX is (potentially) necessary
					if (autoseq_on_decode(&new_decoded[i]))
					{
						// Fetch TX msg
						if (autoseq_get_next_tx(autoseq_txbuf))
						{
							_debug("QSO_xmit,rspnd");
							queue_custom_text(autoseq_txbuf);
							QSO_xmit = 1;
							tx_display_update();
							break;
						}
					}
				}

				// No valid response has received to advance auto sequencing.
				// Check TX retry is needed?
				// Yes => QSO_xmit = True;
				// No  => check in beacon mode?
				//       Yes => start_cq, QSO_xmit = True;
				//       No  => QSO_xmit = False;
				if (!QSO_xmit)
				{
					// Check if retry is necessary
					if (autoseq_get_next_tx(autoseq_txbuf))
					{
						queue_custom_text(autoseq_txbuf);
						_debug("QSO_xmit,retry");
						QSO_xmit = 1;
					}
					else if (Beacon_On)
					{
						target_slot = slot_state ^ 1;
						autoseq_start_cq();
						autoseq_get_next_tx(autoseq_txbuf);
						queue_custom_text(autoseq_txbuf);
						_debug("QSO_xmit,CQ...");
						QSO_xmit = 1;
						tx_display_update();
					}
				}
			}
			decode_flag = 0;
		} // end of servicing FT_Decode

		// No TX required by retrying or auto-sequencing
		// Check if touch happened
		// Note: In HOST_HAL_MOCK mode, touch events are simulated via JSON test data

		Process_Touch();

		if (clr_pressed) {
			terminate_QSO();
			QSO_xmit = 0;
			was_txing = 0;
			autoseq_init(Station_Call, Locator);
			autoseq_txbuf[0] = '\0';
			tx_display_update();
			clr_pressed = false;
		}

		if (tx_pressed) {
			worked_qsos_in_display = display_worked_qsos();
			tx_pressed = false;
			tx_display_update();
		}

		if (!Tune_On && FT8_Touch_Flag && FT_8_TouchIndex < master_decoded) {
			process_selected_Station(master_decoded, FT_8_TouchIndex);
			autoseq_on_touch(&new_decoded[FT_8_TouchIndex]);
			autoseq_get_next_tx(autoseq_txbuf);
			queue_custom_text(autoseq_txbuf);
			QSO_xmit = 1;
			FT8_Touch_Flag = 0;
			tx_display_update();
		}

		update_synchronization();
	}
}

#ifndef HOST_HAL_MOCK
/**
 * @brief  HID application Init
 * @param  None
 * @retval None
 */
static void HID_InitApplication(void)
{
	/* Configure Key button */
	BSP_PB_Init(BUTTON_TAMPER, BUTTON_MODE_GPIO);

	/* Configure LED1 */
	BSP_LED_Init(LED1);

	/* Initialize the LCD */
	BSP_LCD_Init();

	/* LCD Layer Initialization */
	BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS);

	/* Select the LCD Layer */
	BSP_LCD_SelectLayer(1);

	/* Enable the display */
	BSP_LCD_DisplayOn();
}

/**
 * @brief This function provides accurate delay (in milliseconds) based
 *        on SysTick counter flag.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @param Delay: specifies the delay time length, in milliseconds.
 * @retval None
 */

void HAL_Delay(__IO uint32_t Delay)
{
	while (Delay != 0)
	{
		if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
		{
			Delay--;
		}
	}
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 200000000
 *            HCLK(Hz)                       = 200000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 4
 *            APB2 Prescaler                 = 2
 *            HSE Frequency(Hz)              = 25000000
 *            PLL_M                          = 25
 *            PLL_N                          = 400
 *            PLL_P                          = 2
 *            PLLSAI_N                       = 384
 *            PLLSAI_P                       = 8
 *            VDD(V)                         = 3.3
 *            Main regulator output voltage  = Scale1 mode
 *            Flash Latency(WS)              = 7
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 400;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 8;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/* Activate the OverDrive to reach the 200 Mhz Frequency */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK)
	{
		Error_Handler();
	}

	/* Select PLLSAI output as USB clock source */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
	PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
	PeriphClkInitStruct.PLLSAI.PLLSAIQ = 4;
	PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV4;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
static void Error_Handler(void)
{
	/* User may add here some code to deal with this error */
	while (1)
	{
	}
}

/**
 * @brief  CPU L1-Cache enable.
 * @param  None
 * @retval None
 */
static void CPU_CACHE_Enable(void)
{
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
