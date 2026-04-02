# Project Summary: LoRa32 Boiler Control

## Architecture Overview
1.  **Gateway (`modes/GateWay/main.cpp`)**: 
    - Always-on node with WiFi and MQTT capabilities.
    - Synchronizes internal RTC with NTP in UTC.
    - Receives telemetry payloads from sensors via LoRa.
    - Sends configuration and time sync payloads back to sensors on demand.
    - Publishes sensor data to an MQTT broker over TLS.
    - Has an OLED display to show live sensor telemetry and gateway status.
2.  **Sensor (`modes/Sensor/main.cpp`)**: 
    - Low-power battery-operated node.
    - Reads temperature (DS18B20) and battery metrics (INA226).
    - Transmits telemetry via LoRa.
    - Opens a short RX window post-transmission to check for updates (beacon + config payload).
    - Deep sleeps between operations to save power.
    - Operates in either Operation Mode (deep sleep) or Dev Mode (simulates sleep, updates display, logs heavily).

## Hardware
-   **Microcontroller**: ESP32 with LoRa (SX1276/SX1278/SX1262/etc. via LilyGo LoRa series boards).
-   **Sensors**: DS18B20 (Temperature on GPIO4), INA226 (Battery Voltage/Current/Power via I2C).
-   **Pins**: `DEV_MODE_PIN` (GPIO13) for hardware override into Dev Mode.

## Payload Formats (AES-128-CTR Encrypted)
- **TelemetryPayload**: Packed binary C++ `struct`.
  - **Size**: 16 bytes (Normal Operation Mode) or 27 bytes (Dev Mode with extra metrics).
  - **Content**: Includes `sleepInterval`, `isDevMode`, `needsTimeSync` flags, and sensor readings.
- **ConfigPayload**: 19-byte binary structure.
  - `header`: 2 bytes (`"CF"`)
  - `targetMac`: 6 bytes
  - `networkKey`: 4 bytes
  - `sleepInterval`: 1 byte (deep sleep time in seconds)
  - `isDevMode`: 1 byte (`1` to enable, `0` to disable)
  - `timeOffset`: 4 bytes (UTC epoch sync)

## Operating Modes
- **Operation Mode**: Standard power-saving mode utilizing ESP32 deep sleep.
- **Development Mode**: Bypasses deep sleep for continuous operation and simulation/testing. Can be toggled remotely via the configuration payload or physically overridden via `DEV_MODE_PIN` (GPIO13). Configuration states survive soft resets via `RTC_NOINIT_ATTR` memory.

*Note: This file is intended to provide a condensed context for AI coding assistants. Keep it updated alongside major structural changes.*
## Recent Changes
- Fixed ESP32 compilation errors in `modes/GateWay/main.cpp`.
- Replaced deprecated `ICACHE_RAM_ATTR` with `IRAM_ATTR` for ESP32.
- Handled NTP synchronization validation without relying on the non-standard `timeSync()` function.
- Added cross-version compatibility support for `ArduinoJson` (v6 and v7).
- Replaced `Serial.print` with native ESP-IDF `esp_log` macros, redirecting to an asynchronous FreeRTOS queue for non-blocking SD card logging.
- Removed redundant function name injection (`[%s]`, `__func__`) from `ESP_LOG` statements.
- Formatted human-readable date strings into the MQTT telemetry payload if the hardware clock is synced.
- Prepended local datetime stamps to all asynchronous `esp_log` outputs (UART and SD card) when the clock is valid.