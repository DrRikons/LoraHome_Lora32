# lora32_boiler_control
Based on https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md
## Configuration

The sensor node can be configured remotely by sending a specific packed binary payload over LoRa. The device listens for this configuration message for 5 seconds on startup (and every 10 sleep cycles).

### Configuration Payload Format

The configuration payload is a 22-byte binary structure (`ConfigPayload`):
-   `header`: 2 bytes (`"CF"`) to reject noise.
-   `targetMac`: 6 bytes. Target MAC address (or `FF:FF:FF:FF:FF:FF` for broadcast).
-   `networkKey`: 4 bytes. Shared secret key (`NETWORK_KEY`) to prevent unauthorized spoofing.
-   `sleepInterval`: 4 bytes. Time in seconds that the device will deep sleep.
-   `configVersion`: 1 byte. Version number for the configuration.
-   `isDevMode`: 1 byte. `1` to enable developer mode continuously without deep sleeping, `0` to disable.
-   `timeOffset`: 4 bytes. Seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC. A value of `0` ignores synchronization.

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. Configuration values survive this reset by utilizing the ESP32's RTC_NOINIT_ATTR memory section. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node that receives, decrypts (via AES-128-CTR), parses, displays data from the sensor nodes, and transmits remote configuration payloads back (with a brief turnaround delay to prevent preamble clipping, and TX interrupt suppression to avoid feedback loops). Connects to WiFi (hostname: `LoRa-Gateway`) to fetch real NTP time for accurate sensor clock synchronization. The local OLED display cycles between live sensor telemetry and gateway status (IP, clock, message count).
-   `Sensor`: The sensor node that reads the boiler temperature and sends it to the gateway.

The core logic files are located in `modes/Sensor/main.cpp` and `modes/GateWay/main.cpp` (a legacy `GateWay.ino` is also retained for Arduino IDE compatibility).

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
To maximize LoRa time-on-air efficiency and save battery, the sensor node transmits telemetry data as a tightly packed binary C++ `struct`. The size dynamically depends on the operating mode (15 bytes in normal mode, 26 bytes in Dev Mode). The payload is secured using AES-128-CTR encryption.

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
When running in `devMode` (GPIO13 pulled LOW), the sensor accurately simulates its full operational cycle without deep sleeping. This includes the "listen for configuration" window, which only opens on startup and every 10th cycle, just like in Operation Mode. The simulation loop is: Wake -> Transmit -> (Maybe) Receive -> Display -> Simulated Sleep (for exactly the configured sleep duration, mirroring deep sleep timer behavior).

*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs. Serial logging is managed by a manual `serialEnabled` flag, which is forced `true` in Dev Mode but can be toggled in Operation Mode for testing.*

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.