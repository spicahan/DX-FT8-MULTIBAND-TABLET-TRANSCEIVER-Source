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

// From Process_DSP.h
#define ft8_msg_samples 91
// From Display.h
#define MESSAGE_SIZE 40

// Forward declaration
void advance_mock_tick(uint32_t ms);
void init_mock_timing(void);

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
int decode_flag;

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
    
	if (FT_8_counter == ft8_msg_samples) { 
		decode_flag = 1;
	}
    
    // Simulate real audio processing timing
    usleep(400); // 0.4ms so 100X faster
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

// Initialize FT8 timing for mock
static bool timing_initialized = false;
void init_mock_timing(void) {
    if (!timing_initialized) {
        start_time = HAL_GetTick();
        timing_initialized = true;
    }
}

// -----------------------------------------------------------------------------
// JSON Test Data Loader
#define MAX_PERIODS 20
#define MAX_MESSAGES_PER_SLOT 10
#define MAX_JSON_SIZE 16384

typedef struct {
    char my_callsign[14];
    char my_grid[7];
    char dx_callsign[14];
    char dx_grid[7];
    bool tx_on_even;
    int beacon_on;
} TestConfig;

typedef struct {
    int message_count;
    Decode messages[MAX_MESSAGES_PER_SLOT];
} TestPeriod;

typedef struct {
    TestConfig config;
    int period_count;
    TestPeriod periods[MAX_PERIODS];
} TestData;

static TestData test_data = {0};
static bool test_data_loaded = false;

// Variable substitution helper - works with buffer and max length
static void substitute_variables(char* str, size_t max_len, const TestConfig* config) {
    char temp[64];
    char* pos;
    
    // Replace ${MY_CALLSIGN}
    while ((pos = strstr(str, "${MY_CALLSIGN}")) != NULL) {
        *pos = '\0'; // Temporarily terminate at variable position
        snprintf(temp, sizeof(temp), "%s%s%s", str, config->my_callsign, pos + strlen("${MY_CALLSIGN}"));
        strncpy(str, temp, max_len - 1);
        str[max_len - 1] = '\0';
    }
    
    // Replace ${MY_GRID}
    while ((pos = strstr(str, "${MY_GRID}")) != NULL) {
        *pos = '\0';
        snprintf(temp, sizeof(temp), "%s%s%s", str, config->my_grid, pos + strlen("${MY_GRID}"));
        strncpy(str, temp, max_len - 1);
        str[max_len - 1] = '\0';
    }
    
    // Replace ${DX_CALLSIGN}
    while ((pos = strstr(str, "${DX_CALLSIGN}")) != NULL) {
        *pos = '\0';
        snprintf(temp, sizeof(temp), "%s%s%s", str, config->dx_callsign, pos + strlen("${DX_CALLSIGN}"));
        strncpy(str, temp, max_len - 1);
        str[max_len - 1] = '\0';
    }
    
    // Replace ${DX_GRID}
    while ((pos = strstr(str, "${DX_GRID}")) != NULL) {
        *pos = '\0';
        snprintf(temp, sizeof(temp), "%s%s%s", str, config->dx_grid, pos + strlen("${DX_GRID}"));
        strncpy(str, temp, max_len - 1);
        str[max_len - 1] = '\0';
    }
}

// Simple JSON string extraction helper
static char* extract_string_value(const char* json, const char* key, char* output, size_t max_len) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    const char* start = strstr(json, search_key);
    if (!start) return NULL;
    
    start = strchr(start + strlen(search_key), '"');
    if (!start) return NULL;
    start++; // Skip opening quote
    
    const char* end = strchr(start, '"');
    if (!end) return NULL;
    
    size_t len = end - start;
    if (len >= max_len) len = max_len - 1;
    
    strncpy(output, start, len);
    output[len] = '\0';
    return output;
}

// Simple JSON integer extraction helper
static int extract_int_value(const char* json, const char* key) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    const char* start = strstr(json, search_key);
    if (!start) return 0;
    
    start += strlen(search_key);
    while (*start && (*start == ' ' || *start == '\t')) start++; // Skip whitespace
    
    return atoi(start);
}

// Simple JSON boolean extraction helper  
static bool extract_bool_value(const char* json, const char* key) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    const char* start = strstr(json, search_key);
    if (!start) return false;
    
    start += strlen(search_key);
    while (*start && (*start == ' ' || *start == '\t')) start++; // Skip whitespace
    
    return (strncmp(start, "true", 4) == 0);
}

