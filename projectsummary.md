# Project Summary: LoRa32 Boiler Control

`CLOUD_IOT_HANDOFF.md` is the cloud-side integration contract and implementation handoff. It records the exact MQTT topics, configurable MQTT endpoint format, current JSON schema/units, timestamp caveat, ingestion/storage guidance, security limitations, recommended MQTT-to-InfluxDB-to-Grafana architecture, and a ready-to-use AI implementation prompt.
Gateway tracks up to 16 sensor MACs in RAM and publishes retained `lorahome/sensor/<mac>/status` JSON (`online`, continuous Gateway-observed sensor `uptime`, and timestamp). A sensor changes offline after three missed advertised sleep intervals plus five seconds, and online on its next valid packet. Presence tracking resets on Gateway reboot, so old retained sensor statuses can be stale until their next packet.

Gateway logging queues completed lines to one low-priority writer task, which exclusively writes UART and `/logs/active.log`; the file rotates to `/logs/archive/` at 1 MiB or daily. Lines use `[YYYY-MM-DD HH:MM:SS] [LEVEL] [Function] "message"`.
Gateway reads `/config.json` on boot for WiFi, MQTT (bare hostname plus validated `port`, default `8883`), default sensor values, and `control.commandSecret`; absent files are created with blank network values without read-only VFS errors. Valid JSON files are normalized by adding missing supported objects/keys with defaults, while malformed JSON is not modified and invalid values keep defaults.
When no card is present at boot, the Gateway retries SD initialization every five seconds after insertion.
Valid configuration from a card inserted after boot is applied to WiFi and MQTT without rebooting.
SD mount failures are emitted once through the Gateway logger instead of raw SD/VFS diagnostics.
Gateway loop processing gives queued radio events priority over SD, network, display, and other background work.
Gateway subscribes to `lorahome/sensor/+/config` and accepts signed, non-expired per-sensor MQTT overrides for Dev Mode, 10–255 second sleep intervals, and signed 8-bit TX-power requests (-128–127 dBm). Commands require a known Sensor MAC, unique UUID, and HMAC-SHA256 with `control.commandSecret` from Gateway SD configuration; accepted overrides are used by the next LoRa downlink and live only in Gateway RAM. The Sensor rejects TX-power requests unsupported by its installed radio; cloud implementations must enforce the fixed sleep range and signed-8-bit TX-power range.

Detailed implementation documentation and separate Sensor/Gateway workflow diagrams are in `FIRMWARE_GUIDE.md`.
The pending Sensor power-consumption and battery-charging audit is recorded in `SENSOR_POWER_REVIEW.md`.

Sensors derive their periodic update-beacon cadence from the configured sleep interval, targeting no more than 60 seconds between checks; sleep intervals of 60 seconds or longer check every wake. The counter resets after each check so an absent beacon does not cause continuous RX polling.
Before operation-mode deep sleep, Sensors put the LoRa radio to sleep and the INA226 into power-down mode. A radio initialization failure uses a five-minute fail-safe sleep before retrying.
Sensor temperature uses 9-bit DS18B20 resolution (0.5 C steps, one-decimal display) to limit blocking conversion time to 93.75 ms.

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
  - **Size**: 21 bytes (Normal Operation Mode) or 31 bytes (Dev Mode with extra metrics).
  - **Nonce**: The public MAC, random per-boot nonce, and message counter scope AES-CTR encryption.
  - **Content**: Includes `sleepInterval`, `isDevMode`, `needsTimeSync` flags, and sensor readings.
- **ConfigPayload**: 20-byte binary structure.
  - `header`: 2 bytes (`"CF"`)
  - `targetMac`: 6 bytes
  - `networkKey`: 4 bytes
  - `sleepInterval`: 1 byte (deep sleep time in seconds, 10–255)
  - `configVersion`: 1 byte (currently `2`)
  - `isDevMode`: 1 byte (`1` to enable, `0` to disable)
  - `txPower`: 1 signed byte, Gateway-requested LoRa output power in dBm
  - `timeOffset`: 4 bytes (UTC epoch sync)

## Operating Modes
- **Operation Mode**: Standard power-saving mode utilizing ESP32 deep sleep.
- **Development Mode**: Bypasses deep sleep for continuous operation and simulation/testing. Can be toggled remotely via the configuration payload or physically overridden via `DEV_MODE_PIN` (GPIO13). Configuration states survive soft resets via `RTC_NOINIT_ATTR` memory.
- **Sensor logging**: Serial diagnostics are prefixed with UTC after synchronization, otherwise elapsed boot time.
- **Sensor link metrics**: Last configuration-downlink SNR/RSSI persist in RTC memory across deep sleep and soft resets so the next development telemetry packet reports them.
- **Downlink timing**: The Sensor enters beacon RX immediately after uplink; the Gateway waits 50 ms before beacon TX for radio turnaround.
- **Development power metric**: The pre-TX INA226 reading is retained as idle baseline. Development firmware samples voltage and signed current every 20 ms during asynchronous TX and active RX windows, then sends the signed power average in the existing `battPower` field on the next development telemetry cycle; normal-mode payloads and sizes are unchanged.
- **MQTT telemetry**: The Gateway publishes core data for both 21-byte operation-mode and 31-byte development-mode telemetry packets. TX power and the one-byte sleep interval are included in both formats so Gateway can detect and correct a mismatch.

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
