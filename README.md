# lora32_boiler_control
Based on https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/docs/en/t3_v161_sx1276/t3_v161_sx1276_hw.md

For a complete firmware walkthrough, separate Sensor/Gateway architecture diagrams, and LoRa message flows, see [FIRMWARE_GUIDE.md](FIRMWARE_GUIDE.md).
## Configuration
At boot, the Gateway reads `/config.json` from the SD-card root. If absent, it creates the file with blank WiFi/MQTT values, MQTT TLS port `8883`, sensor defaults, and an empty `control.commandSecret`. A valid JSON configuration missing any supported section or key is completed with its current default value; invalid fields retain those defaults.
If a card is inserted after boot, the Gateway retries SD initialization every five seconds and creates the same initial configuration and log directories. A newly created configuration is reported as a template to edit; missing files do not generate read-only VFS errors.
If the inserted card already contains a valid `/config.json`, its WiFi and MQTT settings are applied immediately without rebooting.

```json
{"wifi":{"ssid":"SSID","password":"PASSWORD"},"mqtt":{"host":"broker.example","port":8883,"user":"USER","password":"PASSWORD"},"sensor":{"defaultSleepSeconds":20,"defaultDevMode":true,"defaultTxPower":17},"control":{"commandSecret":"PRIVATE_SHARED_SECRET"}}
```

The sensor node is configured remotely over a two-step LoRa downlink. To conserve power, it checks for a configuration beacon at least every 60 seconds (or every wake for sleep intervals of 60 seconds or more), and whenever its clock is not synchronized. It opens the beacon RX window immediately after the telemetry uplink; the Gateway waits 50 ms for this RX turnaround before transmitting the 1-byte beacon. Only if that beacon is received does the Sensor keep its radio on to listen for the full configuration payload. The reception logic dynamically allocates buffer space based on reported packet length to prevent false-negative evaluations or FIFO lockups during RX.

The Gateway subscribes to `lorahome/sensor/+/config` for signed cloud commands. Commands can override sleep interval (10–255 seconds), Dev Mode, and TX power (-128–127 dBm request range) for a known sensor; the Gateway verifies the HMAC-SHA256 signature, expiry, command UUID, and duplicate status before applying the override to the next Sensor downlink. The Sensor rejects power values unsupported by its installed radio. Set `control.commandSecret` on the Gateway SD card to the cloud service's shared secret.

### Configuration Payload Format

The configuration payload is a 20-byte binary structure (`ConfigPayload`):
-   `header`: 2 bytes (`"CF"`) to reject noise.
-   `targetMac`: 6 bytes. Target MAC address (or `FF:FF:FF:FF:FF:FF` for broadcast).
-   `networkKey`: 4 bytes. Shared secret key (`NETWORK_KEY`) to prevent unauthorized spoofing.
-   `sleepInterval`: 1 byte. Time in seconds that the device will deep sleep (10–255).
-   `configVersion`: 1 byte. Current configuration schema version (`2`).
-   `isDevMode`: 1 byte. `1` to enable developer mode continuously without deep sleeping, `0` to disable.
-   `txPower`: 1 signed byte. Requested LoRa output power in dBm; invalid values for the installed radio are rejected.
-   `timeOffset`: 4 bytes. UTC seconds elapsed since Jan 1, 2024 (1704067200) to synchronize the device's internal RTC. A value of `0` ignores synchronization.

*Note: Changing the mode or TX power remotely triggers a soft reset so the radio is initialized with the accepted setting. Configuration values survive this reset by utilizing the ESP32's RTC_NOINIT_ATTR memory section. If the physical `DEV_MODE_PIN` (GPIO13) is pulled LOW, it acts as a hard hardware override and the device will ignore any remote commands to enter Operation Mode.*

## Project Structure

The project is divided into two main modes:

