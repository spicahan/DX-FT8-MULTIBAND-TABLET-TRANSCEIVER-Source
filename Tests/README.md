# FT8 Host-Side Unit Testing

This directory contains host-side mock implementations for unit testing the FT8 transceiver code on a development machine.

## Architecture

The test system uses a hybrid C/C++ architecture:
- **C files**: Compiled with `gcc` for compatibility with embedded code
- **C++ JSON parser**: Uses `nlohmann/json` library for robust JSON parsing
- **C interface**: Clean C API bridges C and C++ components

### Files

- `host_mocks.c` - Pure C mock implementations and test framework
- `json_parser.cpp` - C++ JSON parsing using nlohmann/json
- `json_parser.h` - C interface for JSON parser
- `test_data.json` - JSON test configuration and message data
- `Makefile` - Builds C and C++ components separately

## Enhanced JSON-Based Test Data

The test system supports loading FT8 decoded messages and configuration from external JSON files, making it easy to create different test scenarios without recompiling.

### JSON Test Data Format

Test data is loaded from `test_data.json` with the following structure:

#### Test Periods vs FT8 Slots

The JSON structure uses **test periods** which are sequential time intervals (0, 1, 2, 3...) that simulate the passage of FT8 transmission cycles. Each test period represents one 15-second FT8 interval during which messages can be received.

**Important distinction:**
- **Test periods**: Sequential indices (0, 1, 2, 3...) used to organize test data
- **FT8 slots**: Actual even/odd transmission timing determined by `TX_ON_EVEN` setting

When `TX_ON_EVEN` is `false`, test periods 0, 1, 2, 3... correspond to FT8 slots 0, 2, 4, 6... (even slots).
When `TX_ON_EVEN` is `true`, test periods 0, 1, 2, 3... correspond to FT8 slots 1, 3, 5, 7... (odd slots).

```json
{
  "config": {
    "MY_CALLSIGN": "N6HAN",
    "MY_GRID": "CM87",
    "DX_CALLSIGN": "AG6AQ", 
    "DX_GRID": "CM97",
    "TX_ON_EVEN": false
  },
  "periods": [
    {
      "messages": []
    },
    {
      "messages": [
        {
          "call_to": "N6HAN",
          "call_from": "AG6AQ",
          "locator": "CM97",
          "sequence": 1,
          "snr": 0,
          "received_snr": 5
        }
      ],
      "beacon_change": {
        "time_offset": 7.3,
        "beacon_on": 1
      },
      "touch_event": {
        "time_offset": 8.0,
        "message_index": 0
      }
    }
  ]
}
```

### Configuration Fields

- **MY_CALLSIGN**: Your station's call sign
- **MY_GRID**: Your station's grid locator  
- **DX_CALLSIGN**: Remote station call sign for test scenarios
- **DX_GRID**: Remote station grid locator
- **TX_ON_EVEN**: Whether the DX-FT8 transmissions happen on even-numbered FT8 slots

### Beacon State Changes

Each period can include a `beacon_change` object to dynamically control beacon mode:

- **time_offset**: Time in seconds (0.0-30.0) within the 30-second period when the change occurs
- **beacon_on**: New beacon state (1=enabled, 0=disabled)

This allows testing scenarios where beacon mode is toggled during operation, such as:
- Starting in receive mode, then enabling beacon at a specific time
- Disabling beacon after a successful QSO
- Testing timing-sensitive beacon behavior

### Touch Events

Each period can include a `touch_event` object to simulate user touch interactions:

- **time_offset**: Time in seconds (0.0-15.0) within the 15-second receive slot when the touch occurs
- **message_index**: Index of the decoded message to select (0-based)

This allows testing QSO scenarios where the user selects specific decoded messages:
- Responding to CQ calls at precise timing
- Testing autoseq engine response logic
- Simulating user interaction patterns

### Message Fields

Each message in a test period can contain:

- **call_to**: Destination call sign (often your station's call)
- **call_from**: Source call sign 
- **locator**: Message content (grid, signal report, or "73", "RR73")
- **sequence**: Message sequence type (1=Seq_Locator, 2=Seq_RSL)
- **snr**: Signal-to-noise ratio
- **received_snr**: Received SNR value

### Variable Substitution

String fields support variable substitution using `${VARIABLE_NAME}` syntax:

- **${MY_CALLSIGN}**: Replaced with the value from config.MY_CALLSIGN
- **${MY_GRID}**: Replaced with the value from config.MY_GRID  
- **${DX_CALLSIGN}**: Replaced with the value from config.DX_CALLSIGN
- **${DX_GRID}**: Replaced with the value from config.DX_GRID

Example:
```json
{
  "call_to": "${MY_CALLSIGN}",
  "call_from": "${DX_CALLSIGN}",
  "locator": "${DX_GRID}"
}
```

This allows you to change station call signs and grids in the config section and have them automatically applied throughout all message definitions.

## Building and Running

```bash
# Build the test (requires g++ with C++17 support)
make

# Run the test with default test_data.json
./host_test

# Run with custom test file
./host_test my_test_scenario.json

# Clean build artifacts
make clean
```

### Dependencies

- **GCC/G++**: C and C++17 compiler
- **nlohmann/json**: Header-only JSON library (included as `json.hpp`)

## Key Improvements

### Accurate FT8 Timing Simulation
- **40ms DSP intervals**: Matches real 32kHz sample rate
- **Frame counter**: Proper 0→1→2→3→0 cycling like real hardware
- **ft8_xmit_counter**: Correct progression (79 tones over ~12.64 seconds)
- **15-second periods**: Realistic FT8 slot timing and synchronization

### Flexible Test Scenarios
- **JSON-configurable**: Easy to create different test scenarios
- **Multiple messages per period**: Test complex decode situations
- **Configurable station info**: Test different call signs and grids
- **TX timing control**: Configure even/odd FT8 slot transmission patterns

### Enhanced Autoseq Testing
- **QSO progression**: Test complete QSO sequences
- **ADIF logging**: Verify log entry generation
- **Beacon mode**: Test CQ beacon functionality
- **Touch simulation**: Automated station selection

## Example Test Scenarios

You can create different `test_data.json` files for various scenarios:

1. **Dynamic beacon control**: Use `beacon_change` to enable/disable beacon at specific times
2. **Beacon startup delay**: Start in receive mode, enable beacon after a few seconds
3. **QSO interruption**: Disable beacon when a QSO is initiated
4. **Lost messages**: Simulating DX's response gets lost
5. **Repeated messages**: Simulating DX can't receive our response
6. **Multiple responses**: Simulating multiple DXes responding to us
7. **Timing-sensitive tests**: Beacon changes at precise moments within periods

### Example: QSO Simulation with Touch Events

```json
{
  "config": {
    "MY_CALLSIGN": "N6HAN",
    "MY_GRID": "CM87",
    "DX_CALLSIGN": "AG6AQ",
    "DX_GRID": "CM97",
    "TX_ON_EVEN": true
  },
  "periods": [
    {
      "messages": []
    },
    {
      "messages": [
        {
          "call_to": "CQ",
          "call_from": "${DX_CALLSIGN}",
          "locator": "${DX_GRID}",
          "sequence": 1,
          "snr": -10
        }
      ],
      "touch_event": {
        "time_offset": 8.0,
        "message_index": 0
      }
    },
    {
      "messages": [
        {
          "call_to": "${MY_CALLSIGN}",
          "call_from": "${DX_CALLSIGN}",
          "locator": "-3",
          "sequence": 2,
          "snr": -12
        }
      ]
    }
  ]
}
```

## Fallback Behavior

If `test_data.json` is not found or cannot be parsed, the system falls back to hardcoded default values and prints a warning message.