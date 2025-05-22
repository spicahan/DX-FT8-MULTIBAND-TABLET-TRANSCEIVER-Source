// host_mocks.c
// Mocks for STM32 HAL, peripheral inits, and radio/DSP hooks
// Build it with this:
// gcc -DHOST_HAL_MOCK -g -Wall -I../Inc -I../FT8_library ../Src/main.c ../Src/autoseq_engine.c host_mocks.c


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "main.h"
#include "host_mocks.h"
#include "decode_ft8.h"
#include "autoseq_engine.h"
#include "ADIF.h"

#define TX_ON_EVEN false
int Tune_On; // 0 = Receive, 1 = Xmit Tune Signal
int Beacon_On = 1;
int Xmit_Mode;
int xmit_flag = 0, ft8_xmit_counter, ft8_xmit_flag, ft8_xmit_delay;
int DSP_Flag = 1;
double ft8_shift;
uint16_t buff_offset;
int ft8_flag, FT_8_counter, ft8_marker;

// Display.c
int FT_8_TouchIndex = 0;
void update_log_display(int mode) {}

uint16_t cursor;
char rtc_date_string[9];
char rtc_time_string[9];
int decode_flag;

int QSO_Xmit_Touch;
int FT8_Touch_Flag;

char current_QSO_receive_message[40];
char current_Beacon_receive_message[40];
char current_Beacon_xmit_message[40];
char current_QSO_xmit_message[40];

void I2S2_RX_ProcessBuffer(uint16_t offset) {
	if (xmit_flag == 1) {
		printf("t");
		ft8_xmit_counter += 26; // After 3x becomes 79
	} else {
		printf("r");
	}
    fflush(stdout);
    static int count = 0;
    if (++count % 3 == 0) {
        decode_flag = 1;
    }
    // TODO mock the actual audio processing interval
    usleep(100000); // 100ms
}

void Process_Touch(void) {}

// constants.c
uint8_t tones[79];

// DS3231.c
void display_RealTime(int x, int y) {}
void display_Real_Date(int x, int y) {}
void make_Real_Time(void) {};
void make_Real_Date(void) {};
char log_rtc_time_string[13] = "RTC_TIME";
char log_rtc_date_string[13] = "RTC_DATE";

#define MY_CALLSIGN "AG6AQ"
#define MY_GRID "CM97"
#define DX_CALLSIGN "N6ACA"
#define DX_GRID "CM97"

// gen_ft8.c
char Station_Call[10] = MY_CALLSIGN;
char Locator[5] = MY_GRID;
char Target_Call[10];	// seven character call sign (e.g. 3DA0XYZ) + optional /P + null terminator
char Target_Locator[5]; // four character locator  + null terminator
int Target_RSL;
int Station_RSL;
void clear_queued_message(void) {}

// button.c
int BandIndex = 2; // 20M
int Logging_State = 1;
const FreqStruct sBand_Data[NumBands] =
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
void receive_sequence(void) {}

// log_file.c
void Write_Log_Data(char *ch) {
	printf("ADIF log: %s\n", ch);
}

// traffic_manager.c
void ft8_receive_sequence(void) {}
void set_FT8_Tone(uint8_t ft8_tone) {}

// decode_ft8.c
const int kMax_decoded_messages = 20;
static char call_blank[8];
static uint8_t call_initialised = 0;
static char locator_blank[5];
static uint8_t locator_initialised = 0;
static int message_limit = 10;
Decode new_decoded[25];

void string_init(char *string, int size, uint8_t *is_initialised, char character)
{
	if (*is_initialised != size)
	{
		for (int i = 0; i < size - 1; i++)
		{
			string[i] = character;
		}
		string[size - 1] = 0;
		*is_initialised = size;
	}
}

void display_messages(int decoded_messages)
{
	const char CQ[] = "CQ";
	const char *paramlist = "%s %s %s";

	for (int i = 0; i < decoded_messages && i < message_limit; i++)
	{
        printf("\nDecoded: ");
		const char *call_to = new_decoded[i].call_to;
		const char *call_from = new_decoded[i].call_from;
		const char *locator = new_decoded[i].locator;

		if (strcmp(CQ, call_to) == 0)
		{
			if (strcmp(Station_Call, call_from) != 0)
			{
				printf("%s %s %s %2i", call_to, call_from, locator, new_decoded[i].snr);
			}
			else
			{
				printf(paramlist, call_to, call_from, locator);
			}
		}
		else
		{
			printf(paramlist, call_to, call_from, locator);
		}
	}
    printf("\n");
}

void clear_decoded_messages(void)
{
	string_init(call_blank, sizeof(call_blank), &call_initialised, ' ');
	string_init(locator_blank, sizeof(locator_blank), &locator_initialised, ' ');
	for (int i = 0; i < kMax_decoded_messages; i++)
	{
		strcpy(new_decoded[i].call_to, call_blank);
		strcpy(new_decoded[i].call_from, call_blank);
		strcpy(new_decoded[i].locator, locator_blank);
		strcpy(new_decoded[i].target_locator, locator_blank);
		new_decoded[i].freq_hz = 0;
		new_decoded[i].sync_score = 0;
		new_decoded[i].received_snr = 99;
		new_decoded[i].slot = 0;
		new_decoded[i].RR73 = 0;
		new_decoded[i].sequence = 0;
	}
}