-   `GateWay`: The gateway node receives, decrypts, parses, and displays sensor data; it prioritizes the beacon/config reply before slower MQTT work. Telemetry AES-CTR nonces include each sensor MAC, a random boot nonce, and the message counter.
-  **Logging & MQTT**: Gateway logging queues completed lines for one low-priority writer task, which exclusively writes UART and `/logs/active.log`; the file rotates into `/logs/archive/` at 1 MiB or a local-day change. Log lines use `[YYYY-MM-DD HH:MM:SS] [LEVEL] [Function] "message"`. Sent MQTT payloads are also automatically injected with an ISO-8601 formatted `timestamp` property once the NTP validates.
Gateway radio events are always serviced before SD, WiFi, MQTT, display, or other background work; queued radio events defer that work to the next loop.
Received sensor data is published securely over TLS as JSON payloads to separate topics. Gateway status is published to `lorahome/gateway/status`. Sensor data is published to `lorahome/sensor/<sensor_mac>/data` for operational mode, and `lorahome/sensor/<sensor_mac>/telemetry` for development mode. Retained sensor presence is published to `lorahome/sensor/<sensor_mac>/status` as `online` plus the sensor's continuous Gateway-observed online duration; it changes to offline after three missed advertised sleep intervals plus five seconds. Presence is held for up to 16 sensors in Gateway RAM and resets on Gateway restart.
Gateway status contains radio RSSI/SNR, WiFi state, IP address, and free RAM. Free RAM is included in status messages but does not independently trigger publication.
Both valid telemetry packet sizes are published: 21-byte operation-mode payloads publish core data, while 31-byte development payloads also publish the development metrics.
-   `Sensor`: The sensor node that reads temperature and battery metrics, encrypts them, and sends the data to the gateway before entering a deep sleep state.

Before ESP32 deep sleep, the Sensor places both the LoRa radio and INA226 into their low-power modes. If radio initialization fails, it enters a five-minute fail-safe sleep before retrying instead of remaining awake.

The core logic files are located in `modes/Sensor/main.cpp` and `modes/GateWay/main.cpp` (a legacy `GateWay.ino` is also retained for Arduino IDE compatibility).

*Note: All functions in `main.cpp` are documented inline to indicate whether they are executed in normal Operation mode, Development mode, or both.*

## Telemetry Payload Format
To maximize LoRa time-on-air efficiency, the sensor transmits telemetry as a packed binary C++ `struct`. The `TelemetryPayload` size is dynamic: 21 bytes in normal Operation Mode and 31 bytes in Dev Mode (which includes extra debugging metrics). The Gateway-configured TX power and one-byte sleep interval are present in both formats so it can request an update when they differ. Its public MAC, random boot nonce, and message counter form the AES-CTR nonce; the remainder is encrypted.
*(Note: The `TelemetryPayload` and `ConfigPayload` structs are defined in `payloads.h` for reusability across Gateway and Sensor modes. Any floating point values like temperature are multiplied before transmission. The Gateway divides them upon receipt to restore the decimal values).*

*Note: SNR and RSSI metrics represent the signal quality of the last received configuration packet from the gateway and persist across sensor deep sleep and soft resets.*

## Automation
This repository includes a GitHub Action (`issue-commenter.yml`) that automatically posts a comment to any GitHub Issue referenced in a commit message (e.g., `#123`).

## Development
When running in `devMode` (either via the `DEV_MODE_PIN` or remote configuration), the sensor accurately simulates its full operational cycle without entering deep sleep. The simulation loop is: Wake -> Transmit -> Short RX for beacon -> (Optional) Long RX for config -> Update Display -> Pause (for the configured sleep duration). This allows for comprehensive testing of the entire device lifecycle without requiring physical resets.
*Note: The codebase uses hardware interrupt-driven, asynchronous RX/TX routines to prevent blocking loops and hangs. Serial logging is managed by a manual `serialEnabled` flag, which is forced `true` in Dev Mode but can be toggled in Operation Mode for testing.*

In development mode, the INA226 reading taken before TX remains the idle baseline. The sensor samples board power throughout the asynchronous TX and any active post-TX RX windows; the average is published through the existing `battPower` development field on the next telemetry cycle. This keeps the 31-byte payload unchanged and avoids an extra uplink.

Sensor serial diagnostics are prefixed with UTC after time synchronization, or elapsed boot time while the RTC is unsynchronized.

An `.aiexclude` file is included to prevent AI coding assistants from indexing large third-party libraries in the `lib/` folder and build artifacts in the `.pio/` folder, preserving context space.

For AI assistants, refer to `.github/projectsummary.md` for a high-level overview of the project's architecture, hardware, and configuration formats. Please keep the project summary updated when introducing major changes or new features.

## Recent Updates
- Fixed a bug in the Gateway's `isClockValid` function where the unsynced RTC time check used incorrect math (adding uptime instead of subtracting it). The gateway will now correctly invalidate the clock if it is free-running from `CUSTOM_EPOCH` without an actual NTP sync.
