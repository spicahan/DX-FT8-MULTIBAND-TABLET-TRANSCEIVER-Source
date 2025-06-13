// host_mocks.c
// Mocks for STM32 HAL, peripheral inits, and radio/DSP hooks


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include "main.h"
#include "host_mocks.h"
#include "decode_ft8.h"
#include "autoseq_engine.h"
#include "ADIF.h"
#include "json_parser.h"

// From Process_DSP.h
#define ft8_msg_samples 91
// From Display.h
#define MESSAGE_SIZE 40

// default test json file name
// override it with ./main <test_file>
const char* test_data_file = "test_data.json";

// Forward declaration
void advance_mock_tick(uint32_t ms);
void init_mock_timing(void);
static void handle_beacon_changes(void);
static void handle_touch_events(void);

// TX_ON_EVEN now loaded from JSON config
int Tune_On; // 0 = Receive, 1 = Xmit Tune Signal
int Beacon_On = 0;
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
extern int decode_flag;

int QSO_Xmit_Touch;
int FT8_Touch_Flag;

char current_QSO_receive_message[40];
char current_Beacon_receive_message[40];
char current_Beacon_xmit_message[40];
char current_QSO_xmit_message[40];

// gen_ft8.c
int CQ_Mode_Index = 0;
int Free_Index = 0;
char Free_Text1[MESSAGE_SIZE];
char Free_Text2[MESSAGE_SIZE];

// FT8 timing variables from main.c 
extern uint32_t current_time, start_time, ft8_time;
extern int target_slot;

void I2S2_RX_ProcessBuffer(uint16_t offset) {
    static int frame_counter = 0;
    
    // Initialize timing on first call
    init_mock_timing();
    
    // Simulate frame processing (matches real SDR_Audio.c:146-150)
    if (++frame_counter == 4) {
        // process_FT8_FFT() would be called here in real code
		FT_8_counter++;
        frame_counter = 0;
    }
    
    if (xmit_flag == 1) {
		if (Xmit_DSP_counter % 80 == 1) {
			printf("t");
		}
    } else {
		if (frame_counter == 0 && FT_8_counter % 20 == 0) {
			printf("r");
		}
    }
    
    fflush(stdout);
    
    // Advance mock time by ~40ms (matches 32kHz sample rate, 1280 samples)
    advance_mock_tick(40);
    
	// if (FT_8_counter == ft8_msg_samples) { 
	decode_flag = (frame_counter == 0 && (FT_8_counter == 71 || FT_8_counter == 91));

	// Simulate real audio processing timing
    usleep(300); // 0.4ms so 100X faster
    
    // Handle beacon changes based on timing
    handle_beacon_changes();
    
    // Handle touch events based on timing
    handle_touch_events();
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

// Config will be loaded from JSON file
static char config_my_callsign[14] = "N6HAN";  // Default values
static char config_my_grid[7] = "CM87";
static char config_dx_callsign[14] = "AG6AQ";
static char config_dx_grid[7] = "CM97";

// gen_ft8.c
char Station_Call[10];
char Locator[5];
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
void Write_Log_Data(const char *ch) {
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
		new_decoded[i].sequence = Seq_RSL;
	}
}