// Load and parse test data from JSON file
static bool load_test_data(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Could not open test data file '%s', using hardcoded data\n", filename);
        return false;
    }
    
    static char json_buffer[MAX_JSON_SIZE];
    size_t bytes_read = fread(json_buffer, 1, sizeof(json_buffer) - 1, file);
    fclose(file);
    
    if (bytes_read == 0) {
        printf("Warning: Empty test data file '%s'\n", filename);
        return false;
    }
    json_buffer[bytes_read] = '\0';
    
    // Parse config section
    const char* config_start = strstr(json_buffer, "\"config\":");
    if (config_start) {
        extract_string_value(config_start, "MY_CALLSIGN", test_data.config.my_callsign, 14);
        extract_string_value(config_start, "MY_GRID", test_data.config.my_grid, 7);
        extract_string_value(config_start, "DX_CALLSIGN", test_data.config.dx_callsign, 14);
        extract_string_value(config_start, "DX_GRID", test_data.config.dx_grid, 7);
        test_data.config.tx_on_even = extract_bool_value(config_start, "TX_ON_EVEN");
        test_data.config.beacon_on = extract_int_value(config_start, "Beacon_On");
    }
    
    // Parse periods array
    const char* periods_start = strstr(json_buffer, "\"periods\":");
    if (periods_start) {
        periods_start = strchr(periods_start, '[');
        if (periods_start) {
            const char* period_pos = periods_start + 1;
            test_data.period_count = 0;
            
            while (test_data.period_count < MAX_PERIODS && period_pos) {
                // Find next period object
                period_pos = strchr(period_pos, '{');
                if (!period_pos) break;
                
                // Find end of this period object
                const char* period_end = period_pos;
                int brace_count = 0;
                do {
                    if (*period_end == '{') brace_count++;
                    else if (*period_end == '}') brace_count--;
                    period_end++;
                } while (brace_count > 0 && *period_end);
                
                // Extract period data
                TestPeriod* period = &test_data.periods[test_data.period_count];
                period->message_count = 0;
                
                // Parse messages array in this period
                const char* messages_start = strstr(period_pos, "\"messages\":");
                if (messages_start && messages_start < period_end) {
                    messages_start = strchr(messages_start, '[');
                    if (messages_start && messages_start < period_end) {
                        const char* msg_pos = messages_start + 1;
                        
                        while (period->message_count < MAX_MESSAGES_PER_SLOT && msg_pos < period_end) {
                            msg_pos = strchr(msg_pos, '{');
                            if (!msg_pos || msg_pos >= period_end) break;
                            
                            // Find end of this message object
                            const char* msg_end = msg_pos;
                            int msg_brace_count = 0;
                            do {
                                if (*msg_end == '{') msg_brace_count++;
                                else if (*msg_end == '}') msg_brace_count--;
                                msg_end++;
                            } while (msg_brace_count > 0 && *msg_end && msg_end < period_end);
                            
                            // Extract message data using temporary buffers for variable substitution
                            Decode* msg = &period->messages[period->message_count];
                            memset(msg, 0, sizeof(Decode));
                            
                            char temp_str[64];
                            
                            // Extract and substitute call_to
                            if (extract_string_value(msg_pos, "call_to", temp_str, sizeof(temp_str))) {
                                substitute_variables(temp_str, sizeof(temp_str), &test_data.config);
                                strncpy(msg->call_to, temp_str, 13);
                                msg->call_to[13] = '\0';
                            }
                            
                            // Extract and substitute call_from
                            if (extract_string_value(msg_pos, "call_from", temp_str, sizeof(temp_str))) {
                                substitute_variables(temp_str, sizeof(temp_str), &test_data.config);
                                strncpy(msg->call_from, temp_str, 13);
                                msg->call_from[13] = '\0';
                            }
                            
                            // Extract and substitute locator
                            if (extract_string_value(msg_pos, "locator", temp_str, sizeof(temp_str))) {
                                substitute_variables(temp_str, sizeof(temp_str), &test_data.config);
                                strncpy(msg->locator, temp_str, 6);
                                msg->locator[6] = '\0';
                            }
                            
                            msg->snr = extract_int_value(msg_pos, "snr");
                            msg->received_snr = extract_int_value(msg_pos, "received_snr");
                            msg->sequence = (Sequence)extract_int_value(msg_pos, "sequence");
                            
                            period->message_count++;
                            msg_pos = msg_end;
                        }
                    }
                }
                
                test_data.period_count++;
                period_pos = period_end;
            }
        }
    }
    
    printf("Loaded test data: %d periods, config: %s/%s\n", 
           test_data.period_count, test_data.config.my_callsign, test_data.config.my_grid);
    return true;
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
        // Try to load from JSON file, fall back to defaults if it fails
        if (load_test_data("test_data.json")) {
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
        
        // Initialize target_slot for beacon mode  
        target_slot = 0; // Default to slot 0 for testing
        
        test_data_loaded = true;

		// init autoseq again
	    autoseq_init(Station_Call, Locator);
    }
}

// Replace the real ft8_decode() to feed our test data
WEAK int ft8_decode(void) {
    static int slot_index = 0;
    
    // Initialize test data on first call
    init_test_data();

	// Inject Beacon_On = 1 at the correct slot
	if (test_data.config.beacon_on) {
		if (slot_state == !test_data.config.tx_on_even) {
            Beacon_On = 1;
		}
	}
    
    // If no test data loaded, return 0 (no messages)
    if (test_data.period_count == 0) {
        return 0;
    }
    
    // Handle TX_ON_EVEN logic
    bool tx_on_even = test_data.config.tx_on_even;
    
    if (slot_state == tx_on_even) {
        return 0;
    }
    
    // Check if slot_index is within bounds (O(1) lookup)
    if (slot_index >= test_data.period_count) {
        return 0;
    }
    
    // Direct array access - O(1) instead of O(N) search
    TestPeriod* current_period = &test_data.periods[slot_index];
    
    slot_index++;
    
    // If no messages, return 0
    if (current_period->message_count == 0) {
        return 0;
    }
    
    // Copy messages to new_decoded array
    int num_decoded = 0;
    for (int i = 0; i < current_period->message_count && i < 25; i++) {
        new_decoded[i] = current_period->messages[i];
        // Set slot to current decode slot
        new_decoded[i].slot = !tx_on_even;
        num_decoded++;
    }
    
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
