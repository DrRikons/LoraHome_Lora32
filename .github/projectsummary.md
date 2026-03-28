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
- **Telemetry Payload Format**: `ID:<MAC>,T:<Temp>,V:<Volt>,I:<Curr>,P:<Power>,B%:<BattPercent>,S:<ConfigVer>,CNT:<MsgCount>,CT:<CpuTemp>,RAM:<RamKB>,TXP:<TxPower>,SNR:<SNR>,RSSI:<RSSI>`
- **Remote Configuration**: Devices listen for a `CONFIG:<sleepInterval>,<version>[,<devMode>]` packet every 10 sleep cycles to update deep sleep intervals and mode via RTC memory.
- **Development Mode**: Triggered via GPIO13 (low) or remotely via config (hardware pin overrides remote config). Mode switches trigger a soft reset. Simulates the full lifecycle (Wake -> Transmit -> Receive -> Multi-screen Display -> Sleep) continuously without entering actual ESP32 deep sleep.
- **Code Documentation**: Functions in `main.cpp` contain inline comments denoting their usage scope (Operation mode vs Dev mode).

## Current Dependencies (platformio.ini)
- milesburton/DallasTemperature
- wollewald/INA226_WE
- olikraus/U8g2
- paulstoffregen/OneWire
- jgromes/RadioLib