void process_selected_Station(int stations_decoded, int TouchIndex)
{
	if (stations_decoded > 0 && TouchIndex <= stations_decoded)
	{
		strcpy(Target_Call, new_decoded[TouchIndex].call_from);
		strcpy(Target_Locator, new_decoded[TouchIndex].target_locator);
		// Target_RSL = new_decoded[TouchIndex].snr;
		target_slot = new_decoded[TouchIndex].slot ^ 1;
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

// Initialize FT8 timing for mock
static bool timing_initialized = false;
void init_mock_timing(void) {
    if (!timing_initialized) {
        start_time = HAL_GetTick();
        timing_initialized = true;
    }
}

// -----------------------------------------------------------------------------
// Test Data Management
static TestData test_data = {0};
static bool test_data_loaded = false;

// Helper function to handle beacon changes
static void handle_beacon_changes(void) {
    if (!test_data_loaded) {
        return;
    }
    int period_index = ft8_time / 30000;
    if (period_index < test_data.period_count)
    {
        TestPeriod *current_period = &test_data.periods[period_index];

        if (current_period->has_beacon_change)
        {
            uint32_t period_time_ms = ft8_time % 30000;   // Time within current 30s period
            float period_time_s = period_time_ms / 1000.0f; // Convert to seconds

            BeaconChange *bc = &current_period->beacon_change;

            // Apply beacon change if time has passed the offset
            if (period_time_s >= bc->time_offset && Beacon_On != bc->beacon_on)
            {
                printf("Setting Beacon_On to: %d\n", bc->beacon_on);
                Beacon_On = bc->beacon_on;
            }
        }
    }
}

// Helper function to handle touch events
static void handle_touch_events(void) {
    static int last_touch_slot = -1;
    
    if (!test_data_loaded) {
        return;
    }
    int period_index = ft8_time / 30000;


    if (period_index < test_data.period_count)
    {
        TestPeriod *current_period = &test_data.periods[period_index];

        if (current_period->has_touch_event && last_touch_slot != period_index)
        {
            uint32_t period_time_ms = ft8_time % 30000;   // Time within current 30s period
            float period_time_s = period_time_ms / 1000.0f; // Convert to seconds

            TouchEvent *te = &current_period->touch_event;

            // Apply touch event if time has passed the offset and we have messages
            if (period_time_s >= te->time_offset && current_period->message_count > 0)
            {
                int msg_index = te->message_index;
                if (msg_index >= 0 && msg_index < current_period->message_count)
                {
                    // Set the touch index and flag
                    FT_8_TouchIndex = msg_index;
                    FT8_Touch_Flag = 1;
                    last_touch_slot = period_index; // Prevent repeated triggers
                    printf("\nSimulating touch on message %d at time %.1fs...\n", msg_index, period_time_s);
                }
            }
        }
    }
}

// Convert TestMessage to Decode structure
static void convert_test_message_to_decode(const TestMessage* src, Decode* dst) {
    memset(dst, 0, sizeof(Decode));
    strncpy(dst->call_to, src->call_to, 13);
    strncpy(dst->call_from, src->call_from, 13);
    strncpy(dst->locator, src->locator, 6);
    strncpy(dst->target_locator, src->target_locator, 6);
    dst->snr = src->snr;
    dst->received_snr = src->received_snr;
    dst->sequence = (Sequence)src->sequence;
    dst->slot = src->slot;
    dst->freq_hz = src->freq_hz;
    dst->sync_score = src->sync_score;
    dst->RR73 = src->RR73;
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

// Enhanced SysTick & tick counter for FT8 timing
static uint32_t mock_tick = 0;
static uint32_t mock_tick_increment = 0;

// Called by I2S2_RX_ProcessBuffer to advance time
void advance_mock_tick(uint32_t ms) {
    mock_tick_increment += ms;
}

WEAK uint32_t HAL_GetTick(void) {
    return mock_tick + mock_tick_increment;
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

// Initialize test data and configuration
static void init_test_data(void) {
    if (!test_data_loaded) {
        // Try to load from JSON file using C++ parser
        if (load_test_data_json(test_data_file, &test_data)) {
            // Use loaded config
            strncpy(config_my_callsign, test_data.config.my_callsign, sizeof(config_my_callsign)-1);
            strncpy(config_my_grid, test_data.config.my_grid, sizeof(config_my_grid)-1);
            strncpy(config_dx_callsign, test_data.config.dx_callsign, sizeof(config_dx_callsign)-1);
            strncpy(config_dx_grid, test_data.config.dx_grid, sizeof(config_dx_grid)-1);
			target_slot = !test_data.config.tx_on_even;
        }
        
        // Set global variables
        strncpy(Station_Call, config_my_callsign, sizeof(Station_Call)-1);
        strncpy(Locator, config_my_grid, sizeof(Locator)-1);
        
        
        test_data_loaded = true;

		// init autoseq again
	    autoseq_init(Station_Call, Locator);
    }
}

// Replace the real ft8_decode() to feed our test data
WEAK int ft8_decode(void) {
    printf("d");
	int period_index = ft8_time / 30000;
    
    // Initialize test data on first call
    init_test_data();
    
    // If no test data loaded, return 0 (no messages)
    if (test_data.period_count == 0) {
        return 0;
    }
    
    // Handle TX_ON_EVEN logic
    bool tx_on_even = test_data.config.tx_on_even;
    
    if (slot_state != tx_on_even) {
        // printf("ERROR!! FT8_DECODE() CALLED IN WRONG SLOT!!\n");
        return 0;
    }
    
    
    // Check if period_index is within bounds (O(1) lookup)
    if (period_index >= test_data.period_count) {
        return -1;
    }
    
    // Direct array access - O(1) instead of O(N) search
    TestPeriod* current_period = &test_data.periods[period_index];
    
    // If no messages, return 0
    if (current_period->message_count == 0) {
        return 0;
    }
    
    // Copy messages to new_decoded array
    int num_decoded = 0;
    for (int i = 0; i < current_period->message_count && i < 25; i++) {
        convert_test_message_to_decode(&current_period->messages[i], &new_decoded[i]);
        // Set slot to current decode slot
        new_decoded[i].slot = slot_state;
        num_decoded++;
    }
    
    // Simulate decoding cost
    advance_mock_tick(300);
    return num_decoded;
}

// -----------------------------------------------------------------------------
// Stub for transmit trigger: dump queued messages to stdout
WEAK void setup_to_transmit_on_next_DSP_Flag(void) {
    printf("\n[=== TRANSMIT START ===]\n");
	printf("Transmitting: %s\n", queued_msg);
	Xmit_DSP_counter = 0;
	ft8_xmit_counter = 0;
	ft8_xmit_delay = 0;  // Start transmission immediately for testing
	xmit_flag = 1;
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
