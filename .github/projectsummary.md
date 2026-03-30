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
- **Gateway Logic**: Located in `modes/GateWay/main.cpp` (legacy `GateWay.ino` retained). Connects to WiFi (hostname: `LoRa-Gateway`) to fetch accurate NTP time. Decrypts the sensor payload on the fly using the MAC address and `msgCount` as the AES Initialization Vector. Cycles the local OLED display to show parsed telemetry and gateway status. Transmits a configuration payload back to the sensor (with a 100ms turnaround delay to prevent preamble clipping), and clears hardware interrupt flags post-transmission to prevent TX-triggered infinite receive loops.
- **Telemetry Payload Format**: Packed binary C++ struct. Dynamically shifts between 17 bytes (core logic) in Operation Mode and 28 bytes (extended logic) in Dev Mode to maximize battery life. Secured using AES-128-CTR.
- **Remote Configuration Format**: 22-byte packed binary C++ `struct` (ConfigPayload) with a 2-byte "CF" header, 6-byte target MAC address, and a 4-byte Network Key for authentication. Devices listen for it every 10 sleep cycles to update deep sleep intervals, mode, and sync the ESP32 RTC clock (offset `0` ignores time sync).
- **Radio Parameters**: Optimized for efficiency (SF9, BW 125kHz, CR 4/5) to significantly reduce power consumption.
- **Development Mode**: Triggered via GPIO13 (low) or remotely via config (hardware pin overrides remote config). Mode switches trigger a soft reset. Accurately simulates the full lifecycle (Wake -> Transmit -> (Maybe) Receive -> Display -> Sleep for exactly the configured interval) continuously without entering deep sleep. The receive window logic (on startup and every 10th cycle) is identical to Operation Mode. RTC memory is validated across deep sleeps and software resets (using RTC_NOINIT_ATTR) via a Magic Word and initialized automatically on cold boots. Base timestamp defaults to Jan 1 2024 before any remote sync.
- **Debugging**: Serial logging is toggled via a manual `serialEnabled` flag (forced true in Dev Mode). When active, it provides a comprehensive, single-line summary of all sensor readings, payload details, and a human-readable UTC timestamp.
- **Code Documentation**: Functions in `main.cpp` contain inline comments denoting their usage scope (Operation mode vs Dev mode).
- **CI/CD Actions**: Includes `.github/workflows/issue-commenter.yml` for auto-commenting on issues referenced in git commits.
- **Stability**: Non-blocking asynchronous RX/TX routines implemented to prevent hardware deadlocks.

## Current Dependencies (platformio.ini)
- milesburton/DallasTemperature
- wollewald/INA226_WE
- olikraus/U8g2
- paulstoffregen/OneWire (Pinned to exactly 2.3.7 to prevent syntax warnings introduced in 2.3.8)
- jgromes/RadioLib