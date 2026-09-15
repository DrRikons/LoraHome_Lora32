# lora32_boiler_control
Based on https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md

For a complete firmware walkthrough, separate Sensor/Gateway architecture diagrams, and LoRa message flows, see [FIRMWARE_GUIDE.md](FIRMWARE_GUIDE.md).
## Configuration
The sensor node is configured remotely over a two-step LoRa downlink. To conserve power, the sensor listens for a configuration beacon once every 10 sleep cycles, or if its internal clock is not synchronized. It opens the beacon RX window immediately after the telemetry uplink; the Gateway waits 50 ms for this RX turnaround before transmitting the 1-byte beacon. Only if that beacon is received does the Sensor keep its radio on to listen for the full configuration payload. The reception logic dynamically allocates buffer space based on reported packet length to prevent false-negative evaluations or FIFO lockups during RX.

### Configuration Payload Format

The configuration payload is a 19-byte binary structure (`ConfigPayload`):
-   `header`: 2 bytes (`"CF"`) to reject noise.
-   `targetMac`: 6 bytes. Target MAC address (or `FF:FF:FF:FF:FF:FF` for broadcast).
-   `networkKey`: 4 bytes. Shared secret key (`NETWORK_KEY`) to prevent unauthorized spoofing.
-   `sleepInterval`: 1 byte. Time in seconds that the device will deep sleep.
-   `configVersion`: 1 byte. Current configuration schema version (`1`).
-   `isDevMode`: 1 byte. `1` to enable developer mode continuously without deep sleeping, `0` to disable.
-   `timeOffset`: 4 bytes. UTC seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC. A value of `0` ignores synchronization.

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. Configuration values survive this reset by utilizing the ESP32's RTC_NOINIT_ATTR memory section. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node receives, decrypts, parses, and displays sensor data; it prioritizes the beacon/config reply before slower MQTT work. Telemetry AES-CTR nonces include each sensor MAC, a random boot nonce, and the message counter.
-  **Logging & MQTT**: The gateway uses asynchronous logging via the `Elog` library with standard severity levels and automatically prepends real-world formatted timestamps when the system clock is synchronized. Log writes are dynamically redirected and queued via a background FreeRTOS task to append seamlessly to the SD card without causing any latency during critical radio interrupts. SD card log files are configured to rotate automatically every 100KB (102400 bytes) via the `registerSd` size limit parameter to prevent filesystem overflow. Sent MQTT payloads are also automatically injected with an ISO-8601 formatted `timestamp` property once the NTP validates.
Received sensor data is published securely over TLS as JSON payloads to separate topics. Gateway status is published to `lorahome/gateway/status`. Sensor data is published to `lorahome/sensor/<sensor_mac>/data` for operational mode, and `lorahome/sensor/<sensor_mac>/telemetry` for development mode.
Gateway status contains radio RSSI/SNR, WiFi state, IP address, and free RAM, and is published on every connected main-loop cycle.
Both valid telemetry packet sizes are published: 20-byte operation-mode payloads publish core data, while 31-byte development payloads also publish the development metrics.
-   `Sensor`: The sensor node that reads temperature and battery metrics, encrypts them, and sends the data to the gateway before entering a deep sleep state.

The core logic files are located in `modes/Sensor/main.cpp` and `modes/GateWay/main.cpp` (a legacy `GateWay.ino` is also retained for Arduino IDE compatibility).

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
To maximize LoRa time-on-air efficiency, the sensor transmits telemetry as a packed binary C++ `struct`. The `TelemetryPayload` size is dynamic: 20 bytes in normal Operation Mode and 31 bytes in Dev Mode (which includes extra debugging metrics). Its public MAC, random boot nonce, and message counter form the AES-CTR nonce; the remainder is encrypted.
*(Note: The `TelemetryPayload` and `ConfigPayload` structs are defined in `payloads.h` for reusability across Gateway and Sensor modes. Any floating point values like temperature are multiplied before transmission. The Gateway divides them upon receipt to restore the decimal values).*

*Note: SNR and RSSI metrics represent the signal quality of the last received configuration packet from the gateway.*

## Automation
This repository includes a GitHub Action (`issue-commenter.yml`) that automatically posts a comment to any GitHub Issue referenced in a commit message (e.g., `#123`).

## Development
When running in `devMode` (either via the `DEV_MODE_PIN` or remote configuration), the sensor accurately simulates its full operational cycle without entering deep sleep. The simulation loop is: Wake -> Transmit -> Short RX for beacon -> (Optional) Long RX for config -> Update Display -> Pause (for the configured sleep duration). This allows for comprehensive testing of the entire device lifecycle without requiring physical resets.
*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs. Serial logging is managed by a manual `serialEnabled` flag, which is forced `true` in Dev Mode but can be toggled in Operation Mode for testing.*

Sensor serial diagnostics are prefixed with UTC after time synchronization, or elapsed boot time while the RTC is unsynchronized.

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.

## Recent Updates
- Fixed a bug in the Gateway's `isClockValid` function where the unsynced RTC time check used incorrect math (adding uptime instead of subtracting it). The gateway will now correctly invalidate the clock if it is free-running from `CUSTOM_EPOCH` without an actual NTP sync.
