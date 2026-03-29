# Project Summary: Lora32 Boiler Control

## Overview
A LoRa-based remote sensor and gateway system for boiler temperature monitoring and control. The sensor node reads environmental data, transmits it via LoRa, and utilizes ESP32 deep sleep to conserve battery.

## Hardware Components
- **Microcontroller**: ESP32 (Support for S3/Base variants)
- **LoRa Radio**: SX1276 / SX1278 / SX1262 / SX1280 / LR1121
- **Sensors**: DS18B20 (Temperature, OneWire), INA226 (Battery Voltage/Current/Power, I2C)
- **Display**: OLED (controlled via U8g2)

## Software Architecture
- **Framework**: Arduino via PlatformIO
- **Core Logic**: Located in `modes/Sensor/main.cpp`.
- **Telemetry Payload Format**: Packed binary C++ struct. Dynamically shifts between 14 bytes (core logic) in Operation Mode and 25 bytes (extended logic) in Dev Mode to maximize battery life. Secured using AES-128-CTR.
- **Telemetry Payload Format**: Packed binary C++ struct. Dynamically shifts between 15 bytes (core logic) in Operation Mode and 26 bytes (extended logic) in Dev Mode to maximize battery life. Secured using AES-128-CTR.
- **Remote Configuration Format**: 22-byte packed binary C++ `struct` (ConfigPayload) with a 2-byte "CF" header, 6-byte target MAC address, and a 4-byte Network Key for authentication. Devices listen for it every 10 sleep cycles to update deep sleep intervals, mode, and sync the ESP32 RTC clock.
- **Radio Parameters**: Optimized for efficiency (SF9, BW 125kHz, CR 4/5) to significantly reduce power consumption.
- **Development Mode**: Triggered via GPIO13 (low) or remotely via config (hardware pin overrides remote config). Mode switches trigger a soft reset. Simulates the full lifecycle (Wake -> Transmit -> Receive -> Multi-screen Display (including total TX/RX power-on time tracking) -> Simulated Sleep with display off) continuously without entering actual ESP32 deep sleep. RTC memory is validated across deep sleeps using a Magic Word. Base timestamp defaults to Jan 1 2024 before any remote sync.
- **Debugging**: Serial output in Dev Mode provides a comprehensive, single-line summary of all sensor readings, including a human-readable UTC timestamp.
- **Code Documentation**: Functions in `main.cpp` contain inline comments denoting their usage scope (Operation mode vs Dev mode).
- **CI/CD Actions**: Includes `.github/workflows/issue-commenter.yml` for auto-commenting on issues referenced in git commits.
- **Stability**: Non-blocking asynchronous RX/TX routines implemented to prevent hardware deadlocks.

## Current Dependencies (platformio.ini)
- milesburton/DallasTemperature
- wollewald/INA226_WE
- olikraus/U8g2
- paulstoffregen/OneWire
- jgromes/RadioLib