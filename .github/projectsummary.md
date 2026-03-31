# Project Summary: Lora32 Boiler Control

## Overview
A LoRa-based remote sensor and gateway system for boiler temperature monitoring and control. The sensor node reads environmental data, transmits it via LoRa, and utilizes ESP32 deep sleep to conserve battery.

## Hardware Components
- **Microcontroller**: ESP32 (Support for S3/Base variants)
- **LoRa Radio**: SX1276 / SX1278 / SX1262 / SX1280 / LR1121
- **Sensors**: DS18B20 (Temperature, OneWire), INA226 (Battery Voltage/Current/Power, I2C)
- **Display**: OLED (controlled via U8g2)

## Software Architecture
- **Framework**: Arduino via PlatformIO (Multi-environment setup allows building Gateway and Sensor simultaneously).
- **Core Logic**: Located in `modes/Sensor/main.cpp`.
- **Gateway Logic**: Located in `modes/GateWay/main.cpp` (legacy `GateWay.ino` retained). Connects to WiFi (hostname: `LoRa-Gateway`) and uses the ESP32 built-in SNTP/timezone support with a POSIX timezone string instead of `ezTime`. Treats the ESP32's hardware RTC as the primary time source to survive internet outages and starts trusting the system clock as soon as SNTP has populated it. Both gateway and sensor hardware clocks stay in UTC. The gateway applies timezone and DST rules only when rendering local time for display, logging, or downstream processing, while config payloads synchronize sensors in UTC. Decrypts the sensor payload on the fly using the `msgCount` as the AES Initialization Vector, cycles the local OLED display, and publishes received sensor data over MQTT/TLS to per-sensor topics.
- **Telemetry Payload Format**: Defined in `payloads.h`. Packed binary C++ struct. Dynamically shifts between 16 bytes (core logic) in Operation Mode and 27 bytes (extended logic) in Dev Mode to maximize battery life. The core payload includes MAC, message count, temperature, battery voltage/percentage, config version, op mode, and a `needsTimeSync` flag. Secured using AES-128-CTR.
- **Remote Configuration Format**: Defined in `payloads.h`. 22-byte packed binary C++ `struct` (ConfigPayload) with a 2-byte "CF" header, 6-byte target MAC address, and a 4-byte Network Key for authentication. Devices listen for it every 10 sleep cycles to update deep sleep intervals, mode, and sync the ESP32 RTC clock in UTC. (offset `0` ignores time sync).
- **Radio Parameters**: Optimized for efficiency (SF9, BW 125kHz, CR 4/5) to significantly reduce power consumption.
- **Development Mode**: Triggered via GPIO13 (low) or remotely via config (hardware pin overrides remote config). Mode switches trigger a soft reset. Accurately simulates the full lifecycle (Wake -> Transmit -> (Maybe) Receive -> Display -> Sleep for exactly the configured interval) continuously without entering deep sleep. The receive window logic (on startup and every 10th cycle) is identical to Operation Mode. RTC memory is validated across deep sleeps and software resets (using RTC_NOINIT_ATTR) via a Magic Word and initialized automatically on cold boots. The sensor considers its clock invalid if the timestamp is before Jan 1, 2024 (`CUSTOM_EPOCH`).
- **Debugging**: Serial logging is toggled via a manual `serialEnabled` flag (forced true in Dev Mode). When active, it provides a comprehensive, single-line summary of all sensor readings, payload details, and a human-readable local timestamp.
- **Code Documentation**: Functions in `main.cpp` contain inline comments denoting their usage scope (Operation mode vs Dev mode).
- **CI/CD Actions**: Includes `.github/workflows/issue-commenter.yml` for auto-commenting on issues referenced in git commits.
- **Stability**: Non-blocking asynchronous RX/TX routines implemented to prevent hardware deadlocks.

## Current Dependencies (platformio.ini)
- milesburton/DallasTemperature
- wollewald/INA226_WE
- olikraus/U8g2
- paulstoffregen/OneWire (Pinned to exactly 2.3.7 to prevent syntax warnings introduced in 2.3.8)
- jgromes/RadioLib
- knolleary/PubSubClient
- bblanchon/ArduinoJson
