#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#ifdef __cplusplus
extern "C" {
#endif

// C interface for JSON parser
typedef struct {
    char my_callsign[14];
    char my_grid[7];
    char dx_callsign[14];
    char dx_grid[7];
    int tx_on_even;  // 0 = false, 1 = true
} TestConfig;

typedef struct {
    float time_offset;
    int beacon_on;
} BeaconChange;

typedef struct {
    float time_offset;
    int message_index;
} TouchEvent;

typedef struct {
    char call_to[14];
    char call_from[14];
    char locator[7];
    char target_locator[7];
    int snr;
    int received_snr;
    int sequence;
    int slot;
    int freq_hz;
    int sync_score;
    int RR73;
} TestMessage;

typedef struct {
    TestMessage* messages;
    int message_count;
    int has_beacon_change;  // 0 = false, 1 = true
    BeaconChange beacon_change;
    int has_touch_event;    // 0 = false, 1 = true
    TouchEvent touch_event;
} TestPeriod;

typedef struct {
    TestConfig config;
    TestPeriod* periods;
    int period_count;
} TestData;

// C interface functions
int load_test_data_json(const char* filename, TestData* test_data);
void free_test_data(TestData* test_data);

#ifdef __cplusplus
}
#endif

#endif // JSON_PARSER_HPP