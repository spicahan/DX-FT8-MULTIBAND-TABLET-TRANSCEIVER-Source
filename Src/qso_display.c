#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "decode_ft8.h"   // for Decode
#include "gen_ft8.h"      // for Station_Call
#include "main.h"         // for was_txing
#include "qso_display.h"

#ifdef HOST_HAL_MOCK
#include "host_mocks.h"
#else
#include "button.h"
#include "fonts.h"
#include "stm32746g_discovery_lcd.h"
#include "DS3231.h"
#endif

#define MAX_RX_ROWS 10
#define MAX_QSO_ROWS 10
#define MAX_QSO_ENTRIES 100
#define START_X_LEFT 0
#define START_X_RIGHT 240
#define START_Y 40 // FFT_H
#define LINE_HT 20

static const char *blank = "                     "; // 21 spaces
static char worked_qso_entries[MAX_QSO_ENTRIES][MAX_LINE_LEN] = {};
static int num_qsos = 0;

typedef enum _MsgColor
{
    Black = 0,
    White,
    Red,
    Green,
    Blue,
    Yellow,
    LastColor
} MsgColor;

const uint32_t lcd_color_map[LastColor] = {
    LCD_COLOR_BLACK,
    LCD_COLOR_WHITE,
    LCD_COLOR_RED,
    LCD_COLOR_GREEN,
    LCD_COLOR_BLUE,
    LCD_COLOR_YELLOW,
};

static void display_line(
    bool right,
    int line,
    MsgColor background,
    MsgColor textcolor,
    const char *text)
{
	BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(lcd_color_map[background]);
    BSP_LCD_SetTextColor(lcd_color_map[textcolor]);
    BSP_LCD_DisplayStringAt(
        right ? START_X_RIGHT : START_X_LEFT,
        START_Y + line * LINE_HT,
        (const uint8_t *)text,
        LEFT_MODE
    );
}

static void clear_rx_region()
{
    for (int i = 0; i < MAX_RX_ROWS; i++) {
        display_line(false, i, Black, Black, blank);
    }
}

static void clear_qso_region()
{
    for (int i = 0; i < MAX_QSO_ROWS; i++) {
        display_line(true, i, Black, Black, blank);
    }
}

void display_messages(Decode new_decoded[], int decoded_messages)
{
	clear_rx_region();

	for (int i = 0; i < decoded_messages && i < MAX_RX_ROWS; i++)
	{
		const char *call_to = new_decoded[i].call_to;
		const char *call_from = new_decoded[i].call_from;
		const char *locator = new_decoded[i].locator;

        char message[MAX_MSG_LEN];
		snprintf(message, MAX_LINE_LEN, "%s %s %s %2i", call_to, call_from, locator, new_decoded[i].snr);
        message[MAX_LINE_LEN] = '\0'; // Make sure it fits the display region
        MsgColor color = White;
		if (strcmp(call_to, "CQ") == 0 || strncmp(call_to, "CQ ", 3) == 0)
		{
			color = Green;
		}
		// Addressed me
		if (strncmp(call_to, Station_Call, CALLSIGN_SIZE) == 0)
		{
			color = Red;
		}
		// Mark own TX in yellow (WSJT-X)
		if (was_txing) {
			color = Yellow;
		}
        display_line(false, i, Black, color, message);
	}
}

void display_queued_message(const char* msg)
{
    display_line(true, 0, Black, Black, blank);
    display_line(true, 0, Black, Red, msg);
}

void display_txing_message(const char*msg)
{
    display_line(true, 0, Red, Black, blank);
    display_line(true, 0, Red, White, msg);
}

void display_qso_state(const char *txt)
{
    display_line(true, 1, Black, Black, blank);
    display_line(true, 1, Black, White, txt);
}

char * add_worked_qso() {
    // Handle circular buffer overflow - use modulo for array indexing
    int entry_index = num_qsos % MAX_QSO_ENTRIES;
    num_qsos++;
    // First 12 chars preserved for number, band, time
    return worked_qso_entries[entry_index] + 12;
}

bool display_worked_qsos()
{
    static const char band_strs[][4] = {
        "40M", "30M", "20M", "17M", "15M", "12M", "10M"
    };
    // Display in pages
    // pi is page index
    static int pi = 0;
    
    // Determine how many entries to show (max 100)
    int total_entries = num_qsos < MAX_QSO_ENTRIES ? num_qsos : MAX_QSO_ENTRIES;
    
    if (pi * MAX_QSO_ROWS > total_entries) {
        pi = 0;
        return false;
    }
    
    // Clear the entire log region first
    clear_qso_region();
    
    // Display the log in reverse order (most recent first)
    for (int ri = 0; ri < MAX_QSO_ROWS && (pi * MAX_QSO_ROWS + ri) < total_entries; ++ri)
    {
        // Calculate the QSO index in reverse chronological order
        int paging_offset = pi * MAX_QSO_ROWS + ri;
        int qso_index = num_qsos - 1 - paging_offset;
        
        // Get the actual array index using modulo for circular buffer
        int array_index = qso_index % MAX_QSO_ENTRIES;
        
        // Add 1-based number, HH:MM, band (11 chars + space)
        // Display number shows original sequence (like odometer)
        snprintf(worked_qso_entries[array_index], 12,
            "%02u %.3s %.4s",
            (qso_index + 1) % MAX_QSO_ENTRIES,
            band_strs[BandIndex],
            log_rtc_time_string
        );
        worked_qso_entries[array_index][11] = ' ';
        display_line(true, ri, Black, Green, worked_qso_entries[array_index]);
    }
    ++pi;
    return true;
}

// show debug text on LCD
void _debug(const char *txt) {
	return;
    display_line(true, 8, Black, Yellow, txt);
}
