#include "json_parser.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>

using json = nlohmann::json;

// Helper function to substitute variables in strings
static void substitute_variables(std::string& str, const TestConfig& config) {
    // Replace ${MY_CALLSIGN}
    size_t pos = 0;
    while ((pos = str.find("${MY_CALLSIGN}", pos)) != std::string::npos) {
        str.replace(pos, 14, config.my_callsign);
        pos += strlen(config.my_callsign);
    }
    
    // Replace ${MY_GRID}
    pos = 0;
    while ((pos = str.find("${MY_GRID}", pos)) != std::string::npos) {
        str.replace(pos, 10, config.my_grid);
        pos += strlen(config.my_grid);
    }
    
    // Replace ${DX_CALLSIGN}
    pos = 0;
    while ((pos = str.find("${DX_CALLSIGN}", pos)) != std::string::npos) {
        str.replace(pos, 14, config.dx_callsign);
        pos += strlen(config.dx_callsign);
    }
    
    // Replace ${DX_GRID}
    pos = 0;
    while ((pos = str.find("${DX_GRID}", pos)) != std::string::npos) {
        str.replace(pos, 10, config.dx_grid);
        pos += strlen(config.dx_grid);
    }
}

extern "C" int load_test_data_json(const char* filename, TestData* test_data) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open test data file '" << filename << "'" << std::endl;
            return 0;
        }
        
        json j;
        file >> j;
        
        // Parse config
        if (j.contains("config")) {
            auto& config_json = j["config"];
            strncpy(test_data->config.my_callsign, config_json.value("MY_CALLSIGN", "").c_str(), 13);
            test_data->config.my_callsign[13] = '\0';
            
            strncpy(test_data->config.my_grid, config_json.value("MY_GRID", "").c_str(), 6);
            test_data->config.my_grid[6] = '\0';
            
            strncpy(test_data->config.dx_callsign, config_json.value("DX_CALLSIGN", "").c_str(), 13);
            test_data->config.dx_callsign[13] = '\0';
            
            strncpy(test_data->config.dx_grid, config_json.value("DX_GRID", "").c_str(), 6);
            test_data->config.dx_grid[6] = '\0';
            
            test_data->config.tx_on_even = config_json.value("TX_ON_EVEN", false) ? 1 : 0;
        }
        
        // Parse periods
        if (j.contains("periods")) {
            auto& periods_json = j["periods"];
            test_data->period_count = periods_json.size();
            test_data->periods = new TestPeriod[test_data->period_count];
            
            for (int i = 0; i < test_data->period_count; i++) {
                auto& period_json = periods_json[i];
                TestPeriod& period = test_data->periods[i];
                
                // Parse messages
                if (period_json.contains("messages")) {
                    auto& messages_json = period_json["messages"];
                    period.message_count = messages_json.size();
                    
                    if (period.message_count > 0) {
                        period.messages = new TestMessage[period.message_count];
                        
                        for (int j = 0; j < period.message_count; j++) {
                            auto& msg_json = messages_json[j];
                            TestMessage& msg = period.messages[j];
                            
                            // Initialize message
                            memset(&msg, 0, sizeof(TestMessage));
                            
                            // Parse and substitute variables
                            std::string call_to = msg_json.value("call_to", "");
                            substitute_variables(call_to, test_data->config);
                            strncpy(msg.call_to, call_to.c_str(), 13);
                            msg.call_to[13] = '\0';
                            
                            std::string call_from = msg_json.value("call_from", "");
                            substitute_variables(call_from, test_data->config);
                            strncpy(msg.call_from, call_from.c_str(), 13);
                            msg.call_from[13] = '\0';
                            
                            std::string locator = msg_json.value("locator", "");
                            substitute_variables(locator, test_data->config);
                            strncpy(msg.locator, locator.c_str(), 6);
                            msg.locator[6] = '\0';
                            
                            std::string target_locator = msg_json.value("target_locator", "");
                            substitute_variables(target_locator, test_data->config);
                            strncpy(msg.target_locator, target_locator.c_str(), 6);
                            msg.target_locator[6] = '\0';
                            
                            msg.snr = msg_json.value("snr", 0);
                            msg.received_snr = msg_json.value("received_snr", 99);
                            msg.sequence = msg_json.value("sequence", 0);
                            msg.slot = 0;  // Will be set during decode
                            msg.freq_hz = 0;
                            msg.sync_score = 0;
                        }
                    } else {
                        period.messages = nullptr;
                    }
                }
                
                // Parse beacon_change
                period.has_beacon_change = 0;
                if (period_json.contains("beacon_change")) {
                    auto& bc_json = period_json["beacon_change"];
                    period.has_beacon_change = 1;
                    period.beacon_change.time_offset = bc_json.value("time_offset", 0.0f);
                    period.beacon_change.beacon_on = bc_json.value("beacon_on", 0);
                }
                
                // Parse touch_event
                period.has_touch_event = 0;
                if (period_json.contains("touch_event")) {
                    auto& te_json = period_json["touch_event"];
                    period.has_touch_event = 1;
                    period.touch_event.time_offset = te_json.value("time_offset", 0.0f);
                    period.touch_event.message_index = te_json.value("message_index", 0);
                }
            }
        }
        
        std::cout << "Loaded test data: " << test_data->period_count << " periods, config: " 
                  << test_data->config.my_callsign << "/" << test_data->config.my_grid << std::endl;
        
        return 1; // Success
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return 0; // Failure
    }
}

extern "C" void free_test_data(TestData* test_data) {
    if (test_data->periods) {
        for (int i = 0; i < test_data->period_count; i++) {
            if (test_data->periods[i].messages) {
                delete[] test_data->periods[i].messages;
            }
        }
        delete[] test_data->periods;
        test_data->periods = nullptr;
    }
    test_data->period_count = 0;
}