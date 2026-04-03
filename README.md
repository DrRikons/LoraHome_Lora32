# lora32_boiler_control
Based on https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md
## Configuration
The sensor node is configured remotely over a two-step LoRa downlink. To conserve power, the sensor listens for a configuration beacon once every 10 sleep cycles, or if its internal clock is not synchronized. Immediately after the telemetry uplink is complete, it opens a short RX window, sized dynamically based on the beacon's airtime, to listen for a 1-byte update beacon. Only if that beacon is received does it keep its radio on to listen for the full configuration payload (with early termination if an unusable payload is detected, maximizing battery savings). The reception logic dynamically allocates buffer space based on reported packet length to prevent false-negative evaluations or FIFO lockups during RX. This conserves significant battery life compared to listening after every transmission or using a long, blind RX window.

### Configuration Payload Format

The configuration payload is a 19-byte binary structure (`ConfigPayload`):
-   `header`: 2 bytes (`"CF"`) to reject noise.
-   `targetMac`: 6 bytes. Target MAC address (or `FF:FF:FF:FF:FF:FF` for broadcast).
-   `networkKey`: 4 bytes. Shared secret key (`NETWORK_KEY`) to prevent unauthorized spoofing.
-   `sleepInterval`: 1 byte. Time in seconds that the device will deep sleep.
-   `isDevMode`: 1 byte. `1` to enable developer mode continuously without deep sleeping, `0` to disable.
-   `timeOffset`: 4 bytes. UTC seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC. A value of `0` ignores synchronization.

*Note: Changing the mode remotely will trigger a soft reset of the ESP32 to cleanly initialize/de-initialize power-heavy peripherals. Configuration values survive this reset by utilizing the ESP32's RTC_NOINIT_ATTR memory section. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node that receives, decrypts (via AES-128-CTR using the message counter as the IV), parses, and displays data from the sensor nodes. It connects to WiFi (hostname: `LoRa-Gateway`) and uses the ESP32 built-in SNTP/timezone support to keep its RTC synchronized in UTC. To ensure a timely response to the sensor's brief listening window, the gateway actively transitions the radio to Standby before any mode change (RX/TX) and during decryption to prevent FIFO corruption. It prioritizes sending configuration downlinks immediately, deferring slower operations like MQTT publishing until after the LoRa transaction is complete. The primary source of truth for the gateway's time is the internal hardware RTC, allowing it to drive sensors and displays seamlessly even after a router failure. Sensors remain timezone-agnostic and are synchronized in UTC only. The gateway converts UTC to local time with DST shifts only for display, logging, and downstream processing. The local OLED display cycles between live sensor telemetry and gateway status. Received sensor data is published securely over TLS as a JSON payload to an MQTT topic (`lora_gateway/<sensor_mac>/telemetry`).
-  **Logging & MQTT**: The gateway uses asynchronous logging via the `Elog` library with standard severity levels and automatically prepends real-world formatted timestamps when the system clock is synchronized. Log writes are dynamically redirected and queued via a background FreeRTOS task to append seamlessly to the SD card without causing any latency during critical radio interrupts. SD card log files are configured to rotate automatically every 100KB (102400 bytes) via the `registerSd` size limit parameter to prevent filesystem overflow. Sent MQTT payloads are also automatically injected with an ISO-8601 formatted `timestamp` property once the NTP validates.
Received sensor data is published securely over TLS as JSON payloads to separate topics. Gateway status is published to LoRaHome/gateway/status. Sensor data is published to LoRaHome/sensor/<sensor_mac>/data for operational mode, and LoRaHome/sensor/<sensor_mac>/telemetry for development mode.
-   `Sensor`: The sensor node that reads temperature and battery metrics, encrypts them, and sends the data to the gateway before entering a deep sleep state.

The core logic files are located in `modes/Sensor/main.cpp` and `modes/GateWay/main.cpp` (a legacy `GateWay.ino` is also retained for Arduino IDE compatibility).

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
To maximize LoRa time-on-air efficiency, the sensor transmits telemetry as a packed binary C++ `struct`. The `TelemetryPayload` size is dynamic: 16 bytes in normal Operation Mode and 27 bytes in Dev Mode (which includes extra debugging metrics). The core payload includes the `sleepInterval`, `isDevMode`, and `needsTimeSync` flags, allowing the gateway to evaluate configuration status without requiring the sensor to keep its receiver open unnecessarily. The payload is secured using AES-128-CTR encryption.
*(Note: The `TelemetryPayload` and `ConfigPayload` structs are defined in `payloads.h` for reusability across Gateway and Sensor modes. Any floating point values like temperature are multiplied before transmission. The Gateway divides them upon receipt to restore the decimal values).*

*Note: SNR and RSSI metrics represent the signal quality of the last received configuration packet from the gateway.*

## Automation
This repository includes a GitHub Action (`issue-commenter.yml`) that automatically posts a comment to any GitHub Issue referenced in a commit message (e.g., `#123`).

## Development
When running in `devMode` (either via the `DEV_MODE_PIN` or remote configuration), the sensor accurately simulates its full operational cycle without entering deep sleep. The simulation loop is: Wake -> Transmit -> Short RX for beacon -> (Optional) Long RX for config -> Update Display -> Pause (for the configured sleep duration). This allows for comprehensive testing of the entire device lifecycle without requiring physical resets.
*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs. Serial logging is managed by a manual `serialEnabled` flag, which is forced `true` in Dev Mode but can be toggled in Operation Mode for testing.*

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.
