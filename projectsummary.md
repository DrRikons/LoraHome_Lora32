# Project Summary: LoRa32 Boiler Control

Gateway logging queues completed lines to one low-priority writer task, which exclusively writes UART and `/logs/active.log`; the file rotates to `/logs/archive/` at 1 MiB or daily. Lines use `[YYYY-MM-DD HH:MM:SS] [LEVEL] [Function] "message"`.

Detailed implementation documentation and separate Sensor/Gateway workflow diagrams are in `FIRMWARE_GUIDE.md`.

## Architecture Overview
1.  **Gateway (`modes/GateWay/main.cpp`)**: 
    - Always-on node with WiFi and MQTT capabilities.
    - Synchronizes internal RTC with NTP in UTC.
    - Receives telemetry payloads from sensors via LoRa.
    - Sends configuration and time sync payloads back to sensors on demand.
    - Publishes sensor data to an MQTT broker over TLS.
    - Publishes `GatewayStatus` (radio RSSI/SNR, WiFi state, IP, and free RAM) to `lorahome/gateway/status` each connected loop cycle.
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
  - **Size**: 20 bytes (Normal Operation Mode) or 31 bytes (Dev Mode with extra metrics).
  - **Nonce**: The public MAC, random per-boot nonce, and message counter scope AES-CTR encryption.
  - **Content**: Includes `sleepInterval`, `isDevMode`, `needsTimeSync` flags, and sensor readings.
- **ConfigPayload**: 19-byte binary structure.
  - `header`: 2 bytes (`"CF"`)
  - `targetMac`: 6 bytes
  - `networkKey`: 4 bytes
  - `sleepInterval`: 1 byte (deep sleep time in seconds)
  - `configVersion`: 1 byte (currently `1`)
  - `isDevMode`: 1 byte (`1` to enable, `0` to disable)
  - `timeOffset`: 4 bytes (UTC epoch sync)

## Operating Modes
- **Operation Mode**: Standard power-saving mode utilizing ESP32 deep sleep.
- **Development Mode**: Bypasses deep sleep for continuous operation and simulation/testing. Can be toggled remotely via the configuration payload or physically overridden via `DEV_MODE_PIN` (GPIO13). Configuration states survive soft resets via `RTC_NOINIT_ATTR` memory.
- **Sensor logging**: Serial diagnostics are prefixed with UTC after synchronization, otherwise elapsed boot time.
- **Downlink timing**: The Sensor enters beacon RX immediately after uplink; the Gateway waits 50 ms before beacon TX for radio turnaround.
- **MQTT telemetry**: The Gateway publishes core data for both 20-byte operation-mode and 31-byte development-mode telemetry packets.

*Note: This file is intended to provide a condensed context for AI coding assistants. Keep it updated alongside major structural changes.*
## Recent Changes
- Enhanced NTP validation in Gateway with boot-time drift detection and WiFi status check in `isClockValid()` to distinguish true NTP sync from Logger.provideTime() epoch.
## Recent Changes
- Fixed ESP32 compilation errors in `modes/GateWay/main.cpp`.
- Replaced deprecated `ICACHE_RAM_ATTR` with `IRAM_ATTR` for ESP32.
- Handled NTP synchronization validation without relying on the non-standard `timeSync()` function.
- Added cross-version compatibility support for `ArduinoJson` (v6 and v7).
- Replaced ESP-IDF log interception and string parsing with queued UART/SD logging; calls capture their function name automatically, and a single writer owns the active/archive file handling.
- Removed redundant function name injection (`[%s]`, `__func__`) from `ESP_LOG` statements.
- Formatted human-readable date strings into the MQTT telemetry payload if the hardware clock is synced.
- Prepended local datetime stamps to all asynchronous `esp_log` outputs (UART and SD card) when the clock is valid.
- Cleaned up `LoRaHomeCommon.h` by removing duplicate struct fields, fixing macro syntax, and making header functions `inline` to prevent linker errors.
- Implemented missing `mqttConnect()` logic, fixed multiple compiler errors in `Gateway/main.cpp` related to JSON and MQTT publishing, and redefined `GATEWAY_STATUS` to hold proper status metrics rather than mirroring configuration payloads.
- Refactored `jsonBuild()` to accept a generic `void*` payload pointer, allowing it to serialize any of the packed structs (`TelemetryPayload`, `ConfigPayload`, `GatewayStatus`) dynamically based on the mode string.
- Fixed ArduinoJson const-correctness compiler error (`JsonArray` vs `JsonArrayConst`) in `deserializeJson` and refactored the function to take a generic `void*` payload pointer to match `jsonBuild`.
- Fixed macro variable shadowing in `FROM_JSON_ARRAY` where the `size` argument incorrectly replaced the `size()` method of ArduinoJson arrays.
- Fixed remaining `JsonVariantConst` const-correctness errors in `FROM_JSON_ARRAY` macro by changing `is<JsonArray>()` to `is<JsonArrayConst>()`.
- Introduced `STRING` macro to properly serialize character arrays (`wifiStatus`, `ip`, `header`) as native JSON strings rather than JSON arrays, fixing the ArduinoJson compilation error when casting to `char`.
- Optimized `jsonBuild()` timestamp caching by removing the redundant `lastTime` check, preventing heavy `time()` and `strcpy()` calls from executing on every invocation when NTP is unsynced.
- Updated `jsonBuild()` to always serialize the hardware RTC time, allowing NTP sync failures to be immediately visible in the JSON payload (defaulting to 1970 dates) instead of returning "unset".
- Simplified `jsonBuild()` timestamp generation to use a standard Unix epoch integer, removing all string formatting and caching for maximum efficiency and simplicity.
- Fixed `mqttTopic()` function to return a `String` rather than discarding locally scoped variables, making it useful for dynamic MQTT topic generation.
- Added null pointer safeguard for the `mode` parameter in `jsonBuild()` to prevent potential ESP32 crash loops.
- Fixed `mqttPublish` calls in the Gateway `loop()` to properly use `.c_str()` with the generated `String` topics from `mqttTopic()` and cleaned up legacy hardcoded topic strings.
- Fixed undefined `getLocalTimeFromUtc` function call in `drawMain()` by using standard `localtime_r` to properly format the local time based on the configured TZ_INFO.