void process_selected_Station(int stations_decoded, int TouchIndex)
{
	if (stations_decoded > 0 && TouchIndex <= stations_decoded)
	{
		strcpy(Target_Call, new_decoded[TouchIndex].call_from);
		strcpy(Target_Locator, new_decoded[TouchIndex].target_locator);
		// Target_RSL = new_decoded[TouchIndex].snr;
		target_slot = new_decoded[TouchIndex].slot;
		target_freq = new_decoded[TouchIndex].freq_hz;

		// if (QSO_Fix == 1)
		// 	set_QSO_Xmit_Freq(target_freq);

		// compose_messages();
		// Auto_QSO_State = 1;
		// RSL_sent = 0;
		// RR73_sent = 0;
	}

	FT8_Touch_Flag = 0;
}

// Needed by autoseq_engine
static char queued_msg[40];
void queue_custom_text(const char *tx_msg) {
	strncpy(queued_msg, tx_msg, sizeof(queued_msg));
}

// SiLabs.c
void output_enable(enum si5351_clock clk, uint8_t enable) {}

// -----------------------------------------------------------------------------
// STM32 HAL stubs (weak definitions override any real HAL code)
#define WEAK __attribute__((weak))

WEAK HAL_StatusTypeDef HAL_Init(void) {return HAL_OK;}
WEAK void SystemClock_Config(void) {}
WEAK void MX_GPIO_Init(void) {}
WEAK void MX_CRC_Init(void) {}
WEAK void MX_I2C1_Init(void) {}
WEAK void MX_QUADSPI_Init(void) {}
WEAK void MX_RTC_Init(void) {}
WEAK void MX_FMC_Init(void) {}
// Add other MX_*_Init() as needed

// Simple SysTick & tick counter
static uint32_t mock_tick = 0;

WEAK uint32_t HAL_GetTick(void) {
    return mock_tick;
}

WEAK void HAL_Delay(uint32_t ms) {
    mock_tick += ms;
}

// -----------------------------------------------------------------------------
// Stub Display & LCD routines so we see output on stdout instead
WEAK void BSP_LCD_Init(void) { printf("[LCD Init]\n"); }
WEAK void BSP_LCD_Clear(uint32_t color) { printf("[LCD Clear]\n"); }
WEAK void BSP_LCD_DisplayStringAtLine(uint16_t Line, uint8_t *ptr) {
    printf("[LCD Line %d] %s\n", Line, ptr);
}
// Add other LCD_ / Display_ stubs as needed

// Inject 2 decoded messages each time
static const Decode injected[][2] = {
	{},
	{},
#if TX_ON_EVEN
	{},
#endif
	// {.call_to = "CQ", .call_from = DX_CALLSIGN, .locator = DX_GRID, .sequence = Seq_Locator},
	{
	{.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = DX_GRID, .sequence = Seq_Locator},
	{.call_to = MY_CALLSIGN, .call_from = "N6LN", .locator = "DM03", .sequence = Seq_Locator},
	},
	// {.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = "8", .sequence = Seq_RSL},
	{
	{.call_to = MY_CALLSIGN, .call_from = "N6LN", .locator = "DM03", .sequence = Seq_Locator},
	{},
    },
	// {.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = "RR73", .sequence = Seq_RSL},
	{
	{.call_to = MY_CALLSIGN, .call_from = "N6LN", .locator = "DM03", .sequence = Seq_Locator},
	{},
	// {.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = "73", .sequence = Seq_RSL},
	},
	{
	{.call_to = MY_CALLSIGN, .call_from = "N6LN", .locator = "DM03", .sequence = Seq_Locator},
	{.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = "R-3", .sequence = Seq_RSL},
	},
	{
	{.call_to = MY_CALLSIGN, .call_from = "N6LN", .locator = "DM03", .sequence = Seq_Locator},
	{.call_to = MY_CALLSIGN, .call_from = DX_CALLSIGN, .locator = "73", .sequence = Seq_RSL},
	},
};
const size_t num_injected = sizeof(injected) / sizeof(Decode) / 2;

// Replace the real ft8_decode() to feed our injected array
WEAK int ft8_decode(void) {
    static int i = 0;
	// DX always sends in even slots, so decoding is always in odd slots
	if (i < (1 + TX_ON_EVEN)) {
		++i;
		return 0;
	}
	if (slot_state == !TX_ON_EVEN) {
		return 0;
	}

    if (i >= num_injected) {
        return 0;
    }
    new_decoded[0] = injected[i][0];
    new_decoded[1] = injected[i][1];
	// DX always sends in even slots, so decoding is always in odd slots
	new_decoded[0].slot = !TX_ON_EVEN;
	new_decoded[1].slot = !TX_ON_EVEN;
	int num_decoded = 0;
	num_decoded += new_decoded[0].call_to[0] != '\0';
	num_decoded += new_decoded[1].call_to[0] != '\0';
	++i;

    return num_decoded;
}

// -----------------------------------------------------------------------------
// Stub for transmit trigger: dump queued messages to stdout
WEAK void setup_to_transmit_on_next_DSP_Flag(void) {
    printf("\n[=== TRANSMIT START ===]\n");
	printf("Transmitting: %s\n", queued_msg);
	Xmit_DSP_counter = 0;
	ft8_xmit_counter = 0;
	ft8_xmit_delay = 28;
	// xmit_sequence();
	// ft8_transmit_sequence();
	xmit_flag = 1;
	// Xmit_DSP_counter = 0;
}

void _debug(const char *txt) {
	printf("\nDEBUG: %s\n", txt);
}

// -----------------------------------------------------------------------------
// Optional: stub or disable any audio / codec / Si5351 dependencies
WEAK void Audio_Init(void) {}
WEAK void Codec_Start(void) {}
// etc.

// -----------------------------------------------------------------------------
// Hook the main loop's timing: we advance mock_tick there too
// If main() uses HAL_Delay or tick, we're covered.


// End of host_mocks.c
