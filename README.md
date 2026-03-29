# lora32_boiler_control

## Configuration

The sensor node can be configured remotely by sending a specific string over LoRa. The device listens for this configuration message for 5 seconds on startup (and every 10 sleep cycles).

### Configuration String Format

The configuration string must follow this format:

`CONFIG:<sleepInterval>,<version>[,<devMode>[,<timeOffset>]]`

-   `CONFIG:`: A required prefix.
-   `<sleepInterval>`: The time in seconds that the device will deep sleep between sensor readings. This must be an integer value.
-   `<version>`: A version number for the configuration. This must be an integer value.
-   `<devMode>`: *(Optional)* 1 to enable developer mode continuously without deep sleeping, 0 to disable.
-   `<timeOffset>`: *(Optional)* Seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC.

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

**Examples:**

- Set sleep interval to 300 seconds, version 2: `CONFIG:300,2`
- Turn on continuous Dev Mode for debugging: `CONFIG:60,3,1`
- Sync time to March 22 2024 (7000000 seconds offset): `CONFIG:60,3,1,7000000`

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node that receives data from the sensor nodes.
-   `Sensor`: The sensor node that reads the boiler temperature and sends it to the gateway.

The main application file for the sensor node is `modes/Sensor/main.cpp`.

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
To maximize LoRa time-on-air efficiency and save battery, the sensor node transmits telemetry data as a tightly packed binary C++ `struct`. The size dynamically depends on the operating mode (14 bytes in normal mode, 25 bytes in Dev Mode). The payload is secured using AES-128-CTR encryption.
The size dynamically depends on the operating mode (15 bytes in normal mode, 26 bytes in Dev Mode). The payload is secured using AES-128-CTR encryption.

```cpp
struct __attribute__((packed)) TelemetryPayload {
  // --- Core variables (15 bytes, always sent) ---
  uint8_t  mac;       // 6 bytes: Raw MAC address
  uint16_t msgCount;     // 2 bytes: Message counter (Used as AES IV)
  int16_t  temperature;  // 2 bytes: External Temp (x 100)
  uint16_t battVoltage;  // 2 bytes: Battery Voltage in mV (x 1000)
  uint8_t  battPercent;  // 1 byte:  Battery capacity (0-100%)
  uint8_t  configVer;    // 1 byte:  Config version
  uint8_t  opMode;       // 1 byte:  0=Op, 1=Dev
  
  // --- The following fields are ONLY sent in Dev Mode (length == 26) ---
  int16_t  battCurrent;  // 2 bytes: Battery Current in mA
  int16_t  battPower;    // 2 bytes: Battery Power in mW
  uint16_t freeRam;      // 2 bytes: Free RAM in KB
  int8_t   cpuTemp;      // 1 byte:  CPU Temp in C
  int8_t   txPower;      // 1 byte:  TX Power in dBm
  int8_t   lastSNR;      // 1 byte:  Last received SNR
  int16_t  lastRSSI;     // 2 bytes: Last received RSSI
};
```
*(Note: Any floating point values like temperature are multiplied before transmission. The Gateway should divide them upon receipt to restore the decimal values).*

*Note: SNR and RSSI metrics represent the signal quality of the last received configuration packet from the gateway.*

## Automation
This repository includes a GitHub Action (`issue-commenter.yml`) that automatically posts a comment to any GitHub Issue referenced in a commit message (e.g., `#123`).

## Development
When running in `devMode` (GPIO13 pulled LOW), the sensor will continuously simulate its full operational cycle without deep sleeping: Wake -> Transmit -> Receive -> Display (cycling through all OLED screens, including total TX/RX power-on times) -> Simulated Sleep (OLED clears).

*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs.*

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.