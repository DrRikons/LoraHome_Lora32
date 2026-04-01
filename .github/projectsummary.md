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
- **Gateway Logic**: Located in `modes/GateWay/main.cpp` (legacy `GateWay.ino` retained). Connects to WiFi (hostname: `LoRa-Gateway`) and uses the ESP32 built-in SNTP/timezone support with a POSIX timezone string. Upon receiving an interrupt, it actively places the radio in Standby to protect the FIFO buffer during decryption. It immediately processes incoming packets to determine if a configuration response is needed, sending it before handling slower tasks like MQTT publishing to ensure it meets the sensor's short listening window. It treats the ESP32's hardware RTC as the primary time source to survive internet outages and starts trusting the system clock as soon as SNTP has populated it. Both gateway and sensor hardware clocks stay in UTC. The gateway applies timezone and DST rules only when rendering local time for display, logging, or downstream processing, while config payloads synchronize sensors in UTC. Decrypts the sensor payload on the fly using the `msgCount` as the AES Initialization Vector, cycles the local OLED display, and publishes received sensor data over MQTT/TLS to per-sensor topics.
- **Telemetry Payload Format**: Defined in `payloads.h`. A packed binary C++ struct that dynamically shifts between a 16-byte core payload in Operation Mode and a 27-byte extended payload in Dev Mode. The core payload includes MAC, message count, temperature, battery voltage/percentage, `isDevMode`, `needsTimeSync`, and the current `sleepInterval`. The gateway evaluates config changes and UTC time sync requests based on these fields. The payload is secured using AES-128-CTR.
- **Remote Configuration Format**: Defined in `payloads.h`. A 19-byte packed binary C++ `struct` (`ConfigPayload`) with a 2-byte "CF" header, 6-byte target MAC address, and a 4-byte Network Key for authentication. To save power, the sensor listens for a beacon every 10th wake cycle or if its internal clock is not synchronized. Immediately after the uplink is complete, it opens a short RX window (dynamically sized based on beacon airtime) for a 1-byte update beacon. Only if the beacon is received does it open a long RX window to receive the full configuration and UTC time sync data. The RX window safely terminates early if a received packet is unusable (e.g., incorrect MAC, bad header, wrong length) to conserve power. The `configVersion` byte is currently reserved. A `timeOffset` value of `0` indicates the gateway's clock is not yet trusted or the sensor doesn't need a sync.
- **Radio Parameters**: Optimized for efficiency (SF9, BW 125kHz, CR 4/5) to significantly reduce power consumption.
- **Development Mode**: Triggered via GPIO13 (low) or remotely via config (the hardware pin acts as an override). A mode switch triggers a soft reset. This mode accurately simulates the full device lifecycle (Wake -> Transmit -> Short RX for beacon -> Optional Long RX for config -> Display -> Simulated Sleep) continuously without entering deep sleep. RTC memory is validated across deep sleeps and software resets (using `RTC_NOINIT_ATTR`) via a Magic Word and is initialized on cold boot. The sensor considers its clock invalid if the timestamp is before Jan 1, 2024 (`CUSTOM_EPOCH`).
- **Debugging**: Serial logging is toggled via a manual `serialEnabled` flag (forced true in Dev Mode). When active, it provides a comprehensive, single-line summary of all sensor readings, payload details, and a human-readable local timestamp.
- **Code Documentation**: Functions in `main.cpp` contain inline comments denoting their usage scope (Operation mode vs Dev mode).
- **CI/CD Actions**: Includes `.github/workflows/issue-commenter.yml` for auto-commenting on issues referenced in git commits.
- **Stability**: Both Gateway and Sensor utilize hardware interrupt-driven RX/TX routines with FreeRTOS binary semaphores to eliminate busy-wait loops. Explicit Standby mode transitions are strictly enforced between RX and TX phases to ensure LoRa state machine stability and prevent FIFO buffer corruption. Dynamic payload sizing is used during hardware interrupt reads to prevent buffer underflows and ensure the LoRa radio's FIFO cleanly flushes after every reception.

## Current Dependencies (platformio.ini)
- milesburton/DallasTemperature
- wollewald/INA226_WE
- olikraus/U8g2
- paulstoffregen/OneWire (Pinned to exactly 2.3.7 to prevent syntax warnings introduced in 2.3.8)
- jgromes/RadioLib
- knolleary/PubSubClient
- bblanchon/ArduinoJson
