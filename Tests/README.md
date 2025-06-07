# FT8 Host-Side Unit Testing

This directory contains host-side mock implementations for unit testing the FT8 transceiver code on a development machine.

## Enhanced JSON-Based Test Data

The test system now supports loading FT8 decoded messages and configuration from external JSON files, making it easy to create different test scenarios without recompiling.

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
    "TX_ON_EVEN": false,
    "Beacon_On": 1
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
      ]
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
- **Beacon_On**: Enable beacon mode (1=on, 0=off)

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
# Build the test
make

# Run the test  
./host_test

# Clean build artifacts
make clean
```

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

1. **Beacon_On: 1**: Simulating we calling CQ
2. **Beacon_On: 0**: Simulating we responding to CQ
3. **Lost messages**: Simulating DX's response get lost
4. **Repeated messages**: Simulating DX can't receive our response
5. **Multiple response**: Simulating multiple DX'es reponding to us

## Fallback Behavior

If `test_data.json` is not found or cannot be parsed, the system falls back to hardcoded default values and prints a warning message